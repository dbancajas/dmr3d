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
// RCSID("$Id: cmp_incl_3l_l1_cache.cc,v 1.1.2.12 2006/03/15 19:38:31 pwells Exp $");

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

#include "cmp_incl_3l_l1_cache.h"

/////////////////////////////////////////////////////////////////////////////////
// Uniprocessor one-level line functions
//
void
cmp_incl_3l_l1_line_t::init(cache_t *_cache, addr_t _idx, uint32 _lsize)
{
	line_t::init(_cache, _idx, _lsize);
	state = StateInvalid;
}

void
cmp_incl_3l_l1_line_t::wb_init(addr_t tag, addr_t idx, line_t *cache_line)
{
	FAIL;
}

void
cmp_incl_3l_l1_line_t::reinit(addr_t _tag)
{
	line_t::reinit(_tag);
    ASSERT(this->tag == _tag);
	state = StateInvalid;
}	
	
bool
cmp_incl_3l_l1_line_t::is_free()
{
	return (state == StateInvalid);
}	
	
bool
cmp_incl_3l_l1_line_t::can_replace()
{
	// Can replace if not in a busy state
	return (state == StateInvalid ||
			state == StateNotPresent ||
			state == StateShared ||
			state == StateExclusive ||
			state == StateModified);
}

bool
cmp_incl_3l_l1_line_t::is_stable()
{
	// Same as can_replace
	return (can_replace());
}

void
cmp_incl_3l_l1_line_t::set_state(uint32 _state)
{
	state = _state;
}
	
uint32
cmp_incl_3l_l1_line_t::get_state()
{
	return state;
}

void
cmp_incl_3l_l1_line_t::to_file(FILE *file)
{
	fprintf(file, "%u ", state);
	line_t::to_file(file);
}

void
cmp_incl_3l_l1_line_t::from_file(FILE *file)
{
	fscanf(file, "%u ", &state);

	line_t::from_file(file);
}

/////////////////////////////////////////////////////////////////////////////////
// CMP Exclusion protocol L2 cache functions
//

cmp_incl_3l_l1_cache_t::cmp_incl_3l_l1_cache_t(string name, cache_config_t *conf,
	uint32 _id, bool _icache, conf_object_t *_cpu, uint64 prefetch_alg)
	: tcache_t<cmp_incl_3l_l1_protocol_t> (name, 2, conf, _id, prefetch_alg), 
	  cpu(_cpu), icache(_icache)
	// Two links for this protocol
{ }

