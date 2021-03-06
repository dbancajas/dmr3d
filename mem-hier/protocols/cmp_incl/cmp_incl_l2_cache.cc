/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    implementation of a two-level CMP L2 cache
 * initial author: Philip Wells 
 *
 */
 
 
//  #include "simics/first.h"
// RCSID("$Id: cmp_incl_l2_cache.cc,v 1.1.2.44 2006/12/12 18:36:13 pwells Exp $");

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

#include "cmp_incl_l2_cache.h"

// Link definitions
const uint8 cmp_incl_l2_protocol_t::interconnect_link;
const uint8 cmp_incl_l2_protocol_t::mainmem_addr_link;
const uint8 cmp_incl_l2_protocol_t::mainmem_data_link;
const uint8 cmp_incl_l2_protocol_t::io_link;  

/////////////////////////////////////////////////////////////////////////////////
// Uniprocessor one-level line functions
//
void
cmp_incl_l2_line_t::init(cache_t *_cache, addr_t _idx, uint32 _lsize)
{
	line_t::init(_cache, _idx, _lsize);
	state = StateInvalid;
	dirty = false;
	sharers = 0ULL;
}

void
cmp_incl_l2_line_t::wb_init(addr_t tag, addr_t idx, line_t *cache_line)
{
	line_t::wb_init(tag, idx, cache_line);
	
	if (cache_line) {
		cmp_incl_l2_line_t *line = static_cast <cmp_incl_l2_line_t *> (cache_line);
		dirty = line->is_dirty();
		sharers = line->get_sharers();
		if (line->get_state() == StateShared)
			state = StateShToInv;
		else if (line->get_state() == StateModified) 
			state = StateModToInv;
		else FAIL;
	} else {
		state = StateInvalid;
		dirty = false;
		sharers = 0ULL;
	}

	
}


uint32
cmp_incl_l2_line_t::get_num_sharers()
{
	uint32 num_sharers = 0;
	for (uint32 i = 0; i < prot_line_t::MAX_SHARERS; i++) {
		if (is_sharer(i))
			num_sharers++;
	}

	return num_sharers;
}

void
cmp_incl_l2_line_t::reinit(addr_t _tag)
{
	line_t::reinit(_tag);
	state = StateInvalid;
	dirty = false;
	sharers = 0ULL;
}	
	
bool
cmp_incl_l2_line_t::is_free()
{
	return (state == StateInvalid);
}	
	
bool
cmp_incl_l2_line_t::can_replace()
{
	// Assume there are enough ways to keep all lines with dont_evict marked
	if (g_conf_keep_thread_state && mem_hier_t::is_thread_state(get_address()))
		return false;

	// Can replace if not in a busy state
	return (state == StateInvalid ||
			state == StateNotPresent ||
			state == StateShared ||
			state == StateModified);
}

bool
cmp_incl_l2_line_t::is_stable()
{
	// Same as can_replace
	return (can_replace());
}

void
cmp_incl_l2_line_t::warmup_state()
{
    state = StateShared;
    sharers = 0ULL;
}

void
cmp_incl_l2_line_t::set_state(uint32 _state)
{
	state = _state;
}
	
uint32
cmp_incl_l2_line_t::get_state()
{
	return state;
}

void
cmp_incl_l2_line_t::set_dirty(bool _dirty)
{
	dirty = _dirty;
}
	
bool
cmp_incl_l2_line_t::is_dirty()
{
	return dirty;
}
	
void
cmp_incl_l2_line_t::add_sharer(uint32 sharer)
{
	sharers |= (1ULL << sharer);
}
	
void
cmp_incl_l2_line_t::remove_sharer(uint32 sharer)
{
	sharers &= ~(1ULL << sharer);
}
	
bool
cmp_incl_l2_line_t::is_sharer(uint32 sharer)
{
	return (sharers & (1ULL << sharer)) != 0ULL;
}

uint64
cmp_incl_l2_line_t::get_sharers() {
	return sharers;	
}

