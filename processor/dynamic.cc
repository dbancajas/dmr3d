/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: dynamic.cc,v 1.13.2.48 2006/08/18 14:30:19 kchak Exp $");

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
#include "shb.h"

#include "profiles.h"
#ifdef POWER_COMPILE
#include "power_profile.h"
#include "core_power.h"
#endif
// mai decode related static const strings

const string dynamic_instr_t::cc0_s = "cc0";
const string dynamic_instr_t::cc2_s = "cc2";
const string dynamic_instr_t::cond_s = "cond";
const string dynamic_instr_t::fcn_s = "fcn";
const string dynamic_instr_t::i_s = "i";
const string dynamic_instr_t::imm22_s = "imm22";
const string dynamic_instr_t::op_s = "op";
const string dynamic_instr_t::op2_s = "op2";
const string dynamic_instr_t::op3_s = "op3";
const string dynamic_instr_t::opf_s = "opf";
const string dynamic_instr_t::rcond_s = "rcond";
const string dynamic_instr_t::rd_s = "rd";
const string dynamic_instr_t::rs1_s = "rs1";
const string dynamic_instr_t::rs2_s = "rs2";
const string dynamic_instr_t::simm13_s = "simm13";
const string dynamic_instr_t::x_s = "x";
const string dynamic_instr_t::a_s = "a";
const string dynamic_instr_t::d16lo_s = "d16lo";
const string dynamic_instr_t::d16hi_s = "d16hi";
const string dynamic_instr_t::disp22_s = "disp22";
const string dynamic_instr_t::disp19_s = "disp19";
const string dynamic_instr_t::disp30_s = "disp30";
const string dynamic_instr_t::imm_asi_s = "imm_asi";


dynamic_instr_t::dynamic_instr_t (sequencer_t * const _seq, uint8 _tid) : 
	seq (_seq), 
	p (seq->get_chip ()),
	ctrl_flow (seq->get_ctrl_flow (_tid)),
	iwindow (seq->get_iwindow ()),
	eventq (seq->get_eventq ()),
    tid(_tid)
{

	gpc = new gpc_t (seq->get_pc (tid), seq->get_npc (tid));
	mai_instr = new mai_instruction_t (seq->get_mai_object (tid), this);
	seq_id = sequencer_t::generate_seq_id ();

	pred_o_gpc = new gpc_t ();
	actual_o_gpc = new gpc_t ();
	bpred_state = new bpred_state_t ();

	dead_wl = new wait_list_t (g_conf_wait_list_size);
	issue_wl = new wait_list_t (g_conf_wait_list_size);

    reg_ids = new int32[96];
	clear ();
}

void 
dynamic_instr_t::recycle (uint8 _tid) {
	delete gpc;
	delete mai_instr;

	// recycle this also
	delete mem_xaction;

    tid = _tid;
	gpc = new gpc_t (seq->get_pc (tid), seq->get_npc (tid));
    mai_instr = new mai_instruction_t (seq->get_mai_object (tid), this);
	seq_id = sequencer_t::generate_seq_id ();
    ctrl_flow = seq->get_ctrl_flow(_tid);

	pred_o_gpc->clear ();
	actual_o_gpc->clear ();
	bpred_state->clear ();

    if (prev_d_instr && prev_d_instr->get_next_d_instr () == this)
        prev_d_instr->set_next_d_instr (0);
    if (next_d_instr && next_d_instr->get_prev_d_instr () == this)
        next_d_instr->set_prev_d_instr (0);
    
	ASSERT (dead_wl->empty ());
	ASSERT (issue_wl->empty ());

	clear ();
    
    
}
	
void 
dynamic_instr_t::clear () {

	seq->set_fu_status (tid, FETCH_GPC_UNKNOWN);

	type = IT_TYPES;
	ainfo = AI_NONE;
	branch_type = BRANCH_TYPES;
	opcode = i_opcodes;
	futype = FU_TYPES;
	fu = 0;

	offset = 0;
	annul = false;

	pstage = PIPE_NONE;
	prev_pstage = PIPE_NONE;
	pstage = PIPE_NONE;
	set_except_pipe_stage (PIPE_NONE);
	slot = -1;
	lsq_slot = -1;
	lsq_entry = LSQ_ENTRY_NONE;
	stq_ptr = ~0;

	st_buffer_entry = ST_BUFFER_NONE;

	event = EVENT_EVENTS;

	done = false;
	sync_instr = SYNC_TYPES;
	outstanding = 0;
	squashed = false;

	taken = true;

	mem_xaction = 0;

	ops_ready = 0;
	enter_wait = 0;

	fetch_entry = FETCH_ENTRY_CREATE;
	imm_asi = -1;

	prev_d_instr = 0;
	next_d_instr = 0;

	next_event = 0;
	tail_event = 0;

	num_srcs = num_dests = 0;
    initiate_fetch = false;
    execute_at_head = false;

	for (int32 k = 0; k < RI_NUMBER; k ++) 
		l_reg [k] = REG_NA;
	

	for (int32 k = 0; k < RN_NUMBER; k ++) {
		reg_sz [k] = 1;  // single-word register
		reg_fp [k] = false;
	}
	
	fetch_miss = 0;
	load_miss = 0;
	fetch_latency = 0;
	load_latency = 0;
	
	sync_shadow = 0;
	page_fault = 0;
    fetch_cycle = 0;
    stage_entry_cycle = p->get_g_cycles ();
    imm_val = 0;
    shift_cnt = 0;
    cmp_imm = false;
    
    core_power_profile = seq->get_core_power_profile();
}

dynamic_instr_t::~dynamic_instr_t (void) {
	delete mai_instr;

	delete gpc;
	delete pred_o_gpc;
	delete actual_o_gpc;
	delete bpred_state;

    delete mem_xaction;
	
	delete dead_wl;
	delete issue_wl;
}


const uint64
dynamic_instr_t::priority (void) {
	return seq_id;
}

mai_instruction_t* 
dynamic_instr_t::get_mai_instruction () {
	return mai_instr;
}

void 
dynamic_instr_t::insert () {
	if (g_conf_trace_pipe)
		print ("insert:");

	mai_instr->insert ();
 	event = iwindow->insert (this);

	ASSERT (event == EVENT_OK);
}

void 
dynamic_instr_t::fetch () {
	if (g_conf_trace_pipe)
		print ("fetch:");

	proc_stats_t *pstats = seq->get_pstats (tid);
	proc_stats_t *tstats = seq->get_tstats (tid);
	
	sequencer_t::icache_d_instr = this;
	event = mai_instr->fetch ();
	sequencer_t::icache_d_instr = 0;
    fetch_cycle = p->get_g_cycles();

	switch (event) {

	case EVENT_SPEC_EXCEPTION:
		STAT_INC (pstats->stat_spec_exceptions);
		STAT_INC (tstats->stat_spec_exceptions);

		preparefor_exception ();
		break;

	case EVENT_EXCEPTION: 
		STAT_INC (pstats->stat_fetch_exc);
		STAT_INC (pstats->stat_exceptions);
		STAT_INC (tstats->stat_fetch_exc);
		STAT_INC (tstats->stat_exceptions);
		
		preparefor_exception ();
		break;
	
	case EVENT_STALL: 
		set_pipe_stage (PIPE_FETCH_MEM_ACCESS);

		mem_xaction->setup_regular_access ();
        if (initiate_fetch) {
            initiate_mem_hier ();
        } else {
            set_outstanding (PENDING_MEM_HIER);
        }

		break;
	
	case EVENT_OK:
		STAT_INC (pstats->stat_fetched);
		STAT_INC (tstats->stat_fetched);
		
		set_pipe_stage (PIPE_PRE_DECODE);
		pre_decode ();

		break;

	default:	
		FAIL;
	
	}
}

void dynamic_instr_t::pre_decode () {
	instr_event_t e = EVENT_OK;
    
    if (prev_d_instr && prev_d_instr->get_tid () != tid) {
        WARNING;
        set_prev_d_instr (0);
    }

	set_pipe_stage (PIPE_DECODE);
  	event = mai_instr->decode ();

	if (event != EVENT_OK) {
		eventq->insert (this, g_conf_fetch_stages);
		return;
	}

	// EVENT_OK
 	sequencer_t::mmu_d_instr = this;
	bool b = decode_instruction_info ();
	sequencer_t::mmu_d_instr = NULL;

#if 0
	if (opcode == i_impdep1) // faligndata, etc 
		mai_instr->get_reg (l_reg, num_srcs, num_dests);
   	else mai_instr->test_reg (l_reg, num_srcs, num_dests);
#endif

#if 0
//	ASSERT (opcode != i_opcodes);
	ASSERT (type != IT_TYPES);
	ASSERT (futype != FU_TYPES);
	if (type == IT_CONTROL) ASSERT (branch_type != BRANCH_TYPES);
#endif

	if (!b || get_opcode() == i_trap) {
		//WARNING;

		kill_all ();
		seq->set_fu_status (tid, FETCH_STALL_UNTIL_EMPTY);
		return;
	}

	fu = seq->get_fu_resource (futype);

	set_sync (mai_instr->sync_type ());
//	if (get_opcode () == i_trap) set_sync (SYNC_TWO);
//	if (get_opcode () == i_ldxa) set_sync (SYNC_TWO);
//	if (get_opcode () == i_rdpr) set_sync (SYNC_TWO);
// 	if (get_opcode () == i_rd) set_sync (SYNC_TWO);

//	if (opcode == i_lddfa && imm_asi == -1) set_sync (SYNC_TWO);

	if (!g_conf_sync2_runahead && get_sync () == SYNC_TWO) {
		if (get_next_d_instr ()) 
			get_next_d_instr ()->kill_all ();
		seq->set_fu_status (tid, FETCH_STALL_UNTIL_EMPTY);
	}
	if (g_conf_sync2_runahead &&
		get_prev_d_instr() && (get_prev_d_instr()->get_sync() == SYNC_TWO ||
		get_prev_d_instr()->is_sync_shadow()))
	{
        ASSERT(get_cpu_id() == prev_d_instr->get_cpu_id());
		set_sync_shadow();
	}
	
	lsq_t *lsq = seq->get_lsq ();
	if (is_load ()) {
		e = lsq->insert_load (this);
		if (e != EVENT_OK) seq->set_fu_status (tid, FETCH_LDQ_FULL);
	} else if (is_store ()) {
		e = lsq->insert_store (this);
		if (e != EVENT_OK) seq->set_fu_status (tid, FETCH_STQ_FULL);
	}

	if (e == EVENT_OK) {
		ctrl_flow->advance_gpc (this);
		seq->reset_fu_status (tid, FETCH_GPC_UNKNOWN);

		eventq->insert (this, g_conf_fetch_stages);
	} else {
		ASSERT ( seq->get_fu_status (tid, FETCH_LDQ_FULL) || 
			seq->get_fu_status (tid, FETCH_STQ_FULL) );

		kill_all ();
	}
    
    
}

