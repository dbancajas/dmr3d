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
// RCSID("$Id: tsp_alg.cc,v 1.1.2.7 2006/08/18 14:30:26 kchak Exp $");

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

tsp_algorithm_t::tsp_algorithm_t(string name, hw_context_t **hwc,
   thread_scheduler_t *_t_sched, chip_t *_p, uint32 _num_thr, uint32 _num_ctxt) :
    csp_algorithm_t(name, hwc, _t_sched, _p, _num_thr, _num_ctxt)
{
    preferred_user_ctxt = new uint32[num_threads];
    preferred_os_ctxt   = new uint32[num_threads];
    
    uint32 curr_ctxt = 0;
    for (uint32 i = 0; i < num_threads; i++)
    {
        preferred_user_ctxt[i] = curr_ctxt++;
        if (curr_ctxt == g_conf_num_user_seq) curr_ctxt = 0;
    }
    curr_ctxt = g_conf_num_user_seq;
    for (uint32 i = 0; i < num_threads; i++)
    {
        preferred_os_ctxt[i] = curr_ctxt++;
        if (curr_ctxt == num_ctxt) curr_ctxt = g_conf_num_user_seq;
    }
    
    last_os_mapping_switch = 100;
    last_user_mapping_switch = 100;
    next_os_thread_switch = 0;
    next_user_thread_switch = 0;
    if (g_conf_num_user_seq) {
        require_os_hop = ((num_threads > (num_ctxt - g_conf_num_user_seq)) && 
            (num_threads % (num_ctxt - g_conf_num_user_seq) != 0));
        require_user_hop = (num_threads % g_conf_num_user_seq != 0);
    } else {
        require_os_hop = false;
        require_user_hop = false;
    }
  
    for (uint32 i = 0; i < _num_ctxt; i++) {
        if (i < num_threads) {
            p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, 
                p->get_mai_object(i));
        } else {
            p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, 
                0);
            hw_context[i]->busy = false;
            idle_os_ctxt.push_back(hw_context[i]);
        }
	}
    for (uint32 i = num_ctxt; i < num_threads; i++)
    {
        if (i % 2 == 0) wait_for_user.push_back(p->get_mai_object(i));
        else wait_for_os.push_back(p->get_mai_object(i));
    }
    
    stat_idle_syscall = stats->COUNTER_GROUP("stat_idle_syscall", "Cycles wasted in idle", 600);
    stat_idle_syscall->set_void_zero_entries();
    idle_cycle_start = new tick_t[_num_thr];
    bzero(idle_cycle_start, sizeof(tick_t) * _num_thr);
    
    
}