void
cmp_incl_l2_line_t::to_file(FILE *file)
{
	fprintf(file, "%d %u %llu ", dirty ? 1 : 0, state, sharers);
	line_t::to_file(file);
}

void
cmp_incl_l2_line_t::from_file(FILE *file)
{
	int temp_dirty;
	fscanf(file, "%d %u %llu ", &temp_dirty, &state, &sharers);
	dirty = (temp_dirty == 1);

	line_t::from_file(file);
}

/////////////////////////////////////////////////////////////////////////////////
// CMP inclusive L2 cache functions
//

cmp_incl_l2_cache_t::cmp_incl_l2_cache_t(string name, cache_config_t *conf,
	uint32 _id, uint64 prefetch_alg, uint32 id_offset)
	: tcache_t<cmp_incl_l2_protocol_t> (name, 3, conf, _id, prefetch_alg),
	interconnect_id_offset(id_offset)
	// Three links for this protocol
{ 
	stat_num_req_fwd = stats->COUNTER_BASIC("request_fwd",
		"# of request forwards");
    stat_num_as_user_fwd = stats->COUNTER_BASIC("request_fwd_as_user", 
        " # of request fwds with AS_USER");
    
        
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
    
    string wr_coher [] = {
        "AS_USER_SPILL",
        "OTHER_SPILL",
        "AS_USER_ACCESS",
        "LWP_STACK_ACCESS",
        "KMAP_ACCESS",
        "OTHER_ACCESS"};
    stat_write_coherence_breakdown = stats->COUNTER_BREAKDOWN("stat_write_c2c_breakdown",
        "write_c2c_breakdown", 6, wr_coher, wr_coher);
}


line_t *
cmp_incl_l2_cache_t::choose_replacement(addr_t idx)
{
	if (!g_conf_evict_singlets)
        return tcache_t<cmp_incl_l2_protocol_t>::choose_replacement(idx);
    // Replace line that has least number of users
    // Among all such candidates, replace the LRU
    
	prot_line_t *line = NULL;
    for (uint32 j = 0; j < assoc; j++) {
		if (lines[idx][j].can_replace()) {
			// If the line is replacable (i.e. not busy), and either we havn't
			// found one yet, or this one was used longer ago, take it as the
			// best so far
            
            if (!line) line = &lines[idx][j];
            else if (line->get_num_sharers() > lines[idx][j].get_num_sharers())
                line = &lines[idx][j];
            else if (line->get_num_sharers() == lines[idx][j].get_num_sharers() &&
                line->get_last_use() > lines[idx][j].get_last_use())
                line = &lines[idx][j];
                
            
			// if (!line || line->get_last_use() >
			// 	lines[idx][j].get_last_use()) {

			// 	line = &lines[idx][j];
			// }
        }
    }
	

	return line;
}


uint32
cmp_incl_l2_cache_t::get_action(message_t *msg)
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
			
		case TypeFwdAck:
			return ActionL1FwdAck;
			
		case TypeFwdNack:
			return ActionL1FwdNack;
			
		case TypeFwdAckData:
			return ActionL1FwdAckData;
			
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
cmp_incl_l2_cache_t::wakeup_signal(message_t* msg, prot_line_t *line, 
    uint32 action)
{
    return true;
}

