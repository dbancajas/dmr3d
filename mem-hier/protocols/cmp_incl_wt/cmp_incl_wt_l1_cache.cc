/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    implementation of a two-level CMP L1 cache
 * initial author: Koushik Chakraborty 
 *
 */
 
 
//  #include "simics/first.h"
// RCSID("$Id: cmp_incl_wt_l1_cache.cc,v 1.1.2.31 2005/11/22 19:33:15 kchak Exp $");

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

#include "cmp_incl_wt_l1_cache.h"
#include "cmp_incl_wt_l2_cache.h"

/////////////////////////////////////////////////////////////////////////////////
// Uniprocessor one-level line functions
//
void
cmp_incl_wt_l1_line_t::init(cache_t *_cache, addr_t _idx, uint32 _lsize)
{
	line_t::init(_cache, _idx, _lsize);
	state = StateInvalid;
	dirty = false;
}

void
cmp_incl_wt_l1_line_t::wb_init(addr_t tag, addr_t idx, line_t *cache_line)
{
	line_t::wb_init(tag, idx, cache_line);
    dirty = true;
    state = StateShToInv;
}

void
cmp_incl_wt_l1_line_t::reinit(addr_t _tag)
{
	line_t::reinit(_tag);
	state = StateInvalid;
	dirty = false;
}	
	
bool
cmp_incl_wt_l1_line_t::is_free()
{
	return (state == StateInvalid);
}	
	
bool
cmp_incl_wt_l1_line_t::can_replace()
{
	// Can replace if not in a busy state
	return (state == StateInvalid ||
			state == StateNotPresent ||
			state == StateShared);
}

bool
cmp_incl_wt_l1_line_t::is_stable()
{
	// Same as can_replace
	return (can_replace());
}

void
cmp_incl_wt_l1_line_t::set_state(uint32 _state)
{
	state = _state;
}
	
uint32
cmp_incl_wt_l1_line_t::get_state()
{
	return state;
}

void
cmp_incl_wt_l1_line_t::set_dirty(bool _dirty)
{
	dirty = _dirty;
}
	
bool
cmp_incl_wt_l1_line_t::is_dirty()
{
	return dirty;
}

	
void
cmp_incl_wt_l1_line_t::to_file(FILE *file)
{
	fprintf(file, "%d %u ", dirty ? 1 : 0, state);
	line_t::to_file(file);
}

void
cmp_incl_wt_l1_line_t::from_file(FILE *file)
{
	int temp_dirty;
	fscanf(file, "%d %u ", &temp_dirty, &state);
	dirty = (temp_dirty == 1);

	line_t::from_file(file);
}

/////////////////////////////////////////////////////////////////////////////////
// CMP Inclusion protocol L1 cache functions
//

cmp_incl_wt_l1_cache_t::cmp_incl_wt_l1_cache_t(string name, cache_config_t *conf,
	uint32 _id, conf_object_t *_cpu, uint64 prefetch_alg)
	: tcache_t<cmp_incl_wt_l1_protocol_t> (name, 2, conf, _id, prefetch_alg), cpu(_cpu),
    icache_device(false)
	// Two links for this protocol
{ 
    
}

uint32
cmp_incl_wt_l1_cache_t::get_action(message_t *msg)
{
    bool t_state = g_conf_tstate_l1_bypass ? 
        msg->get_trans()->is_thread_state() : false;
	
	if (is_up_message(msg)) {
		upmsg_t *upmsg = static_cast<upmsg_t *> (msg);

		switch(upmsg->get_type()) {
		case simple_proc_msg_t::ProcRead:
		case simple_proc_msg_t::ReadPref: // temp
            if (t_state)
                return ActionTSTLD;
			return ActionProcLD;
			
		case simple_proc_msg_t::ProcWrite:
		case simple_proc_msg_t::StorePref: // temp
            if (t_state)
                return ActionTSTST;
			return ActionProcST;
			
		
		default:
			FAIL_MSG("Invalid message type %s\n",
					 upmsg_t::names[(int)upmsg->get_type()][0].c_str());
		}
	} else if (is_down_message(msg)) {
		downmsg_t *downmsg = static_cast<downmsg_t *> (msg);
		
		switch(downmsg->get_type()) {
            
        case TypeDataResp:    
            if (t_state) return ActionTSTResp;
            return ActionDataResp;
        case TypeInvalidate:
            return ActionInvalidate;
         case TypeWriteComplete:
            return ActionWriteComplete;
		default:
			FAIL_MSG("Invalid message type %s %u %u\n",
					 downmsg_t::names[(int)downmsg->get_type()][0].c_str(),
                     downmsg->get_source(), downmsg->get_dest());
		}
	} else {
		FAIL;
	}
}

