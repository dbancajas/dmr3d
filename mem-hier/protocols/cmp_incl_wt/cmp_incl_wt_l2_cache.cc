/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    implementation of a two-level CMP L2 cache
 * initial author: Koushik Chakraborty
 *
 */
 
 
//  #include "simics/first.h"
// RCSID("$Id: cmp_incl_wt_l2_cache.cc,v 1.1.2.42 2005/11/23 16:44:42 kchak Exp $");

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

#include "cmp_incl_wt_l2_cache.h"

// Link definitions
const uint8 cmp_incl_wt_l2_protocol_t::interconnect_link;
const uint8 cmp_incl_wt_l2_protocol_t::mainmem_addr_link;
const uint8 cmp_incl_wt_l2_protocol_t::mainmem_data_link;
const uint8 cmp_incl_wt_l2_protocol_t::io_link;  

/////////////////////////////////////////////////////////////////////////////////
// Uniprocessor one-level line functions
//
void
cmp_incl_wt_l2_line_t::init(cache_t *_cache, addr_t _idx, uint32 _lsize)
{
	line_t::init(_cache, _idx, _lsize);
	state = StateInvalid;
	dirty = false;
	sharers = 0;
}

void
cmp_incl_wt_l2_line_t::wb_init(addr_t tag, addr_t idx, line_t *cache_line)
{
	line_t::wb_init(tag, idx, cache_line);
	
	if (cache_line) {
		cmp_incl_wt_l2_line_t *line = static_cast <cmp_incl_wt_l2_line_t *> (cache_line);
		dirty = line->is_dirty();
		sharers = line->get_sharers();
		state = StateShToInv;
	} else {
		state = StateInvalid;
		dirty = false;
		sharers = 0;
	}

	
}

void
cmp_incl_wt_l2_line_t::reinit(addr_t _tag)
{
	line_t::reinit(_tag);
	state = StateInvalid;
	dirty = false;
	sharers = 0;
}	
	
bool
cmp_incl_wt_l2_line_t::is_free()
{
	return (state == StateInvalid);
}	
	
bool
cmp_incl_wt_l2_line_t::can_replace()
{
	// Can replace if not in a busy state
	return (state == StateInvalid ||
			state == StateNotPresent ||
			state == StateShared);
}

bool
cmp_incl_wt_l2_line_t::is_stable()
{
	// Same as can_replace
	return (can_replace());
}

void
cmp_incl_wt_l2_line_t::warmup_state()
{
    state = StateShared;
    sharers = 0;
}

void
cmp_incl_wt_l2_line_t::set_state(uint32 _state)
{
	state = _state;
}
	
uint32
cmp_incl_wt_l2_line_t::get_state()
{
	return state;
}

void
cmp_incl_wt_l2_line_t::set_dirty(bool _dirty)
{
	dirty = _dirty;
}
	
bool
cmp_incl_wt_l2_line_t::is_dirty()
{
	return dirty;
}
	
void
cmp_incl_wt_l2_line_t::add_sharer(uint32 sharer)
{
	sharers |= (1 << sharer);
}
	
void
cmp_incl_wt_l2_line_t::remove_sharer(uint32 sharer)
{
	sharers &= ~(1 << sharer);
}
	
bool
cmp_incl_wt_l2_line_t::is_sharer(uint32 sharer)
{
	return (sharers & (1 << sharer)) != 0;
}

uint32
cmp_incl_wt_l2_line_t::get_sharers() {
	return sharers;	
}

void
cmp_incl_wt_l2_line_t::to_file(FILE *file)
{
	fprintf(file, "%d %u %u ", dirty ? 1 : 0, state, sharers);
	line_t::to_file(file);
}

void
cmp_incl_wt_l2_line_t::from_file(FILE *file)
{
	int temp_dirty;
	fscanf(file, "%d %u %u ", &temp_dirty, &state, &sharers);
	dirty = (temp_dirty == 1);

	line_t::from_file(file);
}

/////////////////////////////////////////////////////////////////////////////////
// CMP inclusive L2 cache functions
//

