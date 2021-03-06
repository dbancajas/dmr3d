/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */
 
/* $Id: tcache.cc,v 1.7.2.49 2006/08/14 14:08:56 pwells Exp $
 *
 * description:    cache with functionality common to most protocols 
 * initial author: Philip Wells 
 *
 */
 
#include "definitions.h"
#include "config_extern.h"
#include "protocols.h"
#include "device.h"
#include "cache.h"
#include "line.h"
#include "link.h"
#include "mshr.h"
#include "prefetch_engine.h"
#include "statemachine.h"
#include "external.h"
#include "debugio.h"
#include "verbose_level.h"
#include "cache_bank.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "mai.h"
#include "isa.h"
#include "dynamic.h"
#include "mai_instr.h"
#include "sequencer.h"

#include "profiles.h"

#ifdef POWER_COMPILE
#include "power_profile.h"
#include "cache_power.h"
#include "startup.h"
#include "chip.h"
#endif

// Stat manipulation functions
/*
static double stat_add2_fn(st_entry_t ** arr) {
	return (arr[0]->get_total() + arr[1]->get_total());
}
*/
static double stat_add3_fn(st_entry_t ** arr) {
	return (arr[0]->get_total() + arr[1]->get_total() +
	        arr[2]->get_total());
}

static double stat_ratio2_fn(st_entry_t ** arr) {
	return ((double)arr[0]->get_total() / (double)(arr[1]->get_total()));
}

static double stat_add3_ratio4_fn(st_entry_t ** arr) {
	return ((double)(arr[0]->get_total() + arr[1]->get_total() + 
	         arr[2]->get_total()) / (double)(arr[3]->get_total()));
}


template <class prot_t>
tcache_t<prot_t>::
tcache_t(string name, uint32 _num_links, cache_config_t *conf, uint32 _id, 
    uint64 prefetch_mask) : cache_t(name, _num_links, conf), id(_id)
{
	uint32 sets = numlines/assoc;

	// Create lines array
	lines = new prot_line_t*[sets];
	ASSERT(lines);
	
	for (uint32 i = 0; i < sets; i++) {
		//lines[i] = new prot_line_t[assoc](this,i,lsize);
		lines[i] = new prot_line_t[assoc];
		ASSERT(lines[i]);
		
		for (uint32 a = 0; a < assoc; a++) {
			lines[i][a].init(this, i, lsize);
		}
	}

	// create Write Buffers
	
	wb_lines = new prot_line_t[numwb_buffers];
	wb_valid = new bool[numwb_buffers];
	ASSERT(wb_lines && wb_valid);
	for (uint32 w = 0; w < numwb_buffers; w++) {
		wb_lines[w].init(this, (addr_t) 0, lsize);
		wb_lines[w].wb_init((addr_t ) 0, (addr_t ) 0, NULL);
		wb_valid[w] = false;
	}
	numwb_buffers_avail = numwb_buffers;
	
    // Prefetch engine
    prefetch_engine = new prefetch_engine_t<cache_type_t>((cache_type_t *)this, prefetch_mask);
	// IO Stuff
	cur_io_addr = 0;
	cur_io_size = 0;
	num_flush_wait = 0;
	kernel_cache = false;
	
	// Create MSHRS
	mshrs = new mshrs_t<mshr_type_t>(this, conf->num_mshrs,
									 conf->requests_per_mshr);
	
	// Create Banks
	banks = new cache_bank_t * [num_banks];
	for (uint32 i = 0; i < num_banks; i++) {
		banks[i] = new cache_bank_t(this, bank_bw, request_q_size, wait_q_size,
			i);
	}
	
#ifdef POWER_COMPILE    
    // Power profile
    cache_power = new cache_power_t(get_name() + string("-power"), conf, 
        mem_hier_t::ptr()->get_module_obj()->chip->get_power_obj(), is_tagarray());
    if (g_conf_power_profile) profiles->enqueue(cache_power);
#endif

    initialize_stats();
    
    
}

