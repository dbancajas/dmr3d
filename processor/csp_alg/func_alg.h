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
 * description: Function Scheduling Algorithm (general CSP)     
 * initial author: Koushik Chakraborty 
 *
*/
 
#ifndef _FUNC_ALGORITHM_H
#define _FUNC_ALGORITHM_H

class func_algorithm_t : public csp_algorithm_t {
    private:
        
       uint32 t_per_c;
       map<addr_t, uint64> fmap;
       map<addr_t, uint64> rmap;
       void schedule(uint32 i );
       uint32 *yield_state;
       void debug_fmap();
       
    public:
    
       func_algorithm_t(string name, hw_context_t **hwc,
            thread_scheduler_t *t_sched, chip_t *_p,
            uint32 _num_threads, uint32 _num_ctxt);
            
       hw_context_t *find_ctxt_for_thread(mai_t *mai, 
            bool desire_user_ctxt, uint32 syscall);
       mai_t *find_thread_for_ctxt(hw_context_t *hwc, bool user_seq);
       bool thread_yield(addr_t PC, sequencer_t *seq, uint32 t_ctxt);
       bool thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
            ts_yield_reason_t why);
            
	
       void silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai); 
        
       void read_checkpoint(FILE *f);
       void write_checkpoint(FILE *f);
       void ctrl_flow_redirection(uint32 cpu_id);
       
        
    
};

#endif /* FUNC_ALGORITHM_H */
