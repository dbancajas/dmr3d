/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: mai_instr.cc,v 1.9.2.3 2006/08/18 14:30:19 kchak Exp $");

#include "definitions.h"
#include "isa.h"
#include "mai.h"
#include "sequencer.h"
#include "ctrl_flow.h"
#include "dynamic.h"
#include "mai_instr.h"
#include "fu.h"
#include "mmu.h"

mai_instruction_t::mai_instruction_t (mai_t *_mai, dynamic_instr_t *d_instr) {
	cpu = _mai->get_cpu_obj ();
	shadow_cpu = _mai->get_shadow_cpu_obj ();

	mai = _mai;
	fetch_mmu_info = NULL;
	ldst_mmu_info = NULL;
   	instr_id = SIM_instruction_begin (cpu);
	shadow_instr_id = 0;
	
	if (!instr_id || SIM_get_pending_exception ()) { 
		WARNING;
		SIM_clear_exception ();
   	}

	SIM_instruction_set_user_data (instr_id, static_cast <void *> (d_instr));

	attr_value_t mai_pc;
	attr_value_t mai_npc;

	mai_pc.u.integer = d_instr->get_pc () ;
	mai_npc.u.integer = d_instr->get_npc () ;
    
    // if (mai_pc.u.integer & 0xff000000ULL == 0xff000000ULL) {
    //     mai_pc.u.integer != 0x1ffffffffULL;
    //     mai_npc.u.integer != 0x1ffffffffULL;
    // }

	SIM_instruction_write_input_reg (instr_id, V9_Reg_Id_PC, mai_pc);
	SIM_instruction_write_input_reg (instr_id, V9_Reg_Id_NPC, mai_npc);

	set_stage (INSTR_CREATED);
    mode32bit = mai->is_32bit();
	FE_EXCEPTION_CHECK;
}


mai_instruction_t::~mai_instruction_t () {
	delete fetch_mmu_info;
	delete ldst_mmu_info;
}

dynamic_instr_t* 
mai_instruction_t::get_dynamic_instr (void) {
	dynamic_instr_t *d_instr;

	d_instr = static_cast <dynamic_instr_t *> 
	          (SIM_instruction_get_user_data (instr_id));

	FE_EXCEPTION_CHECK;
	return d_instr;		  
}

mai_t*
mai_instruction_t::get_mai_object () {
	return mai;
}

instruction_id_t 
mai_instruction_t::get_instr_id () {
	return instr_id;
}

instruction_id_t 
mai_instruction_t::get_shadow_instr_id () {
	return shadow_instr_id;
}

generic_transaction_t*
mai_instruction_t::get_mem_transaction () {
	generic_transaction_t *mem_op = SIM_instruction_stalling_mem_op (instr_id);
	FE_EXCEPTION_CHECK;
	return mem_op;
}

v9_memory_transaction_t*
mai_instruction_t::get_v9_mem_transaction () {
	generic_transaction_t *mem_op = get_mem_transaction ();

	return ( (v9_memory_transaction_t *) (mem_op) );
}

conf_object_t*
mai_instruction_t::get_cpu () {
	return cpu;
}

instr_event_t
mai_instruction_t::insert () {
	if (!instr_id) return  EVENT_FAILED;
	ASSERT (done_create ());
	
	SIM_instruction_insert (0, instr_id);

	FE_EXCEPTION_CHECK;
	return EVENT_OK;
}