void
cmp_incl_wt_l1_cache_t::trigger(tfn_info_t *t, tfn_ret_t *ret)
{
	ret->block_message = StallNone;
	ret->next_state = StateUnknown;
    
    // ASSERT(!t->trans->completed || t->trans->get_pending_events());
    
    for (uint32 i = 0; i < numwb_buffers; i++) {
		if (wb_lines[i].get_tag() || wb_lines[i].get_index())
			DEBUG_TR_FN("tag %llu index %llu", t, ret, wb_lines[i].get_tag(),
                 wb_lines[i].get_index());
	}
	
	switch(t->curstate) {

	/// NotPresent
	case StateNotPresent:
		switch(t->action) {
			
		case ActionProcLD:
            ret->block_message = can_insert_new_line(t, ret);
            if (ret->block_message != StallNone) goto block_message;
            if (!can_allocate_mshr(t, ret) || 
                !can_enqueue_request_mshr(t, ret))
            {
                ret->block_message = StallPoll;
                goto block_message;
            }
            insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_protocol_msg_down(t, ret, false, downmsg_t::TypeRead, true, t->trans);
			ret->next_state = StateInvToSh;
            t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
            break;
		
        case ActionProcST:
            ret->block_message = can_insert_new_line(t, ret);
            if (ret->block_message != StallNone) goto block_message;
            if (!can_allocate_mshr(t, ret) || 
                !can_enqueue_request_mshr(t, ret))
            {
                ret->block_message = StallPoll;
                goto block_message;
            }
            insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_protocol_msg_down(t, ret, false, downmsg_t::TypeReadEx, true, t->trans);
			ret->next_state = StateInvToSh;
            t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
            break;
		case ActionInvalidate: 
			goto donothing;
			//send_protocol_msg_down(t, ret, false, downmsg_t::TypeInvAck, true);
            //ret->next_state = t->curstate;
            //break;
        // case ActionReadFwd:
        // case ActionReadExFwd:
            // send_protocol_msg_down(t, ret, false, downmsg_t::TypeFwdNack, true);
            // ret->next_state = t->curstate;
            // break;
        case ActionTSTLD:
            t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
            send_protocol_msg_down(t, ret, false, downmsg_t::TypeRead, true, t->trans);
            ret->next_state = t->curstate;
            break;
        case ActionTSTST:
            t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
            send_protocol_msg_down(t, ret, false, downmsg_t::TypeReadEx, true, t->trans);
            ret->next_state = t->curstate;
            break;
        case ActionTSTResp:
            t->trans->clear_pending_event(MEM_EVENT_L1_DEMAND_MISS);
            send_protocol_msg_up(t, ret, simple_proc_msg_t::DataResp, t->trans);
            ret->next_state = t->curstate;
            break;    
		default: goto error;
		}
		break;

	/// Invalid
	case StateInvalid:
		switch(t->action) {
			
		case ActionProcLD:
            if (!can_allocate_mshr(t, ret) || 
                !can_enqueue_request_mshr(t, ret))
            {
                ret->block_message = StallPoll;
                goto block_message;
            }
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_protocol_msg_down(t, ret, false, downmsg_t::TypeRead, true, t->trans);
			ret->next_state = StateInvToSh;
            t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
            break;
            
		case ActionProcST:
        
            if (!can_allocate_mshr(t, ret) || 
                !can_enqueue_request_mshr(t, ret))
            {
                ret->block_message = StallPoll;
                goto block_message;
            }
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_protocol_msg_down(t, ret, false, downmsg_t::TypeReadEx, false, t->trans);
            t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
			ret->next_state = StateInvToSh;
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
            break;
        
		default: goto error;
		}
		break;

	case StateInvToSh:
		switch(t->action) {

		case ActionDataResp:
        case ActionWriteComplete:    
			store_data_to_cache(t, ret);
			perform_mshr_requests(t, ret);
			ret->next_state = StateShared;
            t->trans->clear_pending_event(MEM_EVENT_L1_DEMAND_MISS);
            deallocate_mshr(t, ret);
			break;
        case ActionInvalidate:
            ret->block_message = StallSetEvent;
            goto block_message;
        case ActionProcLD:
        case ActionProcST:
            if (!can_enqueue_request_mshr(t, ret)) {
                ret->block_message = StallPoll;
                goto block_message;
            }
            enqueue_request_mshr(t,ret);
            ret->next_state = t->curstate;
            t->trans->mark_pending(MEM_EVENT_L1_MSHR_PART_HIT);
            break;
		default: goto error;
		}
		break;

	
      case StateShared:
          switch(t->action) {
            case ActionProcLD:
                send_protocol_msg_up(t, ret, upmsg_t::DataResp, t->trans);
                t->trans->mark_event(MEM_EVENT_L1_HIT);
                ret->next_state = t->curstate;
                break;
            case ActionProcST:
                send_protocol_msg_down(t, ret, false, downmsg_t::TypeReadEx, false, t->trans);
                send_protocol_msg_up(t, ret, upmsg_t::DataResp, t->trans);
                t->trans->mark_pending(MEM_EVENT_L1_HIT);
                ret->next_state = StatePendingWrite;
                break;
            case ActionReplace:
                send_protocol_msg_down(t, ret, false, downmsg_t::TypeReplace, true, t->trans);
                ret->next_state = StateInvalid;
                break;
            case ActionInvalidate:
                send_protocol_msg_up(t, ret, upmsg_t::ProcInvalidate, t->trans);
                send_protocol_msg_down(t, ret, false, downmsg_t::TypeInvAck, false, t->trans);
                ret->next_state = StateInvalid;
                break;
            default: goto donothing;
          }
          break;
          
     
      case StatePendingWrite:
          switch(t->action) {
            case ActionProcLD:
                send_protocol_msg_up(t, ret, upmsg_t::DataResp, t->trans);
                t->trans->mark_event(MEM_EVENT_L1_HIT);
                ret->next_state = t->curstate;
                break;
            case ActionProcST:
                ret->block_message = StallSetEvent;
                goto block_message;
            case ActionWriteComplete:
                t->trans->clear_pending_event(MEM_EVENT_L1_HIT);
                ret->next_state = StateShared;
                break;
            default: goto error;
          }
          break;
                
       case StateShToInv:
        switch(t->action) {
            case ActionProcLD:
            case ActionProcST:
                ret->block_message = StallSetEvent;
                goto block_message;
            case ActionInvalidate:
                t->trans->clear_pending_event(MEM_EVENT_L2_REPLACE);
                release_writebackbuffer(get_line_address(t->address));
                ret->next_state = StateInvalid;
                break;
            
            default : goto error;
        }
        break;
	
	default:
		ASSERT(0);

	}


	/// Common return case
	ASSERT(ret->next_state != StateUnknown);
	// May not have line if in NotPresent
	if (t->line) t->line->set_state(ret->next_state);
	else ASSERT(t->curstate == StateNotPresent);
    
    t->trans->clear_pending_event(MEM_EVENT_L1_STALL);
	profile_transition(t, ret);
	return;


block_message:
	ret->next_state = t->curstate;
	t->trans->mark_pending(MEM_EVENT_L1_STALL);
    DEBUG_TR_FN("%s Blocking Message %u", t, ret, __func__, ret->block_message);
	profile_transition(t, ret);
	return;
	

/// Other common actions	
error:
	FAIL_MSG("%10s @ %llu 0x%016llx:Invalid action: %s in state: %s\n",get_cname(), 
		external::get_current_cycle(), t->message->address, action_names[t->action][0].c_str(),
		state_names[t->curstate][0].c_str());
	
donothing:
	ret->next_state = t->curstate;
	profile_transition(t, ret);
	return;
}

