/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    implementation of a three-level CMP L3 cache (inclusive)
 * initial author: Koushik Chakraborty 
 *
 */
 
 
//  #include "simics/first.h"
// RCSID("$Id: cmp_incl_3l_l3_cache.cc,v 1.1.2.15 2006/03/21 19:17:07 kchak Exp $");

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

#include "cmp_incl_3l_l3_cache.h"
#include "counter.h"
#include "stats.h"
#include "proc_stats.h"
#include "mem_hier.h"
#include "startup.h"
#include "chip.h"
#include "sequencer.h"

// Link definitions
const uint8 cmp_incl_3l_l3_protocol_t::interconnect_link;
const uint8 cmp_incl_3l_l3_protocol_t::mainmem_addr_link;
const uint8 cmp_incl_3l_l3_protocol_t::mainmem_data_link;
const uint8 cmp_incl_3l_l3_protocol_t::io_link;  

/////////////////////////////////////////////////////////////////////////////////
// Uniprocessor one-level line functions
//
void
cmp_incl_3l_l3_line_t::init(cache_t *_cache, addr_t _idx, uint32 _lsize)
{
	line_t::init(_cache, _idx, _lsize);
	state = StateInvalid;
	sharers = 0;
}

void
cmp_incl_3l_l3_line_t::wb_init(addr_t tag, addr_t idx, line_t *cache_line)
{
    line_t::wb_init(tag, idx, cache_line);
	state = StateUnknown;
    if (cache_line) {
		cmp_incl_3l_l3_line_t *line = static_cast <cmp_incl_3l_l3_line_t *> (cache_line);
        sharers = line->get_sharers();
        if (line->get_state() == StateShared)
            state = StateShToInv;
        else if (line->get_state() == StateModified)
            state = StateModToInv;
        else
            FAIL;
    }

	
}

void
cmp_incl_3l_l3_line_t::reinit(addr_t _tag)
{
	line_t::reinit(_tag);
	state = StateInvalid;
	sharers = 0;
}	

	
bool
cmp_incl_3l_l3_line_t::is_free()
{
	return (state == StateInvalid);
}	
	
bool
cmp_incl_3l_l3_line_t::can_replace()
{
	// Can replace only if invalid state, Dir is sized so it will never need
	// to replace a line present in an L2
	return (state == StateInvalid ||
			state == StateNotPresent ||
            state == StateShared ||
            state == StateModified);
}

bool
cmp_incl_3l_l3_line_t::is_stable()
{
	return (state == StateInvalid ||
			state == StateNotPresent || 
			state == StateShared ||
			state == StateModified);
}

void
cmp_incl_3l_l3_line_t::set_state(uint32 _state)
{
	state = _state;
}
	
uint32
cmp_incl_3l_l3_line_t::get_state()
{
	return state;
}



void
cmp_incl_3l_l3_line_t::warmup_state()
{
    state = StateShared;
    sharers = 0ULL;
}

void
cmp_incl_3l_l3_line_t::add_sharer(uint32 sharer)
{
	sharers |= (1 << sharer);
}
	
void
cmp_incl_3l_l3_line_t::remove_sharer(uint32 sharer)
{
	sharers &= ~(1 << sharer);
}
	
bool
cmp_incl_3l_l3_line_t::is_sharer(uint32 sharer)
{
	return (sharers & (1 << sharer)) != 0;
}

uint32
cmp_incl_3l_l3_line_t::get_sharers() {
	return sharers;	
}


uint32
cmp_incl_3l_l3_line_t::get_num_sharers()
{
	uint32 num_sharers = 0;
	for (uint32 i = 0; i < prot_line_t::MAX_SHARERS; i++) {
		if (is_sharer(i))
			num_sharers++;
	}

	return num_sharers;
}

void
cmp_incl_3l_l3_line_t::to_file(FILE *file)
{
	fprintf(file, "%u %u ", state, sharers);
	line_t::to_file(file);
}

void
cmp_incl_3l_l3_line_t::from_file(FILE *file)
{
	fscanf(file, "%u %u ", &state, &sharers);
	line_t::from_file(file);
}