instr_event_t 
mai_instruction_t::fetch () {
	instr_event_t e;
	instruction_error_t ie; 
	
	ASSERT (done_create ());

	// If refetched
	if (fetch_mmu_info) {
		delete fetch_mmu_info;
		fetch_mmu_info = NULL;
	}
	if (ldst_mmu_info) {
		delete ldst_mmu_info;
		ldst_mmu_info = NULL;
	}
	sequencer_t::mmu_d_instr = get_dynamic_instr ();
	sequencer_t::icache_instr_id = instr_id;
	instruction_error_t fetch_ie = SIM_instruction_fetch(instr_id);
	sequencer_t::icache_instr_id = NULL;

	switch (fetch_ie) {
	case Sim_IE_OK:
		e = EVENT_OK;
		break;
	case Sim_IE_Exception:
		if (!speculative ()) {
			e = EVENT_EXCEPTION;
		} else {
			e = EVENT_SPEC_EXCEPTION;
			if ( (SIM_instruction_status (instr_id) & Sim_IS_Faulting) 
			        == Sim_IS_Faulting)
//				uncomment in simics 2.0. or, replace with assert. mmu
//				regs being modified when condition is true
				WARNING;
			;
		}
		break;
	case Sim_IE_Stalling:
		e = EVENT_STALL;
		break;
	case Sim_IE_Sync_Instruction:
		e = EVENT_OK;
		break;
	default:
		FAIL;
		e = EVENT_UNRESOLVED;
		break;
	}

	sequencer_t::mmu_d_instr = NULL;
	
	if (e == EVENT_OK) set_stage (INSTR_FETCHED);
	FE_EXCEPTION_CHECK;

    
	shadow_catchup ();
	return e;
}

instr_event_t
mai_instruction_t::decode () {
	instr_event_t e;
	instruction_error_t ie;
	
	sequencer_t::mmu_d_instr = get_dynamic_instr ();

//	ASSERT (done_fetch ());
	switch ((ie = SIM_instruction_decode (instr_id))) {
	case Sim_IE_OK:
		e = EVENT_OK;
		break;
	case Sim_IE_Exception:
		if (!speculative ()) {
			e = EVENT_EXCEPTION;
		} else {
			e = EVENT_SPEC_EXCEPTION;
			ASSERT ((SIM_instruction_status (instr_id) & Sim_IS_Faulting) 
			        != Sim_IS_Faulting);
		}
		break;

	case Sim_IE_Unresolved_Dependencies:
		e = EVENT_UNRESOLVED_DEP;
		break;

	default:
		FAIL;
		e = EVENT_UNRESOLVED;
		break;
	}

	sequencer_t::mmu_d_instr = NULL;

	if (e == EVENT_OK) set_stage (INSTR_DECODED);
	FE_EXCEPTION_CHECK;

	shadow_catchup ();
	return e;	
}

instr_event_t
mai_instruction_t::retire () {
	instr_event_t e;
	instruction_error_t ie;

	sequencer_t::mmu_d_instr = get_dynamic_instr ();

	ie = SIM_instruction_retire (instr_id);
	switch (ie) {
	case Sim_IE_OK:
		e = EVENT_OK;
		break;
	
	case Sim_IE_Stalling:
		e = EVENT_STALL;
		break;

	case Sim_IE_Exception:
		e = EVENT_EXCEPTION;
		break;
	
	default:
		FAIL;
		break;
	}

	sequencer_t::mmu_d_instr = NULL;

	if (e == EVENT_OK) set_stage (INSTR_EXECUTED);

	shadow_catchup ();
	return e;
}