uint32
cmp_incl_3l_l1_cache_t::get_action(message_t *msg)
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
            
        case TypeDataRespShared:
            if (t_state) return ActionTSTResp;
            return ActionDataShared;
        case TypeDataRespExclusive:
            if (t_state) return ActionTSTResp;
            return ActionDataExclusive;
        case TypeInvalidate:
            return ActionInvalidate;
        case TypeReadFwd:
            return ActionReadFwd;
        case TypeUpgradeAck:
            return ActionUpgradeAck;
             
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
cmp_incl_3l_l1_cache_t::trigger(tfn_info_t *t, tfn_ret_t *ret)
{
	ret->block_message = StallNone;
	ret->next_state = StateUnknown;
    
    
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
                ret->block_message = StallOtherEvent;
                goto block_message;
            }
            insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_protocol_msg_down(t, ret, false, TypeRead, true);
			ret->next_state = StateInvToShEx;
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
                ret->block_message = StallOtherEvent;
                goto block_message;
            }
            insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_protocol_msg_down(t, ret, false, TypeReadEx, true);
			ret->next_state = StateInvToMod;
            t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
            break;
		case ActionInvalidate: 
			send_invalidate_up(t, ret);
			ret->next_state = t->curstate;
			break;
		case ActionReadFwd:
			goto donothing;
        case ActionTSTLD:
            t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
            send_protocol_msg_down(t, ret, false, TypeRead, true);
            ret->next_state = t->curstate;
            break;
        case ActionTSTST:
            t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
            send_protocol_msg_down(t, ret, false, TypeReadEx, true);
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
                ret->block_message = StallOtherEvent;
                goto block_message;
            }
            insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_protocol_msg_down(t, ret, false, TypeRead, true);
			ret->next_state = StateInvToShEx;
            t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
             break;
            
		case ActionProcST:
        
            if (!can_allocate_mshr(t, ret) || 
                !can_enqueue_request_mshr(t, ret))
            {
                ret->block_message = StallOtherEvent;
                goto block_message;
            }
            insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			send_protocol_msg_down(t, ret, false, TypeReadEx, true);
            t->trans->mark_pending(MEM_EVENT_L1_DEMAND_MISS);
			ret->next_state = StateInvToMod;
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
             break;
		case ActionInvalidate: 
			send_invalidate_up(t, ret);
			ret->next_state = t->curstate;
			break;
        case ActionReadFwd:
			goto donothing;
		default: goto error;
		}
		break;

	/// InvalidToMod
	case StateInvToMod:
		switch(t->action) {

		case ActionDataExclusive:
			store_data_to_cache(t, ret);
			perform_mshr_requests(t, ret);
			ret->next_state = StateModified;
            deallocate_mshr(t, ret);
			break;
		case ActionReadFwd:
			goto donothing;
		case ActionInvalidate: 
			send_invalidate_up(t, ret);
			ret->next_state = t->curstate;
			break;
			
        case ActionProcLD:
        case ActionProcST:
            if (!can_enqueue_request_mshr(t, ret)) {
                ret->block_message = StallOtherEvent;
                goto block_message;
            }
            enqueue_request_mshr(t,ret);
            ret->next_state = t->curstate;
            t->trans->mark_pending(MEM_EVENT_L1_MSHR_PART_HIT);
            break;
		default: goto error;
		}
		break;

	case StateInvToShEx:
    
        switch(t->action) {
        case ActionDataShared:
            store_data_to_cache(t, ret);
            perform_mshr_requests(t, ret);
            ret->next_state = StateShared;
            deallocate_mshr(t, ret);
            break;
        case ActionDataExclusive:
            store_data_to_cache(t, ret);
            perform_mshr_requests(t, ret);
            ret->next_state = StateExclusive;
            deallocate_mshr(t, ret);
            break;
        case ActionReadFwd:
			goto donothing;
			
		case ActionInvalidate: 
			send_invalidate_up(t, ret);
			ret->next_state = t->curstate;
			break;
        case ActionProcLD:
            if (!can_enqueue_request_mshr(t, ret)) {
                ret->block_message = StallOtherEvent;
                goto block_message;
            }
            enqueue_request_mshr(t,ret);
            ret->next_state = t->curstate;
            t->trans->mark_pending(MEM_EVENT_L1_MSHR_PART_HIT);
            break;
        case ActionProcST:
            ret->block_message = StallSetEvent;
            goto block_message;
        default: goto error;
        }
        break;
        
	case StateShared:
		switch(t->action) {

		case ActionProcLD:
            send_protocol_msg_up(t, ret, simple_proc_msg_t::DataResp, t->trans);
            t->trans->mark_event(MEM_EVENT_L1_HIT);
            ret->next_state = StateShared;
			break;
			
		case ActionProcST:
			if (!can_allocate_mshr(t, ret) || !can_enqueue_request_mshr(t, ret)) 
            {
                ret->block_message = StallOtherEvent;
                goto block_message;
            }
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
            send_protocol_msg_down(t, ret, false, downmsg_t::TypeUpgrade, true);
			ret->next_state = StateShToMod;
            t->trans->mark_pending(MEM_EVENT_L1_COHER_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
 			break;
			
		case ActionReplace:
            send_protocol_msg_down(t, ret, false, downmsg_t::TypeReplace, true);
			t->trans->mark_pending(MEM_EVENT_L1_WRITE_BACK);
			ret->next_state = StateInvalid;
			break;
        case ActionReadFwd:
            send_protocol_msg_down(t, ret, true, downmsg_t::TypeL1DataRespShared, false);
            ret->next_state = t->curstate;
            break;
        case ActionInvalidate:
            send_protocol_msg_down(t, ret, false, downmsg_t::TypeL1InvAck, true);
            send_invalidate_up(t, ret);
            ret->next_state = StateInvalid;
            break;
		default: goto error;
		}
		break;


    case StateExclusive:
        switch(t->action) {
            case ActionProcLD:
                send_protocol_msg_up(t, ret, upmsg_t::DataResp, t->trans);
                ret->next_state = t->curstate;
				t->trans->mark_event(MEM_EVENT_L1_HIT);
                break;
            case ActionProcST:
                store_data_to_cache(t, ret);
                send_protocol_msg_up(t, ret, upmsg_t::DataResp, t->trans);
                ret->next_state = StateModified;
				t->trans->mark_event(MEM_EVENT_L1_HIT);
                break;
            case ActionReplace:
                send_protocol_msg_down(t, ret, false, downmsg_t::TypeWriteBackClean, true);
				t->trans->mark_pending(MEM_EVENT_L1_WRITE_BACK);
                ret->next_state = StateInvalid;
                break;
            case ActionInvalidate:
                send_invalidate_up(t, ret);
                send_protocol_msg_down(t, ret, false, downmsg_t::TypeWriteBackClean, true);
                ret->next_state = StateInvalid;
                break;
            case ActionReadFwd:
                send_protocol_msg_down(t, ret, true, downmsg_t::TypeL1DataRespShared, false);
                ret->next_state = StateShared;
                break;
                    
            default: goto error;    
                
        }
        break;
      case StateModified:
          switch(t->action) {
            case ActionProcLD:
                send_protocol_msg_up(t, ret, upmsg_t::DataResp, t->trans);
                t->trans->mark_event(MEM_EVENT_L1_HIT);
                ret->next_state = t->curstate;
                break;
            case ActionProcST:
                send_protocol_msg_up(t, ret, upmsg_t::DataResp, t->trans);
                t->trans->mark_event(MEM_EVENT_L1_HIT);
                ret->next_state = t->curstate;
                break;
            case ActionReplace:
                send_protocol_msg_down(t, ret, false, downmsg_t::TypeWriteBack, false);
				t->trans->mark_pending(MEM_EVENT_L1_WRITE_BACK);
                ret->next_state = StateInvalid;
                break;
            case ActionInvalidate:
                send_invalidate_up(t, ret);
                send_protocol_msg_down(t, ret, false, downmsg_t::TypeWriteBack, false);
                ret->next_state = StateInvalid;
                break;
            case ActionReadFwd:
                send_protocol_msg_down(t, ret, true, downmsg_t::TypeL1DataRespShared, false);
                ret->next_state = StateShared;
                break;
            default: goto donothing;
          }
          break;
          
        
      case StateShToMod:
        switch (t->action) {
            case ActionProcLD:
                send_protocol_msg_up(t, ret, upmsg_t::DataResp, t->trans);
                ret->next_state = t->curstate;
				t->trans->mark_event(MEM_EVENT_L1_HIT);
                break;
            case ActionProcST:
                if (!can_enqueue_request_mshr(t, ret))
                {
                    ret->block_message = StallSetEvent;
                    goto block_message;
                }
                enqueue_request_mshr(t, ret);
                ret->next_state = t->curstate;
				t->trans->mark_pending(MEM_EVENT_L1_MSHR_PART_HIT);
                break;
            case ActionUpgradeAck:
                ret->next_state = StateModified;
                perform_mshr_requests(t, ret);
                deallocate_mshr(t, ret);
               break;
            case ActionInvalidate:
                ret->next_state = StateInvToMod;
                send_protocol_msg_down(t, ret, false, downmsg_t::TypeL1InvAck, true);
                send_protocol_msg_down(t, ret, false, downmsg_t::TypeReadEx, true,
                    get_enqueued_trans(t, ret));
                break;
            case ActionReadFwd:
                ret->next_state = t->curstate;
                send_protocol_msg_down(t, ret, true, downmsg_t::TypeL1DataRespShared, false);
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


mem_trans_t *
cmp_incl_3l_l1_cache_t::get_enqueued_trans(tfn_info_t *t, tfn_ret_t *ret)
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



////////////////////////////////////////////////////////////////////////////////
// Helper functions

void cmp_incl_3l_l1_cache_t::
send_protocol_msg_down(tfn_info_t *t, tfn_ret_t *ret, bool datafwd, 
    uint32 msgtype, bool addrmsg)
{
    send_protocol_msg_down(t, ret, datafwd,msgtype,addrmsg,t->trans);
}

void cmp_incl_3l_l1_cache_t::
send_protocol_msg_down(tfn_info_t *t, tfn_ret_t *ret, bool datafwd, 
    uint32 msgtype, bool addrmsg, mem_trans_t *mem_trans)
{
    DEBUG_TR_FN("%s sending type %s", t, ret, __func__, 
        downmsg_t::names[msgtype][0].c_str());
    addr_t addr = get_line_address(t->address);
    uint32 request_id = interconnect_id;
    
    if (datafwd) {
        downmsg_t *in_msg = static_cast<downmsg_t *>(t->message);
        request_id = in_msg->get_requester();
    }
    
    downmsg_t *msg = new downmsg_t(mem_trans, addr, get_lsize(), NULL,
        interconnect_id, 0, msgtype, request_id, addrmsg);
    stall_status_t retval = send_message(links[l2_link], msg);
    ASSERT(retval == StallNone);    
    
}

void cmp_incl_3l_l1_cache_t::
send_invalidate_up(tfn_info_t *t, tfn_ret_t *ret)
{
	if (!icache) {
		DEBUG_TR_FN("%s", t, ret, __func__);
		addr_t addr = get_line_address(t->address);
		upmsg_t *msg = new upmsg_t(t->trans, addr, get_lsize(), NULL, upmsg_t::ProcInvalidate);
		stall_status_t retval = send_message(links[cmp_incl_3l_l1_protocol_t::proc_link], msg);
		ASSERT(retval == StallNone);
	}
}

void cmp_incl_3l_l1_cache_t::
send_protocol_msg_up(tfn_info_t *t, tfn_ret_t *ret, uint32 msgtype, mem_trans_t *tr)
{
    DEBUG_TR_FN("%s", t, ret, __func__);
    addr_t addr = get_line_address(t->address);
    const uint8 *data = (g_conf_cache_data && t->line) ? 
		&t->line->get_data()[get_offset(addr)] : NULL;
    upmsg_t *msg = new upmsg_t(tr, tr->phys_addr, tr->size, data, msgtype);
    stall_status_t retval = send_message(links[cmp_incl_3l_l1_protocol_t::proc_link], msg);
    ASSERT(retval == StallNone);
}
// Enqueue this read request to the MSHR for this address
void
cmp_incl_3l_l1_cache_t::enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_3l_l1_cache_t::perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret)
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
	
		ASSERT(request->trans->occurred(
			MEM_EVENT_L1_DEMAND_MISS | MEM_EVENT_L1_COHER_MISS |
			MEM_EVENT_L1_MSHR_PART_HIT));
			
		request->trans->clear_pending_event(
			MEM_EVENT_L1_DEMAND_MISS | MEM_EVENT_L1_COHER_MISS |
			MEM_EVENT_L1_MSHR_PART_HIT);

		send_protocol_msg_up(t, ret, simple_proc_msg_t::DataResp, request->trans);
        delete request;
        request = mshr->pop_request();
	}
	
}

