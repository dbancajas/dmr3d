/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _PROC_STATS_H_
#define _PROC_STATS_H_

class proc_stats_t {

public:
	// stats
	stats_t *stats;

	// sequencer
	base_counter_t *stat_commits;
	base_counter_t *stat_last_commits;

    base_counter_t *stat_user_commits;
	base_counter_t *stat_cycles;
    ratio_print_t  *stat_ipc;

	base_counter_t *stat_stores;
	base_counter_t *stat_loads;
    base_counter_t *stat_unsafe_loads;

	ratio_print_t  *stat_stores_ratio;
	ratio_print_t  *stat_loads_ratio;

	base_counter_t *stat_stores_stbuf;

	base_counter_t *stat_controls;
	base_counter_t *stat_prefetches;
	base_counter_t *stat_atomics;
	
	base_counter_t *stat_fetched;
	base_counter_t *stat_decoded;
	base_counter_t *stat_renamed;
	base_counter_t *stat_executed;

	ratio_print_t  *stat_fetch_ratio;
	ratio_print_t  *stat_decode_ratio;
	ratio_print_t  *stat_rename_ratio;
	ratio_print_t  *stat_execute_ratio;
	
	base_counter_t *stat_loads_exec;
	base_counter_t *stat_stores_exec;
	base_counter_t *stat_control_exec;
	
	base_counter_t *stat_direct_br;
	base_counter_t *stat_indirect_br;
	base_counter_t *stat_call_br;
    histo_1d_t      *stat_call_depth;
	base_counter_t *stat_return;
	base_counter_t *stat_cwp;
	base_counter_t *stat_trap;
	base_counter_t *stat_trap_ret;

	breakdown_counter_t *stat_loads_size;
	breakdown_counter_t *stat_stores_size;
	
	base_counter_t *stat_sync1;
	ratio_print_t  *stat_sync1_ratio;

	base_counter_t *stat_sync2;
	ratio_print_t  *stat_sync2_ratio;
	
	ratio_print_t  *stat_ipb;
	ratio_print_t  *stat_cpi;

	histo_1d_t     *stat_iwindow_occupancy;
	histo_1d_t     *stat_squash_exception;
	histo_1d_t     *stat_wait_for_ops;
    
    breakdown_counter_t *stat_wait_reason;

	base_counter_t *stat_initiate_loads_unresolved;
	
	// mai

	// stbuffer
	base_counter_t *stat_replays;

	histo_1d_t     *stat_stbuf_occupancy;

	// ctrl-flow
	base_counter_t *stat_direct_predictions;
    base_counter_t *stat_direct_predictions_kernel;
	base_counter_t *stat_direct_mispredicts;
    base_counter_t *stat_direct_mispredicts_kernel;
	base_counter_t *stat_indirect_predictions;
    base_counter_t *stat_indirect_predictions_kernel;
	base_counter_t *stat_indirect_mispredicts;
    base_counter_t *stat_indirect_mispredicts_kernel;
	base_counter_t *stat_ras_predictions;
    base_counter_t *stat_ras_predictions_kernel;
	base_counter_t *stat_ras_mispredicts;
    base_counter_t *stat_ras_mispredicts_kernel;
    base_counter_t *stat_speculative_head;
    
	ratio_print_t  *stat_direct_missrate;
	ratio_print_t  *stat_indirect_missrate;
	ratio_print_t  *stat_ras_missrate;
    
	
	// lsq
	base_counter_t *stat_ld_early_issue;
	base_counter_t *stat_ld_held_commit;
	base_counter_t *stat_unresolved_store;
	base_counter_t *stat_ld_unsafe_store;
	base_counter_t *stat_unres_squash;
   	base_counter_t *stat_cache_invalidation;
   	base_counter_t *stat_cache_squash;
	base_counter_t *stat_ld_unres_pref;

	// sim
	base_counter_t *stat_start_sim;
	base_counter_t *stat_elapsed_sim;
	base_counter_t *stat_total_commits;
	base_counter_t *stat_total_cycles;
	ratio_print_t  *stat_ips;
	ratio_print_t  *stat_cps;

	//
	base_counter_t *stat_supervisor_instr;
	base_counter_t *stat_supervisor_count;
	histo_1d_t     *stat_supervisor_histo;

	base_counter_t *stat_supervisor_entry;
	ratio_print_t  *stat_supervisor_avg;

	//
	base_counter_t *stat_user_instr;
	base_counter_t *stat_user_count;
	histo_1d_t     *stat_user_histo;

	base_counter_t *stat_user_entry;
	ratio_print_t  *stat_user_avg;

	// 
	base_counter_t *stat_bypass_stb;
	base_counter_t *stat_bypass_lsq;
	st_print_t   *stat_bypass;

	base_counter_t *stat_bypass_conflict_stb;
	base_counter_t *stat_bypass_conflict_lsq;
	st_print_t *stat_bypass_conflict;

	base_counter_t *stat_fetch_exc;
	base_counter_t *stat_exec_exc;
	base_counter_t *stat_spec_exceptions;
	base_counter_t *stat_exceptions;

