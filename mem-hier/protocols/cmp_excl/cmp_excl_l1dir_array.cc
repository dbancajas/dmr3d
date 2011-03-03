/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    implementation of a two-level CMP L1DIR cache
 * initial author: Philip Wells 
 *
 */
 
 
//  #include "simics/first.h"
// RCSID("$Id: cmp_excl_l1dir_array.cc,v 1.1.2.4 2006/02/13 23:20:29 pwells Exp $");

#include "definitions.h"
#include "protocols.h"
#include "statemachine.h"
#include "device.h"
#include "message.h"
#include "cache.h"
#include "tcache.h"
#include "external.h"
#include "transaction.h"
#include "debugio.h"

#include "cmp_excl_l1dir_array.h"

// Link definitions
const uint8 cmp_excl_l1dir_protocol_t::interconnect_link;
const uint8 cmp_excl_l1dir_protocol_t::l2_addr_link;
const uint8 cmp_excl_l1dir_protocol_t::l2_data_link;
const uint8 cmp_excl_l1dir_protocol_t::io_link;  

/////////////////////////////////////////////////////////////////////////////////
// Uniprocessor one-level line functions
//
void
cmp_excl_l1dir_line_t::init(cache_t *_cache, addr_t _idx, uint32 _lsize)
{
	line_t::init(_cache, _idx, _lsize);
	state = StateInvalid;
	sharers = 0;
}

void
cmp_excl_l1dir_line_t::wb_init(addr_t tag, addr_t idx, line_t *cache_line)
{
	FAIL;
}

void
cmp_excl_l1dir_line_t::reinit(addr_t _tag)
{
	line_t::reinit(_tag);
	state = StateInvalid;
	sharers = 0;
}	
	
bool
cmp_excl_l1dir_line_t::is_free()
{
	return (state == StateInvalid);
}	
	
bool
cmp_excl_l1dir_line_t::can_replace()
{
	// Can replace if not in a busy state
	return (state == StateInvalid ||
			state == StateNotPresent ||
			state == StateShared ||
			state == StateModified);
}

bool
cmp_excl_l1dir_line_t::is_stable()
{
	// Same as can_replace
	return (can_replace());
}

void
cmp_excl_l1dir_line_t::set_state(uint32 _state)
{
	state = _state;
}
	
uint32
cmp_excl_l1dir_line_t::get_state()
{
	return state;
}

void
cmp_excl_l1dir_line_t::add_sharer(uint32 sharer)
{
	sharers |= (1 << sharer);
}
	
void
cmp_excl_l1dir_line_t::remove_sharer(uint32 sharer)
{
	sharers &= ~(1 << sharer);
}
	
bool
cmp_excl_l1dir_line_t::is_sharer(uint32 sharer)
{
	return (sharers & (1 << sharer)) != 0;
}

uint32
cmp_excl_l1dir_line_t::get_sharers() {
	return sharers;	
}

void
cmp_excl_l1dir_line_t::to_file(FILE *file)
{
	fprintf(file, "%u %u ", state, sharers);
	line_t::to_file(file);
}

void
cmp_excl_l1dir_line_t::from_file(FILE *file)
{
	fscanf(file, "%u %u ", &state, &sharers);
	line_t::from_file(file);
}

/////////////////////////////////////////////////////////////////////////////////
// CMP exclusive L1DIR cache functions
//

cmp_excl_l1dir_array_t::cmp_excl_l1dir_array_t(string name, cache_config_t *conf,
	uint32 _id, uint64 prefetch_alg, uint32 id_offset)
	: tcache_t<cmp_excl_l1dir_protocol_t> (name, 3, conf, _id, prefetch_alg),
	interconnect_id_offset(id_offset)
	// Three links for this protocol
{ 
	stat_num_req_fwd = stats->COUNTER_BASIC("request_fwd",
		"# of request forwards");
}