instr_event_t
mai_instruction_t::execute () {
	instr_event_t       e;
	instruction_error_t ie;

	attr_value_t        pc;
	attr_value_t        npc;

	dynamic_instr_t     *d_instr;

	if (done_execute ()) return EVENT_OK;
	ASSERT (done_decode ());

	sequencer_t::mmu_d_instr = get_dynamic_instr ();

	switch ((ie = SIM_instruction_execute (instr_id))) {
	case Sim_IE_OK:
		e = EVENT_OK;
		pc = SIM_instruction_read_output_reg (instr_id, V9_Reg_Id_PC);
		npc = SIM_instruction_read_output_reg (instr_id, V9_Reg_Id_NPC);
		d_instr = get_dynamic_instr ();

        if (mai->is_32bit()) {
         //   pc.u.integer &= 0xffffffffULL;
         //   npc.u.integer &= 0xffffffffULL;
        }
		d_instr->set_actual_o_gpc (
			(static_cast <addr_t> (pc.u.integer)), 
			(static_cast <addr_t> (npc.u.integer))
		);

		if (d_instr->get_branch_type () == BRANCH_COND ||
			d_instr->get_branch_type () == BRANCH_PCOND)
			d_instr->set_taken (branch_taken ());	

		break;
	case Sim_IE_Exception:
		if (!speculative ()) {
			e = EVENT_EXCEPTION;
		} else {
			e = EVENT_SPEC_EXCEPTION;
			ASSERT ((SIM_instruction_status (instr_id) & Sim_IS_Faulting) 
			        != Sim_IS_Faulting);
		}
			
		break;
	case Sim_IE_Sync_Instruction:
		e = EVENT_SYNC;
		break;
	case Sim_IE_Stalling:
		e = EVENT_STALL;
		break;
    case Sim_IE_Speculative:
        e = EVENT_SPECULATIVE;
        break;
	default:
        DEBUG_LOG ("The error code returned is %u\n", ie); DEBUG_FLUSH ();
		FAIL;
		e = EVENT_UNRESOLVED;
		break;
	}
	
	sequencer_t::mmu_d_instr = NULL;

	if (e == EVENT_OK) set_stage (INSTR_EXECUTED);
	FE_EXCEPTION_CHECK;

	shadow_catchup ();
	return e;
}

instr_event_t 
mai_instruction_t::commit () {
	instr_event_t e;
	instruction_error_t ie;

	sequencer_t::mmu_d_instr = get_dynamic_instr ();

	ASSERT (done_execute ());
redo:
	switch ((ie = SIM_instruction_commit (instr_id))) {
	case Sim_IE_OK:
		e = EVENT_OK;
		break;
	case Sim_IE_Hap_Breakpoint:   
	case Sim_IE_Step_Breakpoint: 
		mai->break_sim (0);
		goto redo;
		break;
	case Sim_IE_Speculative:
		e = EVENT_SQUASH;
		break;
	default:
		FAIL;
		e = EVENT_UNRESOLVED;
		break;
	}
	
	sequencer_t::mmu_d_instr = NULL;

	if (e == EVENT_OK) set_stage (INSTR_COMMITTED);
	FE_EXCEPTION_CHECK;

	shadow_replay (INSTR_COMMITTED);
	shadow_catchup ();
	return e;
}


void
mai_instruction_t::shadow_catchup () {

	uint32 i = 0;
	mai_instr_stage_t reach;
//	bool shadow_sync_two = false;

	if (!mai->check_shadow ()) return;

	instruction_id_t index = SIM_instruction_nth_id (cpu, i);
	if (!index) return;

	do {
//		if (shadow_sync_two) break;

		if (index && SIM_instruction_speculative (index)) break;

//		if (SIM_instruction_is_sync (index) == 2) 
//			shadow_sync_two = true;

		dynamic_instr_t *index_d_instr = static_cast <dynamic_instr_t *> 
	          	(SIM_instruction_get_user_data (index));

		ASSERT (index_d_instr);

		mai_instruction_t *index_mai_instr =
		index_d_instr->get_mai_instruction ();		

		FE_EXCEPTION_CHECK;

		if (SIM_instruction_phase (index) == Sim_Phase_Committed) 
			reach = INSTR_COMMITTED;
		if (SIM_instruction_phase (index) == Sim_Phase_Executed)
			reach = INSTR_EXECUTED;
		else if (SIM_instruction_phase (index) == Sim_Phase_Retired)
			reach = INSTR_ST_RETIRED; 
		else if (SIM_instruction_phase (index) == Sim_Phase_Decoded)
			reach = INSTR_DECODED;
		else if (SIM_instruction_phase (index) == Sim_Phase_Fetched)
			reach = INSTR_FETCHED;
		else if (SIM_instruction_phase (index) == Sim_Phase_Initiated) 
			reach = INSTR_CREATED;

		if ((SIM_instruction_status (index) & Sim_IS_Faulting) == Sim_IS_Faulting) {
			reach = (mai_instr_stage_t) ((uint32) reach + 1);
		}

		index_mai_instr->shadow_replay (reach);
			
		i++; index = SIM_instruction_nth_id (cpu, i);
		FE_EXCEPTION_CHECK;

	} while (index);
}

