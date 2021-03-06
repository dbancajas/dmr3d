/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: proc_stats.cc,v 1.3.2.25 2006/08/18 14:30:20 kchak Exp $");

#include "definitions.h"
#include "window.h"
#include "iwindow.h"
#include "sequencer.h"
#include "fu.h"
#include "isa.h"
#include "chip.h"
#include "eventq.h"
#include "dynamic.h"
#include "mai_instr.h"
#include "mai.h"
#include "ras.h"
#include "ctrl_flow.h"
#include "lsq.h"
#include "st_buffer.h"
#include "mem_xaction.h"
#include "v9_mem_xaction.h"
#include "transaction.h"
#include "wait_list.h"
#include "mem_hier_handle.h"
#include "proc_stats.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "config_extern.h"

#if 0

void 
STAT_INC (st_entry_t *ctr, 
	uint32 index, uint32 value_y) {

	if (ctr) 
		STAT_ADD (ctr, 1, index, value_y);
}

void 
STAT_ADD (st_entry_t *ctr, uint32 count, 
	uint32 index, uint32 value_y) {
	
	if (ctr) 
		ctr->inc_total (count, index, value_y);
}

void
STAT_DEC (st_entry_t *ctr, 
	uint32 index, uint32 value_y) {

	if (ctr) 
		STAT_SET (ctr, STAT_GET (ctr) - 1, index, value_y);
}

void 
STAT_SET (st_entry_t *ctr, uint32 value, 
	uint32 index, uint32 value_y) {

	if (ctr)
		ctr->set (value);
}

uint64
STAT_GET (st_entry_t *ctr) {

	if (ctr) 
		return (ctr->get_total ());
	else
		return 0;
}

#endif

double
sum (st_entry_t ** list) {
	return ( STAT_GET (list [0])  + STAT_GET (list [1]) );
}


proc_stats_t::proc_stats_t (string s) {
	zero_stats ();
	stats = new stats_t (s);

	initialize_stats ();
	stat_start_sim->set (static_cast <int64> (time (0)));
}

proc_stats_t::~proc_stats_t () {
	delete stats;
}

void 
proc_stats_t::zero_stats () {
	memset (this, 0, sizeof (*this));
}

// Stat manipulation functions
static double stat_mpki_fn(st_entry_t ** arr) {
	return (arr[0]->get_total()*1000.0/arr[1]->get_total());
}


