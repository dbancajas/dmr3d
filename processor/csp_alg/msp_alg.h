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
 * description: Aggressive migration based algorithm for num_cpus > num_ctxt    
 * initial author: Koushik Chakraborty 
 *
*/
 
class msp_algorithm_t : public csp_algorithm_t {
    private:
        
    
    public:
    
       msp_algorithm_t(string name, hw_context_t **hwc,
            thread_scheduler_t *t_sched, chip_t *_p,
            uint32 _num_threads, uint32 _num_ctxt);
            
       hw_context_t *find_ctxt_for_thread(mai_t *mai, 
            bool desire_user_ctxt, uint32 syscall);
       mai_t *find_thread_for_ctxt(hw_context_t *hwc, bool user_seq);
       
       bool thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
        ts_yield_reason_t why);
	
       void silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai); 
        
       void read_checkpoint(FILE *f);
       void write_checkpoint(FILE *f);
        
     
    
    
    
    
};
