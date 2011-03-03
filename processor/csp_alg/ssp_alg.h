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
 
class ssp_algorithm_t : public tsp_algorithm_t {
    private:
        
       uint32 dynamic_provision_offset;
       uint32 next_hw_context_assignment;
       map <uint32, set<hw_context_t *> > syscall_context;
       void provision_syscall();
       hw_context_t * get_syscall_context(uint32 syscall, uint32 t_id, addr_t user_PC);
       void debug_os_waiter();
       void debug_user_waiter();
       mai_t *get_curr_syscall_thread(hw_context_t *ctxt);
       mai_t *matching_syscall_thread(hw_context_t *ctxt);
       bool syscall_coupling_exists(uint64 syscall_a, uint64 syscall_b);
       uint32 get_coupled_syscall(uint32 syscall);
       hw_context_t *get_assigned_syscall_context(uint32 syscall);
       group_counter_t *stat_successive_syscall;
           
    public:
    
       ssp_algorithm_t(string name, hw_context_t **hwc,
            thread_scheduler_t *t_sched, chip_t *_p,
            uint32 _num_threads, uint32 _num_ctxt);
            
       hw_context_t *find_ctxt_for_thread(mai_t *mai, 
            bool desire_user_ctxt, uint32 syscall);
       mai_t *find_thread_for_ctxt(hw_context_t *hwc, bool user_seq);
       
	
       void silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai);
       void read_checkpoint(FILE *f);
       void write_checkpoint(FILE *f);
        
    
};
