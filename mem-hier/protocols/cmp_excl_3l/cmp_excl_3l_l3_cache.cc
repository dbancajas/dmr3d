/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    implementation of a two-level CMP L3 cache
 * initial author: Philip Wells 
 *
 */
 
 
//  #include "simics/first.h"
// RCSID("$Id: cmp_excl_3l_l3_cache.cc,v 1.1.2.7 2006/03/02 23:58:43 pwells Exp $");

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

#include "cmp_excl_3l_l3_cache.h"

// Link definitions
const uint8 cmp_excl_3l_l3_protocol_t::l2dir_link;
const uint8 cmp_excl_3l_l3_protocol_t::mainmem_addr_link;
const uint8 cmp_excl_3l_l3_protocol_t::mainmem_data_link;
const uint8 cmp_excl_3l_l3_protocol_t::io_link;  

/////////////////////////////////////////////////////////////////////////////////
// Uniprocessor one-level line functions
//
void
cmp_excl_3l_l3_line_t::init(cache_t *_cache, addr_t _idx, uint32 _lsize)
{
	line_t::init(_cache, _idx, _lsize);
	state = StateInvalid;
}

void
cmp_excl_3l_l3_line_t::wb_init(addr_t tag, addr_t idx, line_t *cache_line)
{
	FAIL;
}

void
cmp_excl_3l_l3_line_t::reinit(addr_t _tag)
{
	line_t::reinit(_tag);
	state = StateInvalid;
}	
	
bool
cmp_excl_3l_l3_line_t::is_free()
{
	return (state == StateInvalid);
}	
	
bool
cmp_excl_3l_l3_line_t::can_replace()
{
	// Assume there are enough ways to keep all lines with dont_evict marked
	if (g_conf_keep_thread_state)
		return (!mem_hier_t::is_thread_state(get_address()));

	// Can replace if not in a busy state
	return true;
}

bool
cmp_excl_3l_l3_line_t::is_stable()
{
	// Same as can_replace
	return true;
}

void
cmp_excl_3l_l3_line_t::set_state(uint32 _state)
{
	state = _state;
}
	
uint32
cmp_excl_3l_l3_line_t::get_state()
{
	return state;
}

void
cmp_excl_3l_l3_line_t::to_file(FILE *file)
{
	fprintf(file, "%u ", state);
	line_t::to_file(file);
}

void
cmp_excl_3l_l3_line_t::from_file(FILE *file)
{
	fscanf(file, "%u ", &state);
	line_t::from_file(file);
}

/////////////////////////////////////////////////////////////////////////////////
// CMP exclusive L3 cache functions
//

cmp_excl_3l_l3_cache_t::cmp_excl_3l_l3_cache_t(string name, cache_config_t *conf,
	uint32 _id, uint64 prefetch_alg)
	: tcache_t<cmp_excl_3l_l3_protocol_t> (name, 3, conf, _id, prefetch_alg)
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
}

uint32
cmp_excl_3l_l3_cache_t::get_action(message_t *msg)
{
	if (is_up_message(msg)) {
		upmsg_t *upmsg = static_cast<upmsg_t *> (msg);

		switch(upmsg->get_type()) {
		case TypeRead:
            return ActionL2Get;
			
		case TypeWriteBack:
			return ActionL2WriteBack;
			
		case TypeWriteBackClean:
			return ActionL2Replace;
		case TypeReadPref:
            return ActionPrefetch;
            
		default:
			FAIL_MSG("Invalid message type %s\n",
					 upmsg_t::names[(int)upmsg->get_type()][0].c_str());
		}
	} else if (is_down_message(msg)) {
		downmsg_t *downmsg = static_cast<downmsg_t *> (msg);
		
		switch(downmsg->get_type()) {
		case downmsg_t::DataResp:
			return ActionDataResp;
			
		default:
			FAIL_MSG("Invalid message type %s\n",
					 downmsg_t::names[(int)downmsg->get_type()][0].c_str());
		}
	} else {
		FAIL;
	}
}

void
cmp_excl_3l_l3_cache_t::prefetch_helper(addr_t message_addr, mem_trans_t *trans,
	bool read)
{
    ASSERT(read);
    upmsg_t *msg = new upmsg_t(trans,  message_addr, get_lsize(), NULL,
        id, id, upmsg_t::TypeReadPref, id, true);
        
    message_arrival(msg);
}

bool 
cmp_excl_3l_l3_cache_t::wakeup_signal(message_t* msg, prot_line_t *line, 
    uint32 action)
{
    return true;
}

