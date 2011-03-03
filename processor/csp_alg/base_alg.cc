/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id $
 *
 * description: Base Algorithm to work with num_threads > num_ctxt    
 * initial author: Koushik Chakraborty 
 *
*/

//  #include "simics/first.h"
// RCSID("$Id: base_alg.cc,v 1.1.2.15 2007/04/09 22:24:46 kchak Exp $");

#include "definitions.h"
#include "isa.h"
#include "proc_stats.h"
#include "chip.h"
#include "stats.h"
#include "counter.h"
#include "mai_instr.h"
#include "sequencer.h"
#include "verbose_level.h"
#include "config_extern.h"
#include "histogram.h"
#include "mai.h"
#include "sequencer.h"
#include "checkp_util.h"
#include "thread_scheduler.h"
#include "csp_alg.h"
#include "base_alg.h"

base_algorithm_t::base_algorithm_t(string name, hw_context_t **hwc,
   thread_scheduler_t *_t_sched, chip_t *_p, uint32 _num_thr, uint32 _num_ctxt) 
    : csp_algorithm_t(name, hwc, _t_sched, _p, _num_thr, _num_ctxt)
{
    disabled_core = new bool[num_ctxt];
    // Spinning VCPU evicting other
    spin_eviction_other = new tick_t[num_ctxt];
    for (uint32 i = 0; i < num_ctxt; i++)
        spin_eviction_other[i] = 0;
    
    last_preempt = new tick_t[num_threads];
	require_hop = false;
	num_disabled_cores = 0;
	last_mapping_switch = 0;
    switch (g_conf_scheduling_algorithm) {
        case BASE_SCHEDULING_POLICY:
        case HYBRID_SCHEDULING_POLICY:
            initialize_vanilla_logical();
            break;
        case VANILLA_GANG_SCHEDULING_POLICY:
            initialize_vanilla_gang();
            break;
        default:
            FAIL;
    }
    
}


void base_algorithm_t::schedule(uint32 i)
{
    mai_t *mai = hw_context[i]->wait_list.front();
    hw_context[i]->wait_list.pop_front();
    p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, mai);
}


mai_t * base_algorithm_t::find_thread_for_ctxt(hw_context_t *ctxt, 
    bool desire_user_ctxt)
{
    if (ctxt->wait_list.size() == 0)
        return 0;
    mai_t *mai = ctxt->wait_list.front();
    ctxt->wait_list.pop_front();
    ASSERT(mai);
    if (mai) DEBUG_OUT("tpu%u will run on cpu%u\n", mai->get_id(), ctxt->seq->get_id());
    return mai;
}

hw_context_t *base_algorithm_t::find_ctxt_for_thread(mai_t *thread, 
    bool desire_user_seq, uint32 syscall)
{
    uint32 id = thread->get_id();
    uint32 hwc_id = vcpu_2_pcpu[id];
	
	if (require_hop && next_thread_switch.front() == id &&
		last_mapping_switch + g_conf_thread_hop_interval < p->get_g_cycles())
	{
		next_thread_switch.pop_front();
		
		uint32 hop_core;
		while (hwc_id == (hop_core = get_proxy_core()));
		
		// Assign currently running thread on that core to next_hop list
		// I.e. assume this randomly picks one of the threads currently on that 
		// core
		sequencer_t *seq = hw_context[hop_core]->seq;
		mai_t *next_hop_mai = seq->get_mai_object(hw_context[hop_core]->ctxt);
		ASSERT(next_hop_mai);
		next_thread_switch.push_back(next_hop_mai->get_id());
		
		// There must still be something on prev context as well...
		ASSERT(!hw_context[hwc_id]->wait_list.empty());

		// Put this thread on new core (there must be something already there)
		hw_context[hop_core]->wait_list.push_back(thread);
		vcpu_2_pcpu[id] = hop_core;
		last_mapping_switch = p->get_g_cycles();
        last_preempt[id] = p->get_g_cycles();
		DEBUG_OUT("Moving vcpu%u to pcpu%u, next is vcpu%u\n", 
			id, hop_core, next_hop_mai->get_id());
		
        return 0;
	}
	
	DEBUG_OUT("preempt vcpu%d; last_mapping_switch: %llu @ %llu\n", id,
		last_mapping_switch, p->get_g_cycles());
		
	
    if (hw_context[hwc_id]->seq->get_mai_object(0))
    {
        hw_context[hwc_id]->wait_list.push_back(thread);
        last_preempt[id] = p->get_g_cycles();
        // if (thread->is_supervisor()) 
        //     DEBUG_OUT("vcpu%u is paused with kernel PC %llx\n", thread->get_id(),
        //         SIM_get_program_counter(thread->get_cpu_obj()));
        return 0;
    }
    return hw_context[hwc_id];
}
   
