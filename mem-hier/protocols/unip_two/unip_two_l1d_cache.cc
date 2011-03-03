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
 
 
/* This implements a write-through, write-no-allocate L1 data cache
   
   Links:  0 - Processor,
           1 - L2Cache,
 
TODO: FIX stats
      Prefetching support
*/
 
//  #include "simics/first.h"
// RCSID("$Id: unip_two_l1d_cache.cc,v 1.1.2.2 2005/09/13 14:16:33 kchak Exp $");

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

#include "unip_two_l1d_cache.h"


// Link definitions
const uint8 unip_two_l1d_protocol_t::proc_link;  
const uint8 unip_two_l1d_protocol_t::l2_link;  
const uint8 unip_two_l1d_protocol_t::io_link;  

/////////////////////////////////////////////////////////////////////////////////
// Uniprocessor one-level line functions
//
void
unip_two_l1d_line_t::init(cache_t *_cache, addr_t _idx, uint32 _lsize)
{
	line_t::init(_cache, _idx, _lsize);
	state = StateInvalid;
}

void
unip_two_l1d_line_t::reinit(addr_t _tag)
{
	line_t::reinit(_tag);
	state = StateInvalid;
}	
	
bool
unip_two_l1d_line_t::is_free()
{
	return (state == StateInvalid);
}	
	
bool
unip_two_l1d_line_t::can_replace()
{
	// Can replace if not in a busy state
	return (state == StateInvalid ||
			state == StatePresent);
}

bool
unip_two_l1d_line_t::is_stable()
{
	// Same as can_replace
	return (can_replace());
}

void
unip_two_l1d_line_t::set_state(uint32 _state)
{
	state = _state;
}
	
uint32
unip_two_l1d_line_t::get_state()
{
	return state;
}

void
unip_two_l1d_line_t::to_file(FILE *file)
{
	fprintf(file, "%d ", (int) state);
	line_t::to_file(file);
}

void
unip_two_l1d_line_t::from_file(FILE *file)
{
	fscanf(file, "%d ", (int *) &state);
	line_t::from_file(file);
}

/////////////////////////////////////////////////////////////////////////////////
// Uniprocessor one-level cache functions
//

unip_two_l1d_cache_t::unip_two_l1d_cache_t(string name, cache_config_t *conf, 
    uint32 _id, uint64 prefetch_alg)
	: tcache_t<unip_two_l1d_protocol_t>
		      (name, 2, conf, _id, prefetch_alg) // Two links for this protocol
{ }

uint32
unip_two_l1d_cache_t::get_action(message_t *msg)
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

		switch(upmsg->get_type()) {
		case simple_proc_msg_t::ProcRead:
			return ActionProcRead;
			
		case simple_proc_msg_t::ProcWrite:
		case simple_proc_msg_t::StorePref:
			return ActionProcWrite;
			
		default:
			FAIL_MSG("Invalid message type %s\n",
					 upmsg_t::names[(int)upmsg->get_type()][0].c_str());
		}
	} else if (downmsg) {
		ASSERT(upmsg == NULL);
		
		switch(downmsg->type) {
		case TypeDataResp:
			return ActionRecvData;
			
		case TypeInvalidate:
			return ActionRecvInv;
			
		default:
			FAIL_MSG("Invalid message type %s\n",
					 downmsg_t::names[(int)downmsg->type][0].c_str());
		}
	} else {
		FAIL;
	}
}