void
cmp_excl_3l_l3_cache_t::trigger(tfn_info_t *t, tfn_ret_t *ret)
{
	ret->block_message = StallNone;
	ret->next_state = StateUnknown;

    switch(t->curstate) {

	/// NotPresent
	case StateNotPresent:
		switch(t->action) {
			
		case ActionL2Get:
			send_msg_down(t, ret, downmsg_t::Read);
			ret->next_state = t->curstate;
			t->trans->mark_pending(MEM_EVENT_L3_DEMAND_MISS);
			break;

		case ActionL2WriteBack:
			ret->block_message = can_insert_new_line(t, ret);
			if (ret->block_message != StallNone) goto block_message;
			insert_new_line(t, ret);
			t->line->mark_use();
			store_data_to_cache(t, ret);
			ret->next_state = StateDirty;
            break;
			
        case ActionL2Replace:
			ret->block_message = can_insert_new_line(t, ret);
			if (ret->block_message != StallNone) goto block_message;
			insert_new_line(t, ret);
			store_data_to_cache(t, ret);
			t->line->mark_use();
			ret->next_state = StateClean;
            break;
			
		case ActionDataResp:
			send_msg_up(t, ret, upmsg_t::TypeDataRespExclusive);
			ret->next_state = t->curstate;
			break;
			
        case ActionPrefetch:
            ret->block_message = can_insert_new_line(t, ret);
            if (ret->block_message != StallNone) goto block_message;
            insert_new_line(t, ret);
            send_msg_down(t, ret, downmsg_t::Read);
            t->line->mark_prefetched(true);
            t->trans->mark_pending(MEM_EVENT_L3_DEMAND_MISS);
            ret->next_state = StatePrefetchPend;
            break;
		default: goto error;
		}
		break;

	/// Invalid
    case StatePrefetchPend:
        switch(t->action) {
            case ActionDataResp:
                store_data_to_cache(t, ret);
                ret->next_state = StateClean;
                t->trans->completed = true;
                break;
            case ActionL2Get:
                ret->block_message = StallSetEvent;
                goto block_message;
            case ActionPrefetch:
                ret->next_state = t->curstate;
                break;
            default: goto error;
        }
        break;
                
	case StateInvalid:
		switch(t->action) {
			
		case ActionL2Get:
			send_msg_down(t, ret, downmsg_t::Read);
			ret->next_state = t->curstate;
			t->trans->mark_pending(MEM_EVENT_L3_DEMAND_MISS);
			break;

		case ActionL2WriteBack:
			store_data_to_cache(t, ret);
			t->line->mark_use();
			ret->next_state = StateDirty;
            break;
			
        case ActionL2Replace:
			store_data_to_cache(t, ret);
			t->line->mark_use();
			ret->next_state = StateClean;
            break;
			
		case ActionDataResp:
			send_msg_up(t, ret, upmsg_t::TypeDataRespExclusive);
			ret->next_state = t->curstate;
			break;
        case ActionPrefetch:
            send_msg_down(t, ret, downmsg_t::Read);
            ret->next_state = StatePrefetchPend;
            t->line->mark_prefetched(true);
            t->trans->mark_pending(MEM_EVENT_L3_DEMAND_MISS);
            break;
			
		default: goto error;
		}
		break;

	/// Clean
	case StateClean:
		switch(t->action) {

		case ActionL2Get:
			send_msg_up(t, ret, upmsg_t::TypeDataRespExclusive);
			ret->next_state = StateInvalid;

			t->trans->mark_event(MEM_EVENT_L3_HIT);
			break;
			
		case ActionReplace:
			ret->next_state = StateInvalid;
			break;

            
        case ActionPrefetch:
            ret->next_state = t->curstate;
            break;
		default: goto error;
		}
		break;
	
	/// Dirty
	case StateDirty:
		switch(t->action) {

		case ActionL2Get:
			send_msg_up(t, ret, upmsg_t::TypeDataRespExclusive);
			ret->next_state = StateInvalid;

			t->trans->mark_event(MEM_EVENT_L3_HIT);
			break;
			
		case ActionReplace:
			send_msg_down(t, ret, downmsg_t::WriteBack);
            t->trans->mark_pending(MEM_EVENT_MAINMEM_WRITE_BACK);
			ret->next_state = StateInvalid;
			break;
            
        
        case ActionPrefetch:
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

	profile_transition(t, ret);
	return;


/// Blocking Actions 
block_message:
	ret->next_state = t->curstate;
	t->trans->mark_pending(MEM_EVENT_L3_STALL);
    DEBUG_TR_FN("Blocking Message block_type %u", t, ret, ret->block_message);
	profile_transition(t, ret);
	return;
	

/// Other common actions	
error:
	FAIL_MSG("%10s @%llu 0x%016llx:Invalid action: %s in state: %s\n",get_cname(), 
		external::get_current_cycle(), t->message->address, 
        action_names[t->action][0].c_str(),state_names[t->curstate][0].c_str());
}

bool
cmp_excl_3l_l3_cache_t::is_request_message(message_t *message)
{
	if (!is_up_message(message))
		return false;
	
	upmsg_t *inmsg = static_cast<upmsg_t *>(message);
	return (inmsg->get_type() == TypeRead || inmsg->get_type() == TypeReadPref);
}


////////////////////////////////////////////////////////////////////////////////
// CMP exclusice L3 cache specific transition functions

void 
cmp_excl_3l_l3_cache_t::profile_transition(tfn_info_t *t, tfn_ret_t *ret)
{
	tcache_t<cmp_excl_3l_l3_protocol_t>::profile_transition(t, ret);
    if (!ret->block_message && is_request_message(t->message)) {
		if (mem_hier_t::is_thread_state(t->message->address)) {
			if (t->trans->occurred(MEM_EVENT_L1DIR_HIT))
				STAT_INC(stat_num_vcpu_state_hit);
			else if ((t->trans->occurred(MEM_EVENT_L1DIR_DEMAND_MISS) ||
				t->trans->occurred(MEM_EVENT_L1DIR_COHER_MISS) ||
				t->trans->occurred(MEM_EVENT_L1DIR_REQ_FWD)))
			{
				STAT_INC(stat_num_vcpu_state_miss);
			} else
				STAT_INC(stat_num_vcpu_state_other);
			return;
		}
        
        
        if (t->trans->read) {
            STAT_INC(stat_cpu_ld_requests, t->trans->cpu_id);
            if (t->trans->occurred(MEM_EVENT_L3_DEMAND_MISS))
                STAT_INC(stat_cpu_ld_misses, t->trans->cpu_id);
        } else {
            STAT_INC(stat_cpu_st_requests, t->trans->cpu_id);
            if (t->trans->occurred(MEM_EVENT_L3_DEMAND_MISS))
                STAT_INC(stat_cpu_st_misses, t->trans->cpu_id);
        }
    }
    
}

void
cmp_excl_3l_l3_cache_t::send_msg_down(tfn_info_t *t, tfn_ret_t *ret,
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

void
cmp_excl_3l_l3_cache_t::send_msg_up(tfn_info_t *t, tfn_ret_t *ret, uint32 type)
{
	DEBUG_TR_FN("%s: %s", t, ret, __func__, 
        upmsg_t::names[type][0].c_str());

	bool addr_link = false;
	const uint8 *data = t->line ? t->line->get_data() : t->message->data;
	addr_t addr = get_line_address(t->address);
	
	upmsg_t *msg = new upmsg_t(t->trans, addr, get_lsize(), data,
		0, 0, type, 0, addr_link);
		
	stall_status_t retval = send_message(links[l2dir_link], msg);
	ASSERT(retval == StallNone);
}


////////////////////////////////////////////////////////////////////////////////
// Uniprocessor onelevel cache constant definitions

const string cmp_excl_3l_l3_protocol_t::action_names[][2] = {
	{ "Replace", "Replace line" }, 
	{ "Flush", "Flush line" }, 
	{ "L2Get", "L2 Get Request" }, 
	{ "L2WriteBack", "L2 Write Back (Dirty)" },
	{ "L2Replace", "L2 Replace (Clean)" },
	{ "DataResponse", "Data Response from Memory"} ,
    { "Prefetch", "Cache originated prefetch"},
};
const string cmp_excl_3l_l3_protocol_t::state_names[][2] = {
	{ "Unknown", "Unknown" },
	{ "NotPresent", "Line is not present in cache" },
	{ "Invalid", "Data may be present but is invalid" },
	{ "Clean", "" },
	{ "Dirty", "" },
    { "PrefetchPending", "" },
};

const string cmp_excl_3l_l3_protocol_t::prot_name[] = 
	{"cmp_excl_3l_l3", "Two-level CMP Exclusive L3 Cache"};