void 
mai_instruction_t::shadow_replay (mai_instr_stage_t reach) {
	instruction_error_t ie = Sim_IE_OK;
	attr_value_t pc;
	attr_value_t shadow_pc;
	tuple_int_string_t *s;

	if (!shadow_cpu) return;

sim_start:
	if (!shadow_instr_id || ie == Sim_IE_Illegal_Id) {

		if (ie == Sim_IE_Illegal_Id)
			SIM_clear_exception ();

		shadow_instr_id = SIM_instruction_begin (shadow_cpu);
		ASSERT (shadow_instr_id);

		SIM_instruction_insert (0, shadow_instr_id);
	}

	if (SIM_instruction_phase (shadow_instr_id) == Sim_Phase_Initiated)
		goto sim_fetch;

	if (SIM_instruction_phase (shadow_instr_id) == Sim_Phase_Fetched)
		goto sim_decode; 

	if (SIM_instruction_phase (shadow_instr_id) == Sim_Phase_Decoded)
		goto sim_execute; 
	
	if (SIM_instruction_phase (shadow_instr_id) == Sim_Phase_Executed)
		goto sim_st_retire; 

	if (SIM_instruction_phase (shadow_instr_id) == Sim_Phase_Retired)
		goto sim_commit; 

sim_fetch:
	if (reach <= INSTR_CREATED) return;

	// fetch
	ie = SIM_instruction_fetch (shadow_instr_id);
	if (ie == Sim_IE_Illegal_Id) goto sim_start;
	if (ie == Sim_IE_Exception) return;
	ASSERT (ie == Sim_IE_OK);

sim_decode:
	if (reach <= INSTR_FETCHED) return;

	// decode
	ie = SIM_instruction_decode (shadow_instr_id);
	if (ie == Sim_IE_Illegal_Id) goto sim_start;
	if (ie == Sim_IE_Exception) return;
	ASSERT (ie == Sim_IE_OK);

	pc = SIM_instruction_read_input_reg (instr_id, V9_Reg_Id_PC);
	shadow_pc = SIM_instruction_read_input_reg (shadow_instr_id, V9_Reg_Id_PC);
	ASSERT (pc.u.integer == shadow_pc.u.integer);

	s = SIM_disassemble (cpu, shadow_pc.u.integer, 1);
	if (!s) SIM_clear_exception ();
//	DEBUG_OUT ("0x%llx ", pc.u.integer); 
//	DEBUG_OUT ("SLAVE %s\n", s ? s->string : "(null)");

sim_execute:
	if (reach <= INSTR_DECODED) return;

	// execute
	ie = SIM_instruction_execute (shadow_instr_id);
	if (ie == Sim_IE_Illegal_Id) goto sim_start;
	if (ie == Sim_IE_Exception) return;
	ASSERT (ie == Sim_IE_OK);

	reg_check (false);

sim_st_retire:
	if (reach <= INSTR_EXECUTED) return;

	// retire
	ie = SIM_instruction_retire (shadow_instr_id);
	if (ie == Sim_IE_Illegal_Id) goto sim_start;
	if (ie == Sim_IE_Exception) return;
	ASSERT (ie == Sim_IE_OK || ie == Sim_IE_Sync_Instruction);

sim_commit:
	if (reach <= INSTR_ST_RETIRED) return;

	// commit
	pc = SIM_instruction_read_output_reg (instr_id, V9_Reg_Id_PC);
	shadow_pc = SIM_instruction_read_output_reg (shadow_instr_id, V9_Reg_Id_PC);
	ASSERT (pc.u.integer == shadow_pc.u.integer);

	ie = SIM_instruction_commit (shadow_instr_id);
	if (ie == Sim_IE_Illegal_Id) goto sim_start;

	ASSERT (ie == Sim_IE_OK);

	reg_check (true);

	SIM_instruction_end (shadow_instr_id);
}