void 
dynamic_instr_t::decode () {
	if (g_conf_trace_pipe)
		print ("        decode:");

	proc_stats_t *pstats = seq->get_pstats (tid);
	proc_stats_t *tstats = seq->get_tstats (tid);

	switch (event) {

	case EVENT_SPEC_EXCEPTION:
		STAT_INC (pstats->stat_spec_exceptions);
		STAT_INC (tstats->stat_spec_exceptions);
		
		preparefor_exception (); 
		break;

	case EVENT_EXCEPTION:
		STAT_INC (pstats->stat_exceptions);
		STAT_INC (tstats->stat_exceptions);

		preparefor_exception ();
		break;

	case EVENT_OK:
		STAT_INC (pstats->stat_decoded);
		STAT_INC (tstats->stat_decoded);
	
		set_pipe_stage (PIPE_RENAME);
		eventq->insert (this, g_conf_decode_stages + g_conf_rename_stages);
		break;

	case EVENT_UNRESOLVED_DEP:
		set_pipe_stage (PIPE_PRE_DECODE);
		eventq->insert (this, 1);
		break;

	default:
		FAIL;

	}
    
    
}

void
dynamic_instr_t::rename () {
	if (g_conf_trace_pipe)
		print ("                rename:");

	proc_stats_t *pstats = seq->get_pstats (tid);
	proc_stats_t *tstats = seq->get_tstats (tid);

	STAT_INC (pstats->stat_renamed);
	STAT_INC (tstats->stat_renamed);
	
	enter_wait = p->get_g_cycles ();

	set_pipe_stage (PIPE_WAIT);
//	eventq->insert (this, g_conf_rename_stages);
}

void
dynamic_instr_t::execute () {
	if (g_conf_trace_pipe)
		print ("                        execute:");

	proc_stats_t *pstats = seq->get_pstats (tid);
	proc_stats_t *tstats = seq->get_tstats (tid);

	uint32 stall_time;
	bool   correct;
	
	// Temp...
	if ((ainfo & AI_ALT_SPACE) == AI_ALT_SPACE) {
		// Input reg wasn't available at decode, try again
		if (imm_asi == -1)
			imm_asi = mai_instr->read_input_asi ();
	}

    
    if (get_sync () && speculative ()) {
        WARNING;
        kill_all ();
        seq->reset_fu_status (tid, FETCH_STALL_UNTIL_EMPTY);
        return;
    }
    
    
	sequencer_t::icache_d_instr = this;
	event = mai_instr->execute ();
	sequencer_t::icache_d_instr = 0;

	pop_issue_wait_list ();

	switch (event) {

	case EVENT_STALL:	
		ASSERT (is_load () || is_store ());

		if (get_lsq_entry () == LSQ_ENTRY_CREATE) {
			fu->return_fu ();

			set_pipe_stage (PIPE_WAIT);
//			ASSERT (mai_instr->stall_cycles () == 1);
			
			break;
		}

		STAT_INC (pstats->stat_executed);
		STAT_INC (tstats->stat_executed);
		if (is_load ()) {
			STAT_INC (pstats->stat_loads_exec);
			STAT_INC (tstats->stat_loads_exec);
		}
		if (is_store ()) {
			STAT_INC (pstats->stat_stores_exec);
			STAT_INC (tstats->stat_stores_exec);
		}
        if (is_load() && iwindow->head() == this) {
            execute_at_head = true;
        }
		stall_time = mai_instr->stall_cycles ();
		ASSERT (stall_time);

		fu->return_fu ();
		set_pipe_stage (PIPE_MEM_ACCESS);

		if (is_load () && !g_conf_initiate_loads_commit) ld_mem_access ();
		if (is_store () && g_conf_store_prefetch) st_prefetch ();
        if (g_conf_sync2_runahead && sync_instr == SYNC_TWO)
        {
            if (get_next_d_instr ()) get_next_d_instr ()->kill_all ();
            seq->set_fu_status (tid, FETCH_STALL_UNTIL_EMPTY);
        }
            

		break;
	
	case EVENT_OK:
	case EVENT_SYNC:
		STAT_INC (pstats->stat_executed);
		STAT_INC (tstats->stat_executed);
		if (type == IT_CONTROL) {
			STAT_INC (pstats->stat_control_exec);
			STAT_INC (tstats->stat_control_exec);
		}
		
        //if (is_load()) ASSERT(lsq_entry != LSQ_ENTRY_CREATE);
        if (g_conf_operand_width_analysis)
            analyze_operand_width();
        if (g_conf_analyze_register_dependency)
            mai_instr->analyze_register_dependency();
            
        fu->return_fu ();
		set_pipe_stage (PIPE_COMMIT);
        
		correct = ctrl_flow->check_advanced_gpc (this);

		if (get_sync() == SYNC_TWO) {
			if (g_conf_sync2_runahead) {
                seq->set_fu_status (tid, FETCH_STALL_UNTIL_EMPTY);
				if (event == EVENT_SYNC) {
                    if (get_next_d_instr ()) 
                        get_next_d_instr ()->kill_all ();
                    fu->get_fu();
					return execute();
//					event = mai_instr->execute ();
//					ASSERT_WARN(event == EVENT_OK);
				} 
                else {
                    ASSERT(retire_ready ());
                    //ASSERT(mai_instr->get_mai_object()->get_npc () == actual_o_gpc->pc);
                    ctrl_flow->fix_spec_state(this);
                }
			} else {
                // Is this a SIMICS bug ? ...
                // the instruction returning speculative ... inspite of
                // being only one in the window ... or this is our bug??
                
                ASSERT(!get_next_d_instr () || 
					get_next_d_instr ()->is_squashed());
                // instruction_id_t  instr_id = mai_instr->get_instr_id();
                // if (event == EVENT_SYNC && SIM_instruction_speculative(instr_id)) {
                //     SIM_instruction_force_correct(instr_id);
                //     fu->get_fu();
                //     event = mai_instr->execute ();
                // }
                if (event == EVENT_SYNC) {
                    WARNING;
                    ASSERT (!correct);
                    kill_all ();
                    seq->reset_fu_status (tid, FETCH_STALL_UNTIL_EMPTY);
                    break;
                }
			}
		}
		
		if (!correct) {
			if (get_next_d_instr ()) 
				get_next_d_instr ()->kill_all ();
            else if (event == EVENT_OK) 
                // (branch_type == BRANCH_PCOND || branch_type == BRANCH_INDIRECT
                //     || branch_type == BRANCH_RETURN))
                ctrl_flow->fix_spec_state (this);

			if (get_sync () != SYNC_TWO)
	   			seq->reset_fu_status (tid, FETCH_STALL_UNTIL_EMPTY);
		}
		break;

	case EVENT_SPEC_EXCEPTION:

		STAT_INC (pstats->stat_spec_exceptions);
		STAT_INC (tstats->stat_spec_exceptions);

		fu->return_fu ();
		preparefor_exception ();

		break;

	case EVENT_EXCEPTION: 

		STAT_INC (pstats->stat_exec_exc);
		STAT_INC (pstats->stat_exceptions);
		STAT_INC (tstats->stat_exec_exc);
		STAT_INC (tstats->stat_exceptions);
		
		fu->return_fu ();
		preparefor_exception ();

		break;

    case EVENT_SPECULATIVE:
        ASSERT (g_conf_sync2_runahead && sync_instr == SYNC_TWO);
        fu->return_fu ();
        kill_all ();
        seq->reset_fu_status (tid, FETCH_STALL_UNTIL_EMPTY);
        break;
        
	default:	
		FAIL;
	}
}

void
dynamic_instr_t::mem_hier_done () {

	reset_outstanding (PENDING_MEM_HIER);
	if (is_store ()) 
		st_mem_hier_done ();
	else if (is_load ()) 
		ld_mem_hier_done ();
	else 
		FAIL;
}


void
dynamic_instr_t::safe_ld_mem_access () {
	ASSERT (is_load ());
	ASSERT (get_lsq_entry () == LSQ_ENTRY_HOLD);
	ASSERT (get_pipe_stage () == PIPE_MEM_ACCESS_SAFE);

	// if (get_sync()) print("safe_ld_sync");
	// else print("safe_ld");
	
	set_pipe_stage (PIPE_MEM_ACCESS_PROGRESS);

	mem_xaction->setup_regular_access ();
	initiate_mem_hier ();
}


void 
dynamic_instr_t::ld_mem_access () {
	proc_stats_t *pstats = seq->get_pstats (tid);
	proc_stats_t *tstats = seq->get_tstats (tid);
	lsq_t *lsq = seq->get_lsq ();

	ASSERT (is_load ());
	ASSERT (get_lsq_entry () == LSQ_ENTRY_HOLD);
	ASSERT (get_pipe_stage () == PIPE_MEM_ACCESS);

	set_pipe_stage (PIPE_MEM_ACCESS_PROGRESS);

	if (!g_conf_use_internal_lsq) {
		if (unsafe_load ()) 
			mem_xaction->set_snoop_status (SNOOP_UNKNOWN);
		else 
			lsq->snoop (this);
	} else {

		// use internal lsq.
		if (unsafe_load ()) 
			mem_xaction->set_snoop_status (SNOOP_UNKNOWN);
		else {
			lsq->stq_snoop (this);

			// *Assumption* 
			ASSERT (!g_conf_initiate_loads_unresolved);
			ASSERT (!g_conf_disambig_prediction);

			if (mem_xaction->get_snoop_status () == SNOOP_NO_CONFLICT)
				lsq->stbuf_snoop (this); 
			else if (mem_xaction->get_snoop_status () == SNOOP_BYPASS_COMPLETE || 
				mem_xaction->get_snoop_status () == SNOOP_BYPASS_EXTND) 
				mem_xaction->set_snoop_status (SNOOP_NO_CONFLICT);
		}

		// use this when st buffer is disabled.
		// mem_xaction->set_snoop_status (SNOOP_NO_CONFLICT);
	}

	dynamic_instr_t *dep_instr = mem_xaction->get_dependent ();

	switch (mem_xaction->get_snoop_status ()) {
	
	case SNOOP_BYPASS_EXTND:
		if (g_conf_disable_extended_bypass) {
			if (g_conf_retry) {
				set_pipe_stage (PIPE_MEM_ACCESS_RETRY);
				dep_instr->insert_dead_wait_list (this);
			} else 
				set_pipe_stage (PIPE_REPLAY);
		} else	{
			set_pipe_stage (PIPE_MEM_ACCESS_PROGRESS_S2);
			release_ld_hold ();
		}

		break;

	case SNOOP_BYPASS_COMPLETE:
		if (g_conf_disable_complete_bypass) {
			if (g_conf_retry) {
				set_pipe_stage (PIPE_MEM_ACCESS_RETRY);		
				dep_instr->insert_dead_wait_list (this);
			} else	
				set_pipe_stage (PIPE_REPLAY);
		} else 	{
			set_pipe_stage (PIPE_MEM_ACCESS_PROGRESS_S2);
			release_ld_hold ();
		}

		break;
		
	case SNOOP_NO_CONFLICT:
		mem_xaction->setup_regular_access ();
		initiate_mem_hier ();
		break;

	case SNOOP_REPLAY:
		if (g_conf_retry) {
			set_pipe_stage (PIPE_MEM_ACCESS_RETRY);
			dep_instr->insert_dead_wait_list (this);
		} else	
			set_pipe_stage (PIPE_REPLAY);

		break;

	case SNOOP_UNRESOLVED_ST:
		if (g_conf_initiate_loads_unresolved) {

			STAT_INC (pstats->stat_initiate_loads_unresolved);
			STAT_INC (tstats->stat_initiate_loads_unresolved);

			mem_xaction->setup_regular_access ();
			initiate_mem_hier ();
		} else {
			STAT_INC (pstats->stat_unresolved_store);
			STAT_INC (tstats->stat_unresolved_store);

			set_pipe_stage (PIPE_MEM_ACCESS_RETRY);
			dep_instr->insert_issue_wait_list (this);

			if (g_conf_prefetch_unresolved) {
				STAT_INC (pstats->stat_ld_unres_pref);
				STAT_INC (tstats->stat_ld_unres_pref);
				mem_xaction->setup_store_prefetch ();
				initiate_mem_hier (true); // quiet
			}
		}

		break;

	case SNOOP_PRED_SAFE_ST:
		STAT_INC (pstats->stat_initiate_loads_unresolved);
		STAT_INC (tstats->stat_initiate_loads_unresolved);
		
		mem_xaction->setup_regular_access ();
		initiate_mem_hier ();
		break;

	case SNOOP_UNSAFE:
		STAT_INC (pstats->stat_ld_unsafe_store);
		STAT_INC (tstats->stat_ld_unsafe_store);
		set_pipe_stage (PIPE_MEM_ACCESS_RETRY);
		dep_instr->insert_dead_wait_list (this);

		if (g_conf_prefetch_unresolved) {
			STAT_INC (pstats->stat_ld_unres_pref);
			STAT_INC (tstats->stat_ld_unres_pref);
			mem_xaction->setup_store_prefetch ();
			initiate_mem_hier (true); // quiet
		}
		break;
			
		
	case SNOOP_UNKNOWN:
		set_pipe_stage (PIPE_MEM_ACCESS_SAFE);

		if (g_conf_prefetch_unresolved) {
			STAT_INC (pstats->stat_ld_unres_pref);
			STAT_INC (tstats->stat_ld_unres_pref);
			mem_xaction->setup_store_prefetch ();
			initiate_mem_hier (true); // quiet
		}
		break;

	case SNOOP_STATUS:
		FAIL;
		break;

	default:
		FAIL;
		break;

	}
}