template <class prot_t>
void
tcache_t<prot_t>::initialize_stats()
{
	// Create stat entries
	stat_num_read_hit = stats->COUNTER_BASIC("read_hit", "# of read hits");
    stat_num_read_coher_miss = stats->COUNTER_BASIC("read_coher_miss", "# of read coher miss");
    stat_num_read_coher_miss_kernel= stats->COUNTER_BASIC("read_coher_miss_kernel", "# of kernel read coher miss");
	stat_num_read_miss = stats->COUNTER_BASIC("read_miss", "# of read misses");
    stat_num_read_miss_kernel = stats->COUNTER_BASIC("read_miss_kernel", "# of kernel read misses");
	stat_num_read_part_hit_mshr =
		stats->COUNTER_BASIC("read_part_hit_mshr",
		"# of reads that find an already allocated MSHR");
	
	st_entry_t *stat_num_read_arr[] = {
		stat_num_read_hit, 
        stat_num_read_coher_miss,
		stat_num_read_miss,
		stat_num_read_part_hit_mshr
	};
	stat_num_read = stats->STAT_PRINT("reads", "# of reads", 3, 
		stat_num_read_arr, &stat_add3_fn, true);

	st_entry_t *stat_read_hit_ratio_arr[] = {
		stat_num_read_hit,
		stat_num_read
	};
	stat_read_hit_ratio = stats->STAT_PRINT("read_hit_ratio",
		"ratio of reads that hit", 2, stat_read_hit_ratio_arr,
		&stat_ratio2_fn, false);

	stat_num_fetch_hit = stats->COUNTER_BASIC("fetch_hit", "# of fetch hits");
	stat_num_fetch_miss = stats->COUNTER_BASIC("fetch_miss", "# of fetchs");
    stat_num_fetch_miss_kernel = stats->COUNTER_BASIC("fetch_miss_kernel", "# of fetchs miss from kernel");
	stat_num_fetch_part_hit_mshr =
		stats->COUNTER_BASIC("fetch_part_hit_mshr",
		"# of fetchs that find an alfetchy allocated MSHR");
	
	st_entry_t *stat_num_fetch_arr[] = {
		stat_num_fetch_hit, 
		stat_num_fetch_miss,
		stat_num_fetch_part_hit_mshr
	};
	stat_num_fetch = stats->STAT_PRINT("fetchs", "# of fetchs", 3, 
		stat_num_fetch_arr, &stat_add3_fn, true);

	st_entry_t *stat_fetch_hit_ratio_arr[] = {
		stat_num_fetch_hit,
		stat_num_fetch
	};
	stat_fetch_hit_ratio = stats->STAT_PRINT("fetch_hit_ratio",
		"ratio of fetchs that hit", 2, stat_fetch_hit_ratio_arr,
		&stat_ratio2_fn, false);

	stat_num_ld_speculative = stats->COUNTER_BASIC("ld_speculative",
		"# of speculative lds");
	stat_num_invalidates = stats->COUNTER_BASIC("num invalidates",
		"# of processor invalidates");
	
	stat_num_read_stall = stats->COUNTER_BASIC("read_stalls",
		"# of reads that stall");
	stat_num_fetch_stall = stats->COUNTER_BASIC("fetch_stalls",
		"# of fetchs that stall");

	stat_num_write_hit = stats->COUNTER_BASIC("write_hit", "# of write hits");
    stat_num_write_coher_miss = stats->COUNTER_BASIC("write_coher_miss", "# of write coher miss");
    stat_num_write_coher_miss_kernel = stats->COUNTER_BASIC("write_coher_miss_kernel", "# of kernel write coher miss");
	stat_num_write_miss = stats->COUNTER_BASIC("write_miss", "# of write misses");
    stat_num_write_miss_kernel = stats->COUNTER_BASIC("write_miss_kernel", "# of kernel write misses");
	stat_num_write_part_hit_mshr =
		stats->COUNTER_BASIC("write_part_hit_mshr",
		"# of writes that find an already allocated MSHR");
	st_entry_t *stat_num_write_arr[] = {
		stat_num_write_hit,
        stat_num_write_coher_miss,
		stat_num_write_miss,
		stat_num_write_part_hit_mshr
	};
	stat_num_write = stats->STAT_PRINT("writes", "# of writes", 3, 
		stat_num_write_arr, &stat_add3_fn, true);

	st_entry_t *stat_write_hit_ratio_arr[] = {
		stat_num_write_hit,
		stat_num_write
	};
	stat_write_hit_ratio = stats->STAT_PRINT("write_hit_ratio",
		"ratio of writes that hit", 2, stat_write_hit_ratio_arr,
		&stat_ratio2_fn, false);
	stat_num_write_stall = stats->COUNTER_BASIC("write_stalls",
		"# of writes that stall");

	st_entry_t *stat_num_requests_arr[] = {
		stat_num_read,
		stat_num_fetch,
		stat_num_write
	};
	stat_num_requests = stats->STAT_PRINT("requests", "# of serviced requests",
		3, stat_num_requests_arr, &stat_add3_fn, true);

	st_entry_t *stat_hit_ratio_arr[] = { stat_num_read_hit, stat_num_fetch_hit,
		stat_num_write_hit, stat_num_requests };
	stat_hit_ratio = stats->STAT_PRINT("hit_ratio",
		"ratio of requests that hit", 4, stat_hit_ratio_arr,
		&stat_add3_ratio4_fn, false);
	st_entry_t *stat_num_stall_arr[] = {
		stat_num_read_stall, 
		stat_num_fetch_stall, 
		stat_num_write_stall
	};
	stat_num_stall = stats->STAT_PRINT("stalls",
		"# of request stalls", 3, stat_num_stall_arr, &stat_add3_fn, true);

	stat_num_prefetch = stats->COUNTER_BASIC("prefetch",
		"# of sent prefetches");
	stat_num_useful_pref = stats->COUNTER_BASIC("useful_prefetch",
		"# of useful prefetches");
	stat_num_partial_pref = stats->COUNTER_BASIC("partial_prefetch",
		"# of partially completed useful prefetches");
	stat_num_useless_pref = stats->COUNTER_BASIC("useless_prefetch",
		"# of useless prefetches");
        
	stat_num_vcpu_state_hit = stats->COUNTER_BASIC("vcpu_state_hit", "# of vcpu state hits");
	stat_num_vcpu_state_miss = stats->COUNTER_BASIC("vcpu_state_miss", "# of vcpu misses");
	stat_num_vcpu_state_other = stats->COUNTER_BASIC("vcpu_state_other", "# of vcpu others");
    stat_num_vcpu_c2c       = stats->COUNTER_BASIC("vcpu_c2c", "# of c2c for vcpu state");

	stat_bank_q_lat_histo = stats->HISTO_EXP2("bank_q_latency",
		"distribution of bank request queue latencies", 8,1,1);
    stat_bank_distro = stats->COUNTER_GROUP("bank_distro", 
		"distribution of requests", num_banks);

    stat_coherence_transition = new group_counter_t *[prot_t::StateMax];
    stat_coherence_action     = new group_counter_t *[prot_t::StateMax];
    // Form the String
    string state_desc_string[prot_t::StateMax];
    string action_desc_string[prot_t::ActionMax];
    for (uint32 i  = 0; i < prot_t::StateMax; i++)
    {
        state_desc_string[i] = prot_t::state_names[i][0];
    }
    for (uint32 i = 0; i < prot_t::ActionMax; i++)
    {
        action_desc_string[i] = prot_t::action_names[i][0];
    }
    
    for (uint32 i = 0; i < prot_t::StateMax; i++)
    {
        string counter_name = state_desc_string[i] + "_transition";
        string desc_string = state_desc_string[i] + " Transitions";
        stat_coherence_transition[i] =
			stats->COUNTER_BREAKDOWN(counter_name, desc_string,
            	prot_t::StateMax, state_desc_string, state_desc_string);
        counter_name = state_desc_string[i] + "_action";
        desc_string = state_desc_string[i] + " Actions";
        stat_coherence_action[i] =
			stats->COUNTER_BREAKDOWN(counter_name, desc_string, 
            	prot_t::ActionMax, action_desc_string, action_desc_string);
		if (!g_conf_print_transition_stats) {
			stat_coherence_transition[i]->set_hidden();
			stat_coherence_action[i]->set_hidden();
		}
    }
    
}


template <class prot_t>
tcache_t<prot_t>::~tcache_t()
{
	delete [] lines;
	delete mshrs;
}

// Returns NULL on miss, and the cache line on a hit (regardless of line state)
template <class prot_t>
line_t *
tcache_t<prot_t>::lookup(addr_t addr)
{
	addr_t tag = get_tag(addr);
	addr_t index = get_index(addr);

	VERBOSE_OUT(verb_t::transfn, "%10s @ %12llu 0x%016llx: lookup\n", get_cname(),
	            external::get_current_cycle(), addr);
	
    for (uint32 i = 0; i < assoc; i++) {
        if (lines[index][i].get_tag() == tag) {
            return &(lines[index][i]);
        }
    }
	
	VERBOSE_OUT(verb_t::transfn, "%10s @ %12llu 0x%016llx: lookup FAIL in cachelines\n", 
	            get_cname(), external::get_current_cycle(), addr);
	
	// Look in the Write Back buffers
	
	for (uint32 i = 0; i < numwb_buffers; i++) {
		if (wb_valid[i] && tag == wb_lines[i].get_tag() &&
			index == wb_lines[i].get_index())
		{
			return &(wb_lines[i]);
		}
	}
	
	VERBOSE_OUT(verb_t::transfn, "%10s @ %12llu 0x%016llx: lookup FAIL in WB\n",
	            get_cname(), external::get_current_cycle(), addr);
	
	
    return NULL;
}


template <class prot_t>
bool
tcache_t<prot_t>::can_allocate_writebackbuffer()
{
	if (numwb_buffers == 0) return true;
	
	return (numwb_buffers_avail > 0);
}	