uint32
cmp_excl_l1dir_array_t::get_action(message_t *msg)
{
	// upmsg_t same as downmsg_t
	upmsg_t *message = NULL;
	message = static_cast<upmsg_t *> (msg);
	
	if (message) {
		switch(message->get_type()) {
		case TypeRead:
			return ActionL1Read;
			
		case TypeReadEx:
			return ActionL1ReadEx;
			
		case TypeUpgrade:
			return ActionL1Upgrade;
			
		case TypeReplace:
			return ActionL1Replace;
			
		case TypeWriteBack:
			return ActionL1WriteBack;
			
		case TypeWriteBackClean:
			return ActionL1WriteBackClean;
			
		case TypeInvAck:
			return ActionL1InvAck;
			
		// Down Message
		case TypeDataRespExclusive:
		case TypeDataRespShared:
			return ActionDataResponse;

		default:
			FAIL_MSG("Invalid message type %s\n",
					 upmsg_t::names[(int)message->get_type()][0].c_str());
		}
	} else {
		FAIL;
	}
}

bool 
cmp_excl_l1dir_array_t::wakeup_signal(message_t* msg, prot_line_t *line, 
    uint32 action)
{
    return true;
}

void
cmp_excl_l1dir_array_t::trigger(tfn_info_t *t, tfn_ret_t *ret)
{
	ret->block_message = StallNone;
	ret->next_state = StateUnknown;

    ASSERT(!t->trans->completed || t->trans->get_pending_events());
    
    switch(t->curstate) {

	/// NotPresent
	case StateNotPresent:
		switch(t->action) {
			
		case ActionL1Read:
		case ActionL1ReadEx:
        case ActionL1Upgrade:
			ret->block_message = can_insert_new_line(t, ret);
			ASSERT(ret->block_message == StallNone);

			if (!can_allocate_mshr(t, ret)) ret->block_message = StallPoll;
			if (!can_enqueue_request_mshr(t, ret)) ret->block_message = StallPoll;
			if (!can_send_msg_down(t, ret)) ret->block_message = StallPoll;
			if (ret->block_message != StallNone) goto block_message;
			
			insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			mark_requester_as_sharer(t, ret);
			send_msg_down(t, ret, downmsg_t::TypeRead);
			ret->next_state = StateInvToMod;
			
			t->trans->mark_pending(MEM_EVENT_L1DIR_DEMAND_MISS);
			break;
//		case ActionL1WriteBack:
//        case ActionL1WriteBackClean:
//            send_ack_up(t, ret, TypeWriteBackAck);
//            ret->next_state = t->curstate;
//            break;
		default: goto error;
		}
		break;

	/// Invalid
	case StateInvalid:
		switch(t->action) {
			
		case ActionL1Read:
		case ActionL1ReadEx:
        case ActionL1Upgrade:
			if (!can_allocate_mshr(t, ret))        goto block_message_poll;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			if (!can_send_msg_down(t, ret))       goto block_message_poll;
			
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			mark_requester_as_sharer(t, ret);
			send_msg_down(t, ret, downmsg_t::TypeRead);
			ret->next_state = StateInvToMod;
			
			t->trans->mark_pending(MEM_EVENT_L1DIR_DEMAND_MISS);
			break;
			
		case ActionReplace: goto donothing;
			
		default: goto error;
		}
		break;

	/// InvalidToMod
	case StateInvToMod:
		switch(t->action) {

		case ActionL1Read:
		case ActionL1ReadEx:
			goto block_message_set;

		case ActionDataResponse:
			perform_mshr_requests(t, ret);
			deallocate_mshr(t, ret);
			ret->next_state = StateModified;
			break;
		// case ActionL1WriteBack:
        // case ActionL1WriteBackClean:
            // ASSERT(!requester_is_sharer(t, ret));
            // send_ack_up(t, ret, TypeWriteBackAck);
            // ret->next_state = t->curstate;
            // break;
		default: goto error;
		}
		break;

	/// Shared
	case StateShared:
		switch(t->action) {

		case ActionL1Read:
			send_request_forward(t, ret, TypeReadFwd);
			mark_requester_as_sharer(t, ret);
			ret->next_state = t->curstate;
			t->trans->mark_event(MEM_EVENT_L1DIR_REQ_FWD);
			break;
			
		case ActionL1ReadEx:
			if (!can_allocate_mshr(t, ret))        goto block_message_poll;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
				
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			mark_requester_as_sharer(t, ret);
			send_invalidate_to_sharers(t, ret, false); // not excl. req.
			ret->next_state = StateShToMod;
			t->trans->mark_pending(MEM_EVENT_L1DIR_COHER_MISS);
			break;
			
		case ActionL1Upgrade:
            if (requester_is_only_sharer(t, ret)) { // No other sharers
				send_ack_up(t, ret, TypeUpgradeAck);
                ret->next_state = StateModified;
				t->trans->mark_event(MEM_EVENT_L1DIR_HIT);
            } else {
				if (!can_allocate_mshr(t, ret))        goto block_message_poll;
				if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
                
				allocate_mshr(t, ret);
                enqueue_request_mshr(t, ret);
				send_invalidate_to_sharers(t, ret, false); // not excl. req.
				ret->next_state = StateShToMod;
				t->trans->mark_pending(MEM_EVENT_L1DIR_COHER_MISS);
			}
			break;
			
		case ActionL1Replace:  
            remove_source_sharer(t, ret);
            send_ack_up(t, ret, TypeReplaceAck);
			if (t->line->get_sharers() == 0) {
				send_msg_down(t, ret, TypeWriteBackClean);
				ret->next_state = StateInvalid;	
			} 
			else
				ret->next_state = t->curstate;
            break;
		
        case ActionL1WriteBack:
        case ActionL1WriteBackClean:
			ASSERT(!requester_is_sharer(t, ret) ||
				!requester_is_only_sharer(t, ret));
            remove_source_sharer(t, ret);
			send_ack_up(t, ret, TypeWriteBackAck);
            ret->next_state = t->curstate;
            break;

		default: goto error;
		}
		break;
	
	/// ShToMod
	case StateShToMod:
		switch (t->action) {
		
		case ActionL1Read:
		case ActionL1ReadEx:
            goto block_message_set;
		case ActionL1Upgrade:
            goto donothing; // Drop it ... L1 will reissue
			
		case ActionL1Replace:
            // Need Not Send an ACK up
		case ActionL1InvAck:
			remove_source_sharer(t, ret);
			if (is_only_one_sharer(t, ret)) {
				perform_mshr_requests(t, ret);
				deallocate_mshr(t, ret);
				ret->next_state = StateModified;
            } else 
                ret->next_state = t->curstate;
			break;
        case ActionL1WriteBack:
        case ActionL1WriteBackClean:
			ASSERT(!requester_is_sharer(t, ret) ||
				!requester_is_only_sharer(t, ret));
            remove_source_sharer(t, ret);
            send_ack_up(t, ret, TypeWriteBackAck);
            ret->next_state = t->curstate;
            break;
		default: goto error;
		}
		break;
		
	/// Modified
	case StateModified:
		switch (t->action) {
		
		case ActionL1Read:
			if (!can_allocate_mshr(t, ret))        goto block_message_poll;
			
			// don't enqueue this request (but allocate for future reqs)
			//allocate_mshr(t, ret);
			send_request_forward(t, ret, TypeReadFwd);
            mark_requester_as_sharer(t, ret);
			t->trans->mark_event(MEM_EVENT_L1DIR_REQ_FWD);
			ret->next_state = StateShared;
			break;
		
		case ActionL1ReadEx:
		case ActionL1Upgrade:
            // request Forward must be sent first
            send_request_forward(t, ret, TypeReadExFwd);
			remove_only_sharer(t, ret);
			mark_requester_as_sharer(t, ret);
			t->trans->mark_event(MEM_EVENT_L1DIR_REQ_FWD);
			ret->next_state = t->curstate;
			break;
			
		case ActionL1WriteBack:	
		case ActionL1WriteBackClean:
			remove_source_sharer(t, ret);
			send_ack_up(t, ret, TypeWriteBackAck);
			// Replacement sent before ReadEx Fwd, so someone else is the owner
			if (t->line->get_sharers() > 0) 
				ret->next_state = t->curstate;
			else {
				send_msg_down(t, ret,
					(static_cast <upmsg_t *> (t->message))->get_type());
				ret->next_state = StateInvalid;	
			}
			break;
		
		default: goto error;
		}
		break;

	/// ModToSh
	// case StateModToSh:
		// switch (t->action) {
		// 
		// case ActionL1Read:
			// if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			// enqueue_request_mshr(t, ret);
            // mark_requester_as_sharer(t, ret);
			// t->trans->mark_pending(MEM_EVENT_L1DIR_MSHR_HIT);
			// ret->next_state = t->curstate;
			// break;
		// 
		// case ActionL1ReadEx:
        // case ActionL1Upgrade:
            // goto block_message_set;
			// 
		// case ActionL1WriteBack:	
		// case ActionL1WriteBackClean:
			// remove_source_sharer(t, ret);
			// send_ack_up(t, ret, TypeWriteBackAck);
			// ret->next_state = t->curstate;
			// break;
		// 
		// case ActionL1FwdAck:
			// perform_mshr_requests(t, ret);
            // deallocate_mshr(t, ret);
			// ret->next_state = StateShared;
			// break;
			// 
		// case ActionL1Replace:  
			// remove_source_sharer(t, ret);
            // send_ack_up(t, ret, TypeReplaceAck);
			// ret->next_state = t->curstate;
			// break;
		// 
		// default: goto error;
		// }
		// break;

	default:
		FAIL;

	}

	/// Common return case
	ASSERT(ret->next_state != StateUnknown);
	// May not have line if in NotPresent
	if (t->line) t->line->set_state(ret->next_state);
	else ASSERT(t->curstate == StateNotPresent);

    t->trans->clear_pending_event(MEM_EVENT_L1DIR_STALL);
	profile_transition(t, ret);
	return;


/// Blocking Actions 
block_message_poll:
	if (ret->block_message == StallNone) ret->block_message = StallPoll;
	// fallthrough
block_message_set:
	if (ret->block_message == StallNone) ret->block_message = StallSetEvent;
	// fallthrough
block_message:
	ret->next_state = t->curstate;
	t->trans->mark_pending(MEM_EVENT_L1DIR_STALL);
    DEBUG_TR_FN("Blocking Message block_type %u", t, ret, ret->block_message);
	profile_transition(t, ret);
	return;
	

/// Other common actions	
error:
	FAIL_MSG("%10s @%llu 0x%016llx:Invalid action: %s in state: %s\n",get_cname(), 
		external::get_current_cycle(), t->message->address, 
        action_names[t->action][0].c_str(),state_names[t->curstate][0].c_str());
	
donothing:
	ret->next_state = t->curstate;
	profile_transition(t, ret);
	return;
}