	base_counter_t *stat_interrupts;

	base_counter_t  *stat_tl;
	base_counter_t  *stat_maxtl;

	base_counter_t  **stat_tt;
	base_counter_t  **stat_trap_length;

	group_counter_t *stat_trap_len;
	group_counter_t *stat_trap_hit;

	// 
	base_counter_t  *stat_check_cycle;

	base_counter_t  *stat_recycle_list_size;
	base_counter_t  *stat_dead_list_size;
	
    base_counter_t *stat_intermediate_checkpoint;
    base_counter_t *stat_checkpoint_penalty;
	
	base_counter_t  *stat_immu_hit;
	base_counter_t  *stat_immu_miss;
	base_counter_t  *stat_immu_bypass;
	base_counter_t  *stat_immu_hw_fill;

	base_counter_t  *stat_dmmu_hit;
	base_counter_t  *stat_dmmu_miss;
	base_counter_t  *stat_dmmu_bypass;
	base_counter_t  *stat_dmmu_hw_fill;
    breakdown_counter_t *stat_line_prediction;

	histo_1d_t      *stat_thread_switch_off;
	histo_1d_t      *stat_thread_switch_on;
	base_counter_t  *stat_thread_switch_start;

	base_counter_t  *stat_idle_context_cycles;

	base_counter_t  *stat_idle_loop_start;
	base_counter_t  *stat_idle_loop_cycles;

	base_counter_t  *stat_instr_l1_miss;
	st_print_t      *stat_instr_l1_mpki;
	base_counter_t  *stat_instr_l2_miss;
	st_print_t      *stat_instr_l2_mpki;
	base_counter_t  *stat_instr_l3_miss;
	st_print_t      *stat_instr_l3_mpki;

	base_counter_t  *stat_load_l1_miss;
	st_print_t      *stat_load_l1_mpki;
	base_counter_t  *stat_load_l2_miss;
	st_print_t      *stat_load_l2_mpki;
	base_counter_t  *stat_load_l3_miss;
	st_print_t      *stat_load_l3_mpki;

    group_counter_t *stat_load_l1_miss_type;
    group_counter_t *stat_load_l2_miss_type;
    group_counter_t *stat_load_coher_miss_type;
    group_counter_t *stat_syscall_load_type;
    
    group_counter_t *stat_iwindow_full_cause;
    group_counter_t *stat_iwindow_full_unsafe_st;
    
	histo_1d_t *stat_instr_request_time;
	histo_1d_t *stat_load_request_time;

	base_counter_t  *stat_syscall_num;
	base_counter_t  *stat_syscall_start;
	group_counter_t *stat_syscall_hit;
	group_counter_t *stat_syscall_len;
	group_counter_t *stat_syscall_imiss;
	group_counter_t *stat_syscall_dmiss;
	histo_1d_t      *stat_syscall_len_histo;
    
    breakdown_counter_t *stat_yield_reason;
	group_counter_t *stat_spins;
    group_counter_t *stat_spins_instr;
	base_counter_t *stat_spin_cycles;
    base_counter_t *stat_detected_spins;
    
    base_counter_t *stat_fetch_cycles;
	breakdown_counter_t *stat_fetch_stall_breakdown;
    base_counter_t *stat_useful_fetch_cycles;
    breakdown_counter_t *stat_stage_histo;
    group_counter_t *stat_narrow_operand32;
    group_counter_t *stat_narrow_operand16;
    group_counter_t *stat_narrow_operand24;
    
    breakdown_counter_t *stat_except_opcode32;
    breakdown_counter_t *stat_except_opcode16;
    breakdown_counter_t *stat_except_opcode24;
    
    breakdown_counter_t *stat_except32_PC;
    breakdown_counter_t *stat_except24_PC;
    breakdown_counter_t *stat_except16_PC;
    
    
    base_counter_t      *stat_negative_inpoutp;
    base_counter_t      *stat_allinput_16;
    base_counter_t      *stat_allinput_24;
    base_counter_t      *stat_allinput_32;
    breakdown_counter_t *stat_unaccounted_instr;
    breakdown_counter_t *stat_function_execution;
    breakdown_counter_t *stat_float_exception;
    histo_1d_t          *stat_16bit_runs;
    histo_1d_t          *stat_8bit_runs;
    histo_1d_t          *stat_24bit_runs;
    
    base_counter_t      *stat_negative_except16;
    base_counter_t      *stat_negative_except24;
    base_counter_t      *stat_negative_except32;
    
    histo_1d_t      *stat_register_read_first;
    histo_1d_t      *stat_register_write_first;
    histo_1d_t      *stat_inactive_period_length;
    breakdown_counter_t *stat_register_read_group;
    breakdown_counter_t *stat_register_write_group;
    
    base_counter_t      *stat_narrow_penalty;
    
    
    void set_hidden_stats ();
    
public:
	proc_stats_t (string s);
	~proc_stats_t ();
	void zero_stats ();
	void initialize_stats ();
	void st_buffer_stats (st_buffer_t * const st_buffer);
	void iwindow_stats (iwindow_t * const iwindow);
	void print ();
	
};


#endif