template <class prot_t>
bool
tcache_t<prot_t>::allocate_writebackbuffer(line_t *line)
{
	addr_t addr = line->get_address();
	
	VERBOSE_OUT(verb_t::transfn, "%10s allocate writebuffer 0x%016llx\n", get_cname(), addr);

	// check to see if write buffer is already allocated
	if (search_writebackbuffer(addr))
		return true;
	
    for (uint32 i = 0; i < numwb_buffers; i++) {
		
		if (!wb_valid[i])
		{
			// Can Allocate the buffer here
			wb_valid[i] = true;
			wb_lines[i].wb_init(get_tag(addr), get_index(addr), line);
			numwb_buffers_avail--;
			VERBOSE_OUT(verb_t::transfn, "%10s allocate writebuffer 0x%016llx new state %s\n",
				get_cname(), addr, this->state_names[wb_lines[i].get_state()][0].c_str());
			return true;
		} else {
			VERBOSE_OUT(verb_t::transfn, "%10s %2u index=0x%016llx tag=0x%016llx\n", 
			            get_cname(), i, wb_lines[i].get_index(), wb_lines[i].get_tag());
		}
	
	}
	VERBOSE_OUT(verb_t::transfn, "%10s allocate writebuffer FAIL 0x%016llx\n", get_cname(), addr);
	
	return false;
	
	
}


template <class prot_t>
bool
tcache_t<prot_t>::search_writebackbuffer(addr_t addr)
{
	
	uint32 lookup_index;
	addr_t tag = get_tag(addr);
	addr_t index = get_index(addr);

	for (lookup_index = 0; lookup_index < numwb_buffers; lookup_index++) {
		if (wb_valid[lookup_index] && wb_lines[lookup_index].get_tag() == tag &&
			wb_lines[lookup_index].get_index() == index)
		{
			return true;
		}
	}
	
	return false;
}

// releases the Write Back buffer holding the address
template <class prot_t>
void
tcache_t<prot_t>::release_writebackbuffer(addr_t addr)
{
	
	uint32 lookup_index = numwb_buffers;
    uint32 scan_index;
    uint32 wbcount = 0;
	addr_t tag = get_tag(addr);
	addr_t index = get_index(addr);
    

	for (scan_index = 0; scan_index < numwb_buffers; scan_index++) {
		if (wb_valid[scan_index] && wb_lines[scan_index].get_tag() == tag && 
			wb_lines[scan_index].get_index() == index)
		{
			lookup_index = scan_index;
		}
		
        if (wb_valid[scan_index])
            wbcount++;
	}
	
    
	ASSERT(lookup_index < numwb_buffers);
	numwb_buffers_avail++;
	wb_lines[lookup_index].wb_init((addr_t)0,(addr_t)0,NULL);
	wb_valid[lookup_index] = false;
    if (wbcount == numwb_buffers)
        wakeup_replay_q_all();
	// No other entry should be there with this address either
	ASSERT(!search_writebackbuffer(addr));
	VERBOSE_OUT(verb_t::transfn, "%10s: @ %12llu Successful writebuffer release 0x%016llx\n", 
	            get_cname(), external::get_current_cycle(), addr);
	
	
}

// Find an free (i.e. invalid) line if any
template <class prot_t>
line_t *
tcache_t<prot_t>::find_free_line(addr_t addr)
{
	addr_t index = get_index(addr);
	prot_line_t *line;

	// Try to find an invalid line, if none fail
    for (uint32 i = 0; i < assoc; i++) {
        line = &lines[index][i];
		ASSERT(line);
        if (line->is_free()) return line;
    }
    return NULL;
}

// Choose a non-busy replacement for line idx
template <class prot_t>
line_t *
tcache_t<prot_t>::choose_replacement(addr_t idx)
{
	// LRU Replacement
	prot_line_t *line = NULL;
    for (uint32 j = 0; j < assoc; j++) {
		if (lines[idx][j].can_replace()) {
			// If the line is replacable (i.e. not busy), and either we havn't
			// found one yet, or this one was used longer ago, take it as the
			// best so far

			if (!line || line->get_last_use() >
				lines[idx][j].get_last_use()) {

				line = &lines[idx][j];
			}
        }
    }
	
	if (line)
		VERBOSE_OUT(verb_t::transfn,
	                "%10s @ %12llu 0x%016llx: replacement line address\n",
					get_cname(), external::get_current_cycle(),
					line->get_address());

	return line;
}

// is_up_message
template <class prot_t>
bool
tcache_t<prot_t>::
is_up_message(message_t *message)
{
	return (typeid(*message) == typeid(upmsg_t));
}

// is_down_message
template <class prot_t>
bool
tcache_t<prot_t>::
is_down_message(message_t *message)
{
	return (typeid(*message) == typeid(downmsg_t));
}

// is_request_message
template <class prot_t>
bool
tcache_t<prot_t>::
is_request_message(message_t *message)
{
	return is_up_message(message);
}

//////////////////////////////////////
// Generic network functions
template <class prot_t>
stall_status_t
tcache_t<prot_t>::
send_message(link_t *link, message_t * message)
{
	message->get_trans()->pending_messages++;

	VERBOSE_OUT(verb_t::links, "send_message 0x%016llx\n", message->address);
	return link->send(message);
}


/////////////////////////////////
// Required function for device_t

// is_quiet
template <class prot_t>
bool
tcache_t<prot_t>::
is_quiet()
{
	if (mshrs->get_num_allocated() > 0) return false;
	
	// Check every line to see if it is in a stable state
	uint32 sets = numlines / assoc;
	for (uint32 i = 0; i < sets; i++) {
		for (uint32 j = 0; j < assoc; j++) {
			if (!lines[i][j].is_stable()) {
				return false;
			}
		}
    }
    // Writeback buffers must be empty as well
    for (uint32 i = 0; i < numwb_buffers; i++) {
		if (wb_valid[i])
			return false;
	}
    for (uint32 i = 0; i < num_banks; i++) {
        if (!banks[i]->is_quiet()) {
            return false;
        }
    }
	return true;
}

// message_arrival
template <class prot_t>
stall_status_t
tcache_t<prot_t>::
message_arrival(message_t *message)
{
	VERBOSE_OUT(verb_t::requests,
				"%10s @ %12llu 0x%016llx: tcache::message_arrival() %u bytes trans_id %u\n",
				get_cname(), external::get_current_cycle(),
				message->address, message->size, message->get_trans()->unique_id);

	// First, check if message from I/O Device and handle appropriately
	if (message->io_type != IONone) {
		stall_status_t retval = handle_io_message(message);
		if (retval != StallNone) return StallPoll;
		// Message deleted by handle_io_message
		return StallNone;
	}
	stall_status_t ret;
	//msg_class_t mclass = message->get_msg_class();
    //bool immediate_process = (mclass == Response || mclass == RequestNoPend);
    
	//if (is_up_message(message) && !immediate_process && bank_bw > 0) {
	if (is_up_message(message) && bank_bw > 0) {
		uint32 bank = get_bank(message->address);
		ret = banks[bank]->message_arrival(message);
		stat_bank_distro->inc_total(1, bank);
		// cache bank deletes message
	}
	else {
		ret = process_message(message);
		if (ret == StallNone) delete message;
	}
	
	return ret;
}
template <class prot_t>
bool
tcache_t<prot_t>::
wakeup_signal(message_t *message, prot_line_t *line, uint32 action)
{
    if (is_down_message(message)) return true;
    return false;
}
	