cmp_incl_wt_l2_cache_t::cmp_incl_wt_l2_cache_t(string name, cache_config_t *conf,
	uint32 _id, uint64 prefetch_alg, uint32 id_offset)
	: tcache_t<cmp_incl_wt_l2_protocol_t> (name, 3, conf, _id, prefetch_alg),
	interconnect_id_offset(id_offset)
	// Three links for this protocol
{ 
	stat_num_req_fwd = stats->COUNTER_BASIC("request_fwd",
		"# of request forwards");
        
	num_procs = mem_hier_t::ptr()->get_num_processors() ;
	
	stat_read_c2c = new group_counter_t * [num_procs];
	stat_write_c2c = new group_counter_t * [num_procs];
	char stat_name[128];
	for (uint32 i = 0; i < num_procs; i++) {
		snprintf(stat_name, 128, "read_c2c_proc%u", i);
		stat_read_c2c[i] = stats->COUNTER_GROUP (stat_name,
			"breakdown or read c2c from each cache", num_procs);
	}
	for (uint32 i = 0; i < num_procs; i++) {
		snprintf(stat_name, 128, "write_c2c_proc%u", i);
		stat_write_c2c[i] = stats->COUNTER_GROUP (stat_name,
			"breakdown or write c2c from each cache", num_procs);
	}
}

uint32
cmp_incl_wt_l2_cache_t::get_action(message_t *msg)
{
    bool t_state = g_conf_tstate_l1_bypass ? 
        msg->get_trans()->is_thread_state() : false;
	if (is_up_message(msg)) {
		upmsg_t *upmsg = static_cast<upmsg_t *> (msg);

		switch(upmsg->get_type()) {
		case TypeRead:
            if (t_state) return ActionL1TST;
			return ActionL1Read;
			
		case TypeReadEx:
            if (t_state) return ActionL1TST;
			return ActionL1ReadEx;
			
		case TypeReplace:
			return ActionL1Replace;
			
		case TypeInvAck:
			return ActionL1InvAck;
			
		
			
		default:
			FAIL_MSG("Invalid message type %s\n",
					 upmsg_t::names[(int)upmsg->get_type()][0].c_str());
		}
	} else if (is_down_message(msg)) {
		downmsg_t *downmsg = static_cast<downmsg_t *> (msg);
		
		switch(downmsg->get_type()) {
		case downmsg_t::DataResp:
			return ActionDataResponse;
			
		default:
			FAIL_MSG("Invalid message type %s\n",
					 downmsg_t::names[(int)downmsg->get_type()][0].c_str());
		}
	} else {
		FAIL;
	}
}

bool 
cmp_incl_wt_l2_cache_t::wakeup_signal(message_t* msg, prot_line_t *line, 
    uint32 action)
{
    return true;
}