void
dynamic_instr_t::retry_mem_access () {
	ASSERT (get_pipe_stage () == PIPE_MEM_ACCESS_RETRY);
	ASSERT (is_load ());

	set_pipe_stage (PIPE_MEM_ACCESS);
	mem_xaction->set_snoop_status (SNOOP_STATUS);

	ld_mem_access ();
}

void
dynamic_instr_t::release_ld_hold () {

	mem_trans_t *trans = get_mem_transaction()->get_mem_hier_trans(); 
	if (trans-> occurred(MEM_EVENT_L3_DEMAND_MISS | MEM_EVENT_L3_COHER_MISS))
		load_miss = 3;
	else if (trans->occurred(MEM_EVENT_L2_DEMAND_MISS | MEM_EVENT_L2_COHER_MISS))
		load_miss = 2;
	else if (trans->occurred(MEM_EVENT_L1_DEMAND_MISS | MEM_EVENT_L1_COHER_MISS |
        MEM_EVENT_L1_STALL |  MEM_EVENT_L1_MSHR_PART_HIT))
		load_miss = 1;
	
	load_latency = trans->request_time == 0 ? 0 :
		p->get_mem_hier()->get_mem_g_cycles() - trans->request_time; 

	ASSERT (get_pipe_stage () == PIPE_MEM_ACCESS_PROGRESS_S2 || 
		get_pipe_stage () == PIPE_MEM_ACCESS_SAFE);
	lsq_t *lsq = seq->get_lsq ();
	lsq->release_hold (this);

	set_pipe_stage (PIPE_MEM_ACCESS_REEXEC);

//	eventq->insert (this, 1);
  	wakeup ();
}

void
dynamic_instr_t::execute_after_release () {
	event = mai_instr->execute ();

	switch (event) {

	case EVENT_STALL:	
		//UNIMPLEMENTED;
		break;
	
	case EVENT_OK:
		set_pipe_stage (PIPE_COMMIT);
        if (g_conf_sync2_runahead && get_sync () == SYNC_TWO)
            ctrl_flow->fix_spec_state (this);
		break;

	case EVENT_SPEC_EXCEPTION:
		preparefor_exception ();
		break;

	case EVENT_EXCEPTION: 
		preparefor_exception ();
		break;

    case EVENT_SYNC:
        ASSERT (g_conf_sync2_runahead && get_next_d_instr ());
        get_next_d_instr ()->kill_all ();
        execute_after_release ();
        ctrl_flow->fix_spec_state (this);
        break;
        
	default:	
		FAIL;
	}
	
//#if 0
    if (event == EVENT_STALL) {
        v9_mem_xaction_t *v9_mem_xaction = mem_xaction->get_v9 ();
        v9_mem_xaction->set_mem_op (mai_instr->get_mem_transaction ());
        mem_xaction->release_stall ();
        //reexec_stall = true;
        eventq->insert (this, 1);
        return;
    } 
//#endif

	// bypass check
	if (event == EVENT_OK && is_load () && g_conf_check_lsq_bypass && 
		mem_xaction->get_snoop_status () == SNOOP_NO_CONFLICT &&
		mem_xaction->is_bypass_valid ()) {

		v9_mem_xaction_t *v9_mem_xaction = mem_xaction->get_v9 ();
		if (mem_xaction->get_size() < sizeof(uint64) &&
			mem_xaction->get_bypass_value () !=
			v9_mem_xaction->get_int64_value ())
			WARNING;
	}
}


void
dynamic_instr_t::ld_mem_hier_done () {
	ASSERT (is_load ());
	
	release_ld_hold ();
}

void
dynamic_instr_t::st_prefetch () {
	ASSERT (is_store ());
	ASSERT (get_lsq_entry () == LSQ_ENTRY_HOLD);
	ASSERT (get_pipe_stage () == PIPE_MEM_ACCESS);

//	if (!immediate_release_store ()) {

	mem_xaction->setup_store_prefetch ();
	initiate_mem_hier (true /* quiet, with no locks */);

//	}
}

void
dynamic_instr_t::st_mem_access () {
	if (g_conf_trace_pipe)
		print ("                            st_mem_acc:");

	ASSERT (is_store ()); 
	ASSERT (get_lsq_entry () == LSQ_ENTRY_HOLD);
	ASSERT (get_pipe_stage () == PIPE_MEM_ACCESS);


	if (!immediate_release_store ()) {
		set_pipe_stage (PIPE_MEM_ACCESS_PROGRESS_S2);
		release_st_hold ();
	} else {
		set_pipe_stage (PIPE_MEM_ACCESS_PROGRESS);
		mem_xaction->setup_regular_access ();
		initiate_mem_hier ();
	}
}

void
dynamic_instr_t::release_st_hold () {

	ASSERT (get_pipe_stage () == PIPE_MEM_ACCESS_PROGRESS_S2);
	lsq_t *lsq = seq->get_lsq ();

	lsq->release_hold (this);
	if (mai_instr->stall_cycles () != 0)
		WARNING;

	set_pipe_stage (PIPE_MEM_ACCESS_REEXEC);

//	eventq->insert (this, 1);
  	wakeup ();
}

void
dynamic_instr_t::release_fetch_hold () {

	reset_outstanding (PENDING_MEM_HIER);

	ASSERT (get_pipe_stage () == PIPE_FETCH_MEM_ACCESS);
	ASSERT (get_fetch_entry () == FETCH_ENTRY_HOLD);

	v9_mem_xaction_t *v9_mem_xaction = mem_xaction->get_v9 ();

	if (get_mai_instruction ()->stall_cycles () != 0) 
		ASSERT (get_mai_instruction ()->is_stalling ());

	v9_mem_xaction->set_mem_op (mai_instr->get_mem_transaction ());

	// do not call v9 release directly. some code might be added to
	// mem_xaction release. 
	mem_xaction->release_stall ();

	set_pipe_stage (PIPE_FETCH);
	set_fetch_entry (FETCH_ENTRY_RELEASE);

    fetch_latency = 0;
    if (initiate_fetch) {
        mem_trans_t *trans = get_mem_transaction()->get_mem_hier_trans(); 
        if (trans->occurred(MEM_EVENT_L3_DEMAND_MISS | MEM_EVENT_L3_COHER_MISS))
            fetch_miss = 3;
        else if (trans->occurred(MEM_EVENT_L2_DEMAND_MISS | MEM_EVENT_L2_COHER_MISS))
            fetch_miss = 2;
            else if (trans->occurred(MEM_EVENT_L1_DEMAND_MISS | MEM_EVENT_L1_COHER_MISS |
                MEM_EVENT_L1_STALL |  MEM_EVENT_L1_MSHR_PART_HIT))
            fetch_miss = 1;
            else 
                fetch_miss = 0;
            
            // ASSERT (p->get_mem_hier()->get_mem_g_cycles () > trans->request_time);
            fetch_latency = trans->request_time == 0 ? 0 :
            p->get_mem_hier ()->get_mem_g_cycles() - trans->request_time;
            
    }
//	eventq->insert (this, 1);
	wakeup ();
}

void 
dynamic_instr_t::release_st_buffer_hold () {
	ASSERT (get_pipe_stage () == PIPE_ST_BUF_MEM_ACCESS_PROGRESS);

	set_pipe_stage (PIPE_END);

	ASSERT (get_st_buffer_status () == ST_BUFFER_MEM_HIER);
	set_st_buffer_status (ST_BUFFER_DONE);

}

void
dynamic_instr_t::initiate_mem_hier (bool quiet) {
	if (!quiet) 
		set_outstanding (PENDING_MEM_HIER);
	
	mem_hier_handle_t *mem_hier = seq->get_mem_hier ();
    mem_trans_t *trans = mem_xaction->get_mem_hier_trans ();
    ASSERT (trans->dinst == this && !trans->completed);
	mem_hier->make_request ( mem_xaction->get_mem_hier_trans () );
}

void 
dynamic_instr_t::st_mem_hier_done () {
	ASSERT (is_store ());
	if (immediate_release_store ())
		release_st_hold ();
	else
		release_st_buffer_hold ();
}

void 
dynamic_instr_t::dequeue () {
	if (iwindow->pop_head (this))
        iwindow->reassign_window_slot();
        
    seq->decrement_icount(tid);
	if (is_load ())
		seq->get_lsq ()->pop_load (this);
	else if (is_store ()) 
		seq->get_lsq ()->pop_store (this);
}

void 
dynamic_instr_t::retire_begin () {


	if (pstage == PIPE_COMMIT) {
		set_pipe_stage (PIPE_COMMIT_CONTINUE);
		eventq->insert (this, g_conf_commit_stages);
	} else 
		retire ();
}