////////////////////////////////////////////////////////////////////////////////
// CMP exclusice L1DIR cache specific transition functions

// Can send address requests down
bool
cmp_excl_l1dir_array_t::can_send_msg_down(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	// return links[mainmem_addr_link]->can_send();
	return true;
}
	
void
cmp_excl_l1dir_array_t::send_msg_down(tfn_info_t *t, tfn_ret_t *ret,
	uint32 type)
{
	DEBUG_TR_FN("%s msg type %s", t, ret, __func__, 
        downmsg_t::names[type][0].c_str());
	
	uint32 link;
	if (type == downmsg_t::TypeWriteBack || 
		type == downmsg_t::TypeWriteBackClean)
	{
		link = l2_data_link;
	}
	else 
		link = l2_addr_link;
	
	addr_t addr = get_line_address(t->address);
	downmsg_t *msg = new downmsg_t(t->trans, addr, get_lsize(), NULL, 
		0, 0, type, 0, false);

	stall_status_t retval = send_message(links[link], msg);
	ASSERT(retval == StallNone);
}

// Send data response back to requester
void
cmp_excl_l1dir_array_t::send_data_to_requester(tfn_info_t *t, tfn_ret_t *ret, 
	bool shared)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
				
	addr_t addr = t->address;
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);
	ASSERT(get_lsize() == t->message->size);
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);

	uint32 type = shared ? TypeDataRespShared : TypeDataRespExclusive;
	
	send_protocol_message_up(t, ret, type, inmsg->get_requester(),
		inmsg->get_requester(), t->trans);
}