/////////////////////////////////////////////////////////////////////////////////
// CMP exclusive L2DIR cache functions
//

cmp_incl_3l_l3_cache_t::cmp_incl_3l_l3_cache_t(string name, cache_config_t *conf,
	uint32 _id, uint64 prefetch_alg, uint32 id_offset, uint32 _private_cache)
	: tcache_t<cmp_incl_3l_l3_protocol_t> (name, 3, conf, _id, prefetch_alg),
	interconnect_id_offset(id_offset), num_private_cache(_private_cache)
	// Three links for this protocol
{ 
	uint32 	num_procs = mem_hier_t::ptr()->get_num_processors();
	if (g_conf_sep_kern_cache) 
		num_procs *= 2;
    
    stat_cpu_ld_requests = stats->COUNTER_GROUP ("cpu_ld_requests",
		"breakdown or read requests from each cache", num_procs);
	stat_cpu_ld_misses = stats->COUNTER_GROUP ("cpu_ld_misses",
		"breakdown or read misses from each cache", num_procs);
	stat_cpu_st_requests = stats->COUNTER_GROUP ("cpu_st_requests",
		"breakdown or readex requests from each cache", num_procs);
	stat_cpu_st_misses = stats->COUNTER_GROUP ("cpu_st_misses",
		"breakdown or readex misses from each cache", num_procs);
	stat_num_writebacks = stats->COUNTER_BASIC ("num_writebacks",
		"number of writebacks to MM");
	
	stat_num_req_fwd = stats->COUNTER_BASIC("request_fwd",
		"# of request forwards");
		
    stat_force_invalidation = stats->COUNTER_BASIC("force_invalidation", 
        "# of invalidation due to restricted associativity");
    
	stat_read_c2c = new group_counter_t * [num_procs];
	stat_fetch_c2c = new group_counter_t * [num_procs];
	stat_write_c2c = new group_counter_t * [num_procs];
	char stat_name[128];
	for (uint32 i = 0; i < num_procs; i++) {
		snprintf(stat_name, 128, "read_c2c_proc%u", i);
		stat_read_c2c[i] = stats->COUNTER_GROUP (stat_name,
			"breakdown or read c2c from each cache", num_procs);
	}
	for (uint32 i = 0; i < num_procs; i++) {
		snprintf(stat_name, 128, "fetch_c2c_proc%u", i);
		stat_fetch_c2c[i] = stats->COUNTER_GROUP (stat_name,
			"breakdown or fetch c2c from each cache", num_procs);
	}
	for (uint32 i = 0; i < num_procs; i++) {
		snprintf(stat_name, 128, "write_c2c_proc%u", i);
		stat_write_c2c[i] = stats->COUNTER_GROUP (stat_name,
			"breakdown or write c2c from each cache", num_procs);
	}
    
    stat_atomic_write_c2c = stats->COUNTER_BASIC("stat_atomic_wc2c", "# of atomic wc2c");
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


void
cmp_incl_3l_l3_cache_t::debug_busy_set(tfn_info_t *t)
{
    addr_t index = get_index(t->address);
    for (uint32 i = 0; i < assoc; i++)
        cout << "state " << cmp_incl_3l_l3_protocol_t::state_names[lines[index][i].get_state()][0] << endl;
    
}

uint32
cmp_incl_3l_l3_cache_t::get_action(message_t *msg)
{
	// upmsg_t same as downmsg_t
	upmsg_t *message = NULL;
	
	
	if (is_up_message(msg)) {
        
        message = static_cast<upmsg_t *> (msg);
		switch(message->get_type()) {
		case TypeRead:
			return ActionL2Read;
			
		case TypeReadEx:
			return ActionL2ReadEx;
			
		case TypeUpgrade:
			return ActionL2Upgrade;
			
		case TypeReplace:
			return ActionReplace;
			
        case TypeReplaceNote:
            return ActionL2Replace;
            
		case TypeWriteBack:
			return ActionL2WriteBack;
			
		case TypeWriteBackClean:
			return ActionL2WriteBackClean;
			
		case TypeInvAck:
			return ActionL2InvAck;
		
        case TypeReadPref:
            return ActionL2Prefetch;

		default:
			FAIL_MSG("Invalid message type %s\n",
					 upmsg_t::names[(int)message->get_type()][0].c_str());
		}
	} else  {
        ASSERT(is_down_message(msg));
		downmsg_t *downmsg = static_cast<downmsg_t *> (msg);
		
		switch(downmsg->get_type()) {
		case downmsg_t::DataResp:
			return ActionDataResponse;
			
		default:
			FAIL_MSG("Invalid message type %s\n",
					 downmsg_t::names[(int)downmsg->get_type()][0].c_str());
		}
	}
}


void
cmp_incl_3l_l3_cache_t::prefetch_helper(addr_t message_addr, mem_trans_t *trans,
	bool read)
{
    ASSERT(read);
    upmsg_t *msg = new upmsg_t(trans,  message_addr, get_lsize(), NULL,
        id, id, upmsg_t::TypeReadPref, id, true);
        
    message_arrival(msg);
}


bool 
cmp_incl_3l_l3_cache_t::wakeup_signal(message_t* msg, prot_line_t *line, 
    uint32 action)
{
    return true;
}

void
cmp_incl_3l_l3_cache_t::trigger(tfn_info_t *t, tfn_ret_t *ret)
{
	ret->block_message = StallNone;
	ret->next_state = StateUnknown;
	uint32 i;
    	

    switch(t->curstate) {

	/// NotPresent
	case StateNotPresent:
		switch(t->action) {
			
		case ActionL2Read:
		case ActionL2ReadEx:
        case ActionL2Upgrade:
			ret->block_message = can_insert_new_line(t, ret);
			if (ret->block_message != StallNone) {
                debug_busy_set(t);
                goto block_message;
            }
			// L2 WriteBack/Request can appear reordered because 
			//   L2 waits for L1 ack before sending WB

			if (!can_allocate_mshr(t, ret)) ret->block_message = StallOtherEvent;
			if (!can_enqueue_request_mshr(t, ret)) ret->block_message = StallPoll;
			if (!can_send_msg_down(t, ret)) ret->block_message = StallPoll;
			if (ret->block_message != StallNone) goto block_message;
			
			insert_new_line(t, ret);
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			mark_requester_as_sharer(t, ret);
			send_msg_down(t, ret, downmsg_t::Read);
            if (t->action == ActionL2Read)
                ret->next_state = StateInvToSh;
            else
                ret->next_state = StateInvToMod;
			t->trans->mark_pending(MEM_EVENT_L3_DEMAND_MISS);
			break;
        case ActionL2Prefetch:
            send_msg_down(t, ret, downmsg_t::Read);
            ret->next_state = t->curstate;
            break;
		default: goto error;
		}
		break;

	/// Invalid
	case StateInvalid:
		switch(t->action) {
			
		case ActionL2Read:
		case ActionL2ReadEx:
        case ActionL2Upgrade:
			if (!can_allocate_mshr(t, ret))        goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_other;
			if (!can_send_msg_down(t, ret))       goto block_message_poll;
			
			allocate_mshr(t, ret);
			enqueue_request_mshr(t, ret);
			mark_requester_as_sharer(t, ret);
			send_msg_down(t, ret, downmsg_t::Read);
			ret->next_state = StateInvToMod;
			
			t->trans->mark_pending(MEM_EVENT_L3_DEMAND_MISS);
			break;
			
		case ActionReplace: goto donothing;
		case ActionL2Prefetch : goto donothing;	
		default: goto error;
		}
		break;

	/// InvalidToMod
	case StateInvToMod:
		switch(t->action) {

		case ActionL2Read:
		case ActionL2ReadEx:
			goto block_message_set;

		case ActionDataResponse:
			perform_mshr_requests(t, ret);
			deallocate_mshr(t, ret);
			ret->next_state = StateModified;
			break;
        case ActionL2Prefetch: goto donothing;    
		default: goto error;
		}
		break;

        
    case StateInvToSh:
		switch(t->action) {

		case ActionL2Read:
		case ActionL2ReadEx:
			goto block_message_set;

		case ActionDataResponse:
			perform_mshr_requests(t, ret);
			deallocate_mshr(t, ret);
			ret->next_state = StateShared;
			break;
        case ActionL2Prefetch: goto donothing;    
		default: goto error;
		}
		break;
        
	/// Shared
	case StateShared:
        switch(t->action) {

		case ActionL2Read:
            send_data_to_requester(t, ret, true);    
			mark_requester_as_sharer(t, ret);
			ret->next_state = t->curstate;
			t->trans->mark_pending(MEM_EVENT_L3_HIT);
			break;
			
		case ActionL2ReadEx:
			if (!can_allocate_mshr(t, ret))        goto block_message_other;
			if (!can_enqueue_request_mshr(t, ret)) goto block_message_other;
			// keep stats for invalidates
            ASSERT(t->trans->atomic || !t->trans->read); 
			for (i = 0; i < prot_line_t::MAX_SHARERS; i++) {
				if (t->line->is_sharer(i) && t->trans->cpu_id != i) {
					if (t->trans->is_thread_state())
                        STAT_INC(stat_num_vcpu_c2c);
                    else {
                        STAT_INC(stat_write_c2c[t->trans->cpu_id], i);
                        if (t->trans->atomic) STAT_INC(stat_atomic_write_c2c);
                    }
                }
			}
            if (t->line->get_num_sharers()) {
                allocate_mshr(t, ret);
                enqueue_request_mshr(t, ret);
                mark_requester_as_sharer(t, ret);
                send_invalidate_to_sharers(t, ret, false); // not excl. req.
                ret->next_state = StateShToMod;
                t->trans->mark_pending(MEM_EVENT_L3_COHER_MISS);
            } else {
                mark_requester_as_sharer(t, ret);
                send_data_to_requester(t, ret, false);
                t->trans->mark_pending(MEM_EVENT_L3_HIT);
                ret->next_state = StateModified;
            }
            
			break;
			
		case ActionL2Upgrade:
            if (requester_is_only_sharer(t, ret)) { // No other sharers
				send_ack_up(t, ret, TypeUpgradeAck);
                ret->next_state = StateModified;
				t->trans->mark_event(MEM_EVENT_L3_HIT);
            } else {
				if (!can_allocate_mshr(t, ret))        goto block_message_other;
				if (!can_enqueue_request_mshr(t, ret)) goto block_message_other;
                ASSERT(t->trans->atomic || !t->trans->read); 
				// keep stats for invalidates
				for (i = 0; i < prot_line_t::MAX_SHARERS; i++) {
					if (t->line->is_sharer(i) && t->trans->cpu_id != i) {
                        if (t->trans->is_thread_state())
                            STAT_INC(stat_num_vcpu_c2c);
                        else {
                            STAT_INC(stat_write_c2c[t->trans->cpu_id], i);
                            if (t->trans->atomic) STAT_INC(stat_atomic_write_c2c);
                        }
                    }
				}
				allocate_mshr(t, ret);
                enqueue_request_mshr(t, ret);
				send_invalidate_to_sharers(t, ret, false); // not excl. req.
				ret->next_state = StateShToMod;
				t->trans->mark_pending(MEM_EVENT_L3_COHER_MISS);
			}
			break;
			
		case ActionL2Replace:  
            remove_source_sharer(t, ret);
            send_ack_up(t, ret, TypeReplaceAck);
			ret->next_state = t->curstate;
            break;
		
        case ActionL2WriteBack:
        case ActionL2WriteBackClean:
			ASSERT(!requester_is_sharer(t, ret) ||
				!requester_is_only_sharer(t, ret));
            remove_source_sharer(t, ret);
			send_ack_up(t, ret, TypeWriteBackAck);
            ret->next_state = t->curstate;
            break;

        case ActionL2Prefetch: goto donothing;    
        case ActionReplace:
            if (t->line->get_num_sharers()) {
                if (!can_allocate_writebackbuffer()) goto block_message_other;
                allocate_writebackbuffer(t->line);
                send_invalidate_to_sharers(t, ret, true); // whether incl or not
            }
			ret->next_state = StateInvalid;  // wbb line goes to approp. state
			break;
		default: goto error;
		}
		break;
	
	/// ShToMod
	case StateShToMod:
		switch (t->action) {
		
		case ActionL2Read:
		case ActionL2ReadEx:
            goto block_message_set;
		case ActionL2Upgrade:
            goto donothing; // Drop it ... L2 will reissue
			
		case ActionL2Replace:
            // Need Not Send an ACK up
		case ActionL2InvAck:
			remove_source_sharer(t, ret);
			if (is_only_one_sharer(t, ret)) {
				perform_mshr_requests(t, ret);
				deallocate_mshr(t, ret);
				ret->next_state = StateModified;
            } else 
                ret->next_state = t->curstate;
			break;
        case ActionL2WriteBack:
        case ActionL2WriteBackClean:
			ASSERT(!requester_is_sharer(t, ret) ||
				!requester_is_only_sharer(t, ret));
            remove_source_sharer(t, ret);
            send_ack_up(t, ret, TypeWriteBackAck);
            ret->next_state = t->curstate;
            break;
        case ActionL2Prefetch: goto donothing;    
		default: goto error;
		}
		break;
		
	/// Modified
	case StateModified:
		switch (t->action) {
		
		case ActionL2Read:
			if (!can_allocate_mshr(t, ret))        goto block_message_other;
			
			// don't enqueue this request (but allocate for future reqs)
			//allocate_mshr(t, ret);
			send_request_forward(t, ret, TypeReadFwd);
            mark_requester_as_sharer(t, ret);
			t->trans->mark_pending(MEM_EVENT_L3_COHER_MISS);
			ret->next_state = StateShared;
			break;
		
		case ActionL2ReadEx:
		case ActionL2Upgrade:
            // request Forward must be sent first
            ASSERT(requester_is_sharer(t, ret) == false);
            send_request_forward(t, ret, TypeReadExFwd);
			remove_only_sharer(t, ret);
			mark_requester_as_sharer(t, ret);
			t->trans->mark_pending(MEM_EVENT_L3_COHER_MISS);
			ret->next_state = t->curstate;
			break;
			
		case ActionL2WriteBack:	
		case ActionL2WriteBackClean:
            send_ack_up(t, ret, TypeWriteBackAck);
            // It is possible that another cache may requested a ReadEx 
            // which was forwarded to the previous owner. In the meantime
            // the previous owner replaced the line.
            if (requester_is_sharer(t, ret)) {
                remove_source_sharer(t, ret);
                ret->next_state = StateShared;
            } else {
			// Replacement sent before ReadEx Fwd, so someone else is the owner
			    ret->next_state = t->curstate;
            }
			break;
		case ActionL2Prefetch: goto donothing;
        case ActionReplace:
			if (!can_allocate_writebackbuffer()) goto block_message_other;
			allocate_writebackbuffer(t->line);
			send_invalidate_to_sharers(t, ret, true); // whether incl or not
			ret->next_state = StateInvalid;  // wbb line goes to approp. state
			break;    
		default: goto error;
		}
		break;

        
    case StateShToInv:
        
        switch (t->action) {
        case ActionL2Read:
        case ActionL2ReadEx:
        case ActionL2Upgrade:
            ASSERT(t->line->get_num_sharers());
            goto block_message_set;
        case ActionL2InvAck:
        case ActionL2Replace:    
            STAT_INC(stat_force_invalidation);
			remove_source_sharer(t, ret);
			if (t->line->get_num_sharers() == 0) {
              //  cout << "Successfully releasing the lines back!!" << endl;
				release_writebackbuffer(get_line_address(t->address));
				ret->next_state = StateInvalid;
            } else 
                ret->next_state = t->curstate;
			break;
        default:
            FAIL;
        }
        break;
        
    case StateModToInv:
        
        switch (t->action) {
        case ActionL2Read:
        case ActionL2ReadEx:
        case ActionL2Upgrade:
            goto block_message_set;
        case ActionL2WriteBack:
        case ActionL2WriteBackClean:    
            STAT_INC(stat_force_invalidation);
			remove_source_sharer(t, ret);
			ASSERT(t->line->get_num_sharers() == 0);
            //cout << "Successfully releasing the dirty lines back!!" << endl;
            send_msg_down(t, ret, downmsg_t::WriteBack);
            release_writebackbuffer(get_line_address(t->address));
            ret->next_state = StateInvalid;
			break;
        default:
            FAIL;
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

	profile_transition(t, ret);
	return;


/// Blocking Actions 
block_message_poll:
	if (ret->block_message == StallNone) ret->block_message = StallPoll;
	// fallthrough
block_message_set:
	if (ret->block_message == StallNone) ret->block_message = StallSetEvent;
	// fallthrough
block_message_other:
	if (ret->block_message == StallNone) ret->block_message = StallOtherEvent;
	// fallthrough
block_message:
	ret->next_state = t->curstate;
	t->trans->mark_pending(MEM_EVENT_L3_STALL);
    DEBUG_TR_FN("Blocking Message block_type %u", t, ret, ret->block_message);
	profile_transition(t, ret);
	return;
	

/// Other common actions	
error:
	FAIL_MSG("%10s @%llu 0x%016llx:Invalid action: %s in state: %s\n",get_cname(), 
		mem_hier_t::ptr()->get_module_obj()->chip->get_g_cycles(), t->message->address, 
        action_names[t->action][0].c_str(),state_names[t->curstate][0].c_str());
	
donothing:
	ret->next_state = t->curstate;
	profile_transition(t, ret);
	return;
}


// Send data response back to requester
void
cmp_incl_3l_l3_cache_t::send_data_to_requester(tfn_info_t *t, tfn_ret_t *ret, 
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
		inmsg->get_requester());
        
    
}

////////////////////////////////////////////////////////////////////////////////
// CMP exclusice L2DIR cache specific transition functions

// Can send address requests down
bool
cmp_incl_3l_l3_cache_t::can_send_msg_down(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	// return links[mainmem_addr_link]->can_send();
	return true;
}
	
void
cmp_incl_3l_l3_cache_t::send_msg_down(tfn_info_t *t, tfn_ret_t *ret,
	uint32 type)
{
	DEBUG_TR_FN("%s msg type %s", t, ret, __func__, 
        downmsg_t::names[type][0].c_str());
	
	uint32 link;
	if (type == downmsg_t::WriteBack) {
		link = mainmem_data_link;
		STAT_INC(stat_num_writebacks);
	} else 
		link = mainmem_addr_link;
	
	addr_t addr = get_line_address(t->address);
	downmsg_t *msg = new downmsg_t(t->trans, addr, get_lsize(), NULL, type);

	stall_status_t retval = send_message(links[link], msg);
	ASSERT(retval == StallNone);
}


// Send read forward to some requester
void
cmp_incl_3l_l3_cache_t::send_request_forward(tfn_info_t *t, tfn_ret_t *ret, 
	uint32 type)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	
	// TODO: some heuristic to pick a sharer if more than one
	uint32 sharer;
	uint32 requester = inmsg->get_requester();
	//for (sharer = 0; sharer < prot_line_t::MAX_SHARERS; sharer++)
	//	if (t->line->is_sharer(sharer)) break;
	
	// Pick the one who is numerically closest
	for (uint32 i = 0; i <= num_private_cache/2; i++) {
		sharer = (requester + i) % num_private_cache;
		if (t->line->is_sharer(sharer)) break;
		sharer = (requester - i + num_private_cache) % num_private_cache;
		if (t->line->is_sharer(sharer)) break;
	}
	
    if (t->trans->is_thread_state())
        STAT_INC(stat_num_vcpu_c2c);
	else {	
        if (type == TypeReadFwd) {
        	if (t->trans->ifetch)
                STAT_INC(stat_fetch_c2c[t->trans->cpu_id], sharer);
            else {
                STAT_INC(stat_read_c2c[t->trans->cpu_id], sharer);
            }
        }
        else {
            ASSERT(t->trans->atomic || !t->trans->read); 
            STAT_INC(stat_write_c2c[t->trans->cpu_id], sharer);
            if (t->trans->atomic) STAT_INC(stat_atomic_write_c2c);
        }
    }
    ASSERT(sharer != prot_line_t::MAX_SHARERS);
	send_protocol_message_up(t, ret, type, sharer, inmsg->get_requester());
}

void
cmp_incl_3l_l3_cache_t::send_ack_up(tfn_info_t *t, tfn_ret_t *ret, uint32 type)
{
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
    DEBUG_TR_FN("%s sending %s to %u", t, ret, __func__, 
        cmp_excl_3l_protocol_t::names[type][0].c_str(),inmsg->get_source());
	send_protocol_message_up(t, ret, type, inmsg->get_source(),
		inmsg->get_requester());
}


// Send invalidate to icache if exclusion bit set
void
cmp_incl_3l_l3_cache_t::send_invalidate_to_sharers(tfn_info_t *t, tfn_ret_t *ret,
	bool requester_too)
{
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);  
	uint32 requester = inmsg->get_requester();
	DEBUG_TR_FN("%s requester %u", t, ret, __func__, requester);
	
	for (uint32 i = 0; i < prot_line_t::MAX_SHARERS; i++) {
		if (!t->line->is_sharer(i)) continue;
		if (!requester_too && i == requester) continue;
		
		send_protocol_message_up(t, ret, TypeInvalidate, i, requester);
	}
}


