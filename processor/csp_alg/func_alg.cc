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
 * description: Function Scheduling Algorithm [General CSP]   
 * initial author: Koushik Chakraborty 
 *
*/

//  #include "simics/first.h"
// RCSID("$Id: func_alg.cc,v 1.1.2.1 2006/07/28 01:30:09 pwells Exp $");

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
#include "func_alg.h"

func_algorithm_t::func_algorithm_t(string name, hw_context_t **hwc,
   thread_scheduler_t *_t_sched, chip_t *_p, uint32 _num_thr, uint32 _num_ctxt) :
    csp_algorithm_t(name, hwc, _t_sched, _p, _num_thr, _num_ctxt)
{
    addr_t offset = g_conf_v9_binary ? 0x100000000ULL : 0;
    if (g_conf_function_map[0]) {
        uint32 index = 1;
        for (uint32 i = 0; i < g_conf_function_map[0]; i++)
        {
            index = i * 2 + 1;
            fmap[g_conf_function_map[index] + offset] = g_conf_function_map[index + 1];
        }
    }
    
    
    
    yield_state = new uint32[num_threads];
    bzero(yield_state, sizeof(uint32) * num_threads);
    
    for (uint32 i = 0; i < _num_ctxt; i++) {
        if (i < num_threads) {
            p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, 
                p->get_mai_object(i));
        } else {
            p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, 
                0);
            hw_context[i]->busy = false;
        }
	}
}


hw_context_t *func_algorithm_t::find_ctxt_for_thread(mai_t *thread, 
    bool desire_user_ctxt, uint32 syscall)
{
    sequencer_t *seq = p->get_sequencer_from_thread(thread->get_id());
    addr_t PC = seq->get_pc(0);
    map<addr_t, uint64>::iterator it = fmap.find(PC);
    map<addr_t, uint64>::iterator rit = rmap.find(PC);
    ASSERT(it != fmap.end() || rit != rmap.end());
    uint32 hwc_id;
    if (it != fmap.end()) {
        addr_t rPC = seq->get_return_top(0);
        hwc_id = it->second;
        hw_context[hwc_id]->seq->push_return_top(rPC, 0);
    }
    else 
        hwc_id = rit->second;
    return hw_context[hwc_id];
    
}

mai_t * func_algorithm_t::find_thread_for_ctxt(hw_context_t *ctxt, 
    bool user_ctxt)

{
    mai_t *ret = 0;
    ctxt->busy = false;
    return ret;
}     
       
void func_algorithm_t::read_checkpoint(FILE *f)
{
    csp_algorithm_t::read_checkpoint(f);
}
    
       
void func_algorithm_t::write_checkpoint(FILE *f)
{
    csp_algorithm_t::write_checkpoint(f);
}
    
    
void func_algorithm_t::silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai)
{
    FAIL;
}    

bool func_algorithm_t::thread_yield(addr_t PC, sequencer_t *seq, uint32 t_ctxt)
{
    map<addr_t, uint64>::iterator it = fmap.find(PC);
    uint32 hwc_id;
    
    if (it != fmap.end())
    {
        hwc_id = it->second;
        if (hw_context[hwc_id]->seq != seq ||
            hw_context[hwc_id]->ctxt != t_ctxt)
        {
            yield_state[seq->get_thread(t_ctxt)] = 1;
            addr_t rPC = seq->get_return_top(t_ctxt);
            // DEBUG_OUT("CALLED PC MATCH HERE %llx return %llx @ %llu curr_core %u dest_core %u\n", PC, rPC, 
            //     p->get_g_cycles(), seq->get_id(), hwc_id);
            rmap[rPC] = seq->get_id();
            return true;
        }
    } else {
        it = rmap.find(PC);
        if (it != rmap.end()) {
            hwc_id = it->second;
            if (hw_context[hwc_id]->seq != seq ||
                hw_context[hwc_id]->ctxt != t_ctxt)
            {
                // DEBUG_OUT("RETURN PC MATCH HERE %llx @ %llu curr_core %u dest_core %u\n",
                //     PC, p->get_g_cycles(), seq->get_id(), hwc_id);
                yield_state[seq->get_thread(t_ctxt)] = 1;
                return true;
            }
        }
    }
    
    return false;
    
}

void func_algorithm_t::debug_fmap()
{
    map<addr_t, uint64>::iterator it = fmap.begin();
    for (; it != fmap.end(); it++)
        cout << hex << it->first << "  ->  " << dec << it->second << endl;
}
    

bool func_algorithm_t::thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
            ts_yield_reason_t why)
{
    uint32 thread_id = mai->get_id();
    if (yield_state[thread_id]) {
        yield_state[thread_id] = (yield_state[thread_id] + 1) % 3;
        return true;
    }
    
    return false;
}
      
void func_algorithm_t::ctrl_flow_redirection(uint32 cpu_id)
{
    yield_state[cpu_id] = 0;
}

