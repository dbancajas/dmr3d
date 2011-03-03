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
 * description: Server Consolidation Alg. to work with num_threads > num_ctxt    
 * initial author: Koushik Chakraborty 
 *
*/

//  #include "simics/first.h"
// RCSID("$Id: servc_alg.cc,v 1.1.2.15 2007/04/09 22:24:46 kchak Exp $");

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
#include "servc_alg.h"

servc_algorithm_t::servc_algorithm_t(string name, hw_context_t **hwc,
   thread_scheduler_t *_t_sched, chip_t *_p, uint32 _num_thr, uint32 _num_ctxt) 
    : csp_algorithm_t(name, hwc, _t_sched, _p, _num_thr, _num_ctxt)
{
    max_active_cores = num_threads / g_conf_num_machines;
    
    for (uint32 i = 0; i < num_threads; i++) {
        uint32 ctxt_id = i;
        if (i >= num_ctxt) {
            ctxt_id = i - max_active_cores;
        }
        hw_context[ctxt_id]->wait_list.push_back(p->get_mai_from_thread(i));
    }
    
    for (uint32 i = 0; i < num_ctxt; i++) {
        if (i < max_active_cores)        
            schedule(i);
        else
            p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, 0);
    }
    
}


void servc_algorithm_t::schedule(uint32 i)
{
    mai_t *mai = hw_context[i]->wait_list.front();
    hw_context[i]->wait_list.pop_front();
    p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, mai);
}


mai_t * servc_algorithm_t::find_thread_for_ctxt(hw_context_t *ctxt, 
    bool desire_user_ctxt)
{
    uint32 seq_id = ctxt->seq->get_id();
    uint32 resume_seq_id;
    if (ctxt->wait_list.size())
        resume_seq_id = seq_id;
    else if (seq_id >= max_active_cores) 
        resume_seq_id = (seq_id - max_active_cores);
    else
        resume_seq_id = (seq_id + max_active_cores);
    schedule(resume_seq_id);
    mai_t *thread = ctxt->seq->get_mai_object(0);
    ctxt->wait_list.push_back(thread);    
    return 0;    
}

hw_context_t *servc_algorithm_t::find_ctxt_for_thread(mai_t *thread, 
    bool desire_user_seq, uint32 syscall)
{
    return 0;
	
	
}
   
bool servc_algorithm_t::thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
    ts_yield_reason_t why)
	
{
    switch(why) {
        case YIELD_LONG_RUNNING:
            return true;
        case YIELD_EXCEPTION:
        case YIELD_DONE_RETRY:
        case YIELD_MUTEX_LOCKED:
        case YIELD_PAGE_FAULT:
            return false;
        default:
            FAIL_MSG("Failed with reason %u\n", why);
    }
}
    
       
void servc_algorithm_t::read_checkpoint(FILE *file)
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
    
	
}
    
       
void servc_algorithm_t::write_checkpoint(FILE *file)
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
    
}
    
    
void servc_algorithm_t::silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai)
{

}    
    
