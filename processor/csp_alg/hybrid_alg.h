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
 
class hybrid_algorithm_t : public csp_algorithm_t {
    private:
        
       uint32 t_per_c;
       map<uint32, uint32> vcpu_2_pcpu;
       map<uint32, uint32> vcpu_host_pcpu;
       int32 *last_stolen_vcpu;
       bool *disabled_core;
       tick_t   *last_preempt;
       void schedule(uint32 i );
       void initialize_vanilla_logical();
       void initialize_vanilla_gang();
       bool steal_previous_vcpu(hw_context_t *ctxt, mai_t *& rem_mai);
       mai_t *useful_local_thread(hw_context_t *ctxt);
       mai_t *force_steal_thread(hw_context_t *ctxt);
       
    public:
    
       hybrid_algorithm_t(string name, hw_context_t **hwc,
            thread_scheduler_t *t_sched, chip_t *_p,
            uint32 _num_threads, uint32 _num_ctxt);
            
       hw_context_t *find_ctxt_for_thread(mai_t *mai, 
            bool desire_user_ctxt, uint32 syscall);
       mai_t *find_thread_for_ctxt(hw_context_t *hwc, bool user_seq);
       ts_yield_reason_t prioritize_yield_reason(ts_yield_reason_t whynew_val,
	   	    ts_yield_reason_t old_val);
		
       bool thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
        ts_yield_reason_t why);
	
       void silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai); 
        
       void read_checkpoint(FILE *f);
       void write_checkpoint(FILE *f);
        
       mai_t *find_remote_thread(hw_context_t *ctxt, hw_context_t *&remote_ctxt);
       sequencer_t *handle_interrupt_thread(uint32 thread_id);
       
    
    
    
};
