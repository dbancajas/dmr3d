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
 * description: Base Algorithm to work with num_cpus > num_ctxt    
 * initial author: Koushik Chakraborty 
 *
*/
 
class tsp_algorithm_t : public csp_algorithm_t {
    protected:
        
       uint32 t_per_c;
       
       bool require_user_hop;
       bool require_os_hop;
       tick_t   last_user_mapping_switch;
       tick_t   last_os_mapping_switch;
       
       uint32   next_user_thread_switch;
       uint32   next_os_thread_switch;
       
       uint32 *preferred_user_ctxt;
       uint32 *preferred_os_ctxt;
       group_counter_t *stat_idle_syscall;
       tick_t *idle_cycle_start;
   
       
       hw_context_t *get_preferred_os_context(uint32 id);
       hw_context_t *get_preferred_user_context(uint32 id);
       uint32 get_num_hops(uint32);
       bool user_vcpu_waiting(sequencer_t *seq, uint32 ctxt);
       hw_context_t *get_smt_aware_ctxt(hw_context_t *ctxt);
       void silent_switch_finishup(sequencer_t *, hw_context_t *, uint32 ctxt);
       bool smt_thread_ctxt_match(mai_t *candidate, hw_context_t *, bool);
    
    
    public:
    
       tsp_algorithm_t(string name, hw_context_t **hwc,
            thread_scheduler_t *t_sched, chip_t *_p,
            uint32 _num_threads, uint32 _num_ctxt);
            
       virtual hw_context_t *find_ctxt_for_thread(mai_t *mai, 
            bool desire_user_ctxt, uint32 syscall);
       virtual mai_t *find_thread_for_ctxt(hw_context_t *hwc, bool user_seq);
       
       virtual bool thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
        ts_yield_reason_t why);
	
       virtual void silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai); 
        
       virtual void read_checkpoint(FILE *f);
       virtual void write_checkpoint(FILE *f);
        
     
    
    
    
    
};