void
unip_two_l1d_cache_t::trigger(tfn_info_t *t, tfn_ret_t *ret)
{
	// Set ret->block_message if incoming request should be blocked and
	// presumably retried next cycle
	ret->block_message = StallNone;
	ret->next_state = StateUnknown;

	switch(t->curstate) {

	case StateNotPresent:
		switch(t->action) {
		case ActionProcRead:
			ret->block_message = can_insert_new_line(t, ret);
			if (ret->block_message != StallNone)   goto block_message;
			if (!can_allocate_mshr(t, ret))        goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			if (!can_send_msg_down(t, ret))        goto block_message_poll;

			insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_read_down(t, ret);
			ret->next_state = StateInv_Rd;

			t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
			break;

		case ActionProcWrite:
			if (!can_send_msg_down(t, ret))        goto block_message_poll;
			send_writethrough_down(t, ret);
			send_request_complete_up(t, ret);
			ret->next_state = t->curstate;
			t->trans->mark_pending(MEM_EVENT_L1_WRITE_BACK); // not really
			break;

		case ActionRecvData:   goto error;
		case ActionRecvInv:    goto donothing;
		case ActionReplace:    goto donothing;

		default: goto error;
		}
		break;

	case StateInvalid:
		switch(t->action) {
		case ActionProcRead:
			if (!can_allocate_mshr(t, ret))        goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			if (!can_send_msg_down(t, ret))        goto block_message_poll;

			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_read_down(t, ret);
			ret->next_state = StateInv_Rd;
			t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
			break;
			
		case ActionProcWrite:
			if (!can_send_msg_down(t, ret))        goto block_message_poll;
			send_writethrough_down(t, ret);
			send_request_complete_up(t, ret);
			ret->next_state = t->curstate;
			t->trans->mark_pending(MEM_EVENT_L1_WRITE_BACK); // not really
			break;

		case ActionRecvData:   goto error;
		case ActionRecvInv:    goto donothing;
		case ActionReplace:    goto donothing;

		default: goto error;
		}
		break;

	case StateInv_Rd:
		switch(t->action) {
		case ActionProcRead:
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			enqueue_request_mshr(t, ret);
			ret->next_state = t->curstate;
			t->trans->mark_pending(MEM_EVENT_L1_MSHR_PART_HIT);
			break;

		case ActionProcWrite:
			if (!can_send_msg_down(t, ret))        goto block_message_poll;
			store_data_to_cache(t, ret);
			send_writethrough_down(t, ret);
			send_request_complete_up(t, ret);
			ret->next_state = t->curstate;
			t->trans->mark_pending(MEM_EVENT_L1_WRITE_BACK); // not really
			break;

		case ActionRecvData:
			store_data_to_cache_ifnot_valid(t, ret);
			perform_mshr_requests(t, ret);
			deallocate_mshr(t, ret);
			ret->next_state = StatePresent;
			break;
		
		case ActionRecvInv:    goto donothing;
		case ActionReplace:
			block_message_set_event(t, ret);
			ret->next_state = t->curstate;
			break;

		default: goto error;
		}
		break;

	case StatePresent:
		switch(t->action) {
		case ActionProcRead:
			send_data_up(t, ret);
			ret->next_state = t->curstate;

			STAT_INC(stat_num_read_hit);
			break;
			
		case ActionProcWrite:
			if (!can_send_msg_down(t, ret))        goto block_message_poll;
			store_data_to_cache(t, ret);
			send_writethrough_down(t, ret);
			send_request_complete_up(t, ret);
			ret->next_state = t->curstate;
			t->trans->mark_pending(MEM_EVENT_L1_HIT);
			break;

		case ActionRecvData:   goto error;

		case ActionRecvInv:
			ret->next_state = StateInvalid;
			break;
			
		case ActionReplace:
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
		if (t->action == ActionProcRead) STAT_INC(stat_num_read_stall);
		else if (t->action == ActionProcWrite) STAT_INC(stat_num_write_stall);
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
unip_two_l1d_cache_t::can_send_msg_down(tfn_info_t *t, tfn_ret_t *ret)
{
	return true;
}

// Send read requests down
bool
unip_two_l1d_cache_t::send_read_down(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("send_read_down", t, ret);
	
	addr_t addr = get_line_address(t->address);
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == get_line_address(t->message->address));

    downmsg_t *msg = new downmsg_t(t->trans, addr, get_lsize(), NULL,
		downmsg_t::TypeRead);
	
	stall_status_t retval = send_message(links[l2_link], msg);
	if (retval != StallNone) delete msg;

	return (retval == StallNone);
}

// Send writethrough down
bool
unip_two_l1d_cache_t::send_writethrough_down(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("send_writethrough_down", t, ret);
	
	// NOTE:  send address and size of store, not of line!
	// NOTE:  line may be NULL if not present
	addr_t addr = t->address;
	size_t size = t->message->size;
	ASSERT(addr == t->message->address);
	ASSERT(!t->line || t->line->get_address() == get_line_address(t->address));

    downmsg_t *msg = new downmsg_t(t->trans, addr, size, t->message->data,
		downmsg_t::TypeWrite);
	
	stall_status_t retval = send_message(links[l2_link], msg);
	if (retval != StallNone) delete msg;
	
	return (retval == StallNone);
}

// Send data response back to processor using incoming message *
void
unip_two_l1d_cache_t::send_data_up(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("send_data_up 0x%016llx", t, ret,
	            t->address);
				
	// NOTE:  send address and size of request, not of line
	addr_t addr = t->address;
	size_t size = t->message->size;
	ASSERT(addr == t->message->address);
	ASSERT(get_line_address(addr) == t->line->get_address());
	
	upmsg_t *recv_msg = static_cast <upmsg_t *>(t->message);
	mem_trans_t *trans = recv_msg->get_trans();
	ASSERT(addr == trans->phys_addr);
	ASSERT(size == trans->size);
	
	const uint8 *data = &t->line->get_data()[get_offset(addr)];
	
	upmsg_t *msg = new upmsg_t(trans, addr, size, data, upmsg_t::DataResp);
	
	stall_status_t retval = send_message(links[proc_link], msg);
	ASSERT(retval == StallNone);  // Prot doesn't handle inability to send message
}

// Send request complete message back to processor using incoming message*
void
unip_two_l1d_cache_t::send_request_complete_up(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("send_request_complete_up 0x%016llx", t, ret,
	            t->message->address);
	
	// NOTE:  line may be NULL if not present
	addr_t addr = t->address;
	size_t size = t->message->size;
	ASSERT(addr == t->message->address);
	ASSERT(!t->line || t->line->get_address() == get_line_address(t->address));
	
	upmsg_t *recv_msg = static_cast <upmsg_t *>(t->message);
	mem_trans_t *trans = recv_msg->get_trans();
	ASSERT(addr == trans->phys_addr);
	ASSERT(size == trans->size);
	
	upmsg_t *msg = new upmsg_t(trans, addr, size, NULL, upmsg_t::ReqComplete);
	
	stall_status_t retval = send_message(links[proc_link], msg);
	ASSERT(retval == StallNone);  // Prot doesn't handle inability to send message
}

// Enqueue this read request to the MSHR for this address
bool
unip_two_l1d_cache_t::enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("enqueue_request_mshr", t, ret);

	addr_t addr = get_line_address(t->address);
	ASSERT(addr == get_line_address(t->message->address));
	ASSERT(addr == t->line->get_address());

	mshr_t<mem_trans_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);

	upmsg_t *msg = static_cast <upmsg_t *>(t->message);
	mem_trans_t *trans = msg->get_trans();
	ASSERT(trans);

	return (mshr->enqueue_request(trans));
}