// Enqueue this read request to the MSHR for this address
void
cmp_incl_3l_l3_cache_t::enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_3l_l3_cache_t::perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret)
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
		
		// assert for L1 & L2 too
		ASSERT(request->trans->occurred(
			MEM_EVENT_L1_DEMAND_MISS | MEM_EVENT_L1_COHER_MISS |
			MEM_EVENT_L1_MSHR_PART_HIT));
		ASSERT(request->trans->occurred(
			MEM_EVENT_L2_DEMAND_MISS | MEM_EVENT_L2_COHER_MISS |
			MEM_EVENT_L2_MSHR_PART_HIT));
			
		// Clear events for this transaction
		ASSERT(request->trans->occurred(
			MEM_EVENT_L3_DEMAND_MISS | MEM_EVENT_L3_COHER_MISS));
		request->trans->clear_pending_event(
			MEM_EVENT_L3_DEMAND_MISS | MEM_EVENT_L3_COHER_MISS);

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
            else if (state == StateInvToSh)
                response_type = TypeDataRespShared;
			else
				FAIL;
		} else
			FAIL;
		
		DEBUG_TR_FN("%s: sending %s to %d", t, ret, __func__, 
			names[response_type][0].c_str(), request->requester_id);
	
		request->trans->clear_pending_event(
			MEM_EVENT_L3_DEMAND_MISS | 
			MEM_EVENT_L3_COHER_MISS);

		send_protocol_message_up(t, ret, response_type, request->requester_id,
			request->requester_id);
        delete request;    
		request = mshr->pop_request();
	}
	
}

