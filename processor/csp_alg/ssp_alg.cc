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
// RCSID("$Id: ssp_alg.cc,v 1.1.2.7 2006/08/18 14:30:25 kchak Exp $");

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

#include "thread_scheduler.h"
#include "csp_alg.h"
#include "tsp_alg.h"
#include "ssp_alg.h"

ssp_algorithm_t::ssp_algorithm_t(string name, hw_context_t **hwc,
   thread_scheduler_t *_t_sched, chip_t *_p, uint32 _num_thr, uint32 _num_ctxt) :
    tsp_algorithm_t(name, hwc, _t_sched, _p, _num_thr, _num_ctxt)
{
    provision_syscall();
    next_hw_context_assignment = g_conf_num_user_seq;
    stat_successive_syscall = stats->COUNTER_GROUP ("stat_successive_syscall",
        " Successive Syscall ", 4);
    DEBUG_OUT("Size of idle os ctxt = %u\n", idle_os_ctxt.size());
}


void
ssp_algorithm_t::provision_syscall()
{
    ASSERT(g_conf_num_user_seq);
    uint32 count = g_conf_syscall_provision[0];
    uint32 syscall, context;
    for (uint32 i = 0; i < count; i++)
    {
        syscall = g_conf_syscall_provision[ i * 2 + 1];
        context = g_conf_syscall_provision [ i * 2 + 2];
        syscall_context[syscall].insert(hw_context[context]);
    }
    
    if (g_conf_cache_only_provision) {
        dynamic_provision_offset = next_hw_context_assignment;
    }
    
}

mai_t * ssp_algorithm_t::find_thread_for_ctxt(hw_context_t *ctxt, 
    bool user_ctxt)
{
    mai_t *ret = 0;
    if (user_ctxt && !wait_for_user.empty())
    {
        list<mai_t *>::iterator it = wait_for_user.begin();
        while (it != wait_for_user.end())
        {
            mai_t *candidate = *it;
            if (ctxt == hw_context [preferred_user_ctxt [candidate->get_id()] ])
            {
                ret = candidate;
                wait_for_user.erase(it);
                break;
            }
            it++;
        }
    } else if (!user_ctxt && !wait_for_os.empty()) {
        ret = get_curr_syscall_thread(ctxt);
        if (!ret) ret = matching_syscall_thread(ctxt);
        
    }
    
    if (!ret) {
        if (g_conf_avoid_idle_context) {
            if (wait_for_user.size() && 
                ((user_ctxt && wait_for_user.size()) ||
                (!user_ctxt && wait_for_os.empty()))) {
                    ret = wait_for_user.front();
                    wait_for_user.pop_front();
            } else if (wait_for_os.size()) {
                ret = wait_for_os.front();
                wait_for_os.pop_front();
            }
            ASSERT(ret || (wait_for_user.empty() && wait_for_os.empty()));
            
        } else {
            ctxt->busy = false;
            if (user_ctxt) idle_user_ctxt.push_back(ctxt);
            else {
                ASSERT(find(idle_os_ctxt.begin(), idle_os_ctxt.end(), ctxt) ==
                    idle_os_ctxt.end());
                idle_os_ctxt.push_back(ctxt);
            }
        }
    } 
     
    if (ret && !user_ctxt) {
        uint32 syscall = ret->get_syscall_num();
        uint32 bucket = (ctxt->curr_sys_call == syscall) ?
            (2 + (uint32) (ctxt->user_PC == ret->get_last_user_pc())) : 0;
            
        bucket = (bucket == 0) ? (syscall_coupling_exists(ctxt->curr_sys_call, syscall)) :
                bucket;
        stat_successive_syscall->inc_total(1, bucket);
        ctxt->user_PC = ret->get_last_user_pc();
        ctxt->curr_sys_call = syscall;
    }
    
    return ret;
}
 
hw_context_t *ssp_algorithm_t::find_ctxt_for_thread(mai_t *thread, 
    bool desire_user_ctxt, uint32 syscall)
{
    hw_context_t *ret = 0;
    if (!desire_user_ctxt)
    {
        ret = get_syscall_context(syscall, thread->get_id(), thread->get_last_user_pc());   
    } else {
        hw_context_t *preferred = get_preferred_user_context(thread->get_id());
        if (!preferred->busy) 
        {
            ASSERT(find(idle_user_ctxt.begin(), idle_user_ctxt.end(), preferred)
                    != idle_user_ctxt.end());
            ret = preferred;
            idle_user_ctxt.remove(preferred);
        }
    }
    
    if (!ret) {
        if (desire_user_ctxt) {
            if ((uint32) thread->get_id() >= num_ctxt || thread->is_spin_loop())
                wait_for_user.push_back(thread);
            else
                wait_for_user.push_front(thread);
        }
        else {
            if ((uint32) thread->get_id() >= num_ctxt || thread->is_spin_loop())
                wait_for_os.push_back(thread);
            else
                wait_for_os.push_front(thread);
        }
    } else {
        ret->busy = true;
        if (!desire_user_ctxt) {
            uint32 bucket = (ret->curr_sys_call == syscall) ?
            (2 + (uint32) (ret->user_PC == thread->get_last_user_pc())) : 0;
            
            bucket = (bucket == 0) ? (syscall_coupling_exists(ret->curr_sys_call, syscall)) :
                bucket;
            stat_successive_syscall->inc_total(1, bucket);
            ret->user_PC = thread->get_last_user_pc();
            ret->curr_sys_call = syscall;
        }     
    }
    
    return ret;
}
   
