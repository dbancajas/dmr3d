/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: fastsim.cc,v 1.1.14.1 2005/10/28 18:28:01 pwells Exp $");

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

#include "fastsim.h"


fastsim_t::fastsim_t (conf_object_t *_cpu) {
	cpu = _cpu;
	ivec = Sim_PE_No_Exception;
	in_fastsim = false;
}

fastsim_t::~fastsim_t () {
}

void 
fastsim_t::set_interrupt (int64 v) {
	ivec = v;
}

int64
fastsim_t::get_interrupt (void) {
	return ivec;
}

bool 
fastsim_t::fastsim_mode (void) {
	return in_fastsim;
}

void 
fastsim_t::sim_icount (uint64 _icount) {
	icount = _icount;
	in_fastsim = true;
}

bool
fastsim_t::sim (void) {
	instruction_id_t instr_id;
	instruction_error_t ie; 

	// check
	ASSERT (icount > 0);
	ASSERT (in_fastsim);

	icount--;

	if (!icount) {
		in_fastsim = false;
		return true;
	}

	if (ivec == Sim_PE_No_Exception) goto begin;
	ie = SIM_instruction_handle_interrupt (cpu, ivec);
	
	if (ie == Sim_IE_OK) ivec = Sim_PE_No_Exception;

begin:
	// begin
	instr_id = SIM_instruction_begin (cpu);
	ASSERT (instr_id);

	SIM_instruction_insert (0, instr_id);

	// fetch
	ie = SIM_instruction_fetch (instr_id);
	if (ie == Sim_IE_Exception) goto sim_handle_exception;
	ASSERT (ie == Sim_IE_OK);

	// decode
	ie = SIM_instruction_decode (instr_id);
	if (ie == Sim_IE_Exception) goto sim_handle_exception;
	ASSERT (ie == Sim_IE_OK);

	// execute
	ie = SIM_instruction_execute (instr_id);
	if (ie == Sim_IE_Exception) goto sim_handle_exception;
	ASSERT (ie == Sim_IE_OK);

	// retire
	ie = SIM_instruction_retire (instr_id);
	if (ie == Sim_IE_Exception) goto sim_handle_exception;
	ASSERT (ie == Sim_IE_OK);

	// commit
	ie = SIM_instruction_commit (instr_id);
	ASSERT (ie == Sim_IE_OK);

	goto sim_end;
	
sim_handle_exception:
	SIM_instruction_handle_exception (instr_id); // fall-through

sim_end:
	SIM_instruction_end (instr_id);
	
	return false;
}

void 
fastsim_t::debug (instruction_id_t instr_id) {
	attr_value_t pc;

	pc = SIM_instruction_read_input_reg (instr_id, V9_Reg_Id_PC);
	pr ("%s\n", (SIM_disassemble (cpu, pc.u.integer, 1))->string);
}