void
cmp_incl_3l_l1_cache_t::prefetch_helper(addr_t message_addr, mem_trans_t *trans,
	bool read)
{
    upmsg_t *msg = new upmsg_t(trans,  message_addr, get_lsize(), NULL,
        read ? upmsg_t::ReadPref : upmsg_t::StorePref);
        
    message_arrival(msg);
}

////////////////////////////////////////////////////////////////////////////////
// Uniprocessor onelevel cache constant definitions

const string cmp_incl_3l_l1_protocol_t::action_names[][2] = {
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
    { "Flush", ""},
    {"TSTLoad", ""},
    {"TSTStore", ""},
    {"TSTResp" ,""}
};
const string cmp_incl_3l_l1_protocol_t::state_names[][2] = {
	{ "Unknown", "Unknown" },
	{ "NotPresent", "Line is not present in cache" },
	{ "Invalid", "Data may be present but is invalid" },
	{ "InvToShEx", "Line was invalid, issued a read request, waiting for data, will go to wither shared or exclusive" },
	{ "InvToMod", "Line was invalid, got a write, issued a readex request, waiting for data" },
	{ "Shared", "Line is present and clean and shared" },
	{ "Exclusive", "Line is present and clean and I am owner" },
	{ "Modified", "Line is present and dirty and I am owner" },
	{ "ShToMod", "Line was shared, received write, send Upgrade down, waiting for ack" },
	{ "ModToInv", "Line was modified, received invalidate or replacement, sent writeback waiting for ack" },
    { "ShToInv",  "Line was Shared, replaced and went to WBB"},
};

const string cmp_incl_3l_l1_protocol_t::prot_name[] = 
	{"cmp_incl_3l_l1", "Two-level Uniprocessor L2 Cache"};
