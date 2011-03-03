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
 * description: Eager Scheduling Algorithm   
 * initial author: Koushik Chakraborty 
 *
*/

//  #include "simics/first.h"
// RCSID("$Id: esp_alg.cc,v 1.1.2.1 2005/12/14 21:49:31 kchak Exp $");

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
#include "esp_alg.h"

esp_algorithm_t::esp_algorithm_t(string name, hw_context_t **hwc,
   thread_scheduler_t *_t_sched, chip_t *_p, uint32 _num_thr, uint32 _num_ctxt) :
    csp_algorithm_t(name, hwc, _t_sched, _p, _num_thr, _num_ctxt)
{
    
}


mai_t * esp_algorithm_t::find_thread_for_ctxt(hw_context_t *ctxt, 
    bool user_ctxt)
{
    mai_t *ret = 0;
    if (user_ctxt && !wait_for_user.empty()) {
		ret = wait_for_user.front();
		wait_for_user.pop_front();

	} else if (!user_ctxt && !wait_for_os.empty()) {
		ret = wait_for_os.front();
		wait_for_os.pop_front();
	}
    
    if (!ret) {
        ctxt->busy = false;
        if (user_ctxt) idle_user_ctxt.push_back(ctxt);
        else idle_os_ctxt.push_back(ctxt);
    }
    
    return ret;
}
 
hw_context_t *esp_algorithm_t::find_ctxt_for_thread(mai_t *thread, 
    bool desire_user_ctxt, uint32 syscall)
{
    // Eager scheduling: return h.w context in FIFO manner
    
    hw_context_t *ret = 0;
    if (!desire_user_ctxt && !idle_os_ctxt.empty())
    {
        ret = idle_os_ctxt.front();
        idle_os_ctxt.pop_front();
    } else if (desire_user_ctxt && !idle_user_ctxt.empty()) {
        ret = idle_user_ctxt.front();
        idle_user_ctxt.pop_front();
    }
    
    if (!ret) {
        if (desire_user_ctxt) wait_for_user.push_back(thread);
        else wait_for_os.push_back(thread);
    } else
        ret->busy = true;
    
    return ret;
}
      
       
void esp_algorithm_t::read_checkpoint(FILE *f)
{
    csp_algorithm_t::read_checkpoint(f);
}
    
       
void esp_algorithm_t::write_checkpoint(FILE *f)
{
    csp_algorithm_t::write_checkpoint(f);
}
    
    
void esp_algorithm_t::silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai)
{
    FAIL;
}    
    