void 
mai_instruction_t::reg_check (bool auto_correct) {
	uint32       i = 0;

	reg_info_t  *ri;
	reg_info_t  *shadow_ri;

	attr_value_t val_in;
	attr_value_t shadow_val_in;

	attr_value_t val_out;
	attr_value_t shadow_val_out;

	while ( (ri = SIM_instruction_get_reg_info (instr_id, i)) ) {

		if (ri->id >= 96) { 
			i++;
			continue;
		}

#if 0
		if (ri->id >= 96)
			WARNING;
#endif

		if (ri->input) {
			shadow_ri = SIM_instruction_get_reg_info (shadow_instr_id, i);
			ASSERT (shadow_ri->input);

			ASSERT (ri->id == shadow_ri->id);
			FE_EXCEPTION_CHECK;

			val_in = SIM_instruction_read_input_reg (instr_id, ri->id);
			shadow_val_in = SIM_instruction_read_input_reg (shadow_instr_id, shadow_ri->id);

			ASSERT (val_in.u.integer == shadow_val_in.u.integer);
			FE_EXCEPTION_CHECK;
		} 

		// don't replace with else if. reg can in and out 	
		if (ri->output) {

			shadow_ri = SIM_instruction_get_reg_info (shadow_instr_id, i);
			ASSERT (shadow_ri->output);

			ASSERT (ri->id == shadow_ri->id);
			FE_EXCEPTION_CHECK;

			val_out = SIM_instruction_read_output_reg (instr_id, ri->id);
			shadow_val_out = SIM_instruction_read_output_reg (shadow_instr_id, shadow_ri->id);

			if (val_out.u.integer != shadow_val_out.u.integer) {
				if (auto_correct) {
					debug (0);
					SIM_write_register (cpu, ri->id, shadow_val_out.u.integer);
				} else 
					WARNING;
			}

			FE_EXCEPTION_CHECK;

		}
		i++;
	}
}

instr_event_t 
mai_instruction_t::end () {
	instr_event_t e;
	instruction_error_t ie;

	switch ((ie = SIM_instruction_end (instr_id))) {
	case Sim_IE_OK:
		e = EVENT_OK;
		break;	
	default:
		FAIL;
		e = EVENT_UNRESOLVED;
		break;
	}
	
	FE_EXCEPTION_CHECK;
	return e;
}

instr_event_t
mai_instruction_t::squash_further () {
	instruction_id_t    child;
	instruction_error_t ie;
	instr_event_t       e = EVENT_OK;

	instruction_id_t id = instr_id;

do_again:
	while ((child = SIM_instruction_child (id, 0))) {
		switch ((ie = SIM_instruction_squash (child))) {
		case Sim_IE_OK:
			break;
		default:
			FAIL;
			e = EVENT_UNRESOLVED;
			break;
		}	
	}

	FE_EXCEPTION_CHECK;

	if (id == instr_id && shadow_instr_id) {
		id = shadow_instr_id; 
		goto do_again;
	}

	return e;
}

instr_event_t
mai_instruction_t::squash_from () {
	instruction_error_t ie;
	instruction_id_t    id;
	instr_event_t       e = EVENT_OK;

	id = instr_id;

do_again:
	switch ((ie = SIM_instruction_squash (id))) {
	case Sim_IE_OK:
		 break;
	default:
		FAIL;
		e = EVENT_UNRESOLVED;
		break;
	}

	FE_EXCEPTION_CHECK;

	if (id == instr_id && shadow_instr_id) {
		id = shadow_instr_id; 
		goto do_again;
	}
	return e;
}

instr_event_t 
mai_instruction_t::handle_exception () {
	instr_event_t e;
	instruction_error_t ie;

	shadow_catchup ();

	switch ((ie = SIM_instruction_handle_exception (instr_id))) {
	case Sim_IE_OK:
		e = EVENT_OK;

		if (shadow_instr_id) {
			ASSERT (SIM_instruction_handle_exception (shadow_instr_id) == Sim_IE_OK);
			SIM_instruction_end (shadow_instr_id);
		}
		break;
	case Sim_IE_Hap_Breakpoint:
	case Sim_IE_Step_Breakpoint:
		ie = SIM_instruction_handle_exception (instr_id);
		ASSERT (ie == Sim_IE_OK);
		mai->break_sim (0);
		e = EVENT_OK;
		break;
	default:
		FAIL;
		e = EVENT_UNRESOLVED;
		break;
	}
	
	FE_EXCEPTION_CHECK;
	return e;
}