bool base_algorithm_t::thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
    ts_yield_reason_t why)
	
{
    if (why == YIELD_DISABLE_CORE) return true;
    if (why == YIELD_NARROW_CORE) return true;
    if (hw_context[seq->get_id()]->wait_list.size() == 0) {
        if (g_conf_optimize_faulty_core && why == YIELD_MUTEX_LOCKED) {
            // Assumptions: (1) only one fault core at a time and
            // (2) non-SMT machine
            // Look for the a context with waiting
            // KC
            tick_t current_cycle = p->get_g_cycles();
            for (uint32 i = 0; i < num_ctxt; i++)
            {
                if (hw_context[i]->wait_list.size() != 0 &&
                    current_cycle - spin_eviction_other[i] > g_conf_minm_successive_preempt) {
                    hw_context[i]->seq->potential_thread_switch(0, YIELD_MUTEX_LOCKED);
                    DEBUG_OUT("vcpu %u spinning and pre-empting pcpu %u\n", mai->get_id(), i);
                    spin_eviction_other[i] = current_cycle;
                    break;
                }
            }
        }
        return false;
    }
    
    return (
		(why == YIELD_LONG_RUNNING && (!g_conf_no_os_pause || mai->can_preempt())) ||
        //why == YIELD_LONG_RUNNING ||
		why == YIELD_IDLE_ENTER ||
        why == YIELD_PAUSED_THREAD_INTERRUPT ||
		why == YIELD_MUTEX_LOCKED ||
        why == YIELD_PROC_MODE_CHANGE ||
        why == YIELD_EXTRA_LONG_RUNNING ||
        why == YIELD_DISABLE_CORE
		);
}
    
       
void base_algorithm_t::read_checkpoint(FILE *file)
{
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        uint32 count;
        fscanf(file, "%u\n", &count);
        hw_context[i]->wait_list.clear();
        for (uint32 j = 0; j < count; j++)
        {
            uint32 id;
            fscanf(file, "%u\n", &id);
            hw_context[i]->wait_list.push_back(p->get_mai_object(id));
        }
        fscanf(file, "\n");
		
		int temp_disabled;
		fscanf(file, "%d\n", &temp_disabled);
		disabled_core[i] = (temp_disabled > 0);
    }
    for (uint32 i = 0; i < num_threads; i++)
        fscanf(file, "%llu\n", &last_preempt[i]);
    checkpoint_util_t *checkp = new checkpoint_util_t();
    checkp->map_uint32_uint32_from_file(vcpu_2_pcpu, file);
	
	
	fscanf(file, "%u\n", &num_disabled_cores);
	require_hop = (num_threads % (num_ctxt - num_disabled_cores) != 0);
	fscanf(file, "%llu\n", &last_mapping_switch);
    checkp->list_uint32_from_file(next_thread_switch, file);
}
    
       
void base_algorithm_t::write_checkpoint(FILE *file)
{
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        list<mai_t *>::iterator it = hw_context[i]->wait_list.begin();
        fprintf(file, "%u\n", hw_context[i]->wait_list.size());
        while (it != hw_context[i]->wait_list.end())
        {
            mai_t *candidate = *it;
            fprintf(file, "%u ", candidate->get_id());
            it++;
        }
        fprintf(file, "\n");
		fprintf(file, "%d\n", disabled_core[i] ? 1 : 0);
    }
    for (uint32 i = 0; i < num_threads; i++) 
        fprintf(file, "%llu\n", last_preempt[i]);
    checkpoint_util_t *checkp = new checkpoint_util_t();
    checkp->map_uint32_uint32_to_file(vcpu_2_pcpu, file);

	fprintf(file, "%u\n", num_disabled_cores);
	fprintf(file, "%llu\n", last_mapping_switch);
    checkp->list_uint32_to_file(next_thread_switch, file);
}
    
    
void base_algorithm_t::silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai)
{

}    
    
