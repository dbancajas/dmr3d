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
// RCSID("$Id: gang_opt_alg.cc,v 1.1.2.1 2006/03/23 15:46:44 kchak Exp $");

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
#include "gang_opt_alg.h"

gang_opt_algorithm_t::gang_opt_algorithm_t(string name, hw_context_t **hwc,
   thread_scheduler_t *_t_sched, chip_t *_p, uint32 _num_thr, uint32 _num_ctxt) 
   : csp_algorithm_t(name, hwc, _t_sched, _p, _num_thr, _num_ctxt)
{
    disabled_core = new bool[num_ctxt];
    last_stolen_vcpu = new int32[num_ctxt];
    for (uint32 i = 0; i < num_ctxt; i++)
        last_stolen_vcpu[i] = -1;
    
    last_preempt = new tick_t[num_threads];
    initialize_vanilla_logical();
    
}

void gang_opt_algorithm_t::schedule(uint32 i)
{
    mai_t *mai = hw_context[i]->wait_list.front();
    hw_context[i]->wait_list.pop_front();
    p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, mai);
}


mai_t * gang_opt_algorithm_t::find_thread_for_ctxt(hw_context_t *ctxt, 
    bool desire_user_ctxt)
{
    
    if (ctxt->wait_list.size() == 0)
        return force_steal_thread(ctxt);
        
    mai_t *mai = ctxt->wait_list.front();
    mai_t *remote_mai = 0;
    ctxt->wait_list.pop_front();
        
    if (mai->is_spin_loop()) {
        // We reset the spin bit only when a thread
        // appears in the front of the queue
        mai_t *local_mai = useful_local_thread(ctxt);
        if (local_mai) {
            mai->set_spin_loop(false);
            ctxt->wait_list.push_back(mai);
            mai = local_mai;
        } else {             // All the threads local wait_list in spin loop
            if (!steal_previous_vcpu(ctxt, remote_mai)) {
                //hw_context_t * remote_ctxt = 0;
                //remote_mai = find_remote_thread(ctxt, remote_ctxt);
                remote_mai = force_steal_thread(ctxt);
            }
            if (remote_mai) {
                last_stolen_vcpu[ctxt->seq->get_id()] = remote_mai->get_id(); 
                //DEBUG_OUT("stealing vcpu%u into pcpu%u\n", remote_mai->get_id(),
                //    ctxt->seq->get_id());
                mai->set_spin_loop(false);    
                if (mai != remote_mai) ctxt->wait_list.push_back(mai);
                mai = remote_mai;
            }
        }
    }  
    
    ASSERT(mai);
    //if (mai->is_spin_loop()) DEBUG_OUT("spinning vcpu%u will run on pcpu%u\n", 
    //    mai->get_id(), ctxt->seq->get_id());
    return mai;
}
 

mai_t *gang_opt_algorithm_t::useful_local_thread(hw_context_t *ctxt)
{
    mai_t *ret = 0;
    list<mai_t *>::iterator it;
    for (it = ctxt->wait_list.begin(); it != ctxt->wait_list.end(); it++)
    {
        mai_t *t = *it;
        if (!t->is_spin_loop()) {
            ret = t;
            ctxt->wait_list.erase(it);
            break;
        }
    }
    return ret;
}

hw_context_t *gang_opt_algorithm_t::find_ctxt_for_thread(mai_t *thread, 
    bool desire_user_seq, uint32 syscall)
{
    uint32 id = thread->get_id();
    uint32 hwc_id = vcpu_2_pcpu[id];
    if (hw_context[hwc_id]->seq->get_mai_object(0))
    {
        hw_context[hwc_id]->wait_list.push_back(thread);
        last_preempt[id] = p->get_g_cycles();
        return 0;
    }
    return hw_context[hwc_id];
}
   
bool gang_opt_algorithm_t::thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
    ts_yield_reason_t why)
	
{
    if (why == YIELD_DISABLE_CORE) return true;
    //if (hw_context[seq->get_id()]->wait_list.size() == 0)
    //    return false;
    return (
		(why == YIELD_LONG_RUNNING && (!g_conf_no_os_pause || !mai->is_supervisor())) ||
        //why == YIELD_LONG_RUNNING ||
		why == YIELD_IDLE_ENTER ||
        why == YIELD_PAUSED_THREAD_INTERRUPT ||
		why == YIELD_MUTEX_LOCKED ||
        why == YIELD_PROC_MODE_CHANGE ||
        why == YIELD_EXTRA_LONG_RUNNING ||
        why == YIELD_DISABLE_CORE
		);
}
        