void
cmp_incl_wt_l2_cache_t::trigger(tfn_info_t *t, tfn_ret_t *ret)
{
	ret->block_message = StallNone;
	ret->next_state = StateUnknown;

    switch(t->curstate) {

	/// NotPresent
	case StateNotPresent:
		switch(t->action) {
			
		case ActionL1Read:
		case ActionL1ReadEx:
        case ActionL1TST:    
			ret->block_message = can_insert_new_line(t, ret);
			if (!can_allocate_mshr(t, ret)) ret->block_message = StallPoll;
			if (!can_enqueue_request_mshr(t, ret)) ret->block_message = StallPoll;
			if (!can_send_msg_down(t, ret)) ret->block_message = StallPoll;
			if (ret->block_message != StallNone) goto block_message;
			
			insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			mark_requester_as_sharer(t, ret);
			send_msg_down(t, ret, downmsg_t::Read);
			ret->next_state = StateInvToMod;
			
			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
			break;
		
        case ActionL1Replace:
            if (!g_conf_icache_inclusion)
            {
                //send_ack_up(t, ret, TypeReplaceAck);
                goto donothing;
            }
            // fall through
		default: goto error;
		}
		break;

	/// Invalid
	case StateInvalid:
		switch(t->action) {
			
		case ActionL1Read:
		case ActionL1ReadEx:
			if (!can_allocate_mshr(t, ret))        goto block_message_poll;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			if (!can_send_msg_down(t, ret))        goto block_message_poll;
			
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			mark_requester_as_sharer(t, ret);
			send_msg_down(t, ret, downmsg_t::Read);
			ret->next_state = StateInvToMod;
			
			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
			break;
			
		case ActionReplace: goto donothing;
        case ActionL1Replace:
           if (!g_conf_icache_inclusion)
            {
               // send_ack_up(t, ret, TypeReplaceAck);
                goto donothing;
            }
			// fall through
		default: goto error;
		}
		break;

	/// InvalidToMod
	case StateInvToMod:
		switch(t->action) {

		case ActionL1Read:
		case ActionL1ReadEx:
        case ActionL1TST: 
			goto block_message_set;

		case ActionDataResponse:
			store_data_to_cache(t, ret);
			perform_mshr_requests(t, ret);
			deallocate_mshr(t, ret);
			ret->next_state = StateShared;
			break;
		case ActionL1Replace:
            if (!g_conf_icache_inclusion)
            {
               // send_ack_up(t, ret, TypeReplaceAck);
                goto donothing;
            }
            // fall through
		default: goto error;
		}
		break;

	/// Shared
	case StateShared:
		switch(t->action) {

		case ActionL1Read:
            mark_requester_as_sharer(t, ret);
            send_data_to_requester(t, ret, true);  // shared
            ret->next_state = t->curstate;
            t->trans->mark_event(MEM_EVENT_L2_HIT);
			break;
			
		case ActionL1ReadEx:
            
            if (t->line->get_sharers() == 0) 
                send_update_to_sharers(t, ret);
            mark_requester_as_sharer(t, ret);
            send_ack_up(t, ret, TypeWriteComplete);
            ret->next_state = t->curstate;
            // if (t->line->get_sharers() == 0) {
			// 	mark_requester_as_sharer(t, ret);
			// 	send_data_to_requester(t, ret, false); // Exclusive
			// 	ret->next_state = StateModified;
			// 	t->trans->mark_event(MEM_EVENT_L2_HIT);
            // } else {
			// 	if (!can_allocate_mshr(t, ret))        goto block_message_poll;
			// 	if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
				
			// 	allocate_mshr(t, ret);
			// 	enqueue_request_mshr(t, ret);
			// 	mark_requester_as_sharer(t, ret);
			// 	send_update_to_sharers(t, ret, false); // not incl. req.
			// 	ret->next_state = StateShToMod;
			// 	t->trans->mark_pending(MEM_EVENT_L2_COHER_MISS);
			// }
			break;
			
		case ActionL1Replace:  
            if (requester_is_sharer(t, ret))
                remove_source_sharer(t, ret);
            ret->next_state = t->curstate;
            break;
		
		case ActionReplace:
			if (t->line->get_sharers() > 0) {
				ASSERT(allocate_writebackbuffer(t->line));
				send_invalidate_to_sharers(t, ret); 
				ret->next_state = StateInvalid;  // wbb goes to ShToInv
				t->trans->mark_pending(MEM_EVENT_L2_REPLACE);
			} else {
				ret->next_state = StateInvalid;
				t->trans->mark_event(MEM_EVENT_L2_REPLACE);
			}				
			break;

        case ActionL1TST:
            send_data_to_requester(t, ret, false);
            ret->next_state = t->curstate;
            break;
        
        default: goto error;
		}
		break;
	
	default:
		FAIL;

	}


	/// Common return case
	ASSERT(ret->next_state != StateUnknown);
	// May not have line if in NotPresent
	if (t->line) t->line->set_state(ret->next_state);
	else ASSERT(t->curstate == StateNotPresent);
    t->trans->clear_pending_event(MEM_EVENT_L2_STALL);
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
	t->trans->mark_pending(MEM_EVENT_L2_STALL);
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
// CMP inclusice L2 cache specific transition functions

// Can send address requests down
bool
cmp_incl_wt_l2_cache_t::can_send_msg_down(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	// return links[mainmem_addr_link]->can_send();
	return true;
}
	
void
cmp_incl_wt_l2_cache_t::send_msg_down(tfn_info_t *t, tfn_ret_t *ret,
	uint32 type)
{
	DEBUG_TR_FN("%s msg type %s", t, ret, __func__, 
        downmsg_t::names[type][0].c_str());
	
	uint32 link;
	if (type == downmsg_t::WriteBack) 
		link = mainmem_data_link;
	else 
		link = mainmem_addr_link;
	
	addr_t addr = get_line_address(t->address);
	downmsg_t *msg = new downmsg_t(t->trans, addr, get_lsize(), NULL, type);

	stall_status_t retval = send_message(links[link], msg);
	ASSERT(retval == StallNone);
}

// Send writeback down if line is dirty
void
cmp_incl_wt_l2_cache_t::writeback_if_dirty(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	
	// Better not be any sharers
	ASSERT(!t->line->get_sharers());
	
	if (!t->line->is_dirty()) return;

	t->trans->mark_pending(MEM_EVENT_MAINMEM_WRITE_BACK);
	
	send_msg_down(t, ret, downmsg_t::WriteBack);
}

// Send data response back to requester
void
cmp_incl_wt_l2_cache_t::send_data_to_requester(tfn_info_t *t, tfn_ret_t *ret, 
	bool shared)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
				
	addr_t addr = t->address;
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);
	ASSERT(get_lsize() == t->message->size);
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);

	send_protocol_message_up(t, ret, TypeDataResp, inmsg->get_requester(),
		inmsg->get_requester(), t->trans);
}