void
cmp_incl_l2_cache_t::trigger(tfn_info_t *t, tfn_ret_t *ret)
{
	ret->block_message = StallNone;
	ret->next_state = StateUnknown;

//    ASSERT(!t->trans->completed || t->trans->get_pending_events());
    
    switch(t->curstate) {

	/// NotPresent
	case StateNotPresent:
		switch(t->action) {
			
		case ActionL1Read:
		case ActionL1ReadEx:
        case ActionL1Upgrade:
        case ActionL1TST:    
			ret->block_message = can_insert_new_line(t, ret);
			if (!can_allocate_mshr(t, ret)) ret->block_message = StallPoll;
			if (!can_enqueue_request_mshr(t, ret)) ret->block_message = StallPoll;
			if (!can_send_msg_down(t, ret)) ret->block_message = StallPoll;
			if (ret->block_message != StallNone) goto block_message;
			
			ASSERT(insert_new_line(t, ret) == StallNone) ; 
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			mark_requester_as_sharer(t, ret);
			send_msg_down(t, ret, downmsg_t::Read);
			ret->next_state = StateInvToMod;
			
			t->trans->mark_pending(MEM_EVENT_L2_DEMAND_MISS);
			if (t->trans->is_prefetch()) 
				t->line->mark_prefetched(true);
			break;
		case ActionL1WriteBack:
        case ActionL1WriteBackClean:
            send_ack_up(t, ret, TypeWriteBackAck);
            ret->next_state = t->curstate;
            break;
        case ActionL1Replace:
            if (!g_conf_icache_inclusion)
            {
                send_ack_up(t, ret, TypeReplaceAck);
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
        case ActionL1Upgrade:
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
                send_ack_up(t, ret, TypeReplaceAck);
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
			ret->next_state = StateModified;
			break;
		case ActionL1WriteBack:
        case ActionL1WriteBackClean:
            ASSERT(!requester_is_sharer(t, ret));
            send_ack_up(t, ret, TypeWriteBackAck);
            ret->next_state = t->curstate;
            break;
        case ActionL1Replace:
            if (!g_conf_icache_inclusion)
            {
                send_ack_up(t, ret, TypeReplaceAck);
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
			if (g_conf_cmp_incl_l2_read_fwd && t->line->get_sharers() > 0ULL) {
				send_request_forward(t, ret, TypeReadFwd);
                mark_requester_as_sharer(t, ret);
//				ret->next_state = StateShFwd;
				ret->next_state = t->curstate;
			} 
			else {
				mark_requester_as_sharer(t, ret);
				send_data_to_requester(t, ret, true);  // shared
				ret->next_state = t->curstate;
				t->trans->mark_event(MEM_EVENT_L2_HIT);
			}
			break;
			
		case ActionL1ReadEx:
            if (t->line->get_sharers() == 0ULL) {
				mark_requester_as_sharer(t, ret);
				send_data_to_requester(t, ret, false); // Exclusive
				ret->next_state = StateModified;
				t->trans->mark_event(MEM_EVENT_L2_HIT);
            } else {
				if (!can_allocate_mshr(t, ret))        goto block_message_poll;
				if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
				
				allocate_mshr(t, ret);
				enqueue_request_mshr(t, ret);
				mark_requester_as_sharer(t, ret);
				send_invalidate_to_sharers(t, ret, false); // not incl. req.
				ret->next_state = StateShToMod;
				t->trans->mark_pending(MEM_EVENT_L2_COHER_MISS);
			}
			break;
			
		case ActionL1Upgrade:
            if (requester_is_only_sharer(t, ret)) { // No other sharers
				send_ack_up(t, ret, TypeUpgradeAck);
                ret->next_state = StateModified;
				t->trans->mark_event(MEM_EVENT_L2_HIT);
            } else {
				if (!can_allocate_mshr(t, ret))        goto block_message_poll;
				if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
                
				allocate_mshr(t, ret);
                enqueue_request_mshr(t, ret);
				send_invalidate_to_sharers(t, ret, false); // not incl. req.
				ret->next_state = StateShToMod;
				t->trans->mark_pending(MEM_EVENT_L2_COHER_MISS);
			}
			break;
			
		case ActionL1Replace:  
            if (requester_is_sharer(t, ret))
                remove_source_sharer(t, ret);
            send_ack_up(t, ret, TypeReplaceAck);
            ret->next_state = t->curstate;
            break;
		
		case ActionReplace:
			if (t->line->get_sharers() > 0ULL) {
				ASSERT(allocate_writebackbuffer(t->line));
				send_invalidate_to_sharers(t, ret, true); // all sharers
				ret->next_state = StateInvalid;  // wbb goes to ShToInv
				t->trans->mark_pending(MEM_EVENT_L2_REPLACE);
			} else {
				ret->next_state = StateInvalid;
				t->trans->mark_event(MEM_EVENT_L2_REPLACE);
			}				
			break;

        case ActionL1WriteBack:
        case ActionL1WriteBackClean:
            ASSERT(!requester_is_sharer(t, ret));
            send_ack_up(t, ret, TypeWriteBackAck);
            ret->next_state = t->curstate;
            break;
		default: goto error;
		}
		break;
	
	/// ShToInv
	case StateShToInv:
		switch (t->action) {
		
		case ActionL1Read:
		case ActionL1ReadEx:
            goto block_message_set;
		case ActionL1Upgrade:
			goto donothing; // Drop Upgrade ... L1 will reissue
			
		case ActionL1Replace:
            // Do Not Send Replacement ACK
		case ActionL1InvAck:
			remove_source_sharer(t, ret);
			if (t->line->get_sharers() > 0ULL)
				ret->next_state = t->curstate;
			else {
                t->trans->clear_pending_event(MEM_EVENT_L2_REPLACE);
				release_writebackbuffer(t->address);
				ret->next_state = StateInvalid;
			}
			break;

		default: goto error;
		}
		break;

	/// ShFwd
	// case StateShFwd:
		// switch (t->action) {
		// 
		// case ActionL1Read:
			// // TODO: could have a fwd count and FWD this one as well...
			// mark_requester_as_sharer(t, ret);
			// send_data_to_requester(t, ret, true);  // shared
			// ret->next_state = t->curstate;
			// break;
		// 
		// case ActionL1ReadEx:
		// case ActionL1Upgrade:
			// goto block_message_set;
			// 
		// case ActionL1Replace:
            // send_ack_up(t, ret, TypeReplaceAck);
			// remove_source_sharer(t, ret);
			// ret->next_state = t->curstate;
			// break;
// 
		// case ActionL1FwdAck:
			// t->trans->clear_pending_event(MEM_EVENT_L2_REQ_FWD);
			// ret->next_state = StateShared;
			// break;
			// 
		// case ActionL1FwdNack:
			// send_data_to_requester(t, ret, true); // shared
			// t->trans->clear_pending_event(MEM_EVENT_L2_REQ_FWD);
			// ret->next_state = StateShared;
			// break;
			// 
		// default: goto error;
		// }
		// break;

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
//                mark_requester_as_sharer(t, ret);
				perform_mshr_requests(t, ret);
				deallocate_mshr(t, ret);
				ret->next_state = StateModified;
            } else 
                ret->next_state = t->curstate;
			break;
        case ActionL1WriteBack:
        case ActionL1WriteBackClean:
            ASSERT(!requester_is_sharer(t, ret));
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
			allocate_mshr(t, ret);
			send_request_forward(t, ret, TypeReadFwd);
            mark_requester_as_sharer(t, ret);
			t->trans->mark_pending(MEM_EVENT_L2_REQ_FWD | MEM_EVENT_L2_COHER_MISS);
			ret->next_state = StateModToSh;
			break;
		
		case ActionL1ReadEx:
		case ActionL1Upgrade:
            // request Forward must be sent first
            send_request_forward(t, ret, TypeReadExFwd);
			remove_only_sharer(t, ret);
			mark_requester_as_sharer(t, ret);
			t->trans->mark_event(MEM_EVENT_L2_REQ_FWD | MEM_EVENT_L2_COHER_MISS);
			ret->next_state = t->curstate;
			break;
			
		case ActionL1WriteBack:	
			store_data_to_cache(t, ret);
			t->line->set_dirty(true); // Fallthrough
		case ActionL1WriteBackClean:
			remove_source_sharer(t, ret);
			send_ack_up(t, ret, TypeWriteBackAck);
			// Replacement sent before ReadEx Fwd, so someone else is the owner
			if (t->line->get_sharers() > 0ULL) 
				ret->next_state = t->curstate;
			else
				ret->next_state = StateShared;
			break;
		
        case ActionL1TST:
            send_data_to_requester(t, ret, false);
            ret->next_state = t->curstate;
            break;
            
		case ActionReplace:
			if (!can_allocate_writebackbuffer()) goto block_message_poll;
			if (is_no_sharer(t, ret)) {
                writeback_if_dirty(t, ret);
            } else {
                allocate_writebackbuffer(t->line);
                send_invalidate_to_sharers(t, ret, true);
                t->trans->mark_pending(MEM_EVENT_L2_REPLACE);
            }
			ret->next_state = StateInvalid;
			break;
			
		case ActionL1Replace:
        if (!g_conf_icache_inclusion) {
            send_ack_up(t, ret, TypeReplaceAck);
            goto donothing;
        }
        
            
//		case ActionL1FwdAck:
//        case ActionL1FwdAckData:
//        case ActionL1InvAck:
//            goto donothing;	
		
		default: goto error;
		}
		break;

	/// ModToSh
	case StateModToSh:
		switch (t->action) {
		
		case ActionL1Read:
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_poll;
			enqueue_request_mshr(t, ret);
            mark_requester_as_sharer(t, ret);
			t->trans->mark_pending(MEM_EVENT_L2_MSHR_PART_HIT);
			ret->next_state = t->curstate;
			break;
		
		case ActionL1ReadEx:
        case ActionL1Upgrade:
            goto block_message_set;
			
		case ActionL1WriteBack:	
			store_data_to_cache(t, ret);
			t->line->set_dirty(true); // Fallthrough
		case ActionL1WriteBackClean:
			remove_source_sharer(t, ret);
			send_ack_up(t, ret, TypeWriteBackAck);
			ret->next_state = t->curstate;
			break;
		
		case ActionL1FwdAckData:
			store_data_to_cache(t, ret);
            t->trans->clear_pending_event(MEM_EVENT_L2_REQ_FWD);
			t->line->set_dirty(true);
		case ActionL1FwdAck:
			perform_mshr_requests(t, ret);
            deallocate_mshr(t, ret);
            t->trans->clear_pending_event(MEM_EVENT_L2_REQ_FWD);
			ret->next_state = StateShared;
			break;
			
		case ActionL1Replace:  
			remove_source_sharer(t, ret);
            send_ack_up(t, ret, TypeReplaceAck);
			ret->next_state = t->curstate;
			break;
		
		default: goto error;
		}
		break;

	/// ModToInv
	case StateModToInv:
		switch (t->action) {
		
		case ActionL1Read:
		case ActionL1ReadEx:
        case ActionL1TST:  
			goto block_message_set;
			
		case ActionL1InvAck:
			writeback_if_dirty(t, ret);
            t->trans->clear_pending_event(MEM_EVENT_L2_REPLACE);
			release_writebackbuffer(t->address);
			ret->next_state = StateInvalid;
			break;

		case ActionL1WriteBack:
            if (!requester_is_sharer(t, ret)) {
                send_ack_up(t, ret, TypeWriteBackAck);
                goto donothing;
            }
			store_data_to_cache(t, ret);
			t->line->set_dirty(true); // Fallthrough
		case ActionL1WriteBackClean:
            if (!requester_is_sharer(t, ret)) {
                send_ack_up(t, ret, TypeWriteBackAck);
                goto donothing;
            }
            t->trans->clear_pending_event(MEM_EVENT_L1_WRITE_BACK);
			remove_source_sharer(t, ret);
			store_data_to_cache(t, ret);
			writeback_if_dirty(t, ret);
            t->trans->clear_pending_event(MEM_EVENT_L2_REPLACE);
			release_writebackbuffer(t->address);
            ret->next_state = StateInvalid;
            
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
cmp_incl_l2_cache_t::can_send_msg_down(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	// return links[mainmem_addr_link]->can_send();
	return true;
}
	
void
cmp_incl_l2_cache_t::send_msg_down(tfn_info_t *t, tfn_ret_t *ret,
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
cmp_incl_l2_cache_t::writeback_if_dirty(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_l2_cache_t::send_data_to_requester(tfn_info_t *t, tfn_ret_t *ret, 
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
cmp_incl_l2_cache_t::send_request_forward(tfn_info_t *t, tfn_ret_t *ret, 
	uint32 type)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	
	// TODO: some heuristic to pick a sharer if more than one
	uint32 sharer;
	for (sharer = 0; sharer < prot_line_t::MAX_SHARERS; sharer++)
		if (t->line->is_sharer(sharer)) break;
		
    if (type == TypeReadFwd)
		STAT_INC(stat_read_c2c[t->trans->cpu_id], get_sharer_cpu_id(sharer));
	else 
		STAT_INC(stat_write_c2c[t->trans->cpu_id], get_sharer_cpu_id(sharer));
    
    ASSERT(sharer != prot_line_t::MAX_SHARERS);
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	send_protocol_message_up(t, ret, type, sharer, inmsg->get_requester(), t->trans);
}

void
cmp_incl_l2_cache_t::send_ack_up(tfn_info_t *t, tfn_ret_t *ret, uint32 type)
{
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
    DEBUG_TR_FN("%s sending %s to %u", t, ret, __func__, 
        cmp_incl_protocol_t::names[type][0].c_str(),inmsg->get_source());
	send_protocol_message_up(t, ret, type, inmsg->get_source(),
		inmsg->get_requester(), t->trans);
}


// Send invalidate to icache if inclusion bit set
void
cmp_incl_l2_cache_t::send_invalidate_to_sharers(tfn_info_t *t, tfn_ret_t *ret,
	bool requester_too)
{
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);  
	uint32 requester = inmsg->get_requester();
	DEBUG_TR_FN("%s requester %u", t, ret, __func__, requester);
	
	for (uint32 i = 0; i < prot_line_t::MAX_SHARERS; i++) {
		if (!t->line->is_sharer(i)) continue;
		if (!requester_too && i == requester) continue;
		
        if (!requester_too) STAT_INC(stat_write_c2c[t->trans->cpu_id], 
            get_sharer_cpu_id(i));
        // Don't send invalidation to Icache if no_inclusion
        //if (requester_too && !g_conf_icache_inclusion && i >= num_procs) continue; 
		send_protocol_message_up(t, ret, TypeInvalidate, i, requester, t->trans);
	}
}


// Enqueue this read request to the MSHR for this address
void
cmp_incl_l2_cache_t::enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_l2_cache_t::perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_l2_cache_t::send_protocol_message_up(tfn_info_t *t, tfn_ret_t *ret,
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
	
//	ASSERT(mem_trans->get_pending_events());
	upmsg_t *msg = new upmsg_t(mem_trans, addr, get_lsize(), data,
		get_interconnect_id(addr), destination, type, requester, addr_link);
		
	stall_status_t retval = send_message(links[interconnect_link], msg);
	ASSERT(retval == StallNone);

}


void
cmp_incl_l2_cache_t::mark_requester_as_sharer(tfn_info_t *t, tfn_ret_t *ret) {
	DEBUG_TR_FN("%s", t, ret, __func__);
	
    if (g_conf_tstate_l1_bypass && t->trans->is_thread_state())
        return;
    
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	ASSERT(inmsg->get_requester() <= cmp_incl_l2_line_t::MAX_SHARERS);
	t->line->add_sharer(inmsg->get_requester());
}

void
cmp_incl_l2_cache_t::remove_source_sharer(tfn_info_t *t, tfn_ret_t *ret) {
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
    DEBUG_TR_FN("%s: removing %u", t, ret, __func__, inmsg->get_source());
	t->line->remove_sharer(inmsg->get_source());  // user sender not requester
}

// Remove only sharer (owner)
void
cmp_incl_l2_cache_t::remove_only_sharer(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_l2_cache_t::requester_is_sharer(tfn_info_t *t, tfn_ret_t *ret)
{
    DEBUG_TR_FN("%s", t, ret, __func__);
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	return (t->line->is_sharer(inmsg->get_requester()));
}

bool
cmp_incl_l2_cache_t::is_no_sharer(tfn_info_t *t, tfn_ret_t *ret)
{
	uint32 num_sharers = 0;
	for (uint32 i = 0; i < prot_line_t::MAX_SHARERS; i++) {
		if (t->line->is_sharer(i))
			num_sharers++;
	}

    DEBUG_TR_FN("%s: %d", t, ret, __func__, num_sharers);
	
	return (num_sharers == 0);
}

bool
cmp_incl_l2_cache_t::is_only_one_sharer(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_l2_cache_t::requester_is_only_sharer(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	ASSERT(t->line->is_sharer(inmsg->get_requester()));
    DEBUG_TR_FN("%s sharers %llu requester %u", t, ret, __func__, 
        t->line->get_sharers(), inmsg->get_requester());
	if ((t->line->get_sharers() & (~(1ULL << inmsg->get_requester()))) == 0ULL)
		return true;
	
	return false;
}

uint32
cmp_incl_l2_cache_t::get_interconnect_id(addr_t addr) {
	uint32 bank = get_bank(addr);
	return (bank+interconnect_id);
}

bool
cmp_incl_l2_cache_t::is_request_message(message_t *message)
{
	if (is_up_message(message)) {
		upmsg_t *inmsg = static_cast<upmsg_t *>(message);
	return (inmsg->get_type() == TypeRead ||
		inmsg->get_type() == TypeReadEx ||
		inmsg->get_type() == TypeUpgrade);
	}
	
	return false;
}

void
cmp_incl_l2_cache_t::profile_transition(tfn_info_t *t, tfn_ret_t *ret)
{
	if (!ret->block_message && is_request_message(t->message)) {
		if (t->trans->occurred(MEM_EVENT_L2_REQ_FWD)) {
			if (t->trans->is_prefetch())
				STAT_INC(stat_num_prefetch);
            else {
				STAT_INC(stat_num_req_fwd);
                if (t->trans->is_as_user_asi())
                    STAT_INC(stat_num_as_user_fwd);
                if (!t->trans->read) {
                    uint32 write_coherence_type;
                    bool spill_trap = t->trans->sequencer ? t->trans->sequencer->is_spill_trap(0) : false;
                    if (spill_trap) {
                        if (t->trans->is_as_user_asi()) write_coherence_type = 0;
                        else write_coherence_type = 1;
                    } else if (t->trans->is_as_user_asi()) 
                        write_coherence_type = 2;
                    else {
                        kernel_access_type_t ktype = t->trans->get_access_type();
                        if (ktype == LWP_STACK_ACCESS) write_coherence_type = 3;
                        else if (ktype == KERNEL_MAP_ACCESS) write_coherence_type = 4;
                        else write_coherence_type = 5;
                    }
                    STAT_INC(stat_write_coherence_breakdown, write_coherence_type);    
                }
                
            }
            
			t->trans->mark_event(MEM_EVENT_L2_HIT);
		}
	}
    

	tcache_t<cmp_incl_l2_protocol_t>::profile_transition(t, ret);
}

uint32
cmp_incl_l2_cache_t::get_sharer_cpu_id(uint32 sharer)
{
	// TODO: better mechanism to determine sharer's CPU id
	if (g_conf_memory_topology == "cmp_incl_l2banks") 
		return (sharer/2);  // this topo assigns dcache/icache per cpu
	else
		return (sharer % num_procs);  // this topo assigns all dcache, then all icache
}

////////////////////////////////////////////////////////////////////////////////
// Uniprocessor onelevel cache constant definitions

const string cmp_incl_l2_protocol_t::action_names[][2] = {
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
const string cmp_incl_l2_protocol_t::state_names[][2] = {
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

const string cmp_incl_l2_protocol_t::prot_name[] = 
	{"cmp_incl_l2", "Two-level CMP L2 Cache"};