void ssp_algorithm_t::read_checkpoint(FILE *file)
{
    tsp_algorithm_t::read_checkpoint(file);
    syscall_context.clear();
    uint32 count;
    fscanf(file, "%u\n", &count);
    for (uint32 i = 0; i < count; i++)
    {
        uint32 syscall, size, ctxt;
        fscanf(file, "%u %u ", &syscall, &size);
        for (uint32 j = 0; j < size; j++)
        {
            fscanf(file, "%u ", &ctxt);
            syscall_context[syscall].insert(hw_context[ctxt]);
        }
        fscanf(file ,"\n");
    }
    
    
}
    
       
void ssp_algorithm_t::write_checkpoint(FILE *file)
{
    tsp_algorithm_t::write_checkpoint(file);
    // Now syscall provisions
    fprintf(file, "%u\n", syscall_context.size());
    
    map<uint32, set<hw_context_t *> > ::iterator sit;
    for (sit = syscall_context.begin(); sit != syscall_context.end(); sit++)
    {
        uint32 syscall = sit->first;
        set<hw_context_t *> hwc_set = sit->second;
        fprintf(file, "%u %u ", syscall, hwc_set.size());
        set<hw_context_t *>::iterator i;
        for (i = hwc_set.begin(); i != hwc_set.end(); i++)
        {
            fprintf(file, "%u ", t_sched->get_context_id(*i));
        }
        fprintf(file , "\n");
    }
    
    
}
    
void ssp_algorithm_t::silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai)
{
	bool desire_user_seq = !mai->is_supervisor();  // may == user_seq if preemted

	FE_EXCEPTION_CHECK;
	// find idle seq, or put thread on idle list
    uint64 syscall = 0;
    hw_context_t *preferred_ctxt;
    if (!desire_user_seq) syscall = mai->get_syscall_num();
    
    if (desire_user_seq)
    {
        preferred_ctxt = get_preferred_user_context(mai->get_id());
        
    } else {
        
        map<uint32, set<hw_context_t *> >::iterator it = syscall_context.find(syscall);
        if (it != syscall_context.end())
        {
            uint32 way = mai->get_id() % it->second.size();
            set<hw_context_t *>::iterator c_it = it->second.begin();
            for (; way > 0; way--,c_it++) {}
            preferred_ctxt = *c_it;
            
        } else {
            preferred_ctxt = hw_context[next_hw_context_assignment++];
            // Catch All - code offset to the as much as provisioned
            //if (next_hw_context_assignment == num_ctxt)
            //    next_hw_context_assignment = dynamic_provision_offset;
            if (next_hw_context_assignment == num_ctxt)
                next_hw_context_assignment = g_conf_num_user_seq;
            syscall_context[syscall].insert(preferred_ctxt);
        }
        
    }
    
    // check for number of concurrent threads on it
    silent_switch_finishup(seq, preferred_ctxt, ctxt);
    
}
   

hw_context_t *ssp_algorithm_t::get_syscall_context(uint32 syscall, uint32 t_id, 
    addr_t last_user_pc)
{
    hw_context_t *ret = 0;
    hw_context_t *secondary_choice = 0;
    list<hw_context_t *>::iterator hwcit = idle_os_ctxt.begin();
    for (; hwcit != idle_os_ctxt.end(); hwcit++)
    {
        hw_context_t *candidate = *hwcit;
        if (candidate->curr_sys_call == syscall) {
            if (last_user_pc == candidate->user_PC) {
                ret = candidate;
                idle_os_ctxt.erase(hwcit);
                break;
            } else if (!ret) {
                ret = candidate;
            }
        } else if (!secondary_choice) {
            if (syscall_coupling_exists(candidate->curr_sys_call, syscall))
                secondary_choice = candidate;
        }
    }
    
    if (!ret && secondary_choice) ret = secondary_choice;
    
    if (ret) {
        idle_os_ctxt.remove(ret);
        return ret;
    }
    // Next section eagerly matches a context if there is an available OS context
    // but this is necessary to make sure that system calls that are not mapped to
    // a particular can also execute!
    
    else if (g_conf_avoid_idle_os && idle_os_ctxt.size()) { 
        
        ret = idle_os_ctxt.front();
        idle_os_ctxt.pop_front();
        return ret;
    }
    
    //ASSERT(!ret || ret->busy == false);
    ret = get_assigned_syscall_context(syscall);
    if (!ret) {
        uint32 c_syscall = get_coupled_syscall(syscall);
        if (c_syscall) ret = get_assigned_syscall_context(c_syscall);
    
    
    }
    
    return ret;
    
}