mai_t *gang_opt_algorithm_t::find_remote_thread(hw_context_t *ctxt, 
    hw_context_t *&remote_ctxt)
{
    mai_t *ret = NULL;
    uint32 l_machine = (ctxt->seq->get_id()) / (num_ctxt / g_conf_num_machines);
    for (uint32 i = 0; i < num_ctxt && ret == NULL; i++)
    {
        if (hw_context[i] != ctxt) {
            list<mai_t *>::iterator it = hw_context[i]->wait_list.begin();
            while (it != hw_context[i]->wait_list.end())
            {
                mai_t *candidate = *it;
                if (p->get_machine_id(candidate->get_id()) == l_machine &&
                    !candidate->is_spin_loop() && !candidate->is_idle_loop())
                {
                    ret = candidate;   
                    hw_context[i]->wait_list.erase(it);
                    remote_ctxt = hw_context[i];
                    break;
                }
                it++;
            }
        }
    }
    
    return ret;
}

    
       
void gang_opt_algorithm_t::read_checkpoint(FILE *file)
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
    }
    for (uint32 i = 0; i < num_threads; i++)
        fscanf(file, "%llu\n", &last_preempt[i]);
    checkpoint_util_t *checkp = new checkpoint_util_t();
    checkp->map_uint32_uint32_from_file(vcpu_2_pcpu, file);
}
    
       
void gang_opt_algorithm_t::write_checkpoint(FILE *file)
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
    }
    for (uint32 i = 0; i < num_threads; i++) 
        fprintf(file, "%llu\n", last_preempt[i]);
    checkpoint_util_t *checkp = new checkpoint_util_t();
    checkp->map_uint32_uint32_to_file(vcpu_2_pcpu, file);
}
    
    
void gang_opt_algorithm_t::silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai)
{

}    
    
sequencer_t *gang_opt_algorithm_t::handle_interrupt_thread(uint32 thread_id)
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

ts_yield_reason_t gang_opt_algorithm_t::prioritize_yield_reason(ts_yield_reason_t new_val,
	   	    ts_yield_reason_t old_val)
{
    if (new_val == YIELD_DISABLE_CORE || new_val == YIELD_EXTRA_LONG_RUNNING)
        return new_val;
    return old_val;
}
                
		
void gang_opt_algorithm_t::initialize_vanilla_logical()
{
    // No hopping, num_threads should be exactly divisible by num_ctxt
    ASSERT(num_threads % num_ctxt == 0);
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
    
}

void gang_opt_algorithm_t::initialize_vanilla_gang()
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

bool gang_opt_algorithm_t::steal_previous_vcpu(hw_context_t *ctxt, mai_t *& remote_mai)
{
    uint32 ctxt_id = ctxt->seq->get_id();
    if (last_stolen_vcpu[ctxt_id] == -1) return false;
    if (p->get_sequencer_from_thread(last_stolen_vcpu[ctxt_id]))
        return false;
    // OK waiting in the waitlist
    mai_t *thread = p->get_mai_object(last_stolen_vcpu[ctxt_id]);
    if (thread->is_spin_loop()) {
        thread->set_spin_loop(false);
        return false;
    }
    hw_context[vcpu_2_pcpu[last_stolen_vcpu[ctxt_id]]]->wait_list.remove(thread);
    remote_mai = thread;
    return true;
    
}

mai_t *gang_opt_algorithm_t::force_steal_thread(hw_context_t *ctxt)
{
    uint32 thread_id = num_threads;
    uint32 l_machine = (ctxt->seq->get_id()) / (num_ctxt / g_conf_num_machines);
    uint32 t_per_mach = num_threads/g_conf_num_machines;
    tick_t wait_time = 0;
    for (uint32 i = l_machine * t_per_mach; i < (l_machine + 1) *t_per_mach; i++)
    {
        if (p->get_sequencer_from_thread(i) == NULL)
        {
            tick_t curr_wait = p->get_g_cycles() - last_preempt[i];
            if (curr_wait > wait_time) {
                thread_id = i;
                wait_time = curr_wait;
            }
        }
    }
              
    ASSERT(thread_id != num_threads && !p->get_sequencer_from_thread(thread_id));
    hw_context[vcpu_2_pcpu[thread_id]]->wait_list.remove(p->get_mai_object(thread_id));
    //DEBUG_OUT("force steal vcpu%u\n", thread_id);
    return p->get_mai_object(thread_id);
}
