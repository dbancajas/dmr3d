/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    implementation of a two-level uniprocessor L1 data cache
 * initial author: Philip Wells 
 *
 */
 
 
/* This implements a writeback L2 cache for an Icache and writethrough-DCache
   Consistency is maintained between Dcache writes and Icache
   Inclusion is maintained for both cache (though L2 tags may be out of date)
   L2 Does allocate on Dcache writethroughs
   
   Links:  0 - ICache,
           1 - DCache,
		   2 - MainMem
 
TODO: FIX stats
      Handle block-store-commits (need to writeback and invalidate line)

*/
 
//  #include "simics/first.h"
// RCSID("$Id: unip_two_l2_cache.cc,v 1.1.2.2 2005/09/13 14:16:33 kchak Exp $");

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

#include "unip_two_l2_cache.h"


// Link definitions
const uint8 unip_two_l2_protocol_t::icache_link;  
const uint8 unip_two_l2_protocol_t::dcache_link;  
const uint8 unip_two_l2_protocol_t::mainmem_link;  
const uint8 unip_two_l2_protocol_t::io_link;  

/////////////////////////////////////////////////////////////////////////////////
// Uniprocessor one-level line functions
//
void
unip_two_l2_line_t::init(cache_t *_cache, addr_t _idx, uint32 _lsize)
{
	line_t::init(_cache, _idx, _lsize);
	state = StateInvalid;
	icache_inclusion = false;
	icache_inclusion = false;
}

void
unip_two_l2_line_t::reinit(addr_t _tag)
{
	line_t::reinit(_tag);
	state = StateInvalid;
	icache_inclusion = false;
	icache_inclusion = false;
}	
	
bool
unip_two_l2_line_t::is_free()
{
	return (state == StateInvalid);
}	
	
bool
unip_two_l2_line_t::can_replace()
{
	// Can replace if not in a busy state
	return (state == StateInvalid ||
			state == StateClean ||
			state == StateDirty);
}

bool
unip_two_l2_line_t::is_stable()
{
	// Same as can_replace
	return (can_replace());
}

void
unip_two_l2_line_t::set_state(uint32 _state)
{
	state = _state;
}
	
uint32
unip_two_l2_line_t::get_state()
{
	return state;
}

bool
unip_two_l2_line_t::get_icache_inclusion()
{
	return icache_inclusion;
}
	
bool
unip_two_l2_line_t::get_dcache_inclusion()
{
	return dcache_inclusion;
}
	
void
unip_two_l2_line_t::set_icache_inclusion(bool val)
{
	icache_inclusion = val;
}
	
void
unip_two_l2_line_t::set_dcache_inclusion(bool val)
{
	dcache_inclusion = val;
}
	
void
unip_two_l2_line_t::to_file(FILE *file)
{
	fprintf(file, "%d %d %d ", (int) state, icache_inclusion ? 1 : 0,
	        dcache_inclusion ? 1 : 0);
	line_t::to_file(file);
}

void
unip_two_l2_line_t::from_file(FILE *file)
{
	int temp_icache_inc;
	int temp_dcache_inc;
	fscanf(file, "%d %d %d ", (int *) &state, &temp_icache_inc, &temp_dcache_inc);
	icache_inclusion = (temp_icache_inc == 1);
	dcache_inclusion = (temp_dcache_inc == 1);

	line_t::from_file(file);
}

/////////////////////////////////////////////////////////////////////////////////
// Uniprocessor one-level cache functions
//

unip_two_l2_cache_t::unip_two_l2_cache_t(string name, cache_config_t *conf, uint32 _id,
    uint64 prefetch_alg)
	: tcache_t<unip_two_l2_protocol_t> (name, 3, conf, _id, prefetch_alg)
	// Three links for this protocol
{ }

