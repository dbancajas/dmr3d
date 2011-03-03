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
 * description:    thread scheduler for mapping threads to sequencers
 * initial author: Philip Wells 
 *
 */
 
#ifndef _THREAD_SCHEDULER_H_
#define _THREAD_SCHEDULER_H_

enum scheduling_policy_t {
    BASE_SCHEDULING_POLICY,
    THREAD_SCHEDULING_POLICY,
    SYSCALL_SCHEDULING_POLICY,
    EAGER_SCHEDULING_POLICY,
    MIGRATORY_SCHEDULING_POLICY,
    HYBRID_SCHEDULING_POLICY,
    OS_NOPAUSE_SCHEDULING_POLICY,
    VANILLA_GANG_SCHEDULING_POLICY,
    FUNCTION_SCHEDULING_POLICY,
    BANK_SCHEDULING_POLICY,
    HEATRUN_SCHEDULING_POLICY,
    SERVC_SCHEDULING_POLICY,
    MAX_SCHEDULING_POLICY
} ;


class hw_context_t {
    public:
        sequencer_t *seq;
        uint32      ctxt;
        list<mai_t *> wait_list;
        bool busy;
        uint32 curr_sys_call;
        addr_t user_PC;
        ts_yield_reason_t why;
        hw_context_t(sequencer_t *_s, uint32 _c, bool _b);
        void debug_wait_list();
};



class thread_scheduler_t
{
public:
	thread_scheduler_t(chip_t *chip);

	// Sequencer indicates that it may need to switch
	//   if it should, manage the switch
	bool thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, ts_yield_reason_t why);
    bool thread_yield(addr_t PC, sequencer_t *seq, uint32 t_ctxt);
	void ready_for_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai, ts_yield_reason_t why);
    ts_yield_reason_t prioritize_yield_reason(ts_yield_reason_t whynew_val,
	   	    ts_yield_reason_t old_val);
    
	bool is_user_ctxt(sequencer_t *_s, uint32 _c);
	bool is_os_ctxt(sequencer_t *_s, uint32 _c);
    hw_context_t *search_hw_context(sequencer_t *seq, uint32 _ct);
    
    void read_checkpoint(FILE *file);
    void write_checkpoint(FILE *file);
    
    uint32 dynamic_provision_offset;
	
    uint32 get_context_id(hw_context_t *);
    sequencer_t *handle_interrupt_thread(uint32 thread_id);
    sequencer_t *handle_synchronous_interrupt(uint32 src, uint32 target);
    void disable_core(sequencer_t *seq);
    void enable_core(sequencer_t *seq);
    uint32 waiting_interrupt_threads();
    void update_cycle_stats();
    void print_stats();
    void ctrl_flow_redirection(uint32 cpu_id);
    void migrate();
     
    
    

private:

	chip_t *chip;
	
	uint32 num_seq;
	uint32 num_threads;
    
    uint32 num_hw_ctxt;
    hw_context_t **hw_context;
    
    
    csp_algorithm_t *sched_alg;
	uint32 num_user;
    void debug_idle_context(list<hw_context_t *> l);
    void debug_user_ctxt();
    
    
};


#endif // _THREAD_SCHEDULER_H_