// process_message
template <class prot_t>
stall_status_t
tcache_t<prot_t>::
process_message(message_t *message)
{			
	tfn_ret_t ret;
	tfn_info_t *t;

	VERBOSE_OUT(verb_t::requests,
				"%10s @ %12llu 0x%016llx: tcache::process_message() %u bytes index %u trans_id %u\n",
				get_cname(), external::get_current_cycle(),
				message->address, message->size, get_index(message->address), 
                message->get_trans()->unique_id);
				
	uint32 action = get_action(message);
	uint32 state;
    mem_trans_t *trans = message->get_trans();        
  
    
	// First, see if the line is present, and if not, run action on a
	// "NotPresent" line
	prot_line_t *line = static_cast<prot_line_t *> (lookup(message->address));
	if (!line) state = this->StateNotPresent;
	else state = line->get_state();
	
	ASSERT(!line || line->get_address() == get_line_address(message->address));
	
    t = new tfn_info_t(message->get_trans(), state, action, message->address,
		    message, line);
    
	trigger(t, &ret);
	delete t;
    

#ifdef POWER_COMPILE
    bool hit =  trans->occurred (MEM_EVENT_L1_HIT | MEM_EVENT_L1_MSHR_PART_HIT |
         MEM_EVENT_L2_HIT | MEM_EVENT_L2_MSHR_PART_HIT |
         MEM_EVENT_L3_HIT | MEM_EVENT_L3_MSHR_PART_HIT);
    if (g_conf_power_profile) cache_power->lookup_access(trans, hit);
#endif  
	
	// If the proto doesn't want to deal with this now...
	if (ret.block_message != StallNone) return ret.block_message;
	
	ASSERT(trans->pending_messages);
	trans->pending_messages--;

	if (is_up_message(message) && line) line->mark_use();
    if (wakeup_signal(message, line, action))
       wakeup_replay_q_set(get_line_address(message->address)); 
	
    prefetch_engine->initiate_prefetch(trans, is_up_message(message));

	return StallNone;
}

template <class prot_t>
stall_status_t
tcache_t<prot_t>::
handle_io_message(message_t *message)
{
	VERBOSE_OUT(verb_t::io,
				"%10s @ %12llu 0x%016llx: tcache::handle_io_message() %u bytes %d type\n",
				get_cname(), external::get_current_cycle(),
				message->address, message->size, message->io_type);

	if (message->io_type == IOAccessDone) {
		ASSERT(message->address == cur_io_addr);
		ASSERT(message->size == cur_io_size);
		cur_io_addr = 0;
		cur_io_size = 0;
		delete message;
		return StallNone;
	} else if (message->io_type == IOAccessRead ||
	           message->io_type == IOAccessWrite) {
		// We can ignire these
		delete message;
		return StallNone;
	}
	
	ASSERT(message->io_type == IOAccessBegin);
	
	// This message may have been blocked first time
	ASSERT(cur_io_addr == 0 || cur_io_addr == message->address);
	ASSERT(cur_io_size == 0 || cur_io_size == message->size);
	
	cur_io_addr = message->address;
	cur_io_size = message->size;

	if (!flush(cur_io_addr, cur_io_size, message)) return StallPoll;
	
	// Flush done, send IOAccessOK to IOdevice
	VERBOSE_OUT(verb_t::io,
				"%10s @ %12llu 0x%016llx: tcache::handle_io_message() flush done\n",
				get_cname(), external::get_current_cycle(),
				message->address, message->size, message->io_type);

	// Msg type doesn't matter, io_type does
	message->io_type = IOAccessOK;
		
	stall_status_t retval = send_message(links[2 /* FIXEME: HACK!! */], message);
	ASSERT(retval == StallNone);  // Prot doesn't handle inability to send message
	
	return StallNone;
}


template <class prot_t>
bool
tcache_t<prot_t>::
flush(addr_t addr, size_t size, message_t *message)
{
	addr_t lineaddr = get_line_address(addr);
	addr_t offset = get_offset(addr+size);
	
	uint32 num_flush = (offset + size)/get_lsize();
	if ((offset + size) % get_lsize() != 0) num_flush++;

	ASSERT(num_flush_wait == 0);
	line_t *line;
	bool done = true;
	for (uint32 i = 0; i < num_flush; i++) {
		line = lookup(lineaddr + (get_lsize() * i));
		if (!line) continue;
		if (line->is_free()) continue;
		// Otherwise, flush
		if (!flush_line(line, message)) done = false;
	}
	
	return (done);
}

template <class prot_t>
bool
tcache_t<prot_t>::
flush_line(line_t *line, message_t *message)
{
	// Trigger flush
	tfn_ret_t ret;
	tfn_info_t *t;
	
	prot_line_t *protline = static_cast<prot_line_t *> (line);
	t = new tfn_info_t(message->get_trans(), protline->get_state(),
		this->ActionFlush, line->get_address(), message, protline);
	trigger(t, &ret);
	delete t;
	
	if (ret.block_message) {
		DEBUG_TR_FN("flush_line(): block after flush\n", t, ret);
		return false;
	}
	
	return (protline->is_free());
}

template <class prot_t>
void
tcache_t<prot_t>::to_file(FILE *file)
{
	// Output class name
	fprintf(file, "%s\n", typeid(this).name());
		
	uint32 sets = numlines / assoc;
    
    
	for (uint32 idx = 0; idx < sets; idx++) {
		for (uint j = 0; j < assoc; j++) {
			lines[idx][j].to_file(file);
		}
	}
	
	// TODO: output stats as well
	// Config variables do not need to be written
}

template <class prot_t>
void
tcache_t<prot_t>::cmp_incl_read_cache_lines(FILE *file)
{
    
    bool condor_checkpoint = mem_hier_t::ptr()->is_condor_checkpoint();
    if ((!prot_t::l2_stats) && !condor_checkpoint)
    {
         uint32 read_in_lines = g_conf_warmup_l1_size * 1024 / g_conf_l1i_lsize;
        for (uint32 i = 0; i < read_in_lines; i++)
        {
            lines[0][0].from_file(file);
        }
        lines[0][0].init(this, 0, lsize);
    } else { 
        
        uint32 sets = numlines / assoc;
        for (uint32 idx = 0; idx < sets; idx++) {
            for (uint j = 0; j < assoc; j++) {
                lines[idx][j].from_file(file);
                if (!condor_checkpoint) lines[idx][j].warmup_state();
            }
        }
	}
    
}

template <class prot_t>
void
tcache_t<prot_t>::cmp_3l_read_cache_lines(FILE *file, bool inclusive)

{
    // Cleanup_previous stats will eat up everything for L1 and L2
    if (prot_t::l3_stats) {
        uint32 sets = numlines / assoc;
        for (uint32 idx = 0; idx < sets; idx++) {
            for (uint j = 0; j < assoc; j++) {
                lines[idx][j].from_file(file);
                if (inclusive) lines[idx][j].warmup_state();
            }
        }
    }
        
    
    
}