sequencer_t *base_algorithm_t::handle_interrupt_thread(uint32 thread_id)
{
    uint32 host_pcpu = vcpu_2_pcpu[thread_id];
    mai_t *thread = p->get_mai_object(thread_id);
    sequencer_t *seq = hw_context[host_pcpu]->seq;
    if (hw_context[host_pcpu]->wait_list.size() == 1) {
        ASSERT(hw_context[host_pcpu]->wait_list.front() == thread);
    } else {
        hw_context[host_pcpu]->wait_list.remove(thread);
        hw_context[host_pcpu]->wait_list.push_front(thread);
    }
    return seq;
}

sequencer_t *base_algorithm_t::handle_synchronous_interrupt(uint32 src, uint32 tgt)
{
    return handle_interrupt_thread(tgt);
}

uint32 base_algorithm_t::get_proxy_core()
{
        
    uint32 minm_waitlist = num_threads;
    vector<uint32> candidates;
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        if (!disabled_core[i])
        {
            uint32 wlsize = hw_context[i]->wait_list.size();
            if (wlsize < minm_waitlist) {
                candidates.clear();
                minm_waitlist = wlsize;
                candidates.push_back(i);
            } else if (wlsize == minm_waitlist) {
                candidates.push_back(i);
            }
        }
    }
    ASSERT(candidates.size());
    uint32 ret = random() % candidates.size();
    return candidates[ret];
    
}

void base_algorithm_t::disable_core(sequencer_t *seq)
{
    // search for the hw_context .. non-SMT system it is the id
    uint32 core_id = seq->get_id();
    disabled_core[core_id] = true;
	num_disabled_cores++;

	// Only do thread hopping if we'll be disabled for a long time
	if ((num_threads % (num_ctxt - num_disabled_cores) != 0) && 
		g_conf_disable_period > 2*g_conf_thread_hop_interval)
	{
		require_hop = true;
		last_mapping_switch = p->get_g_cycles();
		DEBUG_OUT("last_mapping_switch: %llu\n", last_mapping_switch);
	} else {
		require_hop = false;
		next_thread_switch.clear();
	}
	
	
    for (uint32 i = 0; i < num_threads; i++)
    {
        if (vcpu_2_pcpu[i] == core_id) {
            mai_t *thread = p->get_mai_object(i);
            uint32 new_core = get_proxy_core();
            vcpu_2_pcpu[i] = new_core;
            DEBUG_OUT("%llu cpu%u disabled : tpu%u pushed intp cpu%u ",
                    p->get_g_cycles(), core_id, i, new_core);
            if (find(hw_context[core_id]->wait_list.begin(),
             hw_context[core_id]->wait_list.end(), thread) !=
             hw_context[core_id]->wait_list.end()) {
                hw_context[new_core]->wait_list.push_back(thread);
                DEBUG_OUT("--- after waitlist manipulation\n");
            }
            DEBUG_OUT("\n");

			if (require_hop) {
				next_thread_switch.remove(thread->get_id());  // In case we double-added
				next_thread_switch.push_front(thread->get_id());
				DEBUG_OUT("Hopping vcpu%u\n", thread->get_id());
			}
        }
    }
    hw_context[core_id]->wait_list.clear();

	ASSERT(next_thread_switch.size() == 
		num_threads % (num_ctxt - num_disabled_cores) ||
		!require_hop);

    
}