void
dynamic_instr_t::retire () {
	if (g_conf_trace_pipe)
		print ("                                retire:");

	proc_stats_t *pstats = seq->get_pstats (tid);
	proc_stats_t *tstats = seq->get_tstats (tid);

	if (pstage == PIPE_EXCEPTION) {

		dequeue ();

		set_except_pipe_stage (PIPE_NONE);
		handle_exception (); 

		end ();
		mark_dead ();		

	} else if (pstage == PIPE_SPEC_EXCEPTION) {

		set_pipe_stage (get_except_pipe_stage ());
		set_except_pipe_stage (PIPE_NONE);

		pipe_stage_t p = get_pipe_stage ();
		ASSERT (p == PIPE_FETCH ||
		        p == PIPE_DECODE ||
		        p == PIPE_EXECUTE_DONE || 
				p == PIPE_MEM_ACCESS_REEXEC);

		if (get_pipe_stage () == PIPE_EXECUTE_DONE) 
			set_pipe_stage (PIPE_WAIT);

		wakeup ();	

		if (get_pipe_stage () != PIPE_EXCEPTION) {
			ASSERT (get_pipe_stage () != PIPE_SPEC_EXCEPTION);
			ASSERT (g_conf_sync2_runahead ||
				seq->get_fu_status (tid, FETCH_STALL_UNTIL_EMPTY));

			seq->reset_fu_status (tid, FETCH_STALL_UNTIL_EMPTY);
		}
	} else if (pstage == PIPE_COMMIT || pstage == PIPE_COMMIT_CONTINUE) {	

		commit_stats ();
		dequeue ();
        
        if (sync_instr == SYNC_TWO && get_next_d_instr ())
        {
            ASSERT (is_load () || is_store ());
            ASSERT (g_conf_sync2_runahead);
            get_next_d_instr ()->kill_all ();
        }
        
        ASSERT(sync_shadow == false);
		commit ();
		end ();

        
        st_buffer_t *st_buffer = seq->get_st_buffer ();
			
        if (is_load() && unsafe_load()) ASSERT(st_buffer->empty_ctxt(tid) || sync_instr == SYNC_NONE);
        if (sequencing_membar()) ASSERT(st_buffer->empty_ctxt(tid));
        
		// if store and not for immediate release, move it into the st
		// buffer, and mark it dead but don't free. 
		if (is_store ()) {
            
            if (immediate_release_store ()) {
				// if (!is_atomic() && !get_sync())
				// 	print("imm_st");
	
                ASSERT(st_buffer->empty_ctxt(tid));
                mark_dead ();
                
            } else {
                
                ASSERT_WARN (!st_buffer->full ());
                ASSERT (mem_xaction);
                ASSERT (mem_xaction->is_void_xaction ());
                
                STAT_INC (pstats->stat_stores_stbuf);
                STAT_INC (tstats->stat_stores_stbuf);
                
                
                st_buffer->insert (this);
                mark_dead_without_insert ();		
            } 
        } else 
			mark_dead ();

	} else
		FAIL;
}

void
dynamic_instr_t::commit () {
	event = mai_instr->commit ();
    mai_t *mai = seq->get_mai_object(tid);
    
    if (sync_instr == SYNC_TWO) ASSERT(seq->get_pc (tid) == mai->get_pc());
	ctrl_flow->update_commit_state (this);
    bool is_supervisor = mai->is_supervisor();
    
	if (!page_fault && (get_opcode() == i_retry || get_opcode() == i_done))	
    {
		seq->potential_thread_switch(tid, YIELD_DONE_RETRY);
        if (!is_supervisor) {
            mai->reset_syscall_num();
        }
    }
	
	if (is_supervisor)
	{
		
		if (get_pc() == (addr_t) g_conf_kernel_idle_enter) {
			seq->get_mai_object(tid)->set_idle_loop(true);
			seq->potential_thread_switch(tid, YIELD_IDLE_ENTER);

			// Thread stats
			STAT_SET(seq->get_tstats(tid)->stat_idle_loop_start,
				p->get_g_cycles());
		}
		else if (get_pc() == (addr_t) g_conf_kernel_idle_exit) {
			mai->set_idle_loop(false);

			uint64 idle_time = p->get_g_cycles() - 
				STAT_GET(seq->get_tstats(tid)->stat_idle_loop_start);
			seq->get_tstats(tid)->stat_idle_loop_cycles->inc_total(idle_time);
			DEBUG_OUT("idle_exit:  cpu%d @ %llu\n",
				seq->get_mai_object(tid)->get_id(), p->get_g_cycles());
		}
		else if (get_pc() == (addr_t) g_conf_kernel_scheduler_pc
            || get_pc() == (addr_t) 0x100998c || get_pc() == (addr_t) 0x1009c88 ) {
		
            // CODE for LWP Experiments     
			//string sched_string = (get_pc() == (addr_t)  g_conf_kernel_scheduler_pc) ?
            //    "sched" : "interrupt";
            //DEBUG_OUT("cpu%u syscall %llu old_syscall %llu sp %llx %s @ %llu\n",
            //    mai->get_id(), mai->get_syscall_num(), mai->get_syscall_b4_interrupt(), 
            //    mai->get_register("sp"), sched_string.c_str(), p->get_g_cycles());
            uint64 syscall_2_cache = (get_pc() == (addr_t ) g_conf_kernel_scheduler_pc) ?
                mai->get_syscall_num() : mai->get_syscall_b4_interrupt();
            //if (syscall_2_cache < 302)    
                p->set_lwp_syscall(mai->get_register("sp"), syscall_2_cache);
            
                       
		} 
        
        if (g_conf_observe_kstack) observe_kstack_op();
	}
}


void dynamic_instr_t::observe_kstack_op()
{
    addr_t pc = get_pc();
    mai_t *mai = seq->get_mai_object(tid);
    bool kstack_operation = false;
    if (pc == (addr_t) 0x102a9a0 ||   /* resume with FP_RESTORE */ 
        /* pc == (addr_t) 0x102a9f0 ||    resume with no FP_RESTORE */
        pc == (addr_t) 0x1009edc  ||  /* After interrupt handle  */
        pc == (addr_t) 0x1009c1c ||   /* After interrupt handle - bottom of intr_thread */
        pc == (addr_t) 0x1042bb4 ||   /* Found the same thread */
        pc == (addr_t) 0x102aafc) {   /* Resume from interrupt */ 
            kstack_operation = true;
    }
        
    
    if (kstack_operation) {
        proc_stats_t *tstats0 = seq->get_tstats_list (tid)[0];
        uint64 syscall = STAT_GET (tstats0->stat_syscall_num);
        mai->set_kstack_region();
        //uint64 lwp_syscall = p->get_lwp_syscall(mai->get_kstack_region());
        uint64 stack_ptr =  mai->get_register("sp");
        uint64 lwp_syscall = p->get_lwp_syscall(stack_ptr);
        // If syscall not found mark it unknown
        if (!lwp_syscall) {
            uint64 syscall = mai->get_syscall_num();
            if (mai->get_kstack_region() == 0)
                lwp_syscall = SYS_UNKNOWN_BOOT;
            else if (syscall == SYS_NUM_OS_SCHED) {
                lwp_syscall = SYS_UNKNOWN_SCHED;
                //DEBUG_OUT("%u: sp %llx stack_region %llx UNKNOWN_SCHED with pc %llx @ %llu\n",
                //mai->get_id(), mai->get_register("sp"), mai->get_kstack_region(), pc, p->get_g_cycles());
                //g_conf_trace_commits = true;
                //g_conf_trace_cpu = mai->get_id();
            }
            else if (syscall == SYS_NUM_SW_INTR || syscall == SYS_NUM_HW_INTR)
                lwp_syscall = SYS_UNKNOWN_INTR;
            else {
                //WARNING;
                lwp_syscall = SYS_UNKNOWN_BOOT;
            }
            //DEBUG_OUT("cpu%u NOMATCH sp %llx pc %llx stat_syscall %llu closest %llx present syscall %llu\n",
            //    mai->get_id(), stack_ptr, pc, syscall,
            //    p->closest_lwp_match(stack_ptr), mai->get_syscall_num());
                
        } else {
            //DEBUG_OUT("cpu%u Matched LWP_STACK to syscall %llu prevcall %llu sp:%llx pc %llx\n",
            //mai->get_id(), lwp_syscall, syscall, stack_ptr, pc);
            ASSERT_WARN(STAT_GET (tstats0->stat_syscall_num) != 0);
            ASSERT_WARN(STAT_GET (tstats0->stat_syscall_start) != 0);
            if (STAT_GET(tstats0->stat_syscall_num) != lwp_syscall) {
                // Set stats for prev syscall
                uint64 syscall_len = p->get_g_cycles() - 
                STAT_GET (tstats0->stat_syscall_start);
                STAT_ADD (tstats0->stat_syscall_len, syscall_len, syscall);
                STAT_ADD (tstats0->stat_syscall_hit, 1, syscall);
                tstats0->stat_syscall_len_histo->inc_total(1, syscall_len);
                
                // setup new 'syscall'
                mai->set_syscall_num(true, lwp_syscall);
                STAT_SET (tstats0->stat_syscall_num, lwp_syscall);
                STAT_SET (tstats0->stat_syscall_start, p->get_g_cycles());
                seq->potential_thread_switch(tid, YIELD_SYSCALL_SWITCH);
                
            }
        }
        
        
    }
    
    
}

void
dynamic_instr_t::end () {
	event = mai_instr->end ();
}

bool 
dynamic_instr_t::retire_ready () {
	// do not replace this with a >= PIPE_COMMIT 
	return (pstage == PIPE_EXCEPTION ||
	        pstage == PIPE_SPEC_EXCEPTION ||
	        pstage == PIPE_COMMIT);
}

void 
dynamic_instr_t::preparefor_exception () {
	if (g_conf_trace_pipe)
		print ("prep_except:");
	
	set_except_pipe_stage (get_pipe_stage ());

	exception_stats ();

	if (event == EVENT_EXCEPTION) 
		set_pipe_stage (PIPE_EXCEPTION);
	else if (event == EVENT_SPEC_EXCEPTION)
		set_pipe_stage (PIPE_SPEC_EXCEPTION);
	else 
		FAIL;

	if (!g_conf_sync2_runahead) {
		if (get_next_d_instr ()) 
			get_next_d_instr ()->kill_all ();

		seq->set_fu_status (tid, FETCH_STALL_UNTIL_EMPTY);
	}
}

void 
dynamic_instr_t::handle_exception () {
	if (g_conf_trace_pipe || g_conf_trace_commits)
		print("handle_exception");
	
	if (g_conf_sync2_runahead) {
		if (get_next_d_instr ()) 
			get_next_d_instr ()->kill_all ();

		seq->set_fu_status (tid, FETCH_STALL_UNTIL_EMPTY);
	}
	
	mai_t *mai = seq->get_mai_object (tid);

	event = mai_instr->handle_exception ();
	ctrl_flow->fix_spec_state ();
	seq->reset_fu_status (tid, FETCH_GPC_UNKNOWN);

	// after mai call

	mai->setup_trap_context ();
	seq->potential_thread_switch(tid, YIELD_EXCEPTION);
}