uint32
unip_two_l2_cache_t::get_action(message_t *msg)
{
	upmsg_t *upmsg = NULL;
	downmsg_t *downmsg = NULL;
	if (typeid(*msg) == typeid(upmsg_t)) {
		upmsg = static_cast<upmsg_t *> (msg);
	} else if (typeid(*msg) == typeid(downmsg_t)) {
		downmsg = static_cast<downmsg_t *> (msg);
	} else {
		FAIL_MSG("Invalid msg typeid: %s\n", typeid(*msg).name());
	}
	
	if (upmsg) {
		ASSERT(downmsg == NULL);

		switch(upmsg->type) {
		case TypeIFetch:
			return ActionIRead;
			
		case TypeRead:
			return ActionDRead;
			
		case TypeWrite:
			return ActionDWrite;
			
		default:
			FAIL_MSG("Invalid message type %s\n",
					 upmsg_t::names[(int)upmsg->type][0].c_str());
		}
	} else if (downmsg) {
		ASSERT(upmsg == NULL);
		
		switch(downmsg->get_type()) {
		case TypeDataResp:
			return ActionRecvData;
			
		default:
			FAIL_MSG("Invalid message type %s\n",
					 downmsg_t::names[(int)downmsg->get_type()][0].c_str());
		}
	} else {
		FAIL;
	}
}

