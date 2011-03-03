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
 * description: Heat and Run Algorithm     
 * initial author: Koushik Chakraborty 
 *
 */
 



//  #include "simics/first.h"
// RCSID("$Id: heatrun_alg.cc,v 1.1.2.15 2007/04/23 22:24:46 kchak Exp $");

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
#include "heatrun_alg.h"


heatrun_algorithm_t::heatrun_algorithm_t(string name, hw_context_t **hwc,
   thread_scheduler_t *_t_sched, chip_t *_p, uint32 _num_thr, uint32 _num_ctxt) 
    : csp_algorithm_t(name, hwc, _t_sched, _p, _num_thr, _num_ctxt) 
    
{
    last_preempt = new tick_t[num_threads];
    idle_epoch = new uint32[num_ctxt];
    uint32 host;
    for (uint32 i = 0; i < num_threads; i++)
    {
        last_preempt[i] = 0;
        host = i % num_ctxt;
        vcpu_2_pcpu[i] = host;
        vcpu_host_pcpu[i] = host;
        hw_context[host]->wait_list.push_back(p->get_mai_object(i));
    }
    
    // Handle the simple case only
    ASSERT( num_ctxt > num_threads && (num_threads % (num_ctxt - num_threads) == 0));
        
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        if (hw_context[i]->wait_list.size()) {
            idle_epoch[i] = 0;    
            schedule(i);
        } else {
            p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, 
                0);
            idle_epoch[i] = 1;
            hw_context[i]->busy = false;
        }
    }
    
}


void heatrun_algorithm_t::schedule(uint32 i)
{
    mai_t *mai = hw_context[i]->wait_list.front();
    hw_context[i]->wait_list.pop_front();
    p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, mai);
}

void heatrun_algorithm_t::migrate()
{
    tick_t max_preempt = 0;
    tick_t current_cycle = p->get_g_cycles();
    
    uint32 m_thread = 0;
    
    for (uint32 i = 0; i < num_threads; i++)
    {
        tick_t  time_at_current_host = current_cycle - last_preempt[i];
        if (time_at_current_host > max_preempt) {
            m_thread = i;
            max_preempt = time_at_current_host;
        }
    }
    
    sequencer_t *seq = p->get_sequencer(vcpu_2_pcpu[m_thread]);
    seq->potential_thread_switch (0, YIELD_HEATRUN_MIGRATE);
    
    uint32 max_epoch = 0;
    uint32 ctxt_id = num_ctxt;
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        if (hw_context[i]->busy == false) {
            if (ctxt_id == num_ctxt)
                ctxt_id = i;
            if (idle_epoch[i] > max_epoch) {
                ctxt_id = i;
                max_epoch = idle_epoch[i];
            }
        }
    }
    
    ASSERT(ctxt_id < num_ctxt && hw_context[ctxt_id]->busy == false);
    vcpu_2_pcpu[m_thread] = ctxt_id;
    update_idle_epochs();
}

 
bool heatrun_algorithm_t::thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
    ts_yield_reason_t why)
{
    if (why == YIELD_HEATRUN_MIGRATE) return true;
    return false;
}


mai_t * heatrun_algorithm_t::find_thread_for_ctxt(hw_context_t *ctxt, 
    bool desire_user_ctxt)
{
    if (ctxt->wait_list.size() == 0) {
        ctxt->busy = false;
        return 0;
    }
    
    mai_t *mai = ctxt->wait_list.front();
    ctxt->wait_list.pop_front();
    ASSERT(mai);
    if (mai) DEBUG_OUT("tpu%u will run on cpu%u\n", mai->get_id(), ctxt->seq->get_id());
    return mai;
}

hw_context_t *heatrun_algorithm_t::find_ctxt_for_thread(mai_t *thread, 
    bool desire_user_seq, uint32 syscall)

{
    uint32 thread_id = thread->get_id();
    last_preempt[thread_id] = p->get_g_cycles();
   
    uint32 ctxt_id = vcpu_2_pcpu[thread_id];
    hw_context[ctxt_id]->busy = true;
    
    return hw_context[ctxt_id];
    
}

void heatrun_algorithm_t::update_idle_epochs()
{
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        if (hw_context[i]->busy == false)
            idle_epoch[i]++;
        else
            idle_epoch[i] = 0;
    }
}


    
void heatrun_algorithm_t::read_checkpoint(FILE *file)
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
    
    for (uint32 i = 0; i < num_ctxt; i++)
        fscanf(file, "%u\n", &idle_epoch[i]);
    
    checkpoint_util_t *checkp = new checkpoint_util_t();
    checkp->map_uint32_uint32_from_file(vcpu_2_pcpu, file);
	
}

  
void heatrun_algorithm_t::silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai)
{
    FAIL;
}    
    
       
void heatrun_algorithm_t::write_checkpoint(FILE *file)
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
        
    }
    for (uint32 i = 0; i < num_threads; i++) 
        fprintf(file, "%llu\n", last_preempt[i]);
    for (uint32 i = 0; i < num_ctxt; i++)
        fprintf(file, "%u\n", idle_epoch[i]);
    
    checkpoint_util_t *checkp = new checkpoint_util_t();
    checkp->map_uint32_uint32_to_file(vcpu_2_pcpu, file);
}
    