void
dynamic_instr_t::set_pipe_stage (pipe_stage_t _pstage) {
	if (pstage == PIPE_REPLAY && _pstage != PIPE_REPLAY && _pstage !=
		PIPE_END && _pstage != PIPE_NONE)
		WARNING;

    
	prev_pstage = pstage;
	pstage = _pstage;
    
#ifdef POWER_COMPILE    
    if (g_conf_power_profile)
        core_power_profile->pipe_stage_movement (prev_pstage, pstage, this);
#endif
    
	if (pstage == PIPE_REPLAY) 
		kill_all ();
    else {
        update_stage_stats ();
        stage_entry_cycle = p->get_g_cycles ();
    }
            

}

void
dynamic_instr_t::set_except_pipe_stage (pipe_stage_t _pstage) {
	except_pstage = _pstage;
}

pipe_stage_t
dynamic_instr_t::get_except_pipe_stage () {
	return except_pstage;
}

pipe_stage_t
dynamic_instr_t::get_pipe_stage () {
	return pstage;
}

void 
dynamic_instr_t::set_window_slot (int32 _slot) {
	slot = _slot;
}

void 
dynamic_instr_t::set_lsq_slot (int32 _slot) {
	lsq_slot = _slot;
}

int32 
dynamic_instr_t::get_window_slot () {
	return slot;
}

int32 
dynamic_instr_t::get_lsq_slot () {
	return lsq_slot;
}

lsq_entry_t
dynamic_instr_t::get_lsq_entry () {
	return lsq_entry;
}

void
dynamic_instr_t::mark_lsq_entry (lsq_entry_t e) {
	lsq_entry = e;
}

fetch_entry_t
dynamic_instr_t::get_fetch_entry () {
	return fetch_entry;
}

void
dynamic_instr_t::set_fetch_entry (fetch_entry_t e) {
	fetch_entry = e;
}

bool
dynamic_instr_t::get_initiate_fetch() {
    return initiate_fetch;
}

void
dynamic_instr_t::set_initiate_fetch(bool flag) {
    initiate_fetch = flag;
}
 
instr_event_t
dynamic_instr_t::get_pending_event () {
	return event;
}

void 
dynamic_instr_t::mark_event (instr_event_t e) {
	event = e;
}

void 
dynamic_instr_t::wakeup () {
	switch (pstage) {

 	case PIPE_INSERT:
		insert (); 
		set_pipe_stage (PIPE_FETCH);

	case PIPE_FETCH: // fall-through
		fetch ();
		break;

	case PIPE_FETCH_MEM_ACCESS:
		release_fetch_hold ();
		break;
		
	case PIPE_PRE_DECODE:
		pre_decode ();
		break;

	case PIPE_DECODE: 
		decode ();
		break;

	case PIPE_RENAME:
		rename ();
		break;

	case PIPE_WAIT:
// 		sequencer will schedule 
		break;

	case PIPE_EXECUTE: 
		set_pipe_stage (PIPE_EXECUTE_DONE);

	case PIPE_EXECUTE_DONE: // fall-through
		execute ();
		break;

	case PIPE_MEM_ACCESS_RETRY:
		retry_mem_access ();
		break;

	case PIPE_MEM_ACCESS_PROGRESS:
		
		if (g_conf_stall_mem_access) {
			ASSERT (g_conf_stall_mem_access_cycles > 0);

			eventq->insert (this, g_conf_stall_mem_access_cycles);

			set_pipe_stage (PIPE_MEM_ACCESS_PROGRESS_S2);
			break;

		} else {
			set_pipe_stage (PIPE_MEM_ACCESS_PROGRESS_S2);
			// fall through
		}
		

	case PIPE_MEM_ACCESS_PROGRESS_S2:
		mem_hier_done (); 
		break;

	case PIPE_ST_BUF_MEM_ACCESS_PROGRESS:
		mem_hier_done ();
		break;

	case PIPE_MEM_ACCESS_REEXEC:
		execute_after_release ();
		break;

	case PIPE_COMMIT_CONTINUE:
		retire ();
		break;

	case PIPE_REPLAY:
	case PIPE_END:

		if (get_outstanding (PENDING_MEM_HIER)) 
			reset_outstanding (PENDING_MEM_HIER);


		break;

	default:
		break;

	}
    
    
}


bool
dynamic_instr_t::get_execute_at_head()
{
    return execute_at_head;
}

sequencer_t * const
dynamic_instr_t::get_sequencer (void) {
	return seq;
}

void 
dynamic_instr_t::mark_dead (void) {
	mark_dead_without_insert ();
	insert_dead_list ();	
}

void
dynamic_instr_t::insert_dead_list (void) {
	ASSERT (get_pipe_stage () == PIPE_END);
	ASSERT (done == true);
	
	// unwind
	pop_dead_wait_list ();
	pop_issue_wait_list ();

	ASSERT (dead_wl->empty ());
	ASSERT (issue_wl->empty ());

	seq->insert_dead (this);

}

void
dynamic_instr_t::mark_dead_without_insert (void) {
	if (get_pipe_stage () == PIPE_EXECUTE) fu->return_fu ();

	set_pipe_stage (PIPE_END);
	done = true;

    if (get_next_d_instr() && get_next_d_instr()->get_prev_d_instr() == this)
        get_next_d_instr()->set_prev_d_instr(0);
    if (get_prev_d_instr() && get_prev_d_instr()->get_next_d_instr() == this)
        get_prev_d_instr()->set_next_d_instr(0);
	set_next_d_instr (0);
	set_prev_d_instr (0);
}

bool
dynamic_instr_t::is_done (void) {
	return done;
}

bool
dynamic_instr_t::is_squashed (void) {
	return squashed;
}

void
dynamic_instr_t::set_squashed (void) {
	squashed = true;

	if (g_conf_trace_pipe)
		print ("squash:");
}

uint32
dynamic_instr_t::get_outstanding (void) {
	return outstanding;
}

bool
dynamic_instr_t::get_outstanding (uint32 type) {
	return ( ((outstanding & type) == type) );
}

void
dynamic_instr_t::set_outstanding (uint32 type) {
	ASSERT ( (outstanding & type) == 0x0);
	outstanding = (outstanding | type);
}

void 
dynamic_instr_t::reset_outstanding (uint32 type) {
	ASSERT ( (outstanding & type) == type);
	outstanding = (outstanding & (~type));
}
	

void
dynamic_instr_t::set_sync (uint32 _type) {
	sync_instr = static_cast <sync_type_t> (_type);
}

sync_type_t 
dynamic_instr_t::get_sync (void) {
	return sync_instr;
}

bool
dynamic_instr_t::safe_commit (void) {
	return (!mai_instr->speculative ());
}

addr_t
dynamic_instr_t::get_pc (void) {
	
    return gpc->pc;
}

addr_t
dynamic_instr_t::get_npc (void) {
    
	return gpc->npc;
}

instruction_type_t 
dynamic_instr_t::get_type (void) {
	return type;
}

branch_type_t 
dynamic_instr_t::get_branch_type (void) {
	return branch_type;
}

addr_t
dynamic_instr_t::get_offset (void) {
	return offset;
}

bool
dynamic_instr_t::get_annul (void) {
	return annul;
}

bool
dynamic_instr_t::is_taken (void) {
	return taken;
}

void
dynamic_instr_t::set_taken (bool _t) {
	taken = _t;
}

void
dynamic_instr_t::set_pred_o_gpc (gpc_t *_gpc) {
	pred_o_gpc->pc = _gpc->pc;
	pred_o_gpc->npc = _gpc->npc;
}

void
dynamic_instr_t::set_actual_o_gpc (addr_t n_pc, addr_t n_gpc) {
	actual_o_gpc->pc = n_pc;
	actual_o_gpc->npc = n_gpc;
}

gpc_t*
dynamic_instr_t::get_actual_o_gpc () {
	return actual_o_gpc;
}

gpc_t*
dynamic_instr_t::get_pred_o_gpc () {
	return pred_o_gpc;
}

bool
dynamic_instr_t::match_o_gpc (void) {
	return (pred_o_gpc->pc == actual_o_gpc->pc && 
		(pred_o_gpc->npc & 0xffffffff) == (actual_o_gpc->npc & 0xffffffff));
}

void
dynamic_instr_t::set_bpred_state (bpred_state_t *bpred) {
	bpred_state->copy_state (bpred);
}

bpred_state_t*
dynamic_instr_t::get_bpred_state (void) {
	return bpred_state;
}

opcodes_t
dynamic_instr_t::get_opcode (void) {
	return opcode;
}

bool
dynamic_instr_t::is_store (void) {
	return (type == IT_STORE);
}

bool
dynamic_instr_t::is_load (void) {
	return (type == IT_LOAD && opcode != i_prefetch && opcode != i_prefetcha);
}

bool
dynamic_instr_t::is_store_alt (void) {
	return (type == IT_STORE && ( (ainfo & AI_ALT_SPACE) == AI_ALT_SPACE));
}

bool
dynamic_instr_t::is_load_alt (void) {
	return (type == IT_LOAD && ( (ainfo & AI_ALT_SPACE) == AI_ALT_SPACE));
}

bool
dynamic_instr_t::is_atomic (void) {
	return (  (ainfo & AI_ATOMIC) == AI_ATOMIC  );
}

bool
dynamic_instr_t::is_prefetch (void) {
	return (  (ainfo & AI_PREFETCH) == AI_PREFETCH  );
}

bool
dynamic_instr_t::speculative (void) {
	if (squashed) // for print();
		return true;

	// XXX for now, return speculative of mai_instr 
  	return (  mai_instr->speculative ()  );
}

bool
dynamic_instr_t::immediate_release_store (void) {
	ASSERT (is_store ());
	
	if (g_conf_disable_stbuffer) return true;

	return ( unsafe_store () );
}


bool
dynamic_instr_t::is_ldd_std (void) {

	if (opcode == i_ldd || opcode == i_ldda) return true;

	if (opcode == i_std || opcode == i_stda) return true;

	return false;
}

bool
dynamic_instr_t::is_unsafe_alt (void) {
	ASSERT(is_store() || is_load());
	
	if ((ainfo & AI_ALT_SPACE) != AI_ALT_SPACE)
		return false;
	
	// Input reg wasn't available at decode, try again
	if (imm_asi == -1)
		imm_asi = mai_instr->read_input_asi ();
	ASSERT_WARN(imm_asi != -1 || get_pipe_stage() <= PIPE_WAIT);
	if (imm_asi == -1 && get_pipe_stage() > PIPE_WAIT)
		WARNING; // breakable point...
	
	return v9_mem_xaction_t::is_unsafe_asi(imm_asi);
}

bool
dynamic_instr_t::unsafe_store (void) {
	ASSERT (is_store ());
	
    if (ainfo == AI_ALT_SPACE)
        return true;
	bool atomic = is_atomic ();
	if (g_conf_ldd_std_safe && is_store() && is_ldd_std()) atomic = false;
	if (!g_conf_ldd_std_safe && is_ldd_std()) return true;
	
	if (mem_xaction) {
		bool ret = ( is_unsafe_alt () || atomic || get_sync () ||
			mem_xaction->unimpl_checks () ||
			//mem_xaction->get_size () > sizeof (uint64) ||
			mem_xaction->get_phys_addr () >= (uint64) g_conf_memory_image_size );
		return ret;
	}
	else 
		return true;
}

