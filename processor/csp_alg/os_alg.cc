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
// RCSID("$Id: os_alg.cc,v 1.1.2.4 2006/07/27 16:15:04 kchak Exp $");

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
#include "os_alg.h"

os_algorithm_t::os_algorithm_t(string name, hw_context_t **hwc,
   thread_scheduler_t *_t_sched, chip_t *_p, uint32 _num_thr, uint32 _num_ctxt) 
   : csp_algorithm_t(name, hwc, _t_sched, _p, _num_thr, _num_ctxt)
{
    // No hopping, num_threads should be exactly divisible by num_ctxt
    ASSERT(num_threads % num_ctxt == 0);
    t_per_c = num_threads / num_ctxt;
    
    last_preempt = new tick_t[num_threads];
    early_interrupt_timer = new tick_t[num_ctxt];
    early = new bool[num_threads];
    memset(early, 1, sizeof(bool) * num_threads);
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        early_interrupt_timer[i] = 0;
        for (uint32 j = 0; j < t_per_c; j++) {
            uint32 thread_id = i * t_per_c + j;
            last_preempt[thread_id] = 0;
            hw_context[i]->wait_list.push_back(p->get_mai_object(thread_id));
            vcpu_2_pcpu[thread_id] = i;
            vcpu_host_pcpu[thread_id] = i;
        }
        schedule(i);
    }        
    global_interrupt_list.clear();
}

void os_algorithm_t::schedule(uint32 i)
{
    mai_t *mai = hw_context[i]->wait_list.front();
    hw_context[i]->wait_list.pop_front();
    early[mai->get_id()] = false;
    p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, mai);
}


mai_t * os_algorithm_t::find_thread_for_ctxt(hw_context_t *ctxt, 
    bool desire_user_ctxt)
{
    if (ctxt->wait_list.size() == 0 && global_interrupt_list.size() == 0)
        return force_steal_thread(ctxt);
    
    mai_t *mai = 0;
    if (global_interrupt_list.size())
    {
        mai = global_interrupt_list.front();
        global_interrupt_list.pop_front();
    } else {
        mai = ctxt->wait_list.front();
        ctxt->wait_list.pop_front();
    }
    
    early[mai->get_id()] = false;
    DEBUG_OUT("vcpu%u will run on pcpu%u @ %llu paused_int : %s\n", mai->get_id(), ctxt->seq->get_id(),
        p->get_g_cycles(), mai->is_pending_interrupt() ? "yes" : "no");
    return mai;
}
 
hw_context_t *os_algorithm_t::find_ctxt_for_thread(mai_t *thread, 
    bool desire_user_seq, uint32 syscall)
{
    uint32 id = thread->get_id();
    uint32 hwc_id = vcpu_2_pcpu[id];
    if (hw_context[hwc_id]->seq->get_mai_object(0))
    {
        ASSERT(find(hw_context[hwc_id]->wait_list.begin(),
            hw_context[hwc_id]->wait_list.end(), thread) == 
                hw_context[hwc_id]->wait_list.end());
        hw_context[hwc_id]->wait_list.push_back(thread);        
        if (thread->is_supervisor()) {
            DEBUG_OUT("vcpu%u is paused with kernel PC %llx\n", id,
                SIM_get_program_counter(thread->get_cpu_obj()));
        }  
        last_preempt[id] = p->get_g_cycles();
        early[id] = true;
        return 0;
    }
    return hw_context[hwc_id];
}
   
bool os_algorithm_t::thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
    ts_yield_reason_t why)
	