// Send read forward to some requester
void
cmp_excl_l1dir_array_t::send_request_forward(tfn_info_t *t, tfn_ret_t *ret, 
	uint32 type)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	
	// TODO: some heuristic to pick a sharer if more than one
	uint32 sharer;
	for (sharer = 0; sharer < prot_line_t::MAX_SHARERS; sharer++)
		if (t->line->is_sharer(sharer)) break;
		
    ASSERT(sharer != prot_line_t::MAX_SHARERS);
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	send_protocol_message_up(t, ret, type, sharer, inmsg->get_requester(), t->trans);
}

void
cmp_excl_l1dir_array_t::send_ack_up(tfn_info_t *t, tfn_ret_t *ret, uint32 type)
{
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
    DEBUG_TR_FN("%s sending %s to %u", t, ret, __func__, 
        cmp_excl_protocol_t::names[type][0].c_str(),inmsg->get_source());
	send_protocol_message_up(t, ret, type, inmsg->get_source(),
		inmsg->get_requester(), t->trans);
}


// Send invalidate to icache if exclusion bit set
void
cmp_excl_l1dir_array_t::send_invalidate_to_sharers(tfn_info_t *t, tfn_ret_t *ret,
	bool requester_too)
{
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);  
	uint32 requester = inmsg->get_requester();
	DEBUG_TR_FN("%s requester %u", t, ret, __func__, requester);
	
	for (uint32 i = 0; i < prot_line_t::MAX_SHARERS; i++) {
		if (!t->line->is_sharer(i)) continue;
		if (!requester_too && i == requester) continue;
		
		send_protocol_message_up(t, ret, TypeInvalidate, i, requester, t->trans);
	}
}