template <class prot_t>
void
tcache_t<prot_t>::from_file(FILE *file)
{
	// Input and check class name
    bool condor_checkpoint = mem_hier_t::ptr()->is_condor_checkpoint();
    if (!condor_checkpoint) {
		if (kernel_cache) return;
        mem_hier_t::ptr()->cleanup_previous_stats(file, typeid(this).name());
	}
	char classname[g_conf_max_classname_len];
	fscanf(file, "%s\n", classname);

    if (strcmp(classname, typeid(this).name()) != 0)
        DEBUG_OUT("Read %s   Require %s file_pos %d\n",classname, 
            typeid(this).name(), ftell(file));
	ASSERT(strcmp(classname, typeid(this).name()) == 0);

	// Read in lines	
	if (condor_checkpoint) {
		uint32 sets = numlines / assoc;
		for (uint32 idx = 0; idx < sets; idx++) {
			for (uint j = 0; j < assoc; j++) {
				lines[idx][j].from_file(file);
			}
		}
	} else {
		// Hack to make sure we dont warmup L1 and L2 -- ISCA 2006 hack
		// Assumes we use the same line size everywhere
		if (g_conf_memory_topology == "cmp_incl")
			cmp_incl_read_cache_lines(file);
		else if (g_conf_memory_topology == "cmp_excl_3l")
			cmp_3l_read_cache_lines(file, false);
        else if (g_conf_memory_topology == "cmp_incl_3l")
            cmp_3l_read_cache_lines(file, true);
		else 
			FAIL;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Common state machine transition functions

template <class prot_t>
void
tcache_t<prot_t>::
profile_transition(tfn_info_t *t, tfn_ret_t *ret)
{
    
	DEBUG_TR_FN("%s -> %s on %s trans addr %llx trans_id %u", t, ret, 
		prot_t::state_names[t->curstate][0].c_str(), 
		prot_t::state_names[ret->next_state][0].c_str(), 
		prot_t::action_names[t->action][0].c_str(), t->trans->phys_addr, 
        t->trans->unique_id);
		
	if (!ret->block_message && is_request_message(t->message)) {
        mai_t *mai_obj = t->trans->sequencer->get_mai_object(0);
        bool kernel_mode = false;
        if (mai_obj) kernel_mode = mai_obj->is_supervisor();
        else if (t->trans->dinst) 
            kernel_mode = t->trans->dinst->get_mai_instruction()->get_mai_object()->is_supervisor();
		if (prot_t::l1_stats) {
			if (mem_hier_t::is_thread_state(t->message->address)) {
				if (t->trans->occurred(MEM_EVENT_L1_HIT))
					STAT_INC(stat_num_vcpu_state_hit);
				else if ((t->trans->occurred(MEM_EVENT_L1_DEMAND_MISS) ||
					t->trans->occurred(MEM_EVENT_L1_COHER_MISS)))
				{
					STAT_INC(stat_num_vcpu_state_miss);
				} else
					STAT_INC(stat_num_vcpu_state_other);

				return;
			}
			
			if (t->trans->is_prefetch()) {
				if ((t->trans->occurred(MEM_EVENT_L1_DEMAND_MISS) ||
					t->trans->occurred(MEM_EVENT_L1_COHER_MISS)))
				{
					STAT_INC(stat_num_prefetch);
				}
			}
			else {
                if (t->trans->read && !t->trans->atomic) {
                    if (t->trans->ifetch) {
                        if (t->trans->occurred(MEM_EVENT_L1_HIT)) {
                            STAT_INC(stat_num_fetch_hit);
                            if (t->line->is_prefetched()) {
                                STAT_INC(stat_num_useful_pref);
                                t->line->mark_prefetched(false);
                            }
                        }
                        if (t->trans->occurred(MEM_EVENT_L1_DEMAND_MISS) ||
                            t->trans->occurred(MEM_EVENT_L1_COHER_MISS))
                        {
                            STAT_INC(stat_num_fetch_miss);
                            if (kernel_mode) STAT_INC(stat_num_fetch_miss_kernel);
                        }
                        if (t->trans->occurred(MEM_EVENT_L1_STALL))
                            STAT_INC(stat_num_fetch_stall);
                            if (t->trans->occurred(MEM_EVENT_L1_MSHR_PART_HIT)) {
                                STAT_INC(stat_num_fetch_part_hit_mshr);		
                                if (t->line->is_prefetched()) {
                                    STAT_INC(stat_num_partial_pref);
                                    t->line->mark_prefetched(false);
                                }
                            }
                    } else {
                        if (t->trans->occurred(MEM_EVENT_L1_HIT)) {
                            STAT_INC(stat_num_read_hit);
                            if (t->line->is_prefetched()) {
                                STAT_INC(stat_num_useful_pref);
                                t->line->mark_prefetched(false);
                            }
                        }
                        if (t->trans->occurred(MEM_EVENT_L1_DEMAND_MISS))
                        {
                            STAT_INC(stat_num_read_miss);
                            if (kernel_mode) STAT_INC(stat_num_read_miss_kernel);
                        } else if (t->trans->occurred(MEM_EVENT_L1_COHER_MISS)) {
                            STAT_INC(stat_num_read_coher_miss);
                            if (kernel_mode) STAT_INC(stat_num_read_coher_miss_kernel);
                        }
                        
                        if (t->trans->occurred(MEM_EVENT_L1_STALL))
                            STAT_INC(stat_num_read_stall);
                            if (t->trans->occurred(MEM_EVENT_L1_MSHR_PART_HIT)) {
                                STAT_INC(stat_num_read_part_hit_mshr);		
                                if (t->line->is_prefetched()) {
                                    STAT_INC(stat_num_partial_pref);
                                    t->line->mark_prefetched(false);
                                }
                            }
                    }
                } else {
                    if (t->trans->occurred(MEM_EVENT_L1_HIT)) {
                        STAT_INC(stat_num_write_hit);
                        if (t->line->is_prefetched()) {
                            STAT_INC(stat_num_useful_pref);
                            t->line->mark_prefetched(false);
                        }
                    }
                    if (t->trans->occurred(MEM_EVENT_L1_DEMAND_MISS))
                    {
                        STAT_INC(stat_num_write_miss);
                        if (kernel_mode) STAT_INC(stat_num_write_miss_kernel);
                    } else if (t->trans->occurred(MEM_EVENT_L1_COHER_MISS)) {
                        STAT_INC(stat_num_write_coher_miss);
                        if (kernel_mode) STAT_INC(stat_num_write_coher_miss_kernel);
                    }
                    
                    if (t->trans->occurred(MEM_EVENT_L1_STALL))
                        STAT_INC(stat_num_write_stall);
                        if (t->trans->occurred(MEM_EVENT_L1_MSHR_PART_HIT)) {
                            STAT_INC(stat_num_write_part_hit_mshr);
                            if (t->line->is_prefetched()) {
                                STAT_INC(stat_num_partial_pref);
                                t->line->mark_prefetched(false);
                            }
                        }
                }
            }
		}
		
		else if (prot_t::l2_stats) {
			if (mem_hier_t::is_thread_state(t->message->address)) {
				if (t->trans->occurred(MEM_EVENT_L2_HIT))
					STAT_INC(stat_num_vcpu_state_hit);
				else if ((t->trans->occurred(MEM_EVENT_L2_DEMAND_MISS) ||
					t->trans->occurred(MEM_EVENT_L2_COHER_MISS)))
				{
					STAT_INC(stat_num_vcpu_state_miss);
				} else
					STAT_INC(stat_num_vcpu_state_other);
				return;
			}
			
			if (t->trans->is_prefetch()) {
				if ((t->trans->occurred(MEM_EVENT_L2_DEMAND_MISS) ||
					t->trans->occurred(MEM_EVENT_L2_COHER_MISS)))
				{
					STAT_INC(stat_num_prefetch);
				}
			}
			else {
                if (t->trans->read && !t->trans->atomic) {
                    if (t->trans->ifetch) {
                        if (t->trans->occurred(MEM_EVENT_L2_HIT) ||
                            t->trans->occurred(MEM_EVENT_L2_REQ_FWD))
                        {
                            STAT_INC(stat_num_fetch_hit);
                        }
                        if (t->trans->occurred(MEM_EVENT_L2_DEMAND_MISS) ||
                            t->trans->occurred(MEM_EVENT_L2_COHER_MISS))
                        {
                            STAT_INC(stat_num_fetch_miss);
                            if (kernel_mode) STAT_INC(stat_num_fetch_miss_kernel);
                        }
                        if (t->trans->occurred(MEM_EVENT_L2_STALL))
                            STAT_INC(stat_num_fetch_stall);
                            if (t->trans->occurred(MEM_EVENT_L2_MSHR_PART_HIT)) {
                                STAT_INC(stat_num_fetch_part_hit_mshr);	
                                if (t->line->is_prefetched()) {
                                    STAT_INC(stat_num_partial_pref);
                                    t->line->mark_prefetched(false);
                                }
                            }
                    } else {
                        if (t->trans->occurred(MEM_EVENT_L2_HIT) ||
                            t->trans->occurred(MEM_EVENT_L2_REQ_FWD))
                        {
                            STAT_INC(stat_num_read_hit);
                        }
                        if (t->trans->occurred(MEM_EVENT_L2_DEMAND_MISS))
                        {
                            STAT_INC(stat_num_read_miss);
                            if (kernel_mode) STAT_INC(stat_num_read_miss_kernel);
                        } else if (t->trans->occurred(MEM_EVENT_L2_COHER_MISS)) {
                            STAT_INC(stat_num_read_coher_miss);
                            if (kernel_mode) STAT_INC(stat_num_read_coher_miss_kernel);
                        }
                        if (t->trans->occurred(MEM_EVENT_L2_STALL))
                            STAT_INC(stat_num_read_stall);
                            if (t->trans->occurred(MEM_EVENT_L2_MSHR_PART_HIT)) {
                                STAT_INC(stat_num_read_part_hit_mshr);	
                                if (t->line->is_prefetched()) {
                                    STAT_INC(stat_num_partial_pref);
                                    t->line->mark_prefetched(false);
                                }
                            }
                    }
                } else {
                    if (t->trans->occurred(MEM_EVENT_L2_HIT))
                    {
                        STAT_INC(stat_num_write_hit);
                    }
                    if (t->trans->occurred(MEM_EVENT_L2_DEMAND_MISS))
                    {
                        STAT_INC(stat_num_write_miss);
                        if (kernel_mode) STAT_INC(stat_num_write_miss_kernel);
                    } else if (t->trans->occurred(MEM_EVENT_L2_COHER_MISS) ||
                        t->trans->occurred(MEM_EVENT_L2_REQ_FWD)) {
                            STAT_INC(stat_num_write_coher_miss);
                            if (kernel_mode) STAT_INC(stat_num_write_coher_miss_kernel);
                        }
                        
                        if (t->trans->occurred(MEM_EVENT_L2_STALL))
                            STAT_INC(stat_num_write_stall);
                            if (t->trans->occurred(MEM_EVENT_L2_MSHR_PART_HIT)) {
                                STAT_INC(stat_num_write_part_hit_mshr);
                                if (t->line->is_prefetched()) {
                                    STAT_INC(stat_num_partial_pref);
                                    t->line->mark_prefetched(false);
                                }
                            }
                }
            }			
		}
				
		else if (prot_t::l3_stats) {
			if (mem_hier_t::is_thread_state(t->message->address)) {
				if (t->trans->occurred(MEM_EVENT_L3_HIT))
					STAT_INC(stat_num_vcpu_state_hit);
				else if ((t->trans->occurred(MEM_EVENT_L3_DEMAND_MISS) ||
					t->trans->occurred(MEM_EVENT_L3_COHER_MISS)))
				{
					STAT_INC(stat_num_vcpu_state_miss);
				} else
					STAT_INC(stat_num_vcpu_state_other);
				return;
			}
			
			if (t->trans->is_prefetch()) {
				if ((t->trans->occurred(MEM_EVENT_L3_DEMAND_MISS) ||
					t->trans->occurred(MEM_EVENT_L3_COHER_MISS)))
				{
					STAT_INC(stat_num_prefetch);
				}
			}
			else {
                if (t->trans->read && !t->trans->atomic) {
                    if (t->trans->ifetch) {
                        if (t->trans->occurred(MEM_EVENT_L3_HIT) ||
                            t->trans->occurred(MEM_EVENT_L3_REQ_FWD))
                        {
                            STAT_INC(stat_num_fetch_hit);
                            if (t->line->is_prefetched()) {
                                STAT_INC(stat_num_useful_pref);
                                t->line->mark_prefetched(false);
                            }
                        }
                        if (t->trans->occurred(MEM_EVENT_L3_DEMAND_MISS) ||
                            t->trans->occurred(MEM_EVENT_L3_COHER_MISS))
                        {
                            //ASSERT(!t->trans->occurred(MEM_EVENT_L3_HIT));
                            STAT_INC(stat_num_fetch_miss);
                            if (kernel_mode) STAT_INC(stat_num_fetch_miss_kernel);
                        }
                        if (t->trans->occurred(MEM_EVENT_L3_STALL))
                            STAT_INC(stat_num_fetch_stall);
                            if (t->trans->occurred(MEM_EVENT_L3_MSHR_PART_HIT)) {
                                STAT_INC(stat_num_fetch_part_hit_mshr);		
                                if (t->line->is_prefetched()) {
                                    STAT_INC(stat_num_partial_pref);
                                    t->line->mark_prefetched(false);
                                }
                            }
                    } else {
                        if (t->trans->occurred(MEM_EVENT_L3_HIT) ||
                            t->trans->occurred(MEM_EVENT_L3_REQ_FWD))
                        {
                            STAT_INC(stat_num_read_hit);
                            if (t->line->is_prefetched()) {
                                STAT_INC(stat_num_useful_pref);
                                t->line->mark_prefetched(false);
                            }
                        }
                        if (t->trans->occurred(MEM_EVENT_L3_DEMAND_MISS))
                        {
                            STAT_INC(stat_num_read_miss);
                            if (kernel_mode) STAT_INC(stat_num_read_miss_kernel);
                        } else if (t->trans->occurred(MEM_EVENT_L3_COHER_MISS)) {
                            STAT_INC(stat_num_read_coher_miss);
                            if (kernel_mode) STAT_INC(stat_num_read_coher_miss_kernel);
                        }
                        if (t->trans->occurred(MEM_EVENT_L3_STALL))
                            STAT_INC(stat_num_read_stall);
                            if (t->trans->occurred(MEM_EVENT_L3_MSHR_PART_HIT)) {
                                STAT_INC(stat_num_read_part_hit_mshr);	
                                if (t->line->is_prefetched()) {
                                    STAT_INC(stat_num_partial_pref);
                                    t->line->mark_prefetched(false);
                                }
                            }
                    }
                } else {
                    if (t->trans->occurred(MEM_EVENT_L3_HIT) || 
                        t->trans->occurred(MEM_EVENT_L3_REQ_FWD))
                    {
                        STAT_INC(stat_num_write_hit);
                        if (t->line->is_prefetched()) {
                            STAT_INC(stat_num_useful_pref);
                            t->line->mark_prefetched(false);
                        }
                    }
                    if (t->trans->occurred(MEM_EVENT_L3_DEMAND_MISS))
                    {
                        STAT_INC(stat_num_write_miss);
                        if (kernel_mode) STAT_INC(stat_num_write_miss_kernel);
                    } else if (t->trans->occurred(MEM_EVENT_L3_COHER_MISS)) {
                        STAT_INC(stat_num_write_coher_miss);
                        if (kernel_mode) STAT_INC(stat_num_write_coher_miss_kernel);
                    }
                    
                    if (t->trans->occurred(MEM_EVENT_L3_STALL))
                        STAT_INC(stat_num_write_stall);
                        if (t->trans->occurred(MEM_EVENT_L3_MSHR_PART_HIT)) {
                            STAT_INC(stat_num_write_part_hit_mshr);
                            if (t->line->is_prefetched()) {
                                STAT_INC(stat_num_partial_pref);
                                t->line->mark_prefetched(false);
                            }
                        }
                }
            }			
		}
	}		

    // Coherence_stat
    stat_coherence_transition[t->curstate]->inc_total(1, ret->next_state);
    stat_coherence_action[t->curstate]->inc_total(1, t->action);    
}


template <class prot_t>
stall_status_t
tcache_t<prot_t>::
can_insert_new_line(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("%s", t, ret, __func__);
	// Conservative check
	if (!can_allocate_writebackbuffer()) return StallPoll;
	
	addr_t index = get_index(t->address);
	prot_line_t *line;

    for (uint32 i = 0; i < assoc; i++) {
        line = &lines[index][i];
		ASSERT(line);
        if (line->can_replace()) return StallNone;
    }
    DEBUG_TR_FN("Returning StallSetEvent index %llu", t, ret, index);
    return StallSetEvent;
}
	
// Insert a line into the cache -- if not possible, return false
template <class prot_t>
stall_status_t
tcache_t<prot_t>::
insert_new_line(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("insert_new_line()", t, ret);

	prot_line_t *line = static_cast<prot_line_t *> 
	                    (find_free_line(t->address));
	tfn_ret_t ret2;
	tfn_info_t *t2;

	if (!line) {
		// no free line
		line = static_cast<prot_line_t *>
		       (choose_replacement(get_index(t->address)));
		if (!line) {
			DEBUG_TR_FN("insert_new_line(): none to replace", t, ret);
			FAIL;
			return StallPoll;  // Block message, none to replace
		}

		DEBUG_TR_FN("insert_new_line(): found replacement 0x%016llx", t, ret,
			line->get_address());


		if (line->is_prefetched())
			STAT_INC(stat_num_useless_pref);
			
		// Else Trigger a Replacement action
		t2 = new tfn_info_t(t->trans, line->get_state(),
			this->ActionReplace, line->get_address(), t->message, line);

		trigger(t2, &ret2);
		delete t2;
		
		if (ret2.block_message != StallNone) {
			DEBUG_TR_FN("%s: block for WB", t, ret, __func__);
			FAIL;
			return ret2.block_message;
		}
	}

	// Initialize our free line
	ASSERT(line->is_free());
	line->reinit(get_tag(t->address));
    line->mark_use();
	t->line = line;
	return StallNone;
}
	
// Block this message; an event from the same set will wake it up
template <class prot_t>
void
tcache_t<prot_t>::
block_message_set_event(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("block_message_set_event 0x%016llx", t, ret,
		t->message->address);
	ret->block_message = StallSetEvent;
}

// Block this message; an event (not necessarily from the same set) will wake
// it up
template <class prot_t>
void
tcache_t<prot_t>::
block_message_other_event(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("block_message_other_event 0x%016llx", t, ret,
		t->message->address);
	ret->block_message = StallOtherEvent;
}


// Block this message; no event will wake it up, so message must poll
template <class prot_t>
void
tcache_t<prot_t>::
block_message_poll(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("block_message_poll 0x%016llx", t, ret, t->message->address);
	ret->block_message = StallPoll;
}

// Block this message
template <class prot_t>
void
tcache_t<prot_t>::
block_message(tfn_info_t *t, tfn_ret_t *ret, stall_status_t type)
{
	DEBUG_TR_FN("block_message 0x%016llx stall type %u", t, ret, t->message->address, type);
	ret->block_message = type;
}

template <class prot_t>
void
tcache_t<prot_t>::
mark_completed(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("mark_completed", t, ret);
	
	ASSERT(t->trans->completed == false);
	t->trans->completed = true;
}

// Check if can allocate an MSHR for this address
template <class prot_t>
bool
tcache_t<prot_t>::
can_allocate_mshr(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("can_allocate_mshr size: %u", t, ret,
	            mshrs->get_num_allocated());

	return (mshrs->can_allocate_mshr());
}

// Allocate an MSHR for this address
template <class prot_t>
bool
tcache_t<prot_t>::
allocate_mshr(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("allocate_mshr prevsize: %u", t, ret,
	            mshrs->get_num_allocated());

	addr_t addr = t->line->get_address();
	ASSERT(addr == get_line_address(t->message->address));

	mshr_t<mshr_type_t> *mshr = mshrs->allocate_mshr(addr);

	return (mshr != NULL);
}

// Deallocate an MSHR for this address
template <class prot_t>
void
tcache_t<prot_t>::
deallocate_mshr(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("deallocate_mshr prevsize: %u", t, ret,
	            mshrs->get_num_allocated());
	
	// If full mshrs, requests (for any address) may have blocked
	if (!mshrs->can_allocate_mshr())
		wakeup_replay_q_all();

	addr_t addr = t->line->get_address();
	ASSERT(addr == get_line_address(t->message->address));

	mshrs->deallocate_mshr(addr);
}

// Check to see if can qnqueue read request to the MSHR for this address
template <class prot_t>
bool
tcache_t<prot_t>::
can_enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("can_enqueue_request_mshr", t, ret);

	//addr_t addr = t->line->get_address();
    addr_t addr = get_line_address(t->message->address);
	ASSERT(addr == get_line_address(t->message->address));

	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);

	// if no MSHR allocated, assume you *could* enqueue if you are able to
	// allocate an MSHR
	if (!mshr) return true;
	else return (mshr->can_enqueue_request());
}