// Send response back to processor using MSHR trans*
void
unip_two_l1d_cache_t::perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret)
{
	addr_t addr = get_line_address(t->address);
	ASSERT(addr == get_line_address(t->message->address));
	ASSERT(addr == t->line->get_address());

	mshr_t<mem_trans_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);

	DEBUG_TR_FN("perform_mshr_requests", t, ret);

	mem_trans_t *enqueued_trans = mshr->pop_request();
	ASSERT(enqueued_trans);

	const uint8 *data = NULL;
	uint32 type;

	while (enqueued_trans) {
		ASSERT(get_line_address(enqueued_trans->phys_addr) == 
	           get_line_address(t->address));
		ASSERT(get_line_address(enqueued_trans->phys_addr) ==
		       t->line->get_address());

		if (enqueued_trans->read){
			DEBUG_TR_FN("perform_mshr_requests: 0x%016llx sending DataResp",
			            t, ret, enqueued_trans->phys_addr);

			data = &t->line->get_data()[get_offset(enqueued_trans->phys_addr)];
			type = upmsg_t::DataResp;
		} else {
			DEBUG_TR_FN("perform_mshr_requests: 0x%016llx sending ReqComplete",
			            t, ret, enqueued_trans->phys_addr);

			data = NULL;
			type = upmsg_t::ReqComplete;
		}

		upmsg_t *msg = new upmsg_t(enqueued_trans, enqueued_trans->phys_addr,
			enqueued_trans->size, data, type);

		stall_status_t retval = send_message(links[proc_link], msg);
		ASSERT(retval == StallNone);  // Prot doesn't handle inability to send message
		
		// Pop next enqueued request
		enqueued_trans = mshr->pop_request();
	}
	
}

////////////////////////////////////////////////////////////////////////////////
// Uniprocessor onelevel cache constant definitions

const string unip_two_l1d_protocol_t::action_names[][2] = {
	{ "Replace", "Replace Line" },
	{ "ProcRead", "Processor Read" }, 
	{ "ProcWrite", "Processor Write" }, 
	{ "RecvData", "Receive Data" }, 
	{ "RecvInv", "Receive Invalidate" }, 
	{ "Prefetch", "Prefetch" }, 
	{ "Flush", "Flush" }, 
};
const string unip_two_l1d_protocol_t::state_names[][2] = {
	{ "Unknown", "Unknown" },
	{ "NotPresent", "Line is not present in cache" },
	{ "Invalid", "Data may be present but is invalid" },
	{ "Inv_Rd", "Line was invalid, issued a read request, waiting for data" },
	{ "Present", "Line is present and valid" },
};

const string unip_two_l1d_protocol_t::prot_name[] = 
	{"unip_two_l1d", "Two-level Uniprocessor L1 D-Cache"};