void base_algorithm_t::enable_core(sequencer_t *seq)
{
    uint32 core_id = seq->get_id();
    disabled_core[core_id] = false;
	num_disabled_cores--;
	if (num_threads % (num_ctxt - num_disabled_cores) == 0) {
		require_hop = false;
		next_thread_switch.clear();
	}
	// else case handled below
		
	
    ASSERT(hw_context[core_id]->wait_list.size() == 0);
    ASSERT(seq->get_mai_object(0) == NULL);
    for (uint32 i = 0; i < num_threads; i++)
    {
        if (vcpu_host_pcpu[i] == core_id)
        {
            DEBUG_OUT("cpu%u enabled!!\n", core_id);
            // move the thread back into here
            mai_t *thread = p->get_mai_object(i);
            uint32 curr_host = vcpu_2_pcpu[i];
            ASSERT(curr_host != core_id);
            if (find(hw_context[curr_host]->wait_list.begin(),
                hw_context[curr_host]->wait_list.end(), thread) !=
                hw_context[curr_host]->wait_list.end())
            {
                hw_context[curr_host]->wait_list.remove(thread);
                p->switch_to_thread(seq, 0, thread);
                DEBUG_OUT("tpu%u immediataly starts running!\n", i);
            }
            vcpu_2_pcpu[i] = i;
			
			// Can do this whether or not currently hopping
			next_thread_switch.remove(thread->get_id());
        }
    }
}

ts_yield_reason_t base_algorithm_t::prioritize_yield_reason(ts_yield_reason_t new_val,
	   	    ts_yield_reason_t old_val)
{
    if (new_val == YIELD_DISABLE_CORE || new_val == YIELD_EXTRA_LONG_RUNNING)
        return new_val;
    return old_val;
}
                
		
void base_algorithm_t::initialize_vanilla_logical()
{
    // No hopping, num_threads should be exactly divisible by num_ctxt
    ASSERT(num_threads % num_ctxt == 0 || num_threads < num_ctxt);
    if (num_threads >= num_ctxt) {
        
        t_per_c = num_threads / num_ctxt;    
        for (uint32 i = 0; i < num_ctxt; i++)
        {
            for (uint32 j = 0; j < t_per_c; j++) {
                uint32 thread_id = i * t_per_c + j;
                last_preempt[thread_id] = 0;
                hw_context[i]->wait_list.push_back(p->get_mai_object(thread_id));
                vcpu_2_pcpu[thread_id] = i;
                vcpu_host_pcpu[thread_id] = i;
            }
            schedule(i);
            disabled_core[i] = false;
        } 
    } else {
        for (uint32 i = 0; i < num_ctxt; i++)
        {
            if (i < num_threads) {
                last_preempt[i] = 0;
                hw_context[i]->wait_list.push_back(p->get_mai_object(i));
                vcpu_2_pcpu[i] = i;
                vcpu_host_pcpu[i] = i;
                schedule(i);
            } else {
                p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, 
                    0);
                hw_context[i]->busy = false;
            }
            
            disabled_core[i] = false;
            
        }
        
    }
    
}

void base_algorithm_t::initialize_vanilla_gang()
{
    ASSERT(num_threads % num_ctxt == 0);
    ASSERT(num_ctxt == (num_threads / g_conf_num_machines));
    
    uint32 host;
    for (uint32 i = 0; i < num_threads; i++)
    {
        last_preempt[i] = 0;
        host = i % num_ctxt;
        vcpu_2_pcpu[i] = host;
        vcpu_host_pcpu[i] = host;
        hw_context[host]->wait_list.push_back(p->get_mai_object(i));
    }
    
      
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        schedule(i);
    }
    
}

