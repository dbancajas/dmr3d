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
 * description: base class for csp algorithms   
 * initial author: Koushik Chakraborty 
 *
*/

//  #include "simics/first.h"
// RCSID("$Id: csp_alg.cc,v 1.1.2.5 2006/07/27 16:15:04 kchak Exp $");

#include "definitions.h"
#include "isa.h"
#include "dynamic.h"
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

csp_algorithm_t::csp_algorithm_t(string _name, hw_context_t **_hwc,
     thread_scheduler_t *_t_sched, chip_t *_p, uint32 _num_thr, uint32 _num_ctxt):
     name(_name), hw_context(_hwc), t_sched(_t_sched), p(_p), num_threads(_num_thr),
     num_ctxt(_num_ctxt)
{
    stats = new stats_t(name);
    stat_syscall_wait = stats->COUNTER_GROUP ("syscall_wait",
		"syscall waits", 1024);
	stat_syscall_wait->set_void_zero_entries ();
    stat_user_wait = stats->COUNTER_GROUP ("stat_user_wait", "user wait", num_threads);
    stat_user_wait->set_void_zero_entries ();
}

void csp_algorithm_t::print_stats()
{
    stats->print();
}

void csp_algorithm_t::clear_stats()
{
    stats->clear();
}

csp_algorithm_t::~csp_algorithm_t()
{
    
}

void csp_algorithm_t::write_checkpoint(FILE *file)
{
    // Put the OS contexts first
    list<hw_context_t *>::iterator it;
    hw_context_t *hwc;
    
    
    fprintf(file, "%u\n", idle_os_ctxt.size());
    for (it = idle_os_ctxt.begin(); it != idle_os_ctxt.end(); it++)
    {
        hwc = *it;
        ASSERT(hwc->seq->get_mai_object(hwc->ctxt) == 0);
        fprintf(file, "%u %u\n", hwc->seq->get_id(), hwc->ctxt);
    }
    
    fprintf(file, "%u\n", idle_user_ctxt.size());
    for (it = idle_user_ctxt.begin(); it != idle_user_ctxt.end(); it++)
    {
        hwc = *it;
        ASSERT(hwc->seq->get_mai_object(hwc->ctxt) == 0);
        fprintf(file, "%u %u\n", hwc->seq->get_id(), hwc->ctxt);
    }
    
    // Now the idle cpus
    list<mai_t *>::iterator t_it;
    fprintf(file, "%u\n", wait_for_os.size());
    for (t_it = wait_for_os.begin(); t_it != wait_for_os.end(); t_it++)
    {
        mai_t *thread = *t_it;
        fprintf(file, "%u\n", thread->get_id());
    }
    
    fprintf(file, "%u\n", wait_for_user.size());
    for (t_it = wait_for_user.begin(); t_it != wait_for_user.end(); t_it++)
    {
        mai_t *thread = *t_it;
        fprintf(file, "%u\n", thread->get_id());
    }
    
    // Hw_context busy flag
    for (uint32 i = 0; i < num_ctxt; i++) {
        fprintf(file, "%u %u\n", hw_context[i]->busy, hw_context[i]->wait_list.size());
        if (hw_context[i]->wait_list.size())
        {
            list<mai_t *>::iterator wit = hw_context[i]->wait_list.begin();
            for (; wit != hw_context[i]->wait_list.end(); wit++)
            {
                mai_t *t = *wit;
                fprintf(file, "%u ", t->get_id());
            }
        }
        fprintf(file, "\n");
    }
   
}

void csp_algorithm_t::read_checkpoint(FILE *file)
{
    idle_os_ctxt.clear();
    idle_user_ctxt.clear();
    wait_for_user.clear();
    wait_for_os.clear();
    
    list<hw_context_t *>::iterator it;
    hw_context_t *hwc;
 
    uint32 idle_ctxt, seq_id, _ct;
    fscanf(file, "%u\n", &idle_ctxt);
    for (uint32 i = 0; i < idle_ctxt; i++)
    {
        fscanf(file, "%u %u\n", &seq_id, &_ct);
        hwc = t_sched->search_hw_context(p->get_sequencer(seq_id), _ct);
        ASSERT(p->get_sequencer(seq_id)->get_mai_object(_ct) == NULL);
        idle_os_ctxt.push_back(hwc);
    }
    
    fscanf(file, "%u\n", &idle_ctxt);
    for (uint32 i = 0; i < idle_ctxt; i++)
    {
        fscanf(file, "%u %u\n", &seq_id, &_ct);
        ASSERT(p->get_sequencer(seq_id)->get_mai_object(_ct) == NULL);
        hwc = t_sched->search_hw_context(p->get_sequencer(seq_id), _ct);
        idle_user_ctxt.push_back(hwc);
    }
    
    uint32 thread_id, idle_threads;
    fscanf(file, "%u\n", &idle_threads);
    for (uint32 i = 0; i < idle_threads; i++)
    {
        fscanf(file , "%u\n", &thread_id);
        mai_t *thread = p->get_mai_object(thread_id);
        wait_for_os.push_back(thread);
    }
    
    fscanf(file, "%u\n", &idle_threads);
    for (uint32 i = 0; i < idle_threads; i++)
    {
        fscanf(file , "%u\n", &thread_id);
        mai_t *thread = p->get_mai_object(thread_id);
        wait_for_user.push_back(thread);
    }
    
    
   
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        uint32 busy, count;
        fscanf(file, "%u %u\n", &busy, &count);
        hw_context[i]->busy = busy ? true : false;
        if (count) {
            for (uint32 j = 0; j < count; j++)
            {
                uint32 tid;
                fscanf(file, "%u ", &tid);
                hw_context[i]->wait_list.push_back(p->get_mai_object(tid));
            }
        }
        fscanf(file, "\n");
    }
    
}

ts_yield_reason_t 
csp_algorithm_t::prioritize_yield_reason(ts_yield_reason_t new_val,
	ts_yield_reason_t old_val)
{
	return old_val;
}

bool csp_algorithm_t::thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
    ts_yield_reason_t why)
{
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
			if (!mai->is_supervisor() && !user_seq)
				return true;
			break;

		case YIELD_EXCEPTION:
			ASSERT(mai->get_tl() > 0);
			ASSERT(mai->is_supervisor());
			tt = mai->get_tt(mai->get_tl());
			if (mai->is_syscall_trap(tt) || mai->is_interrupt_trap(tt))
            {
				return true;
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

	return false;
    
}

void csp_algorithm_t::debug_user_ctxt()
{
    uint32 size = idle_user_ctxt.size();
    uint32 count = 0;
    for (uint32 i = 0; i < g_conf_num_user_seq; i++)
    {
        if (hw_context[i]->busy == false) count++;
    }
    ASSERT(count == size);
}

void csp_algorithm_t::update_cycle_stats()
{
    list<mai_t *>::iterator it;
    for (it = wait_for_os.begin(); it != wait_for_os.end(); it++)
    {
        mai_t *thread = *it;
        stat_syscall_wait->inc_total(1, thread->get_syscall_num());
    }
    stat_user_wait->inc_total(1, wait_for_user.size());   
}


bool csp_algorithm_t::thread_yield(addr_t PC, sequencer_t *seq, uint32 t_ctxt)
{
    return false;
}