{
    //uint32 hwc_id = seq->get_id();
    //if (hw_context[hwc_id]->wait_list.size() == 0 && global_interrupt_list.size() == 0)
    //    return false;
    bool ret = false;
    switch (why) {
        case YIELD_EXTRA_LONG_RUNNING:
        case YIELD_PROC_MODE_CHANGE:
        case YIELD_IDLE_ENTER:
        case YIELD_MUTEX_LOCKED:
            ret = true;
            break;
        case YIELD_LONG_RUNNING:
            if (!mai->can_preempt())
                return false;
            ret = true;
            break;
        case YIELD_PAUSED_THREAD_INTERRUPT:
            if (!mai->can_preempt())
                return false;
            ret = true;
            break;
        case YIELD_EXCEPTION:
        case YIELD_DONE_RETRY:
            break;
        default:
            FAIL;
    }
    /*
    if (ret && global_interrupt_list.size())
    {
        mai_t *thread = global_interrupt_list.front();
        hw_context[hwc_id]->wait_list.push_front(thread);
        global_interrupt_list.pop_front();
    }
    */
    return ret;
}
        
       
void os_algorithm_t::read_checkpoint(FILE *file)
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
    for (uint32 i = 0; i < num_threads; i++) {
        uint32 int_early;
        fscanf(file, "%llu %u\n", &last_preempt[i], &int_early);
        early[i] = (int_early == 1);
    }
    
    uint32 count;
    fscanf(file, "%u\n", &count);
    global_interrupt_list.clear();
    for (uint32 i = 0; i < count; i++)
    {
        uint32 t_id;
        fscanf(file, "%u ", &t_id);
        global_interrupt_list.push_back(p->get_mai_object(t_id));
    }
    fscanf(file, "\n");
    checkpoint_util_t *checkp = new checkpoint_util_t();
    checkp->map_uint32_uint32_from_file(vcpu_2_pcpu, file);
}
    
       
void os_algorithm_t::write_checkpoint(FILE *file)
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
        fprintf(file, "%llu %u\n", last_preempt[i], early[i] ? 1: 0);
        
    fprintf(file, "%u\n", global_interrupt_list.size());
    while (global_interrupt_list.size())
    {
        mai_t *thread = global_interrupt_list.front();
        fprintf(file, "%u ", thread->get_id());
        global_interrupt_list.pop_front();
    }
    fprintf(file, "\n");
    checkpoint_util_t *checkp = new checkpoint_util_t();
    checkp->map_uint32_uint32_to_file(vcpu_2_pcpu, file);
}
    
    
void os_algorithm_t::silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai)
{

}    
    
sequencer_t *os_algorithm_t::handle_interrupt_thread(uint32 thread_id)
{
    ASSERT(early[thread_id]);
    mai_t *thread = p->get_mai_object(thread_id);
    uint32 host_pcpu = vcpu_2_pcpu[thread_id];
    sequencer_t *seq = hw_context[host_pcpu]->seq;
    mai_t *running_thread = seq->get_mai_object(0);
    
    hw_context[host_pcpu]->wait_list.remove(thread);
    
    /*
    // MUST HANDLE THE EARLY INTERRUPT TIMER
    if (running_thread && running_thread->can_preempt()) {
        hw_context[host_pcpu]->wait_list.push_front(thread);
        DEBUG_OUT("vcpu%u preempting seq%u\n", thread_id, seq->get_id());
        return seq;
    }
    
    */
    // Can't pause the currently running thread
    global_interrupt_list.push_back(thread);
    
    seq = 0;
    sequencer_t *alter_seq = 0;
    tick_t longest = 0;
    tick_t longest_alter = 0;
            
    // Try to preempt a thread that is running a 'safe' thread
    // Else, one without a pending interrupt
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        sequencer_t *t_seq = hw_context[i]->seq;
        running_thread = t_seq->get_mai_object(0);
        tick_t last_scheduled = p->get_g_cycles() - early_interrupt_timer[i];
        if (running_thread && running_thread->can_preempt()) {
            if (last_scheduled >= longest) {
                seq = t_seq;
                longest = last_scheduled;
            }
        }
        else if (running_thread && running_thread->is_pending_interrupt() == false
            && last_scheduled > longest_alter) {
            alter_seq = t_seq;
            longest_alter = last_scheduled;
        }
    }
    
    if (seq && longest > (tick_t)g_conf_interrupt_handle_cycle) {
        early_interrupt_timer[seq->get_id()] = p->get_g_cycles();
        DEBUG_OUT("interrupted vcpu%u evicting safe thread on seq%u\n",
            thread_id, seq->get_id());
        return seq;
    }
    
    if (alter_seq) {
        early_interrupt_timer[alter_seq->get_id()] = p->get_g_cycles();
        DEBUG_OUT("interrupted vcpu%u evicting UNSAFE thread on seq%u\n",
            thread_id, alter_seq->get_id());
        return alter_seq;
    }
 
    return 0;
}