uint32 ssp_algorithm_t::get_coupled_syscall(uint32 syscall)
{
    
    uint32 pairs = g_conf_syscall_coupling[0];
    for (uint32 i = 0; i < pairs; i++)
    {
        if (syscall == (uint32) g_conf_syscall_coupling[1 + 2 * i])
            return (uint32) g_conf_syscall_coupling[1 + 2 * i + 1];
        else if (syscall == (uint32) g_conf_syscall_coupling[1 + 2 * i + 1])
            return (uint32) g_conf_syscall_coupling[1 + 2 * i];
    }
            
    return 0;
}
    

hw_context_t * ssp_algorithm_t::get_assigned_syscall_context(uint32 syscall)
{
    hw_context_t *ret = 0;
    hw_context_t *desired;
    map<uint32, set<hw_context_t *> >::iterator it = syscall_context.find(syscall);
    if (it != syscall_context.end())
    {
        
        set<hw_context_t *>::iterator c_it = it->second.begin();
        for (uint32 i = 0; i < it->second.size(); i++,c_it++) 
        {
            
            desired = *c_it;
            if (!desired->busy) break;
            else desired = 0;
        }
        
        if (desired && !desired->busy)
        {
            ASSERT(find(idle_os_ctxt.begin(), idle_os_ctxt.end(), desired)
                != idle_os_ctxt.end());
            ret = desired;
            idle_os_ctxt.remove(desired);
        }
        ASSERT(idle_os_ctxt.size() <= (num_ctxt - g_conf_num_user_seq)); 
        
    } else {
        // First time occurence map to the most easily available
        if (!idle_os_ctxt.empty()) {
            ret = idle_os_ctxt.front();
            syscall_context[syscall].insert(ret);
            idle_os_ctxt.pop_front();
        } else {
            if (next_hw_context_assignment == num_ctxt)
                next_hw_context_assignment = g_conf_num_user_seq;
            syscall_context[syscall].insert(hw_context[next_hw_context_assignment++]);
        }
    }
    
    return ret;
    
}

void ssp_algorithm_t::debug_os_waiter()
{
    list<mai_t *>::iterator i;
    for (i = wait_for_os.begin(); i != wait_for_os.end(); i++)
    {
        mai_t *thread = *i;
        DEBUG_OUT("vcpu%u -- waiting w/ syscall %llu last_pc %llx\n", thread->get_id(), 
            thread->get_syscall_num(), thread->get_last_user_pc());
    }
}
    
void ssp_algorithm_t::debug_user_waiter()
{
    list<mai_t *>::iterator i;
    for (i = wait_for_user.begin(); i != wait_for_user.end(); i++)
    {
        mai_t *thread = *i;
        DEBUG_OUT("vcpu%u -- waiting for preferred user ctxt\n", thread->get_id()); 
    }
}

mai_t *ssp_algorithm_t::matching_syscall_thread(hw_context_t *ctxt)
{
    
    mai_t *ret = 0;
    if (wait_for_os.size()) {
        ret = wait_for_os.front();
        wait_for_os.pop_front();
        return ret;
    }
    
    list<mai_t *>::iterator it = wait_for_os.begin();
    while (it != wait_for_os.end())
    {
        mai_t *candidate = *it;
        uint32 syscall = candidate->get_syscall_num();
        if (syscall == 0) {
            if (!candidate->is_spin_loop()) {
                // boot strap
                ret = candidate;
                wait_for_os.erase(it);
                break;
            } else {
                candidate->set_spin_loop(false);
            }
        }
        map<uint32, set<hw_context_t *> >::iterator syscall_it = syscall_context.find(syscall);
        ASSERT(syscall_it != syscall_context.end());
        if (syscall_it->second.find(ctxt) != syscall_it->second.end())
        {
            ret = candidate;
            wait_for_os.erase(it);
            break;
        }
        it++;
    }   
    
    return ret;
}

mai_t *ssp_algorithm_t::get_curr_syscall_thread(hw_context_t *ctxt)
{
    mai_t *ret = 0;
    list<mai_t *>::iterator it = wait_for_os.begin();
    while (it != wait_for_os.end())
    {
        mai_t *candidate = *it;
        uint32 syscall = candidate->get_syscall_num();
        if (syscall == ctxt->curr_sys_call) {
            if (candidate->is_spin_loop())
                candidate->set_spin_loop(false);
            else {
                ret = candidate;
                wait_for_os.erase(it);
                break;
            }
        }
        it++;
    }
    return ret;
}
    

bool ssp_algorithm_t::syscall_coupling_exists(uint64 syscall_a, uint64 syscall_b)
{
    uint32 pairs = g_conf_syscall_coupling[0];
    for (uint32 i = 0; i < pairs; i++)
    {
        if ((syscall_a == (uint64) g_conf_syscall_coupling[1 + 2 * i] &&
            syscall_b == (uint64) g_conf_syscall_coupling[1 + 2 * i + 1]) ||
            (syscall_a == (uint64) g_conf_syscall_coupling[1 + 2 * i + 1] &&
            syscall_b == (uint64) g_conf_syscall_coupling[1 + 2 * i ]))
            return true;
    }
    
    return false;
    
}
    
    