mai_t * tsp_algorithm_t::find_thread_for_ctxt(hw_context_t *ctxt, 
    bool user_ctxt)
{
    mai_t *ret = 0;
    if (user_ctxt && !wait_for_user.empty())
    {
        list<mai_t *>::iterator it = wait_for_user.begin();
        while (it != wait_for_user.end())
        {
            mai_t *candidate = *it;
            if (smt_thread_ctxt_match(candidate, ctxt, user_ctxt))
            {
                ret = candidate;
                wait_for_user.erase(it);
                break;
            }
            it++;
        }
    } else if (!user_ctxt && !wait_for_os.empty()) {
        // If evicted due to spinning in OS, we must try to
        // schedule some other thread rightaway
        list<mai_t *>::iterator it = wait_for_os.begin();
        while (it != wait_for_os.end())
        {
            mai_t *candidate = *it;
            if (smt_thread_ctxt_match(candidate, ctxt, user_ctxt) ||
                ctxt->why == YIELD_MUTEX_LOCKED)
            {
                ret = candidate;
                wait_for_os.erase(it);
                break;
            }
            it++;
        }   
    }
    
    if (!ret) {
        if (g_conf_avoid_idle_context) {
            if ((user_ctxt && wait_for_user.size()) ||
                (!user_ctxt && wait_for_os.empty())) {
                    ret = wait_for_user.front();
                    wait_for_user.pop_front();
            } else {
                ret = wait_for_os.front();
                wait_for_os.pop_front();
            }
            
            ASSERT(ret);
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
       
    
    return ret;
}
 
hw_context_t *tsp_algorithm_t::find_ctxt_for_thread(mai_t *thread, 
    bool desire_user_ctxt, uint32 syscall)
{
    hw_context_t *ret = 0;
    if (!desire_user_ctxt)
    {
        hw_context_t *preferred = get_preferred_os_context(thread->get_id());
        if (preferred->busy) preferred = get_smt_aware_ctxt(preferred);
        if (!preferred->busy) 
        {
            ASSERT(find(idle_os_ctxt.begin(), idle_os_ctxt.end(), preferred)
                    != idle_os_ctxt.end());
            ret = preferred;
            idle_os_ctxt.remove(preferred);
        }
        
    } else {
        hw_context_t *preferred = get_preferred_user_context(thread->get_id());
        if (preferred->busy) preferred = get_smt_aware_ctxt(preferred);
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
    } else 
        ret->busy = true;
            
    return ret;
    
}
   
bool tsp_algorithm_t::thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
    ts_yield_reason_t why)
	
{
    // TODO --------- MUST READ
    // Must check what thread yield returns. If we are returning
    // don't yield because there is no other thread to run
    // then, we must make sure that we pre-empt this thread as soon
    // as a new thread becomes ready to run on the same sequencer.
    bool user_seq = t_sched->is_user_ctxt(seq, ctxt);
	uint64 tt;
	
	switch(why) {
		case YIELD_LONG_RUNNING:
            if (g_conf_no_os_pause && !mai->can_preempt())
                return false;
			if (user_seq && !wait_for_user.empty())
				return true;
			if (!user_seq && !wait_for_os.empty())
				return true;
			break;
	
		case YIELD_MUTEX_LOCKED:
			//ASSERT(mai->is_supervisor());
			if (!user_seq && !wait_for_os.empty())
				return true;
            if (user_seq && !wait_for_user.empty())
                return true;
            if (g_conf_avoid_idle_context) 
                return true;
			break;
	
		case YIELD_DONE_RETRY:
			// returned to user
			if (!mai->is_supervisor() && !user_seq) {
                if (g_conf_eager_yield)
                    return true;
                else {
                    sequencer_t *seq = p->get_sequencer(mai->get_id() % g_conf_num_user_seq);
                    if (seq->get_mai_object(0) == NULL ||
                        seq->get_last_thread_switch(0) > 100000000)
                        return true;
                }
                DEBUG_OUT("Forced to execute user code on seq%u vcpu%u\n", 
                    seq->get_id(), mai->get_id());
            }
            
			break;

		case YIELD_EXCEPTION:
			ASSERT(mai->get_tl() > 0);
			ASSERT(mai->is_supervisor());
			tt = mai->get_tt(mai->get_tl());
			if (mai->is_syscall_trap(tt) || mai->is_interrupt_trap(tt))
            {
                if (user_seq) {
                    if (g_conf_eager_yield)
                        return true;
                    else {
                        uint32 os_core = g_conf_num_user_seq + (mai->get_id() %
                                (num_ctxt - g_conf_num_user_seq));
                        sequencer_t *seq = p->get_sequencer(os_core);
                        if (seq->get_mai_object(0) == NULL ||
                            seq->get_last_thread_switch(0) > 100000000)
                            return true;
                    }
                }
                            
                // DEBUG_OUT("Forced to execute OS code on seq%u for VCPU%u\n", 
                //     seq->get_id(), mai->get_id());   
            }
			break;
		
		case YIELD_PAGE_FAULT:
			ASSERT (mai->get_syscall_num() == SYS_NUM_PAGE_FAULT); 
			return true;
			
		case YIELD_SYSCALL_SWITCH:
			ASSERT (mai->get_syscall_num() != 0); 
			return true;
			
		case YIELD_IDLE_ENTER:
			ASSERT(mai->is_supervisor());
			if (!wait_for_os.empty())
				return true;
			break;
	
        case YIELD_PROC_MODE_CHANGE:
        case YIELD_EXTRA_LONG_RUNNING:
            return true;
            
		default:
			FAIL;
			return false;
	}

    if (why == YIELD_MUTEX_LOCKED) 
        ASSERT ((user_seq && wait_for_user.empty()) || (!user_seq && wait_for_os.empty()));  
	return false;
}
        
      
bool tsp_algorithm_t::user_vcpu_waiting(sequencer_t *seq, uint32 ctxt)
{
    hw_context_t *hwc = t_sched->search_hw_context(seq, ctxt);
    
    list<mai_t *>::iterator it = wait_for_user.begin();
    while (it != wait_for_user.end())
    {
        mai_t *candidate = *it;
        if (hwc == hw_context [preferred_user_ctxt [candidate->get_id()] ])
            return true;
        it++;
    }
    
    if (g_conf_avoid_idle_context) return (wait_for_user.size() > 0);
    return false;
    
}


void tsp_algorithm_t::read_checkpoint(FILE *file)
{
    csp_algorithm_t::read_checkpoint(file);
    fscanf(file, "%llu %llu %u %u\n", &last_user_mapping_switch,
      &last_os_mapping_switch, &next_user_thread_switch, &next_os_thread_switch);
    
    for (uint32 i = 0; i < num_threads; i++) {
        fscanf(file, "%llu ", &idle_cycle_start[i]);
        if (idle_cycle_start[i]) {
            sequencer_t *idle_seq = p->get_sequencer(i);
            idle_seq->set_mai_thread(0, 0);
            idle_seq->set_mem_hier_seq(0, 0);
        }
    }
    
}
    
       
void tsp_algorithm_t::write_checkpoint(FILE *file)
{
    csp_algorithm_t::write_checkpoint(file);
    fprintf(file, "%llu %llu %u %u\n", last_user_mapping_switch,
      last_os_mapping_switch, next_user_thread_switch, next_os_thread_switch);
    
    for (uint32 i = 0; i < num_threads; i++)
        fprintf(file, "%llu ", idle_cycle_start[i]);
    fprintf(file, "\n");
    
}
    
    

hw_context_t *tsp_algorithm_t::get_preferred_user_context(uint32 t_id)
{
    if (!require_user_hop) return hw_context[preferred_user_ctxt[t_id]];
    tick_t curr_cycle = p->get_g_cycles();
    if (curr_cycle > last_user_mapping_switch &&
        (curr_cycle - last_user_mapping_switch) > (uint32)g_conf_thread_hop_interval)
    {
        uint32 num_hops = get_num_hops(g_conf_num_user_seq);
        for (uint32 i = 0; i < num_hops; i++)
        {
            uint32 home = preferred_user_ctxt[next_user_thread_switch];
            preferred_user_ctxt[next_user_thread_switch] = (home + num_hops) % g_conf_num_user_seq;
            next_user_thread_switch = (next_user_thread_switch + 1) % num_threads;
        }
        last_user_mapping_switch = curr_cycle;
        for (uint32 i = 0; i < num_threads; i++) {
            ASSERT(preferred_user_ctxt[i] < g_conf_num_user_seq);
            DEBUG_OUT("%u ---> %u\n", i, preferred_user_ctxt[i]);
        }
    }
    
    
    
    return hw_context[preferred_user_ctxt[t_id]];
}


hw_context_t *tsp_algorithm_t::get_preferred_os_context(uint32 t_id)
{
 
    if (!require_os_hop) return hw_context[preferred_os_ctxt[t_id]];
    uint32 ctxt  = 0;
    tick_t curr_cycle = p->get_g_cycles();
     if (curr_cycle > last_os_mapping_switch &&
        (curr_cycle - last_os_mapping_switch > (uint32)g_conf_thread_hop_interval)) {
        uint32 num_hops = get_num_hops(num_ctxt - g_conf_num_user_seq);
        for (uint32 i = 0; i < num_hops; i++)
        {
            uint32 home = preferred_os_ctxt[next_os_thread_switch];
            uint32 residual = (home + num_hops) % num_ctxt;
            preferred_os_ctxt[next_os_thread_switch] = (home + num_hops >= num_ctxt) 
                ? g_conf_num_user_seq + residual: (home + num_hops) ;
            next_os_thread_switch = (next_os_thread_switch + 1) % num_threads;
        }
        last_os_mapping_switch = curr_cycle;
    }
    
    for (uint32 i = 0; i < num_threads; i++)
        ASSERT(preferred_os_ctxt[i] >= g_conf_num_user_seq &&
               preferred_os_ctxt[i] < num_ctxt);
    
    ctxt = preferred_os_ctxt[t_id];
    return hw_context[ctxt];
}

    

uint32 tsp_algorithm_t::get_num_hops(uint32 resources)
{
    // Assume that number of threads == 8
    switch(resources) {
        case 3:
            return 2;
            break;
        case 5:
            return 3;
            break;
        case 6:
            return 2;
            break;
        case 7:
            return 1;
            break;
        default:
            FAIL;
    }
    
}


    

void tsp_algorithm_t::silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai)
{
	bool desire_user_ctxt = !mai->is_supervisor();  // may == user_seq if preemted

	FE_EXCEPTION_CHECK;
	// find idle seq, or put thread on idle list
    hw_context_t *preferred_ctxt = 0;
    
    if (desire_user_ctxt)
    {
        preferred_ctxt = get_preferred_user_context(mai->get_id());
        
    } else {
        // Assume single mapping;
        preferred_ctxt = get_preferred_os_context(mai->get_id());
    }
 
    silent_switch_finishup(seq, preferred_ctxt, ctxt);
}