////////////////////////////////////////////////////////////////////////////////
// Helper functions
void cmp_incl_wt_l1_cache_t::
send_protocol_msg_down(tfn_info_t *t, tfn_ret_t *ret, bool datafwd, 
    uint32 msgtype, bool addrmsg, mem_trans_t *trans)
{
    DEBUG_TR_FN("%s sending type %s", t, ret, __func__, 
        downmsg_t::names[msgtype][0].c_str());
    addr_t addr = get_line_address(t->address);
    uint32 dest = l2_cache_ptr->get_interconnect_id(addr);;
    uint32 request_id = interconnect_id;
    
    if (datafwd || msgtype == TypeInvAck ) {
        downmsg_t *in_msg = static_cast<downmsg_t *>(t->message);
        if (datafwd)
            dest = in_msg->get_requester();
        request_id = in_msg->get_requester();
    }
    
    downmsg_t *msg = new downmsg_t(trans, addr, get_lsize(), NULL,
        interconnect_id, dest, msgtype, request_id, addrmsg);
    stall_status_t retval = send_message(links[cmp_incl_wt_l1_protocol_t::interconnect_link], msg);
    ASSERT(retval == StallNone);    
    
}

void cmp_incl_wt_l1_cache_t::
send_protocol_msg_up(tfn_info_t *t, tfn_ret_t *ret, uint32 msgtype, mem_trans_t *tr)
{
    DEBUG_TR_FN("%s", t, ret, __func__);
    addr_t addr = get_line_address(t->address);
    addr_t proc_addr = tr->phys_addr;
    if (msgtype == upmsg_t::ProcInvalidate)
        proc_addr = addr;
    ASSERT(addr == get_line_address(proc_addr));
    const uint8 *data = (g_conf_cache_data && t->line) ? 
		&t->line->get_data()[get_offset(addr)] : NULL;
    upmsg_t *msg = new upmsg_t(tr, proc_addr, tr->size, data, msgtype);
    stall_status_t retval = send_message(links[cmp_incl_wt_l1_protocol_t::proc_link], msg);
    ASSERT(retval == StallNone);
}
// Enqueue this read request to the MSHR for this address
void
cmp_incl_wt_l1_cache_t::enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("enqueue_request_mshr", t, ret);

	addr_t addr = get_line_address(t->address);
	ASSERT(addr == t->line->get_address());
	//ASSERT(addr == t->message->address);

	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);
	
	mshr_type_t *type = new mshr_type_t;
    type->trans = t->trans;
    //type->request_type = (t->trans->write) ? ActionProcST : ActionProcLD;
	mshr->enqueue_request(type);
}