// Enqueue this read request to the MSHR for this address
void
cmp_excl_l1dir_array_t::enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("%s", t, ret, __func__);

	addr_t addr = get_line_address(t->address);
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);

	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);  

	mshr_type_t *request = new mshr_type_t;
	request->trans = t->trans;
	request->requester_id = inmsg->get_requester();
	request->request_type = inmsg->get_type();
	
	bool retval = mshr->enqueue_request(request);
	ASSERT(retval);
}

// Send response back to requester using MSHR
void
cmp_excl_l1dir_array_t::perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret)
{
	addr_t addr = get_line_address(t->address);
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);

	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);

	DEBUG_TR_FN("%s", t, ret, __func__);

	mshr_type_t *request = mshr->pop_request();
//	ASSERT(request);
	
	while (request) {
		
		uint32 response_type;
		uint32 state = t->line->get_state();
		
		if (request->request_type == TypeUpgrade) {
            ASSERT(state == StateShToMod || state == StateInvToMod);
            if (state == StateShToMod)
                response_type = TypeUpgradeAck;
            else
                response_type = TypeDataRespExclusive;
		} else if (request->request_type == TypeReadEx) { 
			response_type = TypeDataRespExclusive; 
			ASSERT(state == StateShToMod || state == StateInvToMod);
		} else if (request->request_type == TypeRead) { 
			if (state == StateModToSh)
				response_type = TypeDataRespShared;
			else if (state == StateInvToMod)
				response_type = TypeDataRespExclusive;
			else
				FAIL;
		} else
			FAIL;
		
		DEBUG_TR_FN("%s: sending %s to %d", t, ret, __func__, 
			names[response_type][0].c_str(), request->requester_id);
	
		request->trans->clear_pending_event(
			MEM_EVENT_L1DIR_DEMAND_MISS | 
			MEM_EVENT_L1DIR_COHER_MISS);

		send_protocol_message_up(t, ret, response_type, request->requester_id,
			request->requester_id, request->trans);
        delete request;    
		request = mshr->pop_request();
	}
	
}