// Set the valid bits in the MSHR for this store data
template <class prot_t>
void
tcache_t<prot_t>::
set_mshr_valid_bits(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("set_mshr_valid_bits", t, ret);

	addr_t addr = t->line->get_address();
	ASSERT(addr == get_line_address(t->message->address));

	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);

	// Set for address and size of incoming request (message)
	mshr->set_valid_bits(t->message->address, t->message->size);
}

// Check the valid bits in the MSHR to see if they can service this read
template <class prot_t>
bool
tcache_t<prot_t>::
check_mshr_valid_bits(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("check_mshr_valid_bits", t, ret);

	addr_t addr = t->line->get_address();
	ASSERT(addr == get_line_address(t->message->address));

	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);

	// Check for address and size of incoming request (message)
	return (mshr->check_valid_bits(t->message->address, t->message->size));
}

// Store the data from the incoming message into the cache
template <class prot_t>
void
tcache_t<prot_t>::
store_data_to_cache(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("store_data_to_cache", t, ret);
	if (!g_conf_cache_data) return;

	// Store data from message reguardless of MSHR valid bits
	
	// Use address and size of incoming request (message)
	ASSERT(t->message->data);
	addr_t offset = get_offset(t->address); 
	t->line->store_data(t->message->data, offset, t->message->size);

	VERBOSE_DATA(get_cname(), external::get_current_cycle(), 
		get_line_address(t->address), get_lsize(), t->line->get_data());
}