void tsp_algorithm_t::silent_switch_finishup(sequencer_t *seq, hw_context_t *preferred_ctxt, uint32 ctxt)
{
    
    
    sequencer_t *vacating_seq = seq->get_mem_hier_seq(ctxt);
    uint32 next_seq_cnt = p->current_occupancy(preferred_ctxt->seq, ctxt);
    ASSERT(next_seq_cnt <= g_conf_max_ssp_context);
    if (next_seq_cnt == g_conf_max_ssp_context)
    {
        // Can't schedule it rightaway
        preferred_ctxt->wait_list.push_back(seq->get_mai_object(ctxt));
        seq->set_mai_thread(ctxt, 0);
        seq->set_mem_hier_seq(0, ctxt);
        DEBUG_OUT("vcpu%u is now going idle @ %llu\n", seq->get_id(), p->get_g_cycles());
        idle_cycle_start[seq->get_id()] = p->get_g_cycles();
        
    } else 
    
        seq->set_mem_hier_seq(preferred_ctxt->seq, ctxt);
        
    hw_context_t *vacating_hw_context = hw_context[vacating_seq->get_id()];    
    if (vacating_hw_context->wait_list.size())
    {
        mai_t *thr = vacating_hw_context->wait_list.front();
        vacating_hw_context->wait_list.pop_front();
        sequencer_t *start_seq = p->get_sequencer(thr->get_id());
        start_seq->set_mai_thread(ctxt, thr);
        start_seq->set_mem_hier_seq(vacating_seq, ctxt);
        DEBUG_OUT("vcpu%u is active @ %llu\n", thr->get_id(), p->get_g_cycles());
        ASSERT(idle_cycle_start[thr->get_id()]);
        stat_idle_syscall->inc_total( p->get_g_cycles() - idle_cycle_start[thr->get_id()],
            thr->get_syscall_num());
        idle_cycle_start[thr->get_id()] = 0;
    }
    
    
}
hw_context_t * tsp_algorithm_t::get_smt_aware_ctxt(hw_context_t *ctxt)
{
    sequencer_t *seq = ctxt->seq;
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        if (hw_context[i]->seq == seq && hw_context[i]->busy == false)
            return hw_context[i];
    }
    return ctxt;
}
    

bool tsp_algorithm_t::smt_thread_ctxt_match(mai_t *candidate, hw_context_t *ctxt,
    bool user_ctxt)
{
    if (user_ctxt)
        return (ctxt->seq->get_id() == 
            hw_context[preferred_user_ctxt[candidate->get_id()]]->seq->get_id());
        
    return (ctxt->seq->get_id() == 
            hw_context[preferred_os_ctxt[candidate->get_id()]]->seq->get_id());
            
}