void
cmp_incl_3l_l3_cache_t::send_protocol_message_up(tfn_info_t *t, tfn_ret_t *ret,
	uint32 type, uint32 destination, uint32 requester)
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
	
	
	upmsg_t *msg = new upmsg_t(t->trans, addr, get_lsize(), data,
		get_interconnect_id(addr), destination, type, requester, addr_link);
		
	stall_status_t retval = send_message(links[interconnect_link], msg);
	ASSERT(retval == StallNone);

}


void
cmp_incl_3l_l3_cache_t::mark_requester_as_sharer(tfn_info_t *t, tfn_ret_t *ret) {
	DEBUG_TR_FN("%s", t, ret, __func__);
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	t->line->add_sharer(inmsg->get_requester());
}

void
cmp_incl_3l_l3_cache_t::remove_source_sharer(tfn_info_t *t, tfn_ret_t *ret) {
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
    DEBUG_TR_FN("%s: removing %u", t, ret, __func__, inmsg->get_source());
	t->line->remove_sharer(inmsg->get_source());  // user sender not requester
}

// Remove only sharer (owner)
void
cmp_incl_3l_l3_cache_t::remove_only_sharer(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_3l_l3_cache_t::requester_is_sharer(tfn_info_t *t, tfn_ret_t *ret)
{
    upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
    DEBUG_TR_FN("%s sharers %u requester %u", t, ret, __func__, 
        t->line->get_sharers(), inmsg->get_requester());
	return (t->line->is_sharer(inmsg->get_requester()));
}



line_t *
cmp_incl_3l_l3_cache_t::choose_replacement(addr_t idx)
{
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



bool
cmp_incl_3l_l3_cache_t::is_only_one_sharer(tfn_info_t *t, tfn_ret_t *ret)
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
cmp_incl_3l_l3_cache_t::requester_is_only_sharer(tfn_info_t *t, tfn_ret_t *ret)
{
	
	upmsg_t *inmsg = static_cast <upmsg_t *> (t->message);
	ASSERT(t->line->is_sharer(inmsg->get_requester()));
    DEBUG_TR_FN("%s sharers %u requester %u", t, ret, __func__, 
        t->line->get_sharers(), inmsg->get_requester());
	if ((t->line->get_sharers() & (~(1 << inmsg->get_requester()))) == 0)
		return true;
	
	return false;
}

uint32
cmp_incl_3l_l3_cache_t::get_interconnect_id(addr_t addr) {
	uint32 bank = get_bank(addr);
	return (bank+interconnect_id);
}

bool
cmp_incl_3l_l3_cache_t::is_request_message(message_t *message)
{
	if (!is_up_message(message))
		return false;
	
	upmsg_t *inmsg = static_cast<upmsg_t *>(message);
	return (inmsg->get_type() == TypeRead ||
		inmsg->get_type() == TypeReadEx ||
		inmsg->get_type() == TypeUpgrade);
}

void
cmp_incl_3l_l3_cache_t::profile_transition(tfn_info_t *t, tfn_ret_t *ret)
{
    // Ignore profiling random_trans
	tcache_t<cmp_incl_3l_l3_protocol_t>::profile_transition(t, ret);
	
}

////////////////////////////////////////////////////////////////////////////////
// Uniprocessor onelevel cache constant definitions

const string cmp_incl_3l_l3_protocol_t::action_names[][2] = {
	{ "Replace", "Replace line" }, 
	{ "Flush", "Flush line" }, 
	{ "L2Read", "L2 Read Request" }, 
	{ "L2ReadEx", "L2 ReadExclusive Request" }, 
	{ "L2Upgrade", "L2 Upgrade Request" },
	{ "L2Replace", "L2 Replace Notify Message" },
	{ "L2WriteBack", "L2 Write Back" },
	{ "L2WriteBackClean", "L2 Write Back" },
	{ "L2InvAck", "L2 Invalidate Acknowledgement" },
	{ "L2FwdAck", "L2 Request Forwared Acknowledgement" },
	{ "DataResponse", "Data Response from Memory" },
    { "L2Prefetch", "Prefetch request into L3"},
};
const string cmp_incl_3l_l3_protocol_t::state_names[][2] = {
	{ "Unknown", "Unknown" },
	{ "NotPresent", "Line is not present in cache" },
	{ "Invalid", "Data may be present but is invalid" },
	{ "InvToMod", "Invalid" },
    { "InvToShared", "Invalid to shared"},
	{ "Shared", "" },
	{ "ShToMod", "" },
	{ "Modified", "" },
	{ "ModToSh", "" },
    { "ShToInv", "" },
    { "ModToInv", ""},
};

const string cmp_incl_3l_l3_protocol_t::prot_name[] = 
	{"cmp_incl_3l_l3", "Two-level CMP L2 Directory Array"};