void
unip_two_l2_cache_t::trigger(tfn_info_t *t, tfn_ret_t *ret)
{
	// Set ret->block_message if incoming request should be blocked and
	// presumably retried next cycle
	ret->block_message = StallNone;
	ret->next_state = StateUnknown;

	switch(t->curstate) {

	case StateNotPresent:
		switch(t->action) {
		case ActionIRead:
			ret->block_message = can_insert_new_line(t, ret);
			if (ret->block_message != StallNone)   goto block_message;
			if (!can_allocate_mshr(t, ret))        goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			if (!can_send_msg_down(t, ret))        goto block_message_poll;

			insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_read_down(t, ret);
			set_icache_inclusion(t, ret);
			ret->next_state = StateInv_Rd;

			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			break;
			
		case ActionDRead:
			ret->block_message = can_insert_new_line(t, ret);
			if (ret->block_message != StallNone)   goto block_message;
			if (!can_allocate_mshr(t, ret))        goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			if (!can_send_msg_down(t, ret))        goto block_message_poll;

			insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_read_down(t, ret);
			set_dcache_inclusion(t, ret);
			ret->next_state = StateInv_Rd;

			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			break;
			
		case ActionDWrite:
			ret->block_message = can_insert_new_line(t, ret);
			if (ret->block_message != StallNone)   goto block_message;
			if (!can_allocate_mshr(t, ret))        goto block_message_other;
			if (!can_send_msg_down(t, ret))        goto block_message_poll;

			insert_new_line(t, ret);
			allocate_mshr(t, ret);
			send_read_down(t, ret);
			store_data_to_cache(t, ret);
			set_mshr_valid_bits(t, ret);
			ret->next_state = StateInv_Write;
			
			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			break;
			
		case ActionRecvData:   goto error;
		case ActionReplace:    goto donothing;

		default: goto error;
		}
		break;

	case StateInvalid:
		switch(t->action) {
		case ActionIRead:
			if (!can_allocate_mshr(t, ret))        goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			if (!can_send_msg_down(t, ret))        goto block_message_poll;

			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_read_down(t, ret);
			set_icache_inclusion(t, ret);
			ret->next_state = StateInv_Rd;

			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			break;
			
		case ActionDRead:
			if (!can_allocate_mshr(t, ret))        goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			if (!can_send_msg_down(t, ret))        goto block_message_poll;

			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_read_down(t, ret);
			set_dcache_inclusion(t, ret);
			ret->next_state = StateInv_Rd;

			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			break;
			
		case ActionDWrite:
			if (!can_allocate_mshr(t, ret))        goto block_message_other;
			if (!can_send_msg_down(t, ret))        goto block_message_poll;

			allocate_mshr(t, ret);
			send_read_down(t, ret);
			store_data_to_cache(t, ret);
			set_mshr_valid_bits(t, ret);
			ret->next_state = StateInv_Write;
			
			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			break;
			
		case ActionRecvData:   goto error;
		case ActionReplace:    goto donothing;

		default: goto error;
		}
		break;

	case StateInv_Rd:
		switch(t->action) {
		case ActionIRead:
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			enqueue_request_mshr(t, ret);
			set_icache_inclusion(t, ret);
			ret->next_state = t->curstate;
			
			t->trans->mark_pending(MEM_EVENT_L2_MSHR_PART_HIT);
			break;
			
		case ActionDRead:
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			enqueue_request_mshr(t, ret);
			set_dcache_inclusion(t, ret);
			ret->next_state = t->curstate;
			
			t->trans->mark_pending(MEM_EVENT_L2_MSHR_PART_HIT);
			break;
		
		case ActionDWrite:
			store_data_to_cache(t, ret);
			set_mshr_valid_bits(t, ret);
			ret->next_state = StateInv_Write;

			STAT_INC(stat_num_write_miss);
			STAT_INC(stat_num_write_part_hit_mshr);
			break;

		case ActionRecvData:
			store_data_to_cache_ifnot_valid(t, ret);
			perform_mshr_requests(t, ret);
			deallocate_mshr(t, ret);
			ret->next_state = StateClean;
			break;
		
		case ActionReplace:
			block_message_set_event(t, ret);
			ret->next_state = t->curstate;
			break;

		default: goto error;
		}
		break;

	case StateInv_Write:
		switch(t->action) {
		case ActionIRead:
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			enqueue_request_mshr(t, ret);
			set_icache_inclusion(t, ret);
			ret->next_state = t->curstate;
			
			t->trans->mark_pending(MEM_EVENT_L2_MSHR_PART_HIT);
			break;
			
		case ActionDRead:
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			enqueue_request_mshr(t, ret);
			set_dcache_inclusion(t, ret);
			ret->next_state = t->curstate;
			
			t->trans->mark_pending(MEM_EVENT_L2_MSHR_PART_HIT);
			break;
		
		case ActionDWrite:
			store_data_to_cache(t, ret);
			set_mshr_valid_bits(t, ret);
			ret->next_state = t->curstate;

			STAT_INC(stat_num_write_miss);
			STAT_INC(stat_num_write_part_hit_mshr);
			break;

		case ActionRecvData:
			store_data_to_cache_ifnot_valid(t, ret);
			perform_mshr_requests(t, ret);
			deallocate_mshr(t, ret);
			ret->next_state = StateDirty;
			break;
		
		case ActionReplace:
			block_message_set_event(t, ret);
			ret->next_state = t->curstate;
			break;

		default: goto error;
		}
		break;

	case StateClean:
		switch(t->action) {
		case ActionIRead:
			send_data_to_icache(t, ret);
			set_icache_inclusion(t, ret);
			ret->next_state = t->curstate;

			STAT_INC(stat_num_read_hit);
			break;
			
		case ActionDRead:
			send_data_to_dcache(t, ret);
			set_dcache_inclusion(t, ret);
			ret->next_state = t->curstate;

			STAT_INC(stat_num_read_hit);
			break;
			
		case ActionDWrite:
			store_data_to_cache(t, ret);
			send_invalidate_to_icache(t, ret);
			ret->next_state = StateDirty;

			STAT_INC(stat_num_write_hit);
			break;

		case ActionRecvData:   goto error;

		case ActionReplace:
			send_invalidate_to_icache(t, ret);
			send_invalidate_to_dcache(t, ret);
			unset_icache_inclusion(t, ret);
			unset_dcache_inclusion(t, ret);
			ret->next_state = StateInvalid;
			break;

		default: goto error;
		}
		break;
		
	case StateDirty:
		switch(t->action) {
		case ActionIRead:
			send_data_to_icache(t, ret);
			set_icache_inclusion(t, ret);
			ret->next_state = t->curstate;

			STAT_INC(stat_num_read_hit);
			break;
			
		case ActionDRead:
			send_data_to_dcache(t, ret);
			set_dcache_inclusion(t, ret);
			ret->next_state = t->curstate;

			STAT_INC(stat_num_read_hit);
			break;
			
		case ActionDWrite:
			store_data_to_cache(t, ret);
			send_invalidate_to_icache(t, ret);
			ret->next_state = t->curstate;

			STAT_INC(stat_num_write_hit);
			break;

		case ActionRecvData:   goto error;

		case ActionReplace:
			send_invalidate_to_icache(t, ret);
			send_invalidate_to_dcache(t, ret);
			unset_icache_inclusion(t, ret);
			unset_dcache_inclusion(t, ret);
			send_writeback_down(t, ret);
			ret->next_state = StateInvalid;
			break;

		default: goto error;
		}
		break;
		
	default:
		ASSERT(0);

	}

	ASSERT(ret->next_state != StateUnknown);
	// May not have line if in NotPresent
	if (t->line) t->line->set_state(ret->next_state);
	else ASSERT(t->curstate == StateNotPresent);

	if (ret->block_message != StallNone) {
		if (t->action == ActionIRead || t->action == ActionDRead)
			STAT_INC(stat_num_read_stall);
		else if (t->action == ActionDWrite) STAT_INC(stat_num_write_stall);
	}
	
	profile_transition(t, ret);
	return;


/// Blocking Actions 
block_message_poll:
	if (ret->block_message == StallNone) ret->block_message = StallPoll;
	// fallthrough
block_message_other:
	if (ret->block_message == StallNone) ret->block_message = StallOtherEvent;
	// fallthrough
block_message:
	ret->next_state = t->curstate;
	t->trans->mark_pending(MEM_EVENT_L1DIR_STALL);
    DEBUG_TR_FN("Blocking Message block_type %u", t, ret, ret->block_message);
	profile_transition(t, ret);
	return;
	
// Common actions	
error:
	FAIL_MSG("%10s 0x%016llx:Invalid action: %s in state: %s\n",get_cname(), t->message->address,
	         action_names[t->action][0].c_str(), state_names[t->curstate][0].c_str());
	
donothing:
	ret->next_state = t->curstate;
	ASSERT(ret->next_state != StateUnknown);
	// May not have line if in NotPresent
	if (t->line) t->line->set_state(ret->next_state);
	else ASSERT(t->curstate == StateNotPresent);

	profile_transition(t, ret);
	return;
}

