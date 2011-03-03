/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    thread scheduler for mapping threads to sequencers
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: thread_scheduler.cc,v 1.1.2.35 2006/08/18 14:30:21 kchak Exp $");

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
#include "base_alg.h"
#include "tsp_alg.h"
#include "ssp_alg.h"
#include "esp_alg.h"
#include "msp_alg.h"
#include "os_alg.h"
#include "hybrid_alg.h"
#include "bank_alg.h"
#include "func_alg.h"
#include "heatrun_alg.h"
#include "servc_alg.h"

hw_context_t::hw_context_t (sequencer_t *_s, uint32 _ct, bool _b)
    : seq(_s), ctxt(_ct), busy(_b), curr_sys_call(0), user_PC(0)
{
    why = YIELD_NONE;
}
        
void hw_context_t::debug_wait_list()
{
    list<mai_t *>::iterator it;
    for (it = wait_list.begin(); it != wait_list.end(); it++)
    {
        mai_t *m = *it;
        DEBUG_OUT("VCPU%u\n", m->get_id());
    }
}

thread_scheduler_t::thread_scheduler_t(chip_t *_chip)
{
	chip = _chip;
	
	num_user = g_conf_num_user_seq;
	num_hw_ctxt = 0;
    
	num_threads = chip->get_num_cpus();
   
    
    if (g_conf_chip_design[0]) {
        uint32 count = 0;
        num_seq = g_conf_chip_design[0];
        for (uint32 i = 1; i <= num_seq; i++)
            num_hw_ctxt += g_conf_chip_design[i];
        hw_context = new hw_context_t *[num_hw_ctxt];
        
        for (uint32 i = 1 ; i <= num_seq; i++)
        {
            for (uint32 j = 0; j < g_conf_chip_design[i]; j++, count++)
                hw_context[count] = new hw_context_t(chip->get_sequencer(i - 1), 
                    j, (count < num_threads));
        }
        
        
        
    } else {
        num_hw_ctxt = num_threads;
        hw_context = new hw_context_t *[num_hw_ctxt];
        for (uint32 i = 0; i < num_hw_ctxt; i++)
            hw_context[i] = new hw_context_t (chip->get_sequencer(i), 0, true);
    }
    
    switch(g_conf_scheduling_algorithm) {
        case BASE_SCHEDULING_POLICY:
        case VANILLA_GANG_SCHEDULING_POLICY:
            sched_alg = new base_algorithm_t("base_alg",  hw_context, this, chip,  
                num_threads, num_hw_ctxt);
            break;
        case THREAD_SCHEDULING_POLICY:
            sched_alg =  new tsp_algorithm_t("tsp_alg",  hw_context, this, chip,  
                num_threads, num_hw_ctxt);
            break;
        case SYSCALL_SCHEDULING_POLICY:
            sched_alg = new ssp_algorithm_t("ssp_alg",  hw_context, this, chip,  
                num_threads, num_hw_ctxt);
            break;    
        case EAGER_SCHEDULING_POLICY:
            sched_alg = new esp_algorithm_t("esp_alg",  hw_context, this, chip,  
                num_threads, num_hw_ctxt);
            break;
        case MIGRATORY_SCHEDULING_POLICY:
            sched_alg = new msp_algorithm_t("msp_alg",  hw_context, this, chip,  
                num_threads, num_hw_ctxt);
            break;  
        case HYBRID_SCHEDULING_POLICY: 
            sched_alg = new hybrid_algorithm_t("hybrid_alg",  hw_context, this, chip,  
                num_threads, num_hw_ctxt);
            break;
        case OS_NOPAUSE_SCHEDULING_POLICY:
            sched_alg = new os_algorithm_t("os_alg", hw_context, this, chip, num_threads,
                num_hw_ctxt);
            break;    
        case BANK_SCHEDULING_POLICY:
            //sched_alg = new bank_algorithm_t("bank_alg",  hw_context, this, chip,  
            //    num_threads, num_hw_ctxt);
            break;
        case FUNCTION_SCHEDULING_POLICY:
            sched_alg = new  func_algorithm_t("func_alg", hw_context, this, chip,
                num_threads, num_hw_ctxt);
            break;
        case HEATRUN_SCHEDULING_POLICY:
            sched_alg = new  heatrun_algorithm_t("heatrun_alg", hw_context, this, chip,
                num_threads, num_hw_ctxt);
            break;
        case SERVC_SCHEDULING_POLICY:
            sched_alg = new  servc_algorithm_t("servc_alg", hw_context, this, chip,
                num_threads, num_hw_ctxt);
            break;
        default:
            FAIL;
    }
   
    
}

ts_yield_reason_t 
thread_scheduler_t::prioritize_yield_reason(ts_yield_reason_t new_val,
	ts_yield_reason_t old_val)
{
	return sched_alg->prioritize_yield_reason(new_val, old_val);
}


bool
thread_scheduler_t::thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai,
	ts_yield_reason_t why)
{
	// A currently running sequencer thinks its thread may need to switch
	// Check for sure if this is what we want to do and do it.
    return sched_alg->thread_yield(seq, ctxt, mai, why);
    
}