int64
mai_instruction_t::get_instruction_field (const string &field) {
	int64 value;

	value = SIM_instruction_get_field_value (instr_id, field.c_str());
	
//	FE_EXCEPTION_CHECK;

	if (SIM_clear_exception ()) {
		WARNING; 
		value = 0;
	}
	return value;
}

void 
mai_instruction_t::debug (bool verbose) {
	debug (verbose, instr_id);
}

void 
mai_instruction_t::shadow_debug (bool verbose) {
	debug (verbose, shadow_instr_id);
}
	
void 
mai_instruction_t::debug (bool verbose, instruction_id_t id) {
	attr_value_t pc;

	uint32       i = 0;
	reg_info_t   *ri;
	attr_value_t val_in;
	attr_value_t val_out;

//	processor_t  *p = SIM_proc_no_2_ptr ( SIM_get_proc_no (cpu) );
	
	// cannot debug instruction if not decoded 
	if (SIM_instruction_phase (id) < Sim_Phase_Decoded) 
		return;
	
	pc = SIM_instruction_read_input_reg (id, V9_Reg_Id_PC);

	tuple_int_string_t *s = SIM_disassemble (cpu, pc.u.integer, 1);
	if (!s) SIM_clear_exception ();
	DEBUG_OUT ("%s\n", s ? s->string : "(null)");

	if (!verbose) return;

	// input and output registers
	while ( (ri = SIM_instruction_get_reg_info (id, i++)) ) {
		if (ri->input) {
			val_in = SIM_instruction_read_input_reg (id, ri->id);
			DEBUG_OUT (" input: r%d    %llx Architectural: %llx %s\n", ri->id, 
                val_in.u.integer,SIM_read_register (cpu, ri->id), 
                SIM_get_register_name (mai->get_cpu_obj (), ri->id));
		} 

		// don't replace with else if. reg can in and out 	
		if (ri->output) {
			val_out = SIM_instruction_read_output_reg (id, ri->id);
			DEBUG_OUT ("output: r%d    %llx    Architectural: %llx %s\n", 
			ri->id, val_out.u.integer, SIM_read_register (cpu, ri->id),
            SIM_get_register_name (mai->get_cpu_obj (), ri->id));
		}
	}

	FE_EXCEPTION_CHECK;
}

mai_instr_stage_t
mai_instruction_t::get_stage () {
	return stage;
}

void 
mai_instruction_t::set_stage (mai_instr_stage_t _stage) {
	stage = _stage;
}

bool 
mai_instruction_t::done_create () {
	return (stage == INSTR_CREATED);
}

bool 
mai_instruction_t::done_fetch () {
	return (stage == INSTR_FETCHED);
}

bool
mai_instruction_t::done_decode () {
	return (stage == INSTR_DECODED);
}

bool 
mai_instruction_t::done_execute () {
	return (stage == INSTR_EXECUTED);
}

bool 
mai_instruction_t::done_commit () {
	return (stage == INSTR_COMMITTED);
}	

const char
mai_instruction_t::debug_stage () {
	if (INSTR_CREATED)
		return 'I';
	if (INSTR_FETCHED)
		return 'F';
	if (INSTR_DECODED)
		return 'D';
	if (INSTR_EXECUTED)
		return 'E';
	if (INSTR_COMMITTED)
		return 'C';
}

bool 
mai_instruction_t::readyto_execute () {
	if (SIM_instruction_phase (instr_id) >= Sim_Phase_Executed |
	   (SIM_instruction_status (instr_id) & Sim_IS_Ready) == Sim_IS_Ready) 
   		return true;
	else
		return false;
}