////////////////////////////////////////////////////////////////////////////////
// Multiprocessor onelevel cache specific transition functions

bool
unip_two_l2_cache_t::can_send_msg_down(tfn_info_t *t, tfn_ret_t *ret)
{
	return true;
}

// Send read requests down
bool
unip_two_l2_cache_t::send_read_down(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("send_read_down", t, ret);
	
	// NOTE: Won't match incoming message size/addr for writebacks
	addr_t addr = get_line_address(t->address);
	
	// NOTE: message type doesn't matter much for this protocol
    downmsg_t *msg = new downmsg_t(t->trans, addr, get_lsize(), NULL,
		downmsg_t::ReadEx);

	stall_status_t retval = send_message(links[mainmem_link], msg);
	if (retval != StallNone) delete msg;

	return (retval == StallNone);
}

// Send writethrough down
bool
unip_two_l2_cache_t::send_writeback_down(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("send_writetback", t, ret);
	
	// NOTE:  Address of line will not match message that triggered repl.
	addr_t addr = get_line_address(t->address);
	
    downmsg_t *msg = new downmsg_t(t->trans, addr, get_lsize(),
		t->line->get_data(), downmsg_t::WriteBack);
	
	stall_status_t retval = send_message(links[mainmem_link], msg);
	if (retval != StallNone) delete msg;
	
	return (retval == StallNone);
}

// Send data response back to icache
void
unip_two_l2_cache_t::send_data_to_icache(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("send_data_to_icache", t, ret);
				
	addr_t addr = t->address;
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);
	ASSERT(get_lsize() == t->message->size);
	
	const uint8 *data = t->line->get_data();
	
	upmsg_t *msg = new upmsg_t(t->trans, addr, get_lsize(), data,
		upmsg_t::TypeDataResp);
	
	stall_status_t retval = send_message(links[icache_link], msg);
	ASSERT(retval == StallNone);  // Prot doesn't handle inability to send message
}

// Send data response back to dcache
void
unip_two_l2_cache_t::send_data_to_dcache(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("send_data_to_dcache", t, ret);
				
	addr_t addr = get_line_address(t->address);
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);
	ASSERT(get_lsize() == t->message->size);
	
	const uint8 *data = t->line->get_data();
	
	upmsg_t *msg = new upmsg_t(t->trans, addr, get_lsize(), data,
		upmsg_t::TypeDataResp);
	
	stall_status_t retval = send_message(links[dcache_link], msg);
	ASSERT(retval == StallNone);  // Prot doesn't handle inability to send message
}

// Send invalidate to icache if inclusion bit set
void
unip_two_l2_cache_t::send_invalidate_to_dcache(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("send_invalidate_to_dcache", t, ret);
	
	if (!t->line->get_dcache_inclusion()) return;
	
	// NOTE:  Address of line will not match message that triggered repl.
	addr_t addr = get_line_address(t->address);
	
	upmsg_t *msg = new upmsg_t(t->trans, addr, get_lsize(), NULL,
		upmsg_t::TypeInvalidate);
	
	stall_status_t retval = send_message(links[dcache_link], msg);
	ASSERT(retval == StallNone);  // Prot doesn't handle inability to send message
}