// Send response back to processor using MSHR trans*
void
cmp_incl_wt_l1_cache_t::perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret)
{
	addr_t addr = get_line_address(t->address);
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);

	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);

	DEBUG_TR_FN("perform_mshr_requests", t, ret);

    
    mshr_type_t *request = mshr->pop_request();
	ASSERT(request);
	
	while (request) {
		// Need to change the transaction in the transfer function
        //t->trans=request->trans;
		DEBUG_TR_FN("%s: sending %s to %d", t, ret, __func__, 
			simple_proc_msg_t::names[simple_proc_msg_t::DataResp][0].c_str(),
			interconnect_id);
	
		request->trans->clear_pending_event(
			MEM_EVENT_L1_DEMAND_MISS | MEM_EVENT_L1_COHER_MISS | 
			MEM_EVENT_L1_HIT | MEM_EVENT_L1_STALL | MEM_EVENT_L1_MSHR_PART_HIT); 

		send_protocol_msg_up(t, ret, simple_proc_msg_t::DataResp, request->trans);
        delete request;
        request = mshr->pop_request();
	}
	
}


mem_trans_t *
cmp_incl_wt_l1_cache_t::get_enqueued_trans(tfn_info_t *t, tfn_ret_t *ret)
{
	addr_t addr = get_line_address(t->address);
	ASSERT(addr == t->line->get_address());
	ASSERT(addr == t->message->address);

	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);

	DEBUG_TR_FN("%s", t, ret, __func__);

    
    mshr_type_t *request = mshr->pop_request();
	ASSERT(request);
	mshr->enqueue_request(request);
    return request->trans;
}


void cmp_incl_wt_l1_cache_t::set_L2_cache_ptr(cmp_incl_wt_l2_cache_t *L2)
{
    l2_cache_ptr = L2;
    icache_device = (interconnect_id >= mem_hier_t::ptr()->get_num_processors());
}

void
cmp_incl_wt_l1_cache_t::prefetch_helper(addr_t message_addr, mem_trans_t *trans,
	bool read)
{
    upmsg_t *msg = new upmsg_t(trans,  message_addr, get_lsize(), NULL,
        read ? upmsg_t::ReadPref : upmsg_t::StorePref);
        
    message_arrival(msg);
}

////////////////////////////////////////////////////////////////////////////////
// Uniprocessor onelevel cache constant definitions

const string cmp_incl_wt_l1_protocol_t::action_names[][2] = {
	{ "Replace", "" }, 
	{ "ProcLD", "" }, 
	{ "ProcSt", "" }, 
	{ "DataShared", "" }, 
	{ "DataExclusive", "" },
	{ "UpgradeAck", "" },
	{ "Invalidate", "" },
	{ "WriteBackAck", "" },
    { "ReplaceAck", ""},
	{ "ReadFwd", "" },
	{ "ReadExFwd", "" },
	{ "Flush", "" },
    { "TSTLD", "" },
    { "TSTST", "" },
    { "TSTResp", "" }
};
const string cmp_incl_wt_l1_protocol_t::state_names[][2] = {
	{ "Unknown", "Unknown" },
	{ "NotPresent", "Line is not present in cache" },
	{ "Invalid", "Data may be present but is invalid" },
	{ "InvToSh", "Line was invalid, issued a read request, waiting for data, will go to wither shared or exclusive" },
	{ "Shared", "Line was invalid, got a write, issued a readex request, waiting for data" },
	{ "ShToInv", "Line is present and clean and I am owner" },
	{ "PendingWrite", "Line is present and dirty and I am owner" }
};

const string cmp_incl_wt_l1_protocol_t::prot_name[] = 
	{"cmp_incl_wt_l1", "Two-level Uniprocessor L1 Cache"};