// Store the data from the incoming message into the cache but don't overwrite
// bytes with their valid bit set in the MSHR
template <class prot_t>
void
tcache_t<prot_t>::
store_data_to_cache_ifnot_valid(tfn_info_t *t, tfn_ret_t *ret)
{
	DEBUG_TR_FN("store_data_to_cache_ifnot_valid", t, ret);
	if (!g_conf_cache_data) return;

	// We should be storing the whole line
	addr_t addr = t->line->get_address();
	ASSERT(addr == t->message->address);

	VERBOSE_DATA(get_cname(), external::get_current_cycle(), 
				 addr, get_lsize(), t->message->data);

	// There should be an mshr, otherwise other function should have been called
	mshr_t<mshr_type_t> *mshr = mshrs->lookup(addr);
	ASSERT(mshr);

	// Don't store data for bytes with valid bits set
	ASSERT(t->message->data);
	for (addr_t offset = 0; offset < get_lsize(); offset++) {
		if (!mshr->check_valid_bits(t->message->address + offset, 1)) {
			t->line->store_data(&t->message->data[offset], offset, 1);
		}
	}
	
	VERBOSE_DATA(get_cname(), external::get_current_cycle(), 
				 addr, get_lsize(), t->line->get_data());
}

template <class prot_t>
void
tcache_t<prot_t>::
prefetch_helper(addr_t message_addr, mem_trans_t *trans, bool read)
{
	trans->pending_messages = 0;  // not sent
	// void default
}

