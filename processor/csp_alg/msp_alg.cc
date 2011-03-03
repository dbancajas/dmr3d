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
// RCSID("$Id: msp_alg.cc,v 1.1.2.2 2006/02/15 20:05:50 kchak Exp $");

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
#include "msp_alg.h"

msp_algorithm_t::msp_algorithm_t(string name, hw_context_t **hwc,
   thread_scheduler_t *_t_sched, chip_t *_p, uint32 _num_thr, uint32 _num_ctxt) :
    csp_algorithm_t(name, hwc, _t_sched, _p, _num_thr, _num_ctxt)
{
    // No hopping, num_threads should be exactly divisible by num_ctxt
    ASSERT(num_threads % num_ctxt == 0);
    
    // assume all needs user ctxt
    for (uint32 i = 0; i < p->get_num_cpus(); i++)
        wait_for_user.push_back(p->get_mai_object(i));
    
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        mai_t *mai = wait_for_user.front();
        p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, mai);
        wait_for_user.pop_front();
    }
}


mai_t * msp_algorithm_t::find_thread_for_ctxt(hw_context_t *ctxt, 
    bool desire_user_ctxt)
{
    mai_t *mai = wait_for_user.front();
    wait_for_user.pop_front();
    return mai;
}
 
hw_context_t *msp_algorithm_t::find_ctxt_for_thread(mai_t *thread, 
    bool desire_user_seq, uint32 syscall)
{
    wait_for_user.push_back(thread);
    return 0;
}
   
bool msp_algorithm_t::thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
    ts_yield_reason_t why)
	
{
    return (
		(why == YIELD_LONG_RUNNING && (!g_conf_no_os_pause || !mai->is_supervisor())) ||
        //why == YIELD_LONG_RUNNING ||
		why == YIELD_IDLE_ENTER ||
        why == YIELD_PAUSED_THREAD_INTERRUPT ||
		why == YIELD_MUTEX_LOCKED ||
        why == YIELD_PROC_MODE_CHANGE ||
        why == YIELD_EXTRA_LONG_RUNNING
		);
}
        
       
void msp_algorithm_t::read_checkpoint(FILE *f)
{
    wait_for_user.clear();
    csp_algorithm_t::read_checkpoint(f);
}
    
       
void msp_algorithm_t::write_checkpoint(FILE *f)
{
    csp_algorithm_t::write_checkpoint(f);
}
    
    
void msp_algorithm_t::silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai)
{

}    
    