// Send invalidate to icache if inclusion bit set
void
unip_two_l2_cache_t::send_invalidate_to_icache(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("send_invalidate_to_icache", t, ret);
	
	if (!t->line->get_icache_inclusion()) return;
	
	// NOTE:  Address of line will not match message that triggered repl.
	addr_t addr = get_line_address(t->address);
	
	upmsg_t *msg = new upmsg_t(t->trans, addr, get_lsize(), NULL,
		upmsg_t::TypeInvalidate);
	
	stall_status_t retval = send_message(links[icache_link], msg);
	ASSERT(retval == StallNone);  // Prot doesn't handle inability to send message
}

// Set Icache Inclusion
void
unip_two_l2_cache_t::set_icache_inclusion(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("set_icache_inclusion", t, ret);
	t->line->set_icache_inclusion(true);
}

// Set Dcache Inclusion
void
unip_two_l2_cache_t::set_dcache_inclusion(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("set_dcache_inclusion", t, ret);
	t->line->set_dcache_inclusion(true);
}

// UnSet Icache Inclusion
void
unip_two_l2_cache_t::unset_icache_inclusion(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("unset_icache_inclusion", t, ret);
	t->line->set_icache_inclusion(false);
}

// UnSet Dcache Inclusion
void
unip_two_l2_cache_t::unset_dcache_inclusion(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("unset_dcache_inclusion", t, ret);
	t->line->set_dcache_inclusion(false);
}

// Enqueue this read request to the MSHR for this address
bool
unip_two_l2_cache_t::enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("enqueue_request_mshr", t, ret);

	addr_t addr = get_line_address(t->address);
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);

	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);
	
	mshr_type_t *type = new mshr_type_t;
	if (t->action == ActionIRead) *type = MSHR_IRead;
	else if (t->action == ActionDRead) *type = MSHR_DRead;
	else if (t->action == ActionDWrite) *type = MSHR_DWrite;
	else FAIL;
	
	return (mshr->enqueue_request(type));
}

// Send response back to processor using MSHR trans*
void
unip_two_l2_cache_t::perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret)
{
	addr_t addr = get_line_address(t->address);
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);

	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);

	DEBUG_TR_FN("perform_mshr_requests", t, ret);

	mshr_type_t *request_type = mshr->pop_request();
	// There might not be an enqueued request if caused by a WriteThrough 
	
	const uint8 *data = NULL;
	uint8 link;
	
	while (request_type) {
		if (*request_type == MSHR_IRead) link = icache_link;
		else if (*request_type == MSHR_DRead) link = dcache_link;
		else FAIL;
		
		DEBUG_TR_FN("perform_mshr_requests: sending DataResp to %s",
		            t, ret, (link == icache_link) ? "ICache" : "DCache");

		data = t->line->get_data();

		upmsg_t *msg = new upmsg_t(t->trans, addr, get_lsize(), data,
			upmsg_t::TypeDataResp);

		stall_status_t retval = send_message(links[link], msg);
		ASSERT(retval == StallNone);  // Prot doesn't handle inability to send message

		// Pop next enqueued request
		request_type = mshr->pop_request();
	}
	
}

////////////////////////////////////////////////////////////////////////////////
// Uniprocessor onelevel cache constant definitions

const string unip_two_l2_protocol_t::action_names[][2] = {
	{ "Replace", "Replace Line" },
	{ "IRead", "I-Cache Read" }, 
	{ "DRead", "D-Cache Read" }, 
	{ "DWrite", "D-Cache Writethrough" }, 
	{ "RecvData", "Receive Data" }, 
	{ "Flush", "Flush Line" },
};
const string unip_two_l2_protocol_t::state_names[][2] = {
	{ "Unknown", "Unknown" },
	{ "NotPresent", "Line is not present in cache" },
	{ "Invalid", "Data may be present but is invalid" },
	{ "Inv_Rd", "Line was invalid, issued a read request, waiting for data" },
	{ "Inv_Write", "Line was invalid, got a writethrough, issued a read request, waiting for data" },
	{ "Clean", "Line is present and clean" },
	{ "Dirty", "Line is present and dirty" },
};

const string unip_two_l2_protocol_t::prot_name[] = 
	{"unip_two_l2", "Two-level Uniprocessor L2 Cache"};