void
cmp_incl_wt_l2_cache_t::send_ack_up(tfn_info_t *t, tfn_ret_t *ret, uint32 type)
{
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
    DEBUG_TR_FN("%s sending %s to %u", t, ret, __func__, 
        cmp_incl_wt_protocol_t::names[type][0].c_str(),inmsg->get_source());
	send_protocol_message_up(t, ret, type, inmsg->get_source(),
		inmsg->get_requester(), t->trans);
}


// Send invalidate to icache if inclusion bit set
void
cmp_incl_wt_l2_cache_t::send_update_to_sharers(tfn_info_t *t, tfn_ret_t *ret)
{
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);  
	uint32 requester = inmsg->get_requester();
	DEBUG_TR_FN("%s requester %u", t, ret, __func__, requester);
	
	for (uint32 i = 0; i < prot_line_t::MAX_SHARERS; i++) {
		if (!t->line->is_sharer(i)) continue;
		
        STAT_INC(stat_write_c2c[t->trans->cpu_id], 
            (i >= num_procs ? i / 2 : i));
        // Don't send invalidation to Icache if no_inclusion
        //if (requester_too && !g_conf_icache_inclusion && i >= num_procs) continue; 
		send_protocol_message_up(t, ret, TypeUpdate, i, requester, t->trans);
	}
}

void
cmp_incl_wt_l2_cache_t::send_invalidate_to_sharers(tfn_info_t *t, tfn_ret_t *ret)
{
    upmsg_t *inmsg = static_cast <upmsg_t *> (t->message); 
	uint32 requester = inmsg->get_requester();
	for (uint32 i = 0; i < prot_line_t::MAX_SHARERS; i++) {
		if (!t->line->is_sharer(i)) continue;
		
        // Don't send invalidation to Icache if no_inclusion
        //if (requester_too && !g_conf_icache_inclusion && i >= num_procs) continue; 
		send_protocol_message_up(t, ret, TypeInvalidate, i, requester, t->trans);
	}
}


// Enqueue this read request to the MSHR for this address
void
cmp_incl_wt_l2_cache_t::enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_wt_l2_cache_t::perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret)
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
		
		response_type = TypeDataResp;
		DEBUG_TR_FN("%s: sending %s to %d", t, ret, __func__, 
			names[response_type][0].c_str(), request->requester_id);
	
		request->trans->clear_pending_event(
			MEM_EVENT_L2_DEMAND_MISS | 
			MEM_EVENT_L2_COHER_MISS |
			MEM_EVENT_L2_MSHR_PART_HIT | MEM_EVENT_L2_REQ_FWD);

		send_protocol_message_up(t, ret, response_type, request->requester_id,
			request->requester_id, request->trans);
        delete request;    
		request = mshr->pop_request();
	}
	
}