// Decide whether to send prefetches
template <class prot_t>
void
tcache_t<prot_t>::
prefetch_heuristic(tfn_info_t *t, tfn_ret_t *ret)
{
	addr_t addr = get_line_address(t->address);
	ASSERT(addr == get_line_address(t->message->address));

	DEBUG_TR_FN("prefetch_heuristic", t, ret);

	if (g_conf_l1i_next_line_prefetch && t->trans->ifetch) {
		for (uint32 n = 0; n < g_conf_l1i_next_line_pref_num; n++) {

			addr += get_lsize();
			
			uint32 state;
			prot_line_t *line = static_cast<prot_line_t *> (lookup(addr));
			if (!line) state = this->StateNotPresent;
			else state = line->get_state();
			
			tfn_ret_t ret2;
			tfn_info_t *t2;
			t2 = new tfn_info_t(t->message->get_trans(), state, 
				this->ActionPrefetch, addr, t->message, line);
			
			trigger(t2, &ret2);
			delete t2;
		}
	}

	
}


// Wakeup bank replay queue
template <class prot_t>
void
tcache_t<prot_t>::
wakeup_replay_q_set(addr_t line_address)
{
	uint32 bank = get_bank(line_address);
    VERBOSE_OUT(verb_t::transfn,
				"%10s Bank %3u @ %12llu 0x%016llx: tcache::wakeup index %u \n",
				get_cname(), bank, external::get_current_cycle(),
				line_address, get_index(line_address));
	banks[bank]->wakeup_set(get_index(line_address));
}

// Wakeup every bank's replay queue
template <class prot_t>
void
tcache_t<prot_t>::
wakeup_replay_q_all()
{
	for (uint32 bank = 0; bank < num_banks; bank++)
		banks[bank]->wakeup_all();
}

template <class prot_t>
void
tcache_t<prot_t>::set_kernel_cache(bool val)
{
	kernel_cache = val;
}

template <class prot_t>
bool
tcache_t<prot_t>::is_tagarray()
{ 
    return (prot_t::l1_stats == false &&
            prot_t::l2_stats == false &&
            prot_t::l3_stats == false);
}


/*
////////////////////////////////
/// Statistics functions
template <class prot_t>
void
tcache_t<prot_t>::
stat_read_hit(tfn_info_t *t, tfn_ret_t *ret)
{
	STAT_INC(stat_num_read);
	STAT_INC(stat_num_read_hit);
}

template <class prot_t>
void
tcache_t<prot_t>::
stat_read_miss(tfn_info_t *t, tfn_ret_t *ret)
{
	STAT_INC(stat_num_read);
}
template <class prot_t>
void
tcache_t<prot_t>::
stat_read_miss_mshr_hit(tfn_info_t *t, tfn_ret_t *ret)
{
	STAT_INC(stat_num_read);
}
template <class prot_t>
void
tcache_t<prot_t>::
stat_read_hit_mshr_hit(tfn_info_t *t, tfn_ret_t *ret)
{
	STAT_INC(stat_num_read);
	STAT_INC(stat_num_read_hit);
}
template <class prot_t>
void
tcache_t<prot_t>::
stat_write_hit(tfn_info_t *t, tfn_ret_t *ret)
{
	STAT_INC(stat_num_write);
	STAT_INC(stat_num_write_hit);
}
template <class prot_t>
void
tcache_t<prot_t>::
stat_write_miss(tfn_info_t *t, tfn_ret_t *ret)
{
	STAT_INC(stat_num_write);
}
template <class prot_t>
void
tcache_t<prot_t>::
stat_write_miss_mshr_hit(tfn_info_t *t, tfn_ret_t *ret)
{
	STAT_INC(stat_num_write);
}

*/