void
cmp_excl_l1dir_array_t::send_protocol_message_up(tfn_info_t *t, tfn_ret_t *ret,
	uint32 type, uint32 destination, uint32 requester, mem_trans_t *mem_trans)
{
	DEBUG_TR_FN("%s: %s to %u", t, ret, __func__, 
        upmsg_t::names[type][0].c_str(), destination);

	bool addr_link = true;
	const uint8 *data = NULL;
	
	addr_t addr = get_line_address(t->address);
	
	if (type == TypeDataRespShared || type == TypeDataRespExclusive) {
		addr_link = false;
		data = t->line->get_data();
	}
	
	
	upmsg_t *msg = new upmsg_t(mem_trans, addr, get_lsize(), data,
		get_interconnect_id(addr), destination, type, requester, addr_link);
		
	stall_status_t retval = send_message(links[interconnect_link], msg);
	ASSERT(retval == StallNone);

}


void
cmp_excl_l1dir_array_t::mark_requester_as_sharer(tfn_info_t *t, tfn_ret_t *ret) {
	DEBUG_TR_FN("%s", t, ret, __func__);
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	t->line->add_sharer(inmsg->get_requester());
}

void
cmp_excl_l1dir_array_t::remove_source_sharer(tfn_info_t *t, tfn_ret_t *ret) {
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
    DEBUG_TR_FN("%s: removing %u", t, ret, __func__, inmsg->get_source());
	t->line->remove_sharer(inmsg->get_source());  // user sender not requester
}

// Remove only sharer (owner)
void
cmp_excl_l1dir_array_t::remove_only_sharer(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	
	for (uint32 i = 0; i < prot_line_t::MAX_SHARERS; i++) {
		if (t->line->is_sharer(i)) {
			t->line->remove_sharer(i);
			break;
		}
	}
}

bool
cmp_excl_l1dir_array_t::requester_is_sharer(tfn_info_t *t, tfn_ret_t *ret)
{
    DEBUG_TR_FN("%s", t, ret, __func__);
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	return (t->line->is_sharer(inmsg->get_requester()));
}

bool
cmp_excl_l1dir_array_t::is_only_one_sharer(tfn_info_t *t, tfn_ret_t *ret)
{
	uint32 num_sharers = 0;
	for (uint32 i = 0; i < prot_line_t::MAX_SHARERS; i++) {
		if (t->line->is_sharer(i))
			num_sharers++;
	}

    DEBUG_TR_FN("%s: %d", t, ret, __func__, num_sharers);
	
	if (num_sharers > 1) return false;
	ASSERT(num_sharers == 1);
	return true;
}

bool
cmp_excl_l1dir_array_t::requester_is_only_sharer(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	ASSERT(t->line->is_sharer(inmsg->get_requester()));
    DEBUG_TR_FN("%s sharers %u requester %u", t, ret, __func__, 
        t->line->get_sharers(), inmsg->get_requester());
	if ((t->line->get_sharers() & (~(1 << inmsg->get_requester()))) == 0)
		return true;
	
	return false;
}