tick_t
mai_instruction_t::stall_cycles (void) {
	tick_t c;

	if ((SIM_instruction_status (instr_id) & Sim_IS_Stalling) == Sim_IS_Stalling) 
		c = SIM_instruction_remaining_stall_time (instr_id);
	else {
		c = 0;
//		FAIL;
	}

 	FE_EXCEPTION_CHECK;	
	return c;
}

bool
mai_instruction_t::finished_stalling (void) {
   	instruction_status_t status = SIM_instruction_status (instr_id);
   	FE_EXCEPTION_CHECK;

	// this function not to be called on ifetch path	
	if ( (status & (Sim_IS_Stalling | Sim_IS_Ready)) == (Sim_IS_Stalling | Sim_IS_Ready) )
	   	return true;
	else 
		return false;
}

bool
mai_instruction_t::is_stalling (void) {
	if ( (SIM_instruction_status (instr_id) & Sim_IS_Stalling) 
		== Sim_IS_Stalling ) {
 		FE_EXCEPTION_CHECK;	
		return true;
	} else {
 		FE_EXCEPTION_CHECK;	
		return false;
	}
		
}

bool 
mai_instruction_t::speculative (void) {
	bool t = SIM_instruction_speculative (instr_id);
	
	FE_EXCEPTION_CHECK;
	return t;
}

uint32 
mai_instruction_t::sync_type (void) {
	uint32 ret = SIM_instruction_is_sync (instr_id);

	FE_EXCEPTION_CHECK;
	return ret;
}


bool
mai_instruction_t::branch_taken (void) {
	bool t = 	
		((SIM_instruction_status (instr_id) & 
			Sim_IS_Branch_Taken) == Sim_IS_Branch_Taken);
	
	FE_EXCEPTION_CHECK;
	return t;
}

int32
mai_instruction_t::read_input_asi () {

	attr_value_t asi = SIM_instruction_read_input_reg (instr_id, V9_Reg_Id_ASI);
	
	FE_EXCEPTION_CHECK;

	if (asi.kind != Sim_Val_Integer) {
		ASSERT_WARN (asi.kind == Sim_Val_Nil);  // not avail yet
		return -1;
	}
	
   	return asi.u.integer;
}

void mai_instruction_t::analyze_register_dependency()
{
    int32 num_src; 
    int32 num_dest;
    int32 * regs = new int32[96];
    bzero(regs, sizeof(int32) * 96);
    get_reg(regs, num_src, num_dest);
    
    for (int32 i = 0; i < num_src; i++)
        mai->update_register_read(regs[i]);
    for (int32 i = 0; i < num_dest; i++)
        mai->update_register_write(regs[RD + i]);
    
    
    
}


void
mai_instruction_t::get_reg (int32 *reg, int32 &num_src, int32 &num_dest) {
    int32 i = 0;
    reg_info_t *ri;

    num_src = num_dest = 0;

    while ( (ri = SIM_instruction_get_reg_info (instr_id, i++)) ) {
        if ( (ri->id != 0) && (ri->input)  && ((ri->id < 96) || (ri->id == (uint32)CC)) ) {
            reg [num_src] = ri->id;
            num_src ++;
        }

        if ( (ri->id != 0) && (ri->output) && ((ri->id < 96) || (ri->id == (uint32)CC)) ) {
            reg [RD + num_dest] = ri->id;
            num_dest ++;
        }
    }
}

