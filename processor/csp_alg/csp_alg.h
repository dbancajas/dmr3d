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
 * description:    Base class for CSP algorithms
 * initial author: Koushik Chakraborty 
 *
 */
 
#ifndef _CSP_ALGORITHM_H_
#define _CSP_ALGORITHM_H_

class csp_algorithm_t {
    
    protected:
    
        list<hw_context_t *> idle_os_ctxt;    // idle os sequencers
        list<hw_context_t *> idle_user_ctxt;  // idle user sequencers
        
        list<mai_t *> wait_for_os;   // threads waiting for an os core
        list<mai_t *> wait_for_user; // threads waiting for a user core
        
        string name;
        hw_context_t **hw_context;
        thread_scheduler_t *t_sched;
        chip_t *p;
        uint32 num_threads;
        uint32 num_ctxt;
        
        stats_t *stats;
        group_counter_t *stat_syscall_wait;
        group_counter_t *stat_user_wait;
        
    public:
    
        csp_algorithm_t (string _name, hw_context_t **_hwc, 
            thread_scheduler_t *t_sched, chip_t *_p, uint32 _n_thr, 
            uint32 _num_ctxt);
            
        virtual ~csp_algorithm_t();    
        virtual hw_context_t *find_ctxt_for_thread(mai_t *mai, 
            bool desire_user_ctxt, uint32 syscall) = 0;
        virtual mai_t *find_thread_for_ctxt(hw_context_t *hwc, 
            bool user_seq) = 0;
       
        virtual ts_yield_reason_t prioritize_yield_reason(ts_yield_reason_t whynew_val,
	   	    ts_yield_reason_t old_val);
			
        virtual bool thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
            ts_yield_reason_t why);
            
        virtual void silent_switch(sequencer_t *seq, uint32 ctxt, 
            mai_t *mai) = 0;
	    
	    virtual bool thread_yield(addr_t PC, sequencer_t *seq, uint32 tctxt);
        virtual void read_checkpoint(FILE *f) ;
        virtual void write_checkpoint(FILE *f) ;
        void print_stats();
        void clear_stats();
        void debug_user_ctxt();
        virtual sequencer_t *handle_interrupt_thread(uint32 thread_id) { return NULL;}
        virtual sequencer_t *handle_synchronous_interrupt(uint32 src,uint32 tgt) { return NULL;}
        
        virtual void enable_core(sequencer_t *seq) {}
        virtual void disable_core(sequencer_t *seq) {}
        virtual uint32 waiting_interrupt_threads() { return 0;}
        virtual void update_cycle_stats();
        virtual void ctrl_flow_redirection(uint32 cpu_id) {}
        virtual void migrate() {}
    
    
    
};

#endif /* CSP_ALGORITHM */