uint32
cmp_excl_l1dir_array_t::get_interconnect_id(addr_t addr) {
	uint32 bank = get_bank(addr);
	return (bank+interconnect_id);
}

bool
cmp_excl_l1dir_array_t::is_request_message(message_t *message)
{
	if (!is_up_message(message))
		return;
	
	upmsg_t *inmsg = static_cast<upmsg_t *>(message);
	return (inmsg->get_type() == TypeRead ||
		inmsg->get_type() == TypeReadEx ||
		inmsg->get_type() == TypeUpgrade);
}

void
cmp_excl_l1dir_array_t::profile_transition(tfn_info_t *t, tfn_ret_t *ret)
{
	tcache_t<cmp_excl_l1dir_protocol_t>::profile_transition(t, ret);
	
	if (!ret->block_message && is_request_message(t->message)) {
		if (t->trans->is_prefetch()) {
			if ((t->trans->occurred(MEM_EVENT_L1DIR_DEMAND_MISS) ||
				t->trans->occurred(MEM_EVENT_L1DIR_COHER_MISS) ||
				t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD)))
			{
				STAT_INC(stat_num_prefetch);
			}
			return;
		}
		if (t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD))
			STAT_INC(stat_num_req_fwd);
		
		if (t->trans->read) {
			if (t->trans->occurred(MEM_EVENT_L1DIR_HIT) ||
				t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD))
			{
				STAT_INC(stat_num_read_hit);
			}
			if (t->trans->occurred(MEM_EVENT_L1DIR_DEMAND_MISS) ||
				t->trans->occurred(MEM_EVENT_L1DIR_COHER_MISS))
			{
				STAT_INC(stat_num_read_miss);
			}
			if (t->trans->occurred(MEM_EVENT_L1DIR_STALL))
				STAT_INC(stat_num_read_stall);
		} else {
			if (t->trans->occurred(MEM_EVENT_L1DIR_HIT) ||
				t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD))
			{
				STAT_INC(stat_num_write_hit);
			}
			if (t->trans->occurred(MEM_EVENT_L1DIR_DEMAND_MISS) ||
				t->trans->occurred(MEM_EVENT_L1DIR_COHER_MISS))
			{
				STAT_INC(stat_num_write_miss);
			}
			if (t->trans->occurred(MEM_EVENT_L1DIR_STALL))
				STAT_INC(stat_num_write_stall);
		}
	}	
}

////////////////////////////////////////////////////////////////////////////////
// Uniprocessor onelevel cache constant definitions

const string cmp_excl_l1dir_protocol_t::action_names[][2] = {
	{ "Replace", "Replace line" }, 
	{ "Flush", "Flush line" }, 
	{ "L1Read", "L1 Read Request" }, 
	{ "L1ReadEx", "L1 ReadExclusive Request" }, 
	{ "L1Upgrade", "L1 Upgrade Request" },
	{ "L1Replace", "L1 Replace Notify Message" },
	{ "L1WriteBack", "L1 Write Back" },
	{ "L1WriteBackClean", "L1 Write Back" },
	{ "L1InvAck", "L1 Invalidate Acknowledgement" },
	{ "L1FwdAck", "L1 Request Forwared Acknowledgement" },
	{ "DataResponse", "Data Response from Memory" },
};
const string cmp_excl_l1dir_protocol_t::state_names[][2] = {
	{ "Unknown", "Unknown" },
	{ "NotPresent", "Line is not present in cache" },
	{ "Invalid", "Data may be present but is invalid" },
	{ "InvToMod", "Invalid" },
	{ "Shared", "" },
	{ "ShToMod", "" },
	{ "Modified", "" },
	{ "ModToSh", "" },
};

const string cmp_excl_l1dir_protocol_t::prot_name[] = 
	{"cmp_excl_l1dir", "Two-level CMP L1 Directory Array"};