void 
proc_stats_t::initialize_stats () {
	page_flts 	      =	stats->COUNTER_BASIC("page faults", "# of tlb traps");
	// sequencer

	stat_fetched          = stats->COUNTER_BASIC ("fetched", 
	"# of instructions fetched");

	stat_decoded          = stats->COUNTER_BASIC ( "decoded", 
	"# of instructions decoded");

	stat_renamed          = stats->COUNTER_BASIC ("renamed", 
	"# of instructions renamed");

	stat_executed         = stats->COUNTER_BASIC ("executed", 
	"# of instructions executed");

	stat_last_commits = stats->COUNTER_BASIC ("last_commits", 
	"# of instructions last committed");
	stat_last_commits->set_hidden ();

	stat_commits          = stats->COUNTER_BASIC ("committed", 
	"# of instructions committed");
    stat_user_commits     = stats->COUNTER_BASIC ("user_committed",
    "# of instructions commited by user");

	stat_fetch_ratio = stats->STAT_RATIO ("fetch_ratio", 
	"fetch ratio", 
	stat_fetched, stat_commits);

	stat_decode_ratio = stats->STAT_RATIO ("decode_ratio", 
	"decode ratio", 
	stat_decoded, stat_commits);

	stat_rename_ratio = stats->STAT_RATIO ("rename_ratio", 
	"rename ratio", 
	stat_renamed, stat_commits);

	stat_execute_ratio = stats->STAT_RATIO ("execute_ratio", 
	"execute ratio", 
	stat_executed, stat_commits);

	stat_cycles           = stats->COUNTER_BASIC ("cycles",  
	"# of cycles elapsed");

	stat_ipc              = stats->STAT_RATIO    ("ipc", 
	"instructions per cycle", 
	stat_commits, stat_cycles);
	
	stat_cpi              = stats->STAT_RATIO    ("cpi", 
	"cycles per instruction", 
	stat_cycles, stat_commits);

	// ----- loads ------
	stat_loads            = stats->COUNTER_BASIC ("loads", 
	"# of committed loads");

    stat_unsafe_loads     = stats->COUNTER_BASIC ("unsafe_lds", 
        " # of committed unsafe loads");
	string loads_name [] = {
		"load_byte", 
		"load_half", 
		"load_word", 
		"load_quad", 
		"load_block", 
	};

	string loads_desc [] = {
		"# of committed byte loads", 
		"# of committed half loads",
		"# of committed word loads",
		"# of committed quad loads",
		"# of committed block loads"
	};

	stat_loads_size       = stats->COUNTER_BREAKDOWN ("loads_size",
	"size of committed loads", 5, loads_name, loads_desc);

	stat_loads_ratio      = stats->STAT_RATIO    ("loads_ratio", 
	"ratio of committed loads", 
	stat_loads, stat_commits);

	stat_loads_exec       = stats->COUNTER_BASIC ("loads_exec", 
	"# of executed loads");

	// ----- stores ------
	stat_stores           = stats->COUNTER_BASIC ("stores", 
	"# of committed stores");

	string stores_name [] = {
		"store_byte", 
		"store_half", 
		"store_word", 
		"store_quad",
		"store_block", 
	};

	string stores_desc [] = {
		"# of committed byte stores", 
		"# of committed half stores",
		"# of committed word stores",
		"# of committed quad stores",
		"# of committed block stores"
	};

	stat_stores_size       = stats->COUNTER_BREAKDOWN ("stores_size",
	"size of committed stores", 5, stores_name, stores_desc);

	stat_stores_ratio     = stats->STAT_RATIO    ("stores_ratio", 
	"ratio of committed stores", 
	stat_stores, stat_commits);

	stat_stores_exec      = stats->COUNTER_BASIC ("stores_exec", 
	"# of executed stores");

	stat_stores_stbuf     = stats->COUNTER_BASIC ("stores_stbuf", 
	"# of committed stores sent to store buffer");

	stat_controls         = stats->COUNTER_BASIC ("controls", 
	"# of committed control instructions");

	stat_control_exec     = stats->COUNTER_BASIC ("control_exec", 
	"# of executed control instr");

	stat_direct_br        = stats->COUNTER_BASIC ("direct_br", 
	"# of committed direct branches");

	stat_indirect_br      = stats->COUNTER_BASIC ("indirect_br", 
	"# of committed indirect branches");

	stat_call_br          = stats->COUNTER_BASIC ("call_br", 
	"# of committed calls");

    stat_call_depth       = stats->HISTO_UNIFORM ("call_depth", "Call depth",
        64, 1, 0);
    stat_call_depth->set_terse_format ();
        
	stat_return = stats->COUNTER_BASIC ("return", 
	"# of committed returns");

	stat_cwp = stats->COUNTER_BASIC ("cwp", 
	"# of save and restore");

	stat_trap = stats->COUNTER_BASIC ("trap", 
	"# of explicit traps");

	stat_trap_ret = stats->COUNTER_BASIC ("trap_ret", 
	"# of retry and done");

	stat_ipb              = stats->STAT_RATIO    ("IPB", 
	"ratio of committed branches", 
	stat_controls, stat_commits);

	stat_prefetches       = stats->COUNTER_BASIC ("prefetches", 
	"# of committed prefetch instructions");

	stat_atomics          = stats->COUNTER_BASIC ("atomics", 
	"# of committed atomics");

	stat_sync1            = stats->COUNTER_BASIC ("sync1", 
	"# of sync1 instructions committed");

	stat_sync1_ratio      = stats->STAT_RATIO ("sync1_ratio", 
	"ratio of sync1 instructions",
	stat_sync1, stat_commits);

	stat_sync2            = stats->COUNTER_BASIC ("sync2", 
	"# of sync2 instructions committed");

	stat_sync2_ratio      = stats->STAT_RATIO ("sync2_ratio", 
	"ratio of sync2 instructions",
	stat_sync2, stat_commits);

	stat_iwindow_occupancy = stats->HISTO_UNIFORM ("iwindow_occupancy", 
	"cycles spent with each occupancy",
	g_conf_window_size + 1, 1, 0);	

	stat_iwindow_occupancy->set_terse_format ();

	stat_squash_exception = stats->HISTO_UNIFORM ("squash_exception", 
	"instructions squashed due to exception", 
	g_conf_window_size, 1, 0);	

	stat_squash_exception->set_terse_format ();

	stat_wait_for_ops     = stats->HISTO_UNIFORM ("wait_for_ops",
	"wait x seconds for its operands to be ready", 
	100, 1, 0);	

	stat_wait_for_ops->set_terse_format ();

    string wait_reason_desc [] = {
        "sync_pipeline_flush", "load_commit", "store_commit", 
        "non_spec_issue", "load_non_spec", "unsafe_load", "unsafe_store",
    "non_spec_store", "operand_ready", "max"};
    
    stat_wait_reason = stats->COUNTER_BREAKDOWN ("stat_wait_reason", 
        " Reason for pipe_wait", 10, wait_reason_desc, wait_reason_desc );
	// ctrl-flow
	stat_direct_predictions = stats->COUNTER_BASIC ("direct_predictions", 
	"# of direct predictions made");

    stat_direct_predictions_kernel = stats->COUNTER_BASIC ("direct_predictions_kernel", 
	"# of direct predictions made for kernels");

	stat_direct_mispredicts = stats->COUNTER_BASIC ("direct_mispredicts", 
	"# of direct mispredictions");
    
    stat_direct_mispredicts_kernel = stats->COUNTER_BASIC ("direct_mispredicts_kernel", 
	"# of direct mispredictions in kernel");


	stat_indirect_predictions = stats->COUNTER_BASIC ("indirect_predictions",
	"# of indirect predictions made");
    
    stat_indirect_predictions_kernel = stats->COUNTER_BASIC ("indirect_predictions_kernel",
	"# of indirect predictions made for kernels");

	stat_indirect_mispredicts = stats->COUNTER_BASIC ("indirect_mispredicts", 
	"# of indirect mispredictions");
    
    stat_indirect_mispredicts_kernel = stats->COUNTER_BASIC ("indirect_mispredicts_kernel", 
	"# of indirect mispredictions in kernel");


	stat_ras_predictions = stats->COUNTER_BASIC ("ras_predictions", 
	"# of ras predictions made");

    stat_ras_predictions_kernel = stats->COUNTER_BASIC ("ras_predictions_kernel", 
	"# of ras predictions made in kernel");

	stat_ras_mispredicts = stats->COUNTER_BASIC ("ras_mispredicts",
	"# of ras mispredictions");

    stat_ras_mispredicts_kernel = stats->COUNTER_BASIC ("ras_mispredicts_kernel",
	"# of ras mispredictions in kernel");
 
	stat_direct_missrate = stats->STAT_RATIO ("direct_missrate", 
	"direct predictor miss rate", 
	stat_direct_mispredicts, stat_direct_predictions);

	stat_indirect_missrate = stats->STAT_RATIO ("indirect_missrate", 
	"indirect predictor miss rate", 
	stat_indirect_mispredicts, stat_indirect_predictions);

	stat_ras_missrate = stats->STAT_RATIO ("ras_missrate", 
	"ras miss rate", 
	stat_ras_mispredicts, stat_ras_predictions);
    
    stat_speculative_head = stats->COUNTER_BASIC("speculative_head",
        "Instruction at the ROB head speculatve");
    
    string line_pred_desc [] = {
        "# of no_prefetch",
        "# of useful_prefetch",
        "#of useless_prefetch"};
        
    string line_pred_names [] = {
        "no_prefetch",
        "useful_prefetch",
        "useless_prefetch"};
            
        
    stat_line_prediction = stats->COUNTER_BREAKDOWN("line_prediction",
      "Line Prediction stats", 3, line_pred_names, line_pred_desc);    
	// st buffer
	stat_replays = stats->COUNTER_BASIC ("replays", 
	"# of loads replayed");

	stat_stbuf_occupancy = stats->HISTO_UNIFORM ("stbuf_occupancy",
	"histogram of number of cycles spent with each occupancy",
	g_conf_stbuf_size+1, 1, 0);

	stat_stbuf_occupancy->set_terse_format ();

	// lsq
	stat_ld_early_issue = stats->COUNTER_BASIC ("ld_early_issue", 
	"# of loads issued immediately");
	
	stat_ld_held_commit = stats->COUNTER_BASIC ("ld_held_commit", 
	"# of loads issued at commit");
	
	stat_initiate_loads_unresolved = stats->COUNTER_BASIC ("ld_unres_store", 
	"# of loads initiated with unresolved stores");

	stat_unresolved_store = stats->COUNTER_BASIC ("unresolved_store", 
	"# of loads blocked: unresolved store");
	
	stat_ld_unsafe_store = stats->COUNTER_BASIC ("ld_unsafe_store", 
	"# of loads blocked: unsafe store");
	
	stat_ld_unres_pref = stats->COUNTER_BASIC ("ld_unres_pref", 
	"# of loads prefetches issued for unresolved/safe values");
	
	stat_unres_squash = stats->COUNTER_BASIC ("ld_unres_squash",
	"# of loads squashed by earlier unresolved stores");
	
	stat_bypass_stb = stats->COUNTER_BASIC ("bypass_stb", 
	"# of bypasses from stbuffer");

	stat_bypass_lsq = stats->COUNTER_BASIC ("bypass_lsq", 
	"# of complete bypasses from lsq");

	st_entry_t *stat_bypass_list [] = {stat_bypass_lsq, stat_bypass_stb};
	
	stat_bypass = stats->STAT_PRINT ("bypass", 
	"# of bypasses", 2, stat_bypass_list, &sum, true);

	stat_bypass_conflict_stb = stats->COUNTER_BASIC ("bypass_conflict_stb", 
	"# of bypass conflicts from stbuffer");

	stat_bypass_conflict_lsq = stats->COUNTER_BASIC ("bypass_conflict_lsq", 
	"# of bypass conflicts from lsq");

	st_entry_t *stat_bypass_conflict_list [] = 
		{stat_bypass_conflict_lsq, stat_bypass_conflict_stb};

	stat_bypass_conflict = stats->STAT_PRINT ("bypass_conflict", 
	"# of bypass conflicts", 2, stat_bypass_conflict_list, &sum, true);

	stat_cache_squash = stats->COUNTER_BASIC ("squashed_by_cache",
	"# of squashes caused by cache");

	stat_cache_invalidation = stats->COUNTER_BASIC ("invalidation_from_cache",
	"# of invalidations from cache");
	
	// sim
	stat_start_sim        = stats->COUNTER_BASIC ("start_sim", 
	"simulation start time");
	stat_start_sim->set_hidden ();

	stat_elapsed_sim      = stats->COUNTER_BASIC ("elapsed_sim", 
	"simulation time elapsed");
	stat_elapsed_sim->set_hidden ();

	stat_total_commits      = stats->COUNTER_BASIC ("total_commits", 
	"total commits for this seq (all pstats)");
	stat_total_commits->set_hidden ();

	stat_total_cycles      = stats->COUNTER_BASIC ("total_cycles", 
	"total commits for this seq (all pstats)");
	stat_total_cycles->set_hidden ();

	stat_ips              = stats->STAT_RATIO    ("ips", 
	"instructions executed per second", 
	stat_total_commits, stat_elapsed_sim);

	stat_cps              = stats->STAT_RATIO    ("cps",
	"cycles per second",
	stat_total_cycles, stat_elapsed_sim);


	stat_supervisor_instr = stats->COUNTER_BASIC ("supervisor_instr", 
	"# of committed supervisor instructions");
	
	stat_supervisor_count = stats->COUNTER_BASIC ("supervisor_count", 
	"# of cons. supervisor instructions");

	stat_supervisor_count->set_hidden ();

	stat_supervisor_histo = stats->HISTO_UNIFORM ("supervisor_histo", 
	"# of instructions in supervisor mode",
	500, 100, 0);

	stat_supervisor_histo->set_void_zero_entries ();

	stat_supervisor_entry = stats->COUNTER_BASIC ("supervisor_entry", 
	"# of times entering into supervisor");

	stat_supervisor_avg = stats->STAT_RATIO ("supervisor_avg", 
	"average length of supervisor instructions", 
	stat_supervisor_instr, stat_supervisor_entry);


	stat_user_instr = stats->COUNTER_BASIC ("user_instr", 
	"# of committed user instructions");

	stat_user_count = stats->COUNTER_BASIC ("user_count", 
	"# of cons. user instructions");

	stat_user_count->set_hidden ();

	stat_user_histo = stats->HISTO_UNIFORM ("user_histo", 
	"# of instructions in user mode",
	500, 100, 0);

	stat_user_histo->set_void_zero_entries ();

	stat_user_entry = stats->COUNTER_BASIC ("user_entry", 
	"# of times entering into user");
	
	stat_user_avg = stats->STAT_RATIO ("user_avg", 
	"average length of user instructions", 
	stat_user_instr, stat_user_entry);

	// exceptions
	stat_fetch_exc        = stats->COUNTER_BASIC ("fetch_exceptions", 
	"# of exceptions during fetch");

	stat_exec_exc         = stats->COUNTER_BASIC ("execute_exceptions", 
	"# of exceptions during execute");

	stat_spec_exceptions = stats->COUNTER_BASIC ("spec_exceptions", 
	"# of speculative exceptions");

	stat_exceptions = stats->COUNTER_BASIC ("exceptions", 
	"# of exceptions");

	stat_interrupts = stats->COUNTER_BASIC ("interrupts", 
	"# of interrupts");

	stat_tl = stats->COUNTER_BASIC ("tl", "tl");
	stat_tl->set_hidden ();

	stat_maxtl = stats->COUNTER_BASIC ("maxtl", "maxtl");
	stat_maxtl->set_hidden ();

	STAT_SET (stat_maxtl, g_conf_trap_levels);

	stat_tt = new base_counter_t *[STAT_GET (stat_maxtl)];
	stat_trap_length = new base_counter_t *[STAT_GET (stat_maxtl)];

	for (uint32 i = 0; i < STAT_GET (stat_maxtl); i++) {
		char s [20];

		sprintf (s, "trap_length%d", i);
		stat_trap_length [i] = stats->COUNTER_BASIC (s, 
		"trap_length");
		stat_trap_length [i]->set_hidden ();

		sprintf (s, "tt%d", i);
		stat_tt [i] = stats->COUNTER_BASIC (s, "tt");
		stat_tt [i]->set_hidden ();
	}

	stat_trap_len = stats->COUNTER_GROUP ("trap_len",
	"trap length", 1024);

	stat_trap_len->set_void_zero_entries ();
	stat_trap_len->set_hex_index ();

	stat_trap_hit = stats->COUNTER_GROUP ("trap_hit", 
	"trap hits", 1024);
	
	stat_trap_hit->set_void_zero_entries ();
	stat_trap_hit->set_hex_index ();

	// stats
	stat_check_cycle = stats->COUNTER_BASIC ("check_cycle", "check_cycle");
	stat_check_cycle->set_hidden ();
	STAT_SET (stat_check_cycle, g_conf_check_freq);

	stat_recycle_list_size = stats->COUNTER_BASIC ("recycle_list_size", 
	"# of recycle list windows");

	stat_dead_list_size = stats->COUNTER_BASIC ("dead_list_size", 
	"# of dead list windows");

    stat_intermediate_checkpoint = stats->COUNTER_BASIC("Intermediate checkpoint", 
        "# of intermediate checkpoints during simulation");
    stat_checkpoint_penalty = stats->COUNTER_BASIC("Checkpoint Penalty", 
        "Cycles lost due to intermediate checkpoints");    
	
	stat_immu_hit = stats->COUNTER_BASIC ("immu_hits", "# of immu hits");
	stat_immu_miss = stats->COUNTER_BASIC ("immu_misses", "# of immu misses");
	stat_immu_bypass = stats->COUNTER_BASIC ("immu_bypasses", "# of immu bypasses");
	stat_immu_hw_fill = stats->COUNTER_BASIC ("immu_hw_fill", "# of immu HW fills");
	stat_dmmu_hit = stats->COUNTER_BASIC ("dmmu_hits", "# of dmmu hits");
	stat_dmmu_miss = stats->COUNTER_BASIC ("dmmu_misses", "# of dmmu misses");
	stat_dmmu_bypass = stats->COUNTER_BASIC ("dmmu_bypasses", "# of dmmu bypasses");
	stat_dmmu_hw_fill = stats->COUNTER_BASIC ("dmmu_hw_fill", "# of dmmu HW fills");

	stat_thread_switch_off = stats->HISTO_EXP2("thread_switch_off",
		"cycles spend switching threads off a seq", 10,2,1);
	stat_thread_switch_on = stats->HISTO_EXP2("thread_switch_on",
		"cycles spend switching threads onto a seq", 10,2,1);
	stat_thread_switch_start = stats->COUNTER_BASIC ("thread_switch_start",
		"thread_switch_start");
	stat_thread_switch_start->set_hidden();

	stat_idle_context_cycles = stats->COUNTER_BASIC ("idle_context_cycles",
		"cycles spend with idle context");

	stat_idle_loop_cycles = stats->COUNTER_BASIC ("idle_loop_cycles",
		"cycles spend in OS idle loop");
	stat_idle_loop_start = stats->COUNTER_BASIC ("idle_loop_start",
		"begin of OS idle loop");
	stat_idle_loop_start->set_hidden();


	stat_load_l1_miss = stats->COUNTER_BASIC ("load_l1_miss",
		"# of L1 load misses for committed instr");
	st_entry_t *arr_load_l1_mpki[] = { stat_load_l1_miss, stat_commits };
	stat_load_l1_mpki = stats->STAT_PRINT ("load_l1_mpki",
		"L1 load misses per 1000 committed instr",
		2, arr_load_l1_mpki, &stat_mpki_fn, false);

	stat_load_l2_miss = stats->COUNTER_BASIC ("load_l2_miss",
		"# of L2 load misses for committed instr");
	st_entry_t *arr_load_l2_mpki[] = { stat_load_l2_miss, stat_commits };
	stat_load_l2_mpki = stats->STAT_PRINT ("load_l2_mpki",
		"L2 load misses per 1000 committed instr",
		2, arr_load_l2_mpki, &stat_mpki_fn, false);

	stat_load_l3_miss = stats->COUNTER_BASIC ("load_l3_miss",
		"# of L3 load misses for committed instr");
	st_entry_t *arr_load_l3_mpki[] = { stat_load_l3_miss, stat_commits };
	stat_load_l3_mpki = stats->STAT_PRINT ("load_l3_mpki",
		"L3 load misses per 1000 committed instr",
		2, arr_load_l3_mpki, &stat_mpki_fn, false);

	stat_instr_l1_miss = stats->COUNTER_BASIC ("instr_l1_miss",
		"# of L1 instr misses for committed instr");
	st_entry_t *arr_instr_l1_mpki[] = { stat_instr_l1_miss, stat_commits };
	stat_instr_l1_mpki = stats->STAT_PRINT ("instr_l1_mpki",
		"L1 instr misses per 1000 committed instr",
		2, arr_instr_l1_mpki, &stat_mpki_fn, false);

	stat_instr_l2_miss = stats->COUNTER_BASIC ("instr_l2_miss",
		"# of L2 instr misses for committed instr");
	st_entry_t *arr_instr_l2_mpki[] = { stat_instr_l2_miss, stat_commits };
	stat_instr_l2_mpki = stats->STAT_PRINT ("instr_l2_mpki",
		"L2 instr misses per 1000 committed instr",
		2, arr_instr_l2_mpki, &stat_mpki_fn, false);

	stat_instr_l3_miss = stats->COUNTER_BASIC ("instr_l3_miss",
		"# of L3 instr misses for committed instr");
	st_entry_t *arr_instr_l3_mpki[] = { stat_instr_l3_miss, stat_commits };
	stat_instr_l3_mpki = stats->STAT_PRINT ("instr_l3_mpki",
		"L3 instr misses per 1000 committed instr",
		2, arr_instr_l3_mpki, &stat_mpki_fn, false);

	stat_load_request_time = stats->HISTO_EXP2("load_request_time",
		"distribution of load miss latencies", 10,1,1);
	stat_instr_request_time = stats->HISTO_EXP2("instr_request_time",
		"distribution of instr miss latencies", 10,1,1);
	
    stat_load_l1_miss_type = stats->COUNTER_GROUP("load_l1_miss_type",
        "Type of load l1 misses", MAX_KERNEL_ACCESS);
    stat_load_l2_miss_type = stats->COUNTER_GROUP("load_l2_miss_type",
        "Type of load l2 misses", MAX_KERNEL_ACCESS);    
    stat_load_coher_miss_type = stats->COUNTER_GROUP("load_coher_miss_type",
        "Type of coherence misses", MAX_KERNEL_ACCESS);    
    stat_syscall_load_type = stats->COUNTER_GROUP("syscall_load_type",
        "Type of loads under system call", MAX_KERNEL_ACCESS);
    string win_full_desc [] = {
		"atomic",
		"unsafe_st",
		"store",
		"unsafe_ld",
		"load",
		"other",
	};
    stat_iwindow_full_cause = stats->COUNTER_BREAKDOWN("iwindow_full_cause", 
        "Instruction Type causing iwindow-full", 6, win_full_desc, win_full_desc);    
	stat_iwindow_full_unsafe_st = stats->COUNTER_BREAKDOWN ("iwindow_full_unsafe_st", 
	"iwindow_full_unsafe_st opcode distribution", i_opcodes - i_add + 1, opcode_names,
	opcode_names);	
	stat_iwindow_full_unsafe_st->set_void_zero_entries ();
	
	/// Syscall stats
	stat_syscall_num = stats->COUNTER_BASIC ("syscall_num", "syscall_num");
	stat_syscall_num->set_hidden ();
	stat_syscall_start = stats->COUNTER_BASIC ("syscall_start", "syscall_start");
	stat_syscall_start->set_hidden ();

	stat_syscall_len = stats->COUNTER_GROUP ("syscall_len",
		"syscall length in cycles", 1024);
	stat_syscall_len->set_void_zero_entries ();

	stat_syscall_hit = stats->COUNTER_GROUP ("syscall_hit", 
		"syscall hits", 1024);
	stat_syscall_hit->set_void_zero_entries ();

	stat_syscall_imiss = stats->COUNTER_GROUP ("syscall_imiss",
		"syscall icache misses", 1024);
	stat_syscall_imiss->set_void_zero_entries ();

	stat_syscall_dmiss = stats->COUNTER_GROUP ("syscall_dmiss",
		"syscall dcache misses", 1024);
	stat_syscall_dmiss->set_void_zero_entries ();

	stat_syscall_len_histo = stats->HISTO_EXP2 ("syscall_len_histo", 
	"# of cycles execute in a system call", 10, 1, 128);

	stat_spins = stats->COUNTER_GROUP ("spins",
	"spins identified by PC", 100);
	stat_spins->set_void_zero_entries ();
    stat_spin_cycles = stats->COUNTER_BASIC("stat_spin_cycles", 
        "# of cycles spent spinning on recognized PCs");
        
    stat_spins_instr = stats->COUNTER_GROUP("stat_spins_instr", 
        "spin instructions on recognized PCs", 100);
    stat_spins_instr->set_void_zero_entries ();
    stat_detected_spins = stats->COUNTER_BASIC ("stats_detected_spins", "# of Detected spins from SHB");
    
    string yield_names [] = {
        "YIELD_NONE",
        "YIELD_RETRY",
        "YIELD_EXCEPTION",
        "YIELD_PAGE_FAULT",
        "YIELD_LONG_RUNNING",
        "YIELD_MUTEX_LOCKED",
        "YIELD_IDLE_ENTER",
        "YIELD_SYSCALL_SWITCH",
        "YIELD_PAUSED_THREAD_INTERRUPT",
        "YIELD_EXTRA_LONG_RUNNING",
        "YIELD_PROC_MODE_CHANGE", "YIELD_DISABLE_CORE", "YIELD_FUNCTION",
        "YIELD_HEATRUN_MIGRATION", "YIELD_NARROW_CORE"};
        
        
    stat_yield_reason = stats->COUNTER_BREAKDOWN("yield_reason",
      "Yield Reason stats", 15 , yield_names, yield_names);  
      
    string fetch_stall_desc [] = {
      "IWINDOW_FULL", "LDQ_FULL", "STQ_FULL", "FETCH_MISS", "EXCEPTION", "SYNC", "OTHER"};
    stat_fetch_cycles = stats->COUNTER_BASIC("stat_fetch_cycles", "# of cycles fetched instr");
    stat_useful_fetch_cycles = stats->COUNTER_BASIC("stat_useful_fetch_cycles", "# of cycles of useful fetch");
    stat_fetch_stall_breakdown = stats->COUNTER_BREAKDOWN("fetch_stall_breakdown", 
        "Fetch Stall Breakdown", 7, fetch_stall_desc, fetch_stall_desc);
    
    
    string pipe_stages_desc [] = {
	"PIPE_NONE",
	"PIPE_FETCH",
	"PIPE_FETCH_MEM_ACCESS",
	"PIPE_INSERT",
	"PIPE_PRE_DECODE",
	"PIPE_DECODE",
	"PIPE_RENAME", 
	"PIPE_WAIT",
	"PIPE_EXECUTE",
	"PIPE_EXECUTE_DONE",
	// mem access not started
	"PIPE_MEM_ACCESS",
	// mem access not started. should start when safe
	"PIPE_MEM_ACCESS_SAFE",
	// mem access aborted. should restart
	"PIPE_MEM_ACCESS_RETRY",
	// mem access in progress
	"PIPE_MEM_ACCESS_PROGRESS",
	// mem access in progress, stage 2
	"PIPE_MEM_ACCESS_PROGRESS_S2", 
	// mem access soon to complete,
	"PIPE_MEM_ACCESS_REEXEC",
	// mem access complete
	"PIPE_COMMIT",
	"PIPE_COMMIT_CONTINUE",
	// other ``interesting'' stages
	"PIPE_EXCEPTION",
	"PIPE_SPEC_EXCEPTION",
	"PIPE_REPLAY",
	// stages when dinstr not in iwindow
	"PIPE_ST_BUF_MEM_ACCESS_PROGRESS",
	"PIPE_END",
	"PIPE_STAGES" };

    stat_stage_histo = stats->COUNTER_BREAKDOWN ("stat_stage_histo", "time spent histo", PIPE_STAGES,
        pipe_stages_desc, pipe_stages_desc);
    
    stat_narrow_operand32 = stats->COUNTER_GROUP ("stat_narrow_operand32", "Narrow operand 32", 2);
    stat_narrow_operand24 = stats->COUNTER_GROUP ("stat_narrow_operand24", "Narrow operand 24", 2);
    stat_narrow_operand16 = stats->COUNTER_GROUP ("stat_narrow_operand16", "Narrow operand 16", 2);
    
    stat_allinput_16 = stats->COUNTER_BASIC ("stat_input_16", "All 16 bit input");
    stat_allinput_24 = stats->COUNTER_BASIC ("stat_input_24", "All 24 bit input");
    stat_allinput_32 = stats->COUNTER_BASIC ("stat_input_32", "All 32 bit input");
    
    stat_narrow_penalty = stats->COUNTER_BASIC("stat_narrow_penalty", "Additional Latency for narrow core");
    
    stat_except_opcode16 = stats->COUNTER_BREAKDOWN ("except_opcode16", " Excepting Opcodes for 16 bit",
        i_opcodes - i_add + 1, opcode_names, opcode_names);
    stat_except_opcode16->set_void_zero_entries ();
    stat_except_opcode24 = stats->COUNTER_BREAKDOWN ("except_opcode24", " Excepting Opcodes for 24 bit",
        i_opcodes - i_add + 1, opcode_names, opcode_names);
    stat_except_opcode24->set_void_zero_entries ();
    
    stat_except_opcode32 = stats->COUNTER_BREAKDOWN ("except_opcode32", " Excepting Opcodes for 32 bit",
        i_opcodes - i_add + 1, opcode_names, opcode_names);
    stat_except_opcode32->set_void_zero_entries ();
    
    stat_negative_inpoutp = stats->COUNTER_BASIC ("nagative_inpoutp", " Number of Negative Value");
    
    stat_unaccounted_instr = stats->COUNTER_BREAKDOWN ("stat_unaccounted_instr", "Unaccounted Instruction",
        i_opcodes - i_add + 1, opcode_names, opcode_names);
    stat_unaccounted_instr->set_void_zero_entries ();
    
    stat_8bit_runs = stats->HISTO_EXP ("stat_8bit_runs", "8 bit operand runs", 10, 4, 1);
    stat_16bit_runs = stats->HISTO_EXP ("stat_16bit_runs", "16 bit operand runs", 10, 4, 1);
    stat_24bit_runs = stats->HISTO_EXP ("stat_24bit_runs", "24 bit operand runs", 10, 4, 1);
    stat_negative_except16 = stats->COUNTER_BASIC ("stat_negative_except16", "16 bit exception for negative operands");
    stat_negative_except24 = stats->COUNTER_BASIC ("stat_negative_except24", "24 bit exception for negative operands");
    stat_negative_except32 = stats->COUNTER_BASIC ("stat_negative_except32", "32 bit exception for negative operands");
    
    stat_register_read_first = stats->HISTO_EXP2("stat_reg_read_first", "Register read first", 10, 1, 1);
    stat_register_write_first = stats->HISTO_EXP2("stat_reg_write_first", "Register read first", 10, 1, 1);
    int32 register_count = 96;
    string reg_names[register_count];
    conf_object_t *cpu = SIM_current_processor();
    for (int32 i = 0; i < register_count; i++)
    {
        const char *name = SIM_get_register_name(cpu, i);
        if (!name) SIM_clear_exception ();
        reg_names[i] = string(name != NULL ? name : "Unknown");
    }
    
    stat_register_read_group = stats->COUNTER_BREAKDOWN("stat_reg_read_group", "Register Read First Group", 
        register_count, reg_names, reg_names);
    stat_register_write_group = stats->COUNTER_BREAKDOWN("stat_reg_write_group", "Register Write First Group", 
        register_count, reg_names, reg_names);
    stat_register_read_group->set_void_zero_entries ();
    stat_register_write_group->set_void_zero_entries ();
    
    stat_inactive_period_length = stats->HISTO_EXP("stat_inactive_period_length", 
        "Periods of Inactivity in the processor cores", 10, 8, 1);
    stat_inactive_period_length->set_void_zero_entries ();

    
    if (g_conf_compressed_stats ) set_hidden_stats();
}