void
cmp_incl_wt_l2_cache_t::send_protocol_message_up(tfn_info_t *t, tfn_ret_t *ret,
	uint32 type, uint32 destination, uint32 requester, mem_trans_t *mem_trans)
{
	DEBUG_TR_FN("%s: %s to %u", t, ret, __func__, 
        upmsg_t::names[type][0].c_str(), destination);

	bool addr_link = true;
	const uint8 *data = NULL;
	
	addr_t addr = get_line_address(t->address);
	
	if (type == TypeDataResp) {
		addr_link = false;
		data = t->line->get_data();
	}
	
	upmsg_t *msg = new upmsg_t(mem_trans, addr, get_lsize(), data,
		get_interconnect_id(addr), destination, type, requester, addr_link);
		
	stall_status_t retval = send_message(links[interconnect_link], msg);
	ASSERT(retval == StallNone);

}


void
cmp_incl_wt_l2_cache_t::mark_requester_as_sharer(tfn_info_t *t, tfn_ret_t *ret) {
	DEBUG_TR_FN("%s", t, ret, __func__);
	
    if (g_conf_tstate_l1_bypass && t->trans->is_thread_state())
        return;
    
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	t->line->add_sharer(inmsg->get_requester());
}

void
cmp_incl_wt_l2_cache_t::remove_source_sharer(tfn_info_t *t, tfn_ret_t *ret) {
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
    DEBUG_TR_FN("%s: removing %u", t, ret, __func__, inmsg->get_source());
	t->line->remove_sharer(inmsg->get_source());  // user sender not requester
}

// Remove only sharer (owner)
void
cmp_incl_wt_l2_cache_t::remove_only_sharer(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_wt_l2_cache_t::requester_is_sharer(tfn_info_t *t, tfn_ret_t *ret)
{
    DEBUG_TR_FN("%s", t, ret, __func__);
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	return (t->line->is_sharer(inmsg->get_requester()));
}

bool
cmp_incl_wt_l2_cache_t::is_only_one_sharer(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_wt_l2_cache_t::requester_is_only_sharer(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_wt_l2_cache_t::get_interconnect_id(addr_t addr) {
	uint32 bank = get_bank(addr);
	return (bank+interconnect_id);
}

bool
cmp_incl_wt_l2_cache_t::is_request_message(message_t *message)
{
	if (is_up_message(message)) {
		upmsg_t *inmsg = static_cast<upmsg_t *>(message);
	return (inmsg->get_type() == TypeRead ||
		inmsg->get_type() == TypeReadEx);
		
	}
	
	return false;
}

void
cmp_incl_wt_l2_cache_t::profile_transition(tfn_info_t *t, tfn_ret_t *ret)
{
	if (!ret->block_message && is_request_message(t->message)) {
		if (t->trans->occurred(MEM_EVENT_L2_REQ_FWD)) {
			if (t->trans->is_prefetch())
				STAT_INC(stat_num_prefetch);
			else
				STAT_INC(stat_num_req_fwd);
			t->trans->mark_event(MEM_EVENT_L2_HIT);
		}
	}

	tcache_t<cmp_incl_wt_l2_protocol_t>::profile_transition(t, ret);
}

////////////////////////////////////////////////////////////////////////////////
// Uniprocessor onelevel cache constant definitions

const string cmp_incl_wt_l2_protocol_t::action_names[][2] = {
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
	{ "L1FwdNack", "L1 Request Forward Nack Message" },
	{ "L1FwdAckData", "L1 Request Forwared Acknowledgement with Dirty Data" },
	{ "DataResponse", "Data Response from Memory" },
    { "L1TST", "Thread State request from L1 (bypass)" }
};
const string cmp_incl_wt_l2_protocol_t::state_names[][2] = {
	{ "Unknown", "Unknown" },
	{ "NotPresent", "Line is not present in cache" },
	{ "Invalid", "Data may be present but is invalid" },
	{ "InvToMod", "Invalid" },
	{ "Shared", "" },
	{ "ShToInv", "" },
	{ "ShFwd", "" },
	{ "ShToMod", "" },
	{ "Modified", "" },
	{ "ModToSh", "" },
	{ "ModToInv", "" },
};

const string cmp_incl_wt_l2_protocol_t::prot_name[] = 
	{"cmp_incl_wt_l2", "Two-level CMP L2 Cache"};