bool
dynamic_instr_t::unsafe_load (void) {
	ASSERT (is_load ());

        return true;
    if (ainfo == AI_ALT_SPACE)
        return true;
	if ((opcode == i_ldd || opcode == i_ldda) && !g_conf_ldd_std_safe)
		return true;
	// if ((opcode == i_ldd || opcode == i_ldda || opcode == i_lddf || opcode == i_lddfa) && !g_conf_ldd_std_safe)
	// 	return true;

	if (mem_xaction) {
		bool ret = ( is_unsafe_alt () || is_atomic () || get_sync () ||
			mem_xaction->unimpl_checks () ||
			//mem_xaction->get_size () > sizeof (uint64) || 
			mem_xaction->get_phys_addr () >= (uint64) g_conf_memory_image_size );
		return ret;
	}
	
	FAIL;
	return true;

}

mem_xaction_t*
dynamic_instr_t::get_mem_transaction (void) {
	return mem_xaction;
}

void
dynamic_instr_t::set_mem_transaction (mem_xaction_t *_t) {
	ASSERT (_t);

    if (mem_xaction != _t && mem_xaction) {
		_t->merge_changes (mem_xaction);
        delete mem_xaction;
	}
    
	mem_xaction = _t;
}

void 
dynamic_instr_t::set_st_buffer_status (st_buffer_entry_t status) {
	st_buffer_entry = status;
}

st_buffer_entry_t 
dynamic_instr_t::get_st_buffer_status (void) {
	return st_buffer_entry;
}

bool
dynamic_instr_t::not_issued (void) {
	return (get_pipe_stage () < PIPE_EXECUTE);
}

void
dynamic_instr_t::insert_dead_wait_list (dynamic_instr_t *d_instr) {
	insert_wait_list (dead_wl, d_instr);
}

void
dynamic_instr_t::pop_dead_wait_list (void) {
	pop_wait_list (dead_wl);
}

void
dynamic_instr_t::insert_issue_wait_list (dynamic_instr_t *d_instr) {
	insert_wait_list (issue_wl, d_instr);
}

void 
dynamic_instr_t::pop_issue_wait_list (void) {
	pop_wait_list (issue_wl);
}

void
dynamic_instr_t::pop_wait_list (wait_list_t *wl) {
	
	for (; wl; wl = wl->get_next_wl ()) {
		while (wl->head ()) {
			dynamic_instr_t *d_instr = wl->head ();
			d_instr->reset_outstanding (PENDING_WAIT_LIST);
		
			// do not replace this with wakeup
			eventq->insert (d_instr, 1);

			wl->pop_head (d_instr);
		}
	}
}

void
dynamic_instr_t::insert_wait_list (wait_list_t *start_wl, 
	dynamic_instr_t *d_instr) {

	wait_list_t *wl = start_wl->get_end_wait_list (); 
	if (wl->full ()) wl = start_wl->create_wait_list ();

	d_instr->set_outstanding (PENDING_WAIT_LIST);
	wl->insert (d_instr);
}

void 
dynamic_instr_t::commit_stats () {
	if (g_conf_trace_commits || g_conf_trace_pipe)
		print ("                                        commit:");

    mai_t *mai = seq->get_mai_object (tid);
    
    
	proc_stats_t *pstats = seq->get_pstats (tid);
	proc_stats_t *tstats = seq->get_tstats (tid);
	proc_stats_t *pstats0 = seq->get_pstats_list (tid)[0];
	proc_stats_t *tstats0 = seq->get_tstats_list (tid)[0];
	proc_stats_t *pstats1 = g_conf_kernel_stats ? seq->get_pstats_list (tid)[1] : pstats0;
	proc_stats_t *tstats1 = g_conf_kernel_stats ? seq->get_tstats_list (tid)[1] : tstats0;
	
    // Spin Heuristic updates
    seq->get_spin_heuristic(tid)->observe_commit_pc(this);
    if (g_conf_spin_check_interval && g_conf_spinloop_threshold && 
        (is_store() || is_load()) && mem_xaction)
        seq->get_spin_heuristic(tid)->observe_d_instr(this);
    // End Spin heuristi updates    
        
	if (is_store ()) {
		STAT_INC (pstats->stat_stores);
		STAT_INC (tstats->stat_stores);
	}
	if (is_store () && mem_xaction) {
		uint32 size_index = mem_xaction->get_size_index ();
		STAT_INC (pstats->stat_stores_size, size_index);
		STAT_INC (tstats->stat_stores_size, size_index);
	}
	if (is_load ()) {
        
        STAT_INC (pstats->stat_loads);
		STAT_INC (tstats->stat_loads);
        
        if (mem_xaction) {
            
            uint32 size_index = mem_xaction->get_size_index ();
            STAT_INC (pstats->stat_loads_size, size_index);
            STAT_INC (tstats->stat_loads_size, size_index);
            mem_trans_t *trans = mem_xaction ? mem_xaction->get_mem_hier_trans() : 0;
            
            
            uint64 syscall = mai->get_syscall_num();
            
            if (unsafe_load ()) {
                STAT_INC (pstats->stat_unsafe_loads);
                STAT_INC (tstats->stat_unsafe_loads);
            }
            
            if (syscall &&  mai->get_kstack_region() == 0 &&
                trans && trans->get_access_type() == LWP_STACK_ACCESS)
            {
                //DEBUG_OUT("%u WTF sp %llx!!\n", mai->get_id(), mai->get_register("sp"));
                mai->set_kstack_region();
                p->set_lwp_syscall(mai->get_kstack_region(), syscall);
            }
            
            
            
            if (syscall && trans) {
                kernel_access_type_t ktype = trans->get_access_type();
                if (load_miss >= 1) {
                    tstats->stat_load_l1_miss_type->inc_total(1, ktype);
                } 
                if (load_miss >= 2) 
                    tstats->stat_load_l2_miss_type->inc_total(1, ktype);
                
                if (trans->occurred(MEM_EVENT_L2_COHER_MISS | MEM_EVENT_L1DIR_COHER_MISS
                    | MEM_EVENT_L1DIR_REQ_FWD))
                tstats->stat_load_coher_miss_type->inc_total(1, ktype);
                tstats->stat_syscall_load_type->inc_total(1, ktype);
                
                //}
                
            } 
            //else if (load_miss >= 1 && mai->get_id() == 0)
            //    DEBUG_OUT("%8llu: va:%llx pa:%llx load_lat %llu user mode\n", p->get_g_cycles(),
            //            trans->virt_addr, trans->phys_addr, load_latency);
        }
	}
	
	if (type == IT_CONTROL) {
		STAT_INC (pstats->stat_controls);
		STAT_INC (tstats->stat_controls);
	}
	if (is_prefetch ()) {
		STAT_INC (pstats->stat_prefetches);
		STAT_INC (tstats->stat_prefetches);
	}
	if (is_atomic ()) { 
		STAT_INC (pstats->stat_atomics);
		STAT_INC (tstats->stat_atomics);
	}
	
	if (is_load()) {
		if (load_miss >= 1) {
			STAT_INC (pstats->stat_load_l1_miss);
			STAT_INC (tstats->stat_load_l1_miss);
			if (STAT_GET(tstats0->stat_syscall_num) != 0) {
				uint64 syscall = STAT_GET (tstats0->stat_syscall_num);
				STAT_INC (pstats->stat_syscall_dmiss, syscall);
				STAT_INC (tstats->stat_syscall_dmiss, syscall);
			}
		}
		if (load_miss >= 2) {
			STAT_INC (pstats->stat_load_l2_miss);
			STAT_INC (tstats->stat_load_l2_miss);
		}
		if (load_miss >= 3) {
			STAT_INC (pstats->stat_load_l3_miss);
			STAT_INC (tstats->stat_load_l3_miss);
		}

		pstats->stat_load_request_time->inc_total(1, load_latency);	
		tstats->stat_load_request_time->inc_total(1, load_latency);	
	}
	
	if (fetch_miss >= 1) {
		STAT_INC (pstats->stat_instr_l1_miss);
		STAT_INC (tstats->stat_instr_l1_miss);
		if (STAT_GET(tstats0->stat_syscall_num) != 0) {
			uint64 syscall = STAT_GET (tstats0->stat_syscall_num);
			STAT_INC (pstats->stat_syscall_imiss, syscall);
			STAT_INC (tstats->stat_syscall_imiss, syscall);
		}
	}
	if (fetch_miss >= 2) {
		STAT_INC (pstats->stat_instr_l2_miss);
		STAT_INC (tstats->stat_instr_l2_miss);
	}
	if (fetch_miss >= 3) {
		STAT_INC (pstats->stat_instr_l3_miss);
		STAT_INC (tstats->stat_instr_l3_miss);
	}

	pstats->stat_instr_request_time->inc_total(1, fetch_latency);	
	tstats->stat_instr_request_time->inc_total(1, fetch_latency);	


	// supervisor, user stats
	if (mai->is_supervisor ()) {
		STAT_INC (pstats1->stat_supervisor_instr);
		STAT_INC (pstats0->stat_supervisor_count);
		uint64 user_length = STAT_GET (pstats0->stat_user_count);
		if (user_length) {
			STAT_SET (pstats0->stat_user_count, 0);
			STAT_INC (pstats0->stat_user_histo, user_length);
			STAT_INC (pstats0->stat_user_entry);
		}

		STAT_INC (tstats1->stat_supervisor_instr);
		STAT_INC (tstats0->stat_supervisor_count);
		user_length = STAT_GET (tstats0->stat_user_count);
		if (user_length) {
			STAT_SET (tstats0->stat_user_count, 0);
			STAT_INC (tstats0->stat_user_histo, user_length);
			STAT_INC (tstats0->stat_user_entry);
		}
	} else {
		STAT_INC (pstats0->stat_user_instr);
		STAT_INC (pstats0->stat_user_count);
		uint64 supervisor_length = STAT_GET (pstats0->stat_supervisor_count);
		if (supervisor_length) {
 			STAT_SET (pstats0->stat_supervisor_count, 0);
			STAT_INC (pstats1->stat_supervisor_histo, supervisor_length);
			STAT_INC (pstats1->stat_supervisor_entry);
		}

		STAT_INC (tstats0->stat_user_instr);
		STAT_INC (tstats0->stat_user_count);
		supervisor_length = STAT_GET (tstats0->stat_supervisor_count);
		if (supervisor_length) {
 			STAT_SET (tstats0->stat_supervisor_count, 0);
			STAT_INC (tstats1->stat_supervisor_histo, supervisor_length);
			STAT_INC (tstats1->stat_supervisor_entry);
		}

		// Just returned from a syscall
		if (STAT_GET(tstats0->stat_syscall_num) != 0) {
			uint64 syscall = STAT_GET (tstats0->stat_syscall_num);
			uint64 syscall_len = p->get_g_cycles() - 
				STAT_GET (tstats0->stat_syscall_start);

            STAT_ADD (tstats->stat_syscall_len, syscall_len, syscall);
			STAT_ADD (tstats->stat_syscall_hit, 1, syscall);
			tstats->stat_syscall_len_histo->inc_total(1, syscall_len);
			
			STAT_SET (tstats0->stat_syscall_num, 0);
			STAT_SET (tstats0->stat_syscall_start, 0);
		}
	}

	uint64 tl = STAT_GET (tstats0->stat_tl);
	uint64 mai_tl = mai->get_tl ();

	if (tl) 
		STAT_INC (tstats0->stat_trap_length [tl]);
		
	if (opcode == i_wrpr && mai_tl > tl) {
		
		for (uint32 i = tl + 1; i <= mai->get_tl (); i++)
			mai->setup_trap_context (true);
	}

	// Page Fault!
	if (opcode == i_done && mai->is_tlb_trap(mai->get_tt(tl))) {
		// if currently in syscall, finish it up and change to page fault type
		if (STAT_GET (tstats0->stat_syscall_num) != 0) {
			ASSERT(STAT_GET (tstats0->stat_syscall_start) != 0);

			uint64 syscall_len = p->get_g_cycles() - 
				STAT_GET (tstats0->stat_syscall_start);
			uint64 syscall = STAT_GET (tstats0->stat_syscall_num);

			STAT_ADD (tstats0->stat_syscall_len, syscall_len, syscall);
			STAT_ADD (tstats0->stat_syscall_hit, 1, syscall);
			tstats0->stat_syscall_len_histo->inc_total(1, syscall_len);
		}
		mai->set_syscall_num(true, SYS_NUM_PAGE_FAULT);
		STAT_SET (tstats0->stat_syscall_num, SYS_NUM_PAGE_FAULT);
		STAT_SET (tstats0->stat_syscall_start, p->get_g_cycles());
 		seq->potential_thread_switch(tid, YIELD_PAGE_FAULT);
		page_fault = true;
		STAT_INC(tstats0->page_flts);
	}
	
	if ( opcode == i_retry || opcode == i_done || 
		(opcode == i_wrpr && mai_tl < tl) ) {
			
		ASSERT (tl > 0);
		for (uint32 i = tl; i > mai_tl; i--) {

			int64 tt = STAT_GET (tstats0->stat_tt [i]);
			int64 trap_len = STAT_GET (tstats0->stat_trap_length [i]);

			// Apropos tstats for user/os
			STAT_ADD (tstats->stat_trap_len, trap_len, tt);
			STAT_INC (tstats->stat_trap_hit, tt);

			// 
			STAT_DEC (tstats0->stat_tl);
			STAT_SET (tstats0->stat_tt [i], 0);
			STAT_SET (tstats0->stat_trap_length [i], 0);
		}
	}

//	ASSERT (STAT_GET (pstats->stat_tl) == mai->get_tl ());
/*
	if ((is_load() || is_store() || is_atomic()) && 
		STAT_GET(tstats0->stat_syscall_num) != 0)
	{
		mem_trans_t *trans = get_mem_transaction()->get_mem_hier_trans();
		if (trans &&
			trans->occurred(MEM_EVENT_L2_COHER_MISS | MEM_EVENT_L2_REQ_FWD))
		{
			// TEMP!!!
			if (mai_instr->get_mai_object()->get_id() == 1) {
			DEBUG_OUT("p:0x%08llx v:0x%012llx sys:%llu cpu:%d\n", trans->phys_addr,
				trans->virt_addr, STAT_GET(tstats0->stat_syscall_num),
				mai_instr->get_mai_object()->get_id());
			print("");
			}
		}		
	}
*/

	switch (get_branch_type ()) {

	case BRANCH_UNCOND: 
	case BRANCH_COND:
	case BRANCH_PCOND:  
		STAT_INC (pstats->stat_direct_br); 
		STAT_INC (tstats->stat_direct_br); 
		break;

	case BRANCH_INDIRECT: 
		STAT_INC (pstats->stat_indirect_br); 
		STAT_INC (tstats->stat_indirect_br); 
		break;

	case BRANCH_CALL:
		STAT_INC (pstats->stat_call_br); 
		STAT_INC (tstats->stat_call_br); 
		break; 

	case BRANCH_RETURN:
		STAT_INC (pstats->stat_return);
		STAT_INC (tstats->stat_return);
		break;
	
	case BRANCH_CWP:
		STAT_INC (pstats->stat_cwp);
		STAT_INC (tstats->stat_cwp);
		break;

	case BRANCH_TRAP:
		STAT_INC (pstats->stat_trap);
		STAT_INC (tstats->stat_trap);
		break;

	case BRANCH_TRAP_RETURN:
		STAT_INC (pstats->stat_trap_ret);
		STAT_INC (tstats->stat_trap_ret);
		break;

	default: 
		break; 

	}

	switch (get_sync ()) {

	case SYNC_ONE: 
		STAT_INC (pstats->stat_sync1); 
		STAT_INC (tstats->stat_sync1); 
		break; 

	case SYNC_TWO: 
		STAT_INC (pstats->stat_sync2); 
		STAT_INC (tstats->stat_sync2); 
		break;

	default: 
		break;
	}
	
	p->get_mem_hier()->profile_commits(this);
}