bool thread_scheduler_t::thread_yield(addr_t PC, sequencer_t *seq, uint32 t_ctxt)
{
    return sched_alg->thread_yield(PC, seq, t_ctxt);
}

void thread_scheduler_t::debug_idle_context(list<hw_context_t *> l)
{
    list<hw_context_t *>::iterator it = l.begin();
    while (it != l.end())
    {
        hw_context_t *hwc = *it;
        DEBUG_OUT("seq = %d ctxt = %d\n", hwc->seq->get_id(), hwc->ctxt);
        it++;
    }
}

void thread_scheduler_t::migrate()
{
    sched_alg->migrate();
}

void
thread_scheduler_t::ready_for_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai,
	ts_yield_reason_t why)
{
	
	
	// Something may have changed, and we now don't want to switch
    // Can't call the thread_yield twice with OS_NOPAUSE
	if (g_conf_scheduling_algorithm != OS_NOPAUSE_SCHEDULING_POLICY) {
        if (!thread_yield(seq, ctxt, mai, why))
        {
            //FAIL;
            //return;
            chip->switch_to_thread(seq, ctxt, mai);
            return;
        }
    }
    
    if (g_conf_cache_only_provision) {
        sched_alg->silent_switch(seq, ctxt, mai);
        return;
    }
	bool user_ctxt = is_user_ctxt(seq, ctxt); 
	bool desire_user_seq = g_conf_use_processor ? !mai->is_supervisor() : false;  // may == user_seq if preemted
    uint32 syscall = 0;
    if (!desire_user_seq) syscall = mai->get_syscall_num();
   
	mai->set_last_eviction(chip->get_g_cycles());
    //ASSERT(desire_user_seq || syscall || !g_conf_use_processor);
    
	hw_context_t *hw_ctxt = search_hw_context(seq, ctxt);
    hw_ctxt->why = why;
    hw_context_t *new_ctxt = sched_alg->find_ctxt_for_thread(mai, desire_user_seq, syscall);
    
	if (new_ctxt) {
		chip->switch_to_thread(new_ctxt->seq, new_ctxt->ctxt, mai);
	} else {
        chip->idle_thread_context(mai->get_id());
	}

	// check for matching idle thread to run on newly opened seq	
	// or put old seq on idle list
	mai_t *new_thread = sched_alg->find_thread_for_ctxt(hw_ctxt, user_ctxt);
  //  ASSERT(wait_for_os.size() + wait_for_user.size() < num_threads);
	chip->switch_to_thread(seq, ctxt, new_thread);
			
}


void 
thread_scheduler_t::write_checkpoint(FILE *file)
{
    sched_alg->write_checkpoint(file);
}

void thread_scheduler_t::read_checkpoint(FILE *file)
{
    sched_alg->read_checkpoint(file);
}

bool
thread_scheduler_t::is_user_ctxt(sequencer_t *_s, uint32 _c) {
    // Hack for cache_only_provision
    if (g_conf_cache_only_provision) _s = _s->get_mem_hier_seq(_c);
    for (uint32 i = 0; i < num_hw_ctxt; i++)
        if (hw_context[i]->seq == _s && hw_context[i]->ctxt == _c)
            return (i < num_user);
    FAIL;
}

bool
thread_scheduler_t::is_os_ctxt(sequencer_t *_s, uint32 _c) {
	for (uint32 i = 0; i < num_hw_ctxt; i++)
        if (hw_context[i]->seq == _s && hw_context[i]->ctxt == _c)
            return (i >= num_user);
    FAIL;
}

hw_context_t *
thread_scheduler_t::search_hw_context(sequencer_t *_seq, uint32 _ct)
{
    for (uint32 i = 0; i < num_hw_ctxt; i++)
        if (hw_context[i]->seq == _seq && hw_context[i]->ctxt == _ct)
            return hw_context[i];
    FAIL_MSG("Cannot find hardware context");
    return 0;
}

uint32 
thread_scheduler_t::get_context_id(hw_context_t *hwc)
{
    for (uint32 i = 0; i < num_hw_ctxt; i++)
        if (hw_context[i] == hwc) return i;
    FAIL_MSG ("Invalid context id passed");
    return num_hw_ctxt;
}

sequencer_t * 
thread_scheduler_t::handle_interrupt_thread(uint32 thread_id)
{
    return sched_alg->handle_interrupt_thread(thread_id);
}

void thread_scheduler_t::disable_core(sequencer_t *seq)
{
    sched_alg->disable_core(seq);
}

void thread_scheduler_t::enable_core(sequencer_t *seq)
{
    sched_alg->enable_core(seq);
}

sequencer_t * 
thread_scheduler_t::handle_synchronous_interrupt(uint32 src, uint32 target)
{
    return sched_alg->handle_synchronous_interrupt(src, target);
}

uint32 thread_scheduler_t::waiting_interrupt_threads()
{
    return sched_alg->waiting_interrupt_threads();
}

void thread_scheduler_t::update_cycle_stats()
{
    sched_alg->update_cycle_stats();
}

void thread_scheduler_t::print_stats()
{
    sched_alg->print_stats();
}

void thread_scheduler_t::ctrl_flow_redirection(uint32 cpu_id)
{
    sched_alg->ctrl_flow_redirection(cpu_id);
}