ts_yield_reason_t os_algorithm_t::prioritize_yield_reason(ts_yield_reason_t new_val,
	   	    ts_yield_reason_t old_val)
{
    if (old_val == YIELD_EXTRA_LONG_RUNNING || new_val == YIELD_EXTRA_LONG_RUNNING)
        return YIELD_EXTRA_LONG_RUNNING;
    if (new_val == YIELD_PAUSED_THREAD_INTERRUPT || old_val == YIELD_PAUSED_THREAD_INTERRUPT)
        return YIELD_PAUSED_THREAD_INTERRUPT;
    
    return old_val;
}
                
		
tick_t os_algorithm_t::longest_wait(uint32 hwc_id)
{
    tick_t ret = p->get_g_cycles();
    list<mai_t *>::iterator it = hw_context[hwc_id]->wait_list.begin();
    while (it != hw_context[hwc_id]->wait_list.end())
    {
        mai_t *thread = *it;
        if (last_preempt[thread->get_id()] < ret)
            ret = last_preempt[thread->get_id()];
        it++;
    }
    return ret;
}

sequencer_t *os_algorithm_t::handle_synchronous_interrupt(uint32 src, uint32 target)
{
    ASSERT(early[target]);
    mai_t *thread = p->get_mai_object(target);
    uint32 target_host_pcpu = vcpu_2_pcpu[target];
    uint32 src_host_pcpu = vcpu_2_pcpu[src];
    sequencer_t *seq = hw_context[target_host_pcpu]->seq;
    mai_t *running_thread;
    
    hw_context[target_host_pcpu]->wait_list.remove(thread);
    
    // MUST CHANGE THE EARLY _TIMER
    /*
    if (target_host_pcpu != src_host_pcpu) {
        running_thread = seq->get_mai_object(0);
        if (running_thread && running_thread->can_preempt()) {
            DEBUG_OUT("vcpu%u local seq%u\n", target, seq->get_id());
            hw_context[target_host_pcpu]->wait_list.push_front(thread);
            return seq;
        }
    }
    */
    global_interrupt_list.push_back(thread);
    seq = 0;
    sequencer_t *alter_seq = 0;
    tick_t longest = 0;
    tick_t longest_alter = 0;
    
    // Try to preempt a thread that is running a 'safe' thread
    // Else, one without a pending interrupt
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        if (i == src_host_pcpu) continue;
        sequencer_t *t_seq = hw_context[i]->seq;
        running_thread = t_seq->get_mai_object(0);
        tick_t last_scheduled = p->get_g_cycles() - early_interrupt_timer[i];
        if (running_thread && running_thread->can_preempt()) {
            if (last_scheduled >= longest) {
                seq = t_seq;
                longest = last_scheduled;
            }
        }
        else if (running_thread && running_thread->is_pending_interrupt() == false
            && last_scheduled > longest_alter) {
            alter_seq = t_seq;
            longest_alter = last_scheduled;
        }
    }
    
    if (seq && longest > (tick_t)g_conf_interrupt_handle_cycle) {
        early_interrupt_timer[seq->get_id()] = p->get_g_cycles();
        DEBUG_OUT("target vcpu%u evicting safe thread on seq%u\n",
            target, seq->get_id());
        return seq;
    }
    
    if (alter_seq) {
        early_interrupt_timer[alter_seq->get_id()] = p->get_g_cycles();
        DEBUG_OUT("target vcpu%u evicting UNSAFE thread on seq%u\n",
            target, alter_seq->get_id());
        return alter_seq;
    }
    
    early_interrupt_timer[src_host_pcpu] = p->get_g_cycles();
    DEBUG_OUT("Yielding the TLB shootdown initiator --- expect problems soon!\n");
    return hw_context[src_host_pcpu]->seq;
    
}

uint32 os_algorithm_t::waiting_interrupt_threads()
{
    return global_interrupt_list.size();
}


mai_t *os_algorithm_t::force_steal_thread(hw_context_t *ctxt)
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
              
    hw_context[vcpu_2_pcpu[thread_id]]->wait_list.remove(p->get_mai_object(thread_id));
    return p->get_mai_object(thread_id);
}