void
dynamic_instr_t::exception_stats () {
	proc_stats_t *pstats = seq->get_pstats (tid);
	proc_stats_t *tstats = seq->get_tstats (tid);

	/* NOTE: this is a hack and will need to be changed when we implement SMT
	 * One fix would be to count the number of instructions in the window first, 
	 * then squash and recount; or to have a per thread count of instructions 
	 * and take that difference */
	uint32 val;
	uint32 size    = iwindow->get_window_size ();
	int32 last_cr = iwindow->get_last_created ();

	if (last_cr < slot)  
		val = size - (slot - last_cr);
	else 
		val = last_cr - slot;

	STAT_INC (pstats->stat_squash_exception, val);
	STAT_INC (tstats->stat_squash_exception, val);
}

int64
dynamic_instr_t::get_imm_asi () {
	return imm_asi;
}

fu_t*
dynamic_instr_t::get_fu () {
	return fu;
}

string
dynamic_instr_t::pstage_str () {
    switch (pstage) {
    case PIPE_NONE: return "None";
    case PIPE_FETCH: return "Fetch";
    case PIPE_FETCH_MEM_ACCESS: return "Fetch_Mem";
    case PIPE_INSERT: return "Insert";
    case PIPE_PRE_DECODE: return "Pre_Decode";
    case PIPE_DECODE: return "Decode";
    case PIPE_RENAME: return "Rename";
    case PIPE_WAIT: return "Wait";
    case PIPE_EXECUTE: return "Execute";
    case PIPE_EXECUTE_DONE: return "Exec_done";
    case PIPE_MEM_ACCESS: return "Mem_Access";
    case PIPE_MEM_ACCESS_SAFE: return "Mem_Acc_Safe";
    case PIPE_MEM_ACCESS_RETRY: return "Mem_Acc_Retry";
    case PIPE_MEM_ACCESS_PROGRESS: return "Mem_Acc_Progress";
    case PIPE_MEM_ACCESS_PROGRESS_S2: return "Mem_Acc_Progress_S2";
    case PIPE_MEM_ACCESS_REEXEC: return "Mem_Acc_Reexec";
    case PIPE_COMMIT: return "Commit";
    case PIPE_COMMIT_CONTINUE: return "Commit_Continue";
    case PIPE_EXCEPTION: return "Exception";
    case PIPE_SPEC_EXCEPTION: return "Spec_Exception";
    case PIPE_REPLAY: return "Replay";
    case PIPE_ST_BUF_MEM_ACCESS_PROGRESS: return "St_Buf_Mem_Acc_Progress";
    case PIPE_END: return "End";
    default: return "N/A";
    }
}

void 
dynamic_instr_t::kill_all () {
    
    if (get_pipe_stage () > PIPE_REPLAY) return; 

	ctrl_flow->fix_spec_state (get_prev_d_instr ());
	seq->reset_fu_status (tid, FETCH_GPC_UNKNOWN);

	// do not change the order 
	event = mai_instr->squash_from ();
	ASSERT (event == EVENT_OK);

	iwindow->squash (this, seq->get_lsq ());
}


dynamic_instr_t* 
dynamic_instr_t::get_prev_d_instr (void) {
	return prev_d_instr;
}

dynamic_instr_t*
dynamic_instr_t::get_next_d_instr (void) {
    ASSERT (!next_d_instr || next_d_instr->get_tid () == tid);
	return next_d_instr;
}

void 
dynamic_instr_t::set_prev_d_instr (dynamic_instr_t *prev) {
	prev_d_instr = prev;
    ASSERT(!prev || prev->get_tid() == tid);
}

void 
dynamic_instr_t::set_next_d_instr (dynamic_instr_t *next) {
	next_d_instr = next;
    ASSERT(!next || next->get_tid() == tid);
}

uint8 dynamic_instr_t::get_tid() {
    return tid;
}

void dynamic_instr_t::set_tid(uint8 _t) {
    tid = _t;
}

void
dynamic_instr_t::print (string stage) {
    mai_t *mai = mai_instr->get_mai_object();
	
	if (g_conf_trace_cpu < p->get_num_cpus() &&
		g_conf_trace_cpu != mai->get_id()) return;
	
	string instr;
    tuple_int_string_t *assembly = SIM_disassemble (
		mai->get_cpu_obj(), get_pc (), 1);
	if (SIM_get_pending_exception ()) {
		SIM_clear_exception();
		instr = "???";
	} else {
		instr = assembly->string;
	}
		
	DEBUG_OUT ("%02d %02d %02d %10llu %-48s 0x%08llx %llu %s %s\n", 
		seq->get_id(), tid, mai->get_id(), p->get_g_cycles(), stage.c_str(), get_pc(),
//		seq_id, speculative() ? "S" : " ", instr.c_str());
		seq_id, " ", instr.c_str());
	if (SIM_get_pending_exception ())
		SIM_clear_exception();
}

uint64
dynamic_instr_t::get_stq_ptr() {
	return stq_ptr;
}

void
dynamic_instr_t::set_stq_ptr(uint64 ptr) {
	stq_ptr = ptr;
}

bool
dynamic_instr_t::is_sync_shadow() {
	return sync_shadow;
}

void
dynamic_instr_t::set_sync_shadow() {
	sync_shadow = true;
}

uint8
dynamic_instr_t::get_membar_type() {
	ASSERT(get_opcode() == i_membar);
	return membar_type;
}

bool
dynamic_instr_t::sequencing_membar() {
    //return opcode == i_membar;
	// any cmask (bits 7-3), plus #StoreLoad (bit 1)
    return ((get_opcode() == i_membar) && (membar_type & 0x72));
}

tick_t dynamic_instr_t::get_fetch_cycle() {
    return fetch_cycle;
}

void dynamic_instr_t::update_stage_stats ()
{
    // if (pstage == PIPE_NONE) return;
    
    if (pstage == PIPE_NONE || prev_pstage == PIPE_END) return;
    if (pstage == PIPE_END && prev_pstage != PIPE_COMMIT && 
        prev_pstage != PIPE_ST_BUF_MEM_ACCESS_PROGRESS)
        return;
        
    proc_stats_t *pstats = seq->get_pstats (tid);
	proc_stats_t *tstats = seq->get_tstats (tid);
	tick_t time_spent = p->get_g_cycles () - stage_entry_cycle;
    if (pstats) pstats->stat_stage_histo->inc_total (time_spent, prev_pstage);
    if (tstats) tstats->stat_stage_histo->inc_total (time_spent, prev_pstage);
    
}