void
mai_instruction_t::test_reg (int32 *reg, int32 num_src, int32 num_dest) {
    int32 k;
    int32 in [RD];
    int32 out [RD_NUMBER];

    /* init in/out reg to hold reg info provided by Simics */
    int32 ind_in = 0, ind_out = 0;
    for (k = 0; k < RD; k ++)
        in [k] = REG_NA;
    for (k = 0; k < RD_NUMBER; k++)
        out [k] = REG_NA;

    /* get reg info from Simics, compare with those in the input parameter */
    int32 i = 0;
    reg_info_t *ri;
    while ( (ri = SIM_instruction_get_reg_info (instr_id, i++)) ) {
        if ( (ri->id != 0) && (ri->input)  && ((ri->id < 96) || (ri->id == (uint32)CC)) ) {
            bool match = false;
            ASSERT (ind_in < RD);
            in [ind_in ++ ] = ri->id;
            for (k = 0; k < RD; k ++)
                    if ((int32)(ri->id) == reg [k])
                        match = true;
            ASSERT (match);
        }

        if ( (ri->id != 0) && (ri->output) && ((ri->id < 96) || (ri->id == (uint32)CC)) ) {
            bool match = false;
            ASSERT (ind_out < RD_NUMBER);
            out [ind_out ++ ] = ri->id;
            for (k = RD; k < RI_NUMBER; k ++)
                    if ((int32)(ri->id) == reg [k])
                        match = true;
            ASSERT (match);
        }
    }

	/* for illegal instr */
	if ((ind_in + ind_out) == 0) {
		// DEBUG_OUT ("Illegal instruction on %s? ", cpu->name); 
		// debug (0);
		return;
	}


    /* see if all valid regs match those from Simics */
    for (k = 0; k < num_src; k ++) {
        bool match = false;
        for (i = 0; i < ind_in; i ++)
            if (reg [k] == in [i])
                match = true;
        ASSERT (match);
    }

    for (k = RD; k < RD + num_dest; k ++) {
        bool match = false;
        for (i = 0; i < ind_out; i ++)
            if (reg [k] == out [i])
                match = true;
        ASSERT (match);
    }
}

void
mai_instruction_t::write_input_reg (int32 rid, uint64 value) {
	if (rid < N_GPR) {
	   	attr_value_t av;
	   	av.kind = Sim_Val_Integer;
	   	av.u.integer = value;
		
	   	SIM_instruction_write_input_reg (instr_id, (register_id_t)rid, av);
	   	FE_EXCEPTION_CHECK;
	}
}

void
mai_instruction_t::write_output_reg (int32 rid, uint64 value) {
	if (rid < N_GPR) {
	   	attr_value_t av;
	   	av.kind = Sim_Val_Integer;
	   	av.u.integer = value;
		
	   	SIM_instruction_write_output_reg (instr_id, (register_id_t)rid, av);
	   	FE_EXCEPTION_CHECK;
	}
}

/* read input-register value */
int64 
mai_instruction_t::read_input_reg (int32 rid) {
    attr_value_t v = SIM_instruction_read_input_reg (instr_id, rid);
    
    FE_EXCEPTION_CHECK;
    return v.u.integer;
}


/* read the output-register value */
int64 
mai_instruction_t::read_output_reg (int32 rid) {
	attr_value_t v;

	v = SIM_instruction_read_output_reg (instr_id, rid);
	FE_EXCEPTION_CHECK;

	return v.u.integer;
}

void mai_instruction_t::check_pc_npc (const gpc_t *gpc)
{
    attr_value_t mai_pc;
	attr_value_t mai_npc;
    mai_pc.u.integer = (int64) mai->get_pc ();
    mai_npc.u.integer = (int64) mai->get_npc ();
    if ((mai_pc.u.integer & 0xffffffff) == ((int64) gpc->pc & 0xffffffff))
    {
        mai_pc.u.integer = mai_pc.u.integer & 0xffffffff;
        SIM_instruction_write_input_reg (instr_id, V9_Reg_Id_PC, mai_pc);
    }
    
    if ((mai_npc.u.integer & 0xffffffff) == ((int64) gpc->npc & 0xffffffff))
    {
        mai_npc.u.integer = mai_npc.u.integer & 0xffffffff;
        SIM_instruction_write_input_reg (instr_id, V9_Reg_Id_NPC, mai_npc);
    }
        
    
}

bool mai_instruction_t::correct_instruction()
{
    dynamic_instr_t *d_instr = get_dynamic_instr();
    if (mai->is_32bit())
    {
        if ((d_instr->get_pc() & 0xffffffffULL) == mai->get_pc() && 
            (d_instr->get_npc() & 0xffffffffULL) == mai->get_npc())
        {
        
            SIM_instruction_force_correct(instr_id);
            return true;
        }
    }
    
    return false;
    
}

bool
mai_instruction_t::is_32bit()
{
	return mode32bit;
}