void 
proc_stats_t::iwindow_stats (iwindow_t * const iwindow) {
	uint32 val;

	uint32 size    = iwindow->get_window_size ();
	uint32 last_cr = iwindow->get_last_created ();
	uint32 last_co = iwindow->get_last_committed ();

	if (iwindow->empty ()) 
		val = 0; 
	else if (iwindow->full ()) 
		val = size;
	else if (last_cr < last_co) 
		val = size - (last_co - last_cr);
	else 
		val = last_cr - last_co;

	STAT_INC (stat_iwindow_occupancy, val);
}

void
proc_stats_t::st_buffer_stats (st_buffer_t * const st_buffer) {

	uint32 val;
	uint32 size    = st_buffer->get_window_size ();
	uint32 last_cr = st_buffer->get_last_created ();
	uint32 last_co = st_buffer->get_last_committed ();

	if (st_buffer->empty ()) 
		val = 0; 
	else if (st_buffer->full ()) 
		val = size;
	else if (last_cr < last_co) 
		val = size - (last_co - last_cr);
 	else 
		val = last_cr - last_co;

	STAT_INC (stat_stbuf_occupancy, val);
}


void 
proc_stats_t::set_hidden_stats() {
    
    stat_user_histo->set_hidden ();
    stat_supervisor_histo->set_hidden ();
    
    if (g_conf_operand_width_analysis == false) {
        stat_narrow_operand32->set_hidden ();
        stat_narrow_operand16->set_hidden ();
        stat_narrow_operand24->set_hidden ();
        stat_allinput_16->set_hidden ();
        stat_allinput_24->set_hidden ();
        stat_allinput_32->set_hidden ();
        
        stat_except_opcode32->set_hidden ();
        stat_except_opcode24->set_hidden ();
        stat_except_opcode16->set_hidden ();
        
        stat_unaccounted_instr->set_hidden ();
        stat_8bit_runs->set_hidden ();
        stat_16bit_runs->set_hidden ();
        stat_24bit_runs->set_hidden ();
        
    }
    
    stat_stage_histo->set_hidden ();
    stat_yield_reason->set_hidden ();
    stat_syscall_len->set_hidden ();
    stat_syscall_hit->set_hidden ();
    stat_syscall_imiss->set_hidden ();
    stat_syscall_dmiss->set_hidden ();
    stat_stores_size->set_hidden ();
    
    
}

void
proc_stats_t::print () {
	stats->print ();
}