bool dynamic_instr_t:: verify_npc_val ()
{
    ASSERT (actual_o_gpc->pc == gpc->npc);
    mai_instr->check_pc_npc (gpc);
    return speculative ();
}

uint32 dynamic_instr_t::get_cpu_id ()
{
    return mai_instr->get_mai_object ()->get_id ();
}


void dynamic_instr_t::analyze_operand_width()
{
    proc_stats_t *pstats = seq->get_pstats (tid);
	proc_stats_t *tstats = seq->get_tstats (tid);
    
    seq->update_current_function (tid, gpc->pc);
    uint32 function_index = seq->get_function_index (tid);
    uint64 current_commit = STAT_GET (tstats->stat_commits);
    mai_t *mai = mai_instr->get_mai_object ();
    
    if (futype >= FU_FLOATADD && futype <= FU_FLOATSQRT) {
        // Floating point operation
        pstats->stat_float_exception->inc_total (1, function_index);
        tstats->stat_float_exception->inc_total (1, function_index);
        
        pstats->stat_8bit_runs->inc_total (1, current_commit - mai->get_8bit_commit ());
        pstats->stat_16bit_runs->inc_total (1, current_commit - mai->get_16bit_commit ());
        pstats->stat_24bit_runs->inc_total (1, current_commit - mai->get_24bit_commit ());
        
        
        tstats->stat_8bit_runs->inc_total (1, current_commit - mai->get_8bit_commit ());
        tstats->stat_16bit_runs->inc_total (1, current_commit - mai->get_16bit_commit ());
        tstats->stat_24bit_runs->inc_total (1, current_commit - mai->get_24bit_commit ());
        
        mai->set_8bit_commit (current_commit);
        mai->set_16bit_commit (current_commit);
        mai->set_24bit_commit (current_commit);
    }
	
    if  ((futype != FU_INTALU && futype != FU_INTMULT && futype != FU_INTDIV) ||
        (g_conf_ignore_sllx && (opcode == i_sllx && shift_cnt == 12))) {
        pstats->stat_unaccounted_instr->inc_total (1, opcode - i_add);
        tstats->stat_unaccounted_instr->inc_total (1, opcode - i_add);
        return;
    }
         
    pstats->stat_function_execution->inc_total (1, function_index);
    tstats->stat_function_execution->inc_total (1, function_index);
    
    bzero(reg_ids, sizeof(int32) * 96);
    mai_instr->get_reg(reg_ids, num_srcs, num_dests);
    // mai_instr->debug(false);
    // if (imm_val) DEBUG_OUT("Imm = %d ", imm_val);
    uint64 max_ival32 = 0;
    uint64 max_oval32 = 0;
    uint64 max_ival24 = 0;
    uint64 max_oval24 = 0;
    uint64 max_ival16 = 0;
    uint64 max_oval16 = 0;
    uint64 max_ival8  = 0;
    uint64 max_oval8  = 0;
    
    bool ignore_cmp = (opcode == i_cmp && g_conf_ignore_cmp);
    
    for (int32 i = 0; i < num_srcs; i++)
    {
        // uint64 value = mai_instr->read_output_reg (regs[RD + i]);
        int64 i_value = mai_instr->read_input_reg (reg_ids[i]);
        if (i_value < 0) goto negative_value;
        uint64 i_value8 = (i_value >> 8);
        if (i_value8 > max_ival8) max_ival8 = i_value8;
        uint64 i_value16 = (i_value >> 16);
        if (i_value16 > max_ival16) max_ival16 = i_value16;
        uint64 i_value24 = (i_value >> 24);
        if (i_value24 > max_ival24) max_ival24 = i_value24;
        uint64 i_value32 = (i_value >> 32);
        if (i_value32 > max_ival32) max_ival32 = i_value32;
        // DEBUG_OUT ("%d %llx %llx %llx %llx %s\n", i, i_value, i_value16, i_value32, i_value40,
        //     (i_value < 0) ? "Negative" : "Positive");
    }
    for (int32 i = 0; i < num_dests; i++)
    {
        int64 o_value = mai_instr->read_output_reg (reg_ids[RD + i]);
        if (o_value < 0) goto negative_value;
        // DEBUG_OUT ("OUTPUT %d : %llx\n",i, o_value);
        uint64 o_value8 = (o_value >> 8);
        uint64 o_value16 = (o_value >> 16);
        uint64 o_value24 = (o_value >> 24);
        uint64 o_value32 = (o_value >> 32);
        if (o_value32 > max_oval32) max_oval32 = o_value32;
        if (o_value16 > max_oval16) max_oval16 = o_value16;
        if (o_value24 > max_oval24) max_oval24 = o_value24;
        if (o_value8 > max_oval8) max_oval8 = o_value8;
    }
    
    if (max_ival16 == 0) {
        STAT_INC (pstats->stat_allinput_16);
        STAT_INC (tstats->stat_allinput_16);
    }
    
    if(max_ival24 == 0) {
        STAT_INC (pstats->stat_allinput_24);
        STAT_INC (tstats->stat_allinput_24);
    }
    
    if (max_ival32 == 0) {
        STAT_INC (pstats->stat_allinput_32);
        STAT_INC (tstats->stat_allinput_32);
    }
    
    uint32 datawidth;
    if ((max_oval16 == max_ival16) || ignore_cmp) {
        datawidth = 16;
    } else if ((max_oval24 == max_ival24) || ignore_cmp) {
        datawidth = 24;
    } else if ((max_oval32 == max_ival32) || ignore_cmp) {
        datawidth = 32;
    } else {
        datawidth = 64;
    }
    
    
    for (int32 i = 0; i < num_dests; i++)
    {
        seq->set_register_busy(reg_ids[RD + i], datawidth);
    }
    
    
    pstats->stat_narrow_operand32->inc_total (1, max_oval32 == max_ival32 || ignore_cmp);
    tstats->stat_narrow_operand32->inc_total (1, max_oval32 == max_ival32 || ignore_cmp);
    if (max_ival32 != max_oval32 && !ignore_cmp ) {
        pstats->stat_except_opcode32->inc_total (1, opcode - i_add);
        tstats->stat_except_opcode32->inc_total (1, opcode - i_add);
        pstats->stat_except32_PC->inc_total (1, function_index);
        tstats->stat_except32_PC->inc_total (1, function_index);
        
        
    }
    
    pstats->stat_narrow_operand24->inc_total (1, max_oval24 == max_ival24 || ignore_cmp);
    tstats->stat_narrow_operand24->inc_total (1, max_oval24 == max_ival24 || ignore_cmp);
    if (max_ival24 != max_oval24 && !ignore_cmp) {
        pstats->stat_except_opcode24->inc_total (1, opcode - i_add);
        tstats->stat_except_opcode24->inc_total (1, opcode - i_add);
        pstats->stat_except24_PC->inc_total (1, function_index);
        tstats->stat_except24_PC->inc_total (1, function_index);
        
        pstats->stat_24bit_runs->inc_total (1, current_commit - mai->get_24bit_commit ());
        tstats->stat_24bit_runs->inc_total (1, current_commit - mai->get_24bit_commit ());
        mai->set_24bit_commit (current_commit);
    }
    
    pstats->stat_narrow_operand16->inc_total (1, max_oval16 == max_ival16 || ignore_cmp);
    tstats->stat_narrow_operand16->inc_total (1, max_oval16 == max_ival16 || ignore_cmp);
    if (max_ival16 != max_oval16 && !ignore_cmp) {
        pstats->stat_except_opcode16->inc_total (1, opcode - i_add);
        tstats->stat_except_opcode16->inc_total (1, opcode - i_add);
        pstats->stat_except16_PC->inc_total (1, function_index);
        tstats->stat_except16_PC->inc_total (1, function_index);
        
        pstats->stat_16bit_runs->inc_total (1, current_commit - mai->get_16bit_commit ());
        tstats->stat_16bit_runs->inc_total (1, current_commit - mai->get_16bit_commit ());
        mai->set_16bit_commit (current_commit);
    }
    
    if (max_ival8 != max_oval8 && !ignore_cmp) {
        pstats->stat_8bit_runs->inc_total (1, current_commit - mai->get_8bit_commit ());
        tstats->stat_8bit_runs->inc_total (1, current_commit - mai->get_8bit_commit ());
        mai->set_8bit_commit (current_commit);
    }
    return;
    
    negative_value: 
        analyze_negative_ops();
        STAT_INC (pstats->stat_negative_inpoutp);
        STAT_INC (tstats->stat_negative_inpoutp);
    // if (max_oval == max_ival) DEBUG_OUT ("BINGOOO\n");
    //     else DEBUG_OUT("OOPSS!!!\n");
}

void dynamic_instr_t::analyze_negative_ops()
{
    
    bool ignore_cmp = (opcode == i_cmp && g_conf_ignore_cmp);
    proc_stats_t *pstats = seq->get_pstats (tid);
	proc_stats_t *tstats = seq->get_tstats (tid);
    int64 max_ival = 0;
    int64 max_oval = 0;
    for (int32 i = 0; i < num_srcs; i++)
    {
        // uint64 value = mai_instr->read_output_reg (regs[RD + i]);
        int64 i_value = mai_instr->read_input_reg (reg_ids[i]);
        if (i_value < 0) {
            if (-i_value > max_ival) {
                max_ival = -i_value;
            }
        } else if (i_value > max_ival) {
            max_ival = i_value;
        }
    }
    
    for (int32 i = 0; i < num_dests; i++)
    {
        int64 o_value = mai_instr->read_output_reg (reg_ids[RD + i]);
        if (o_value < 0) {
            if (-o_value > max_oval) {
                max_oval = -o_value;
            }
        } else if (o_value > max_oval) {
            max_oval = o_value;
        }
    }
       
    uint64 o_value16 = (max_oval >> 16);
    uint64 o_value24 = (max_oval >> 24);
    uint64 o_value32 = (max_oval >> 32);
    
       
    uint64 i_value16 = (max_ival >> 16);
    uint64 i_value24 = (max_ival >> 24);
    uint64 i_value32 = (max_ival >> 32);
    
    if (i_value16 != o_value16 && !ignore_cmp) {
        STAT_INC (tstats->stat_negative_except16);
        STAT_INC (pstats->stat_negative_except16);
        
    } 
    
    if (i_value24 != o_value24 && !ignore_cmp) {
        STAT_INC (tstats->stat_negative_except24);
        STAT_INC (pstats->stat_negative_except24);
    }
    
    if (i_value32 != o_value32 && !ignore_cmp) {
        
        STAT_INC (tstats->stat_negative_except32);
        STAT_INC (pstats->stat_negative_except32);
    }
        
}

bool dynamic_instr_t::check_register_busy()
{
    
    bzero(reg_ids, sizeof(int32) * 96);
    mai_instr->get_reg(reg_ids, num_srcs, num_dests);
    for (int32 i = 0; i < num_srcs; i++)
    {
        if (seq->get_register_busy(reg_ids[i])) 
            return true;
    }
    return false;
}
    
