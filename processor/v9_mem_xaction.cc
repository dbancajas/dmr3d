/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: v9_mem_xaction.cc,v 1.2.8.3 2006/07/28 01:30:06 pwells Exp $");

// Needed for is_unsafe_alt(), before definitions.h
#define TARGET_ULTRA

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
#include "config_extern.h"

v9_mem_xaction_t::v9_mem_xaction_t (mai_t *_mai, 
    generic_transaction_t *_mem_op, 
    conf_object_t *_space, 
    map_list_t *_map) : mai (_mai), mem_op (_mem_op), 
    space (_space), map (_map) {

    cpu = mai->get_cpu_obj ();
}

v9_mem_xaction_t::~v9_mem_xaction_t () {
}

void
v9_mem_xaction_t::release_stall () {
	ASSERT (mem_op);
	SIM_release_stall (cpu, mem_op->id);
	FE_EXCEPTION_CHECK;
}

void
v9_mem_xaction_t::write_int64_phys_memory (addr_t paddr, 
	uint64 value, 
	uint32 size,
	bool ie) {
	FE_EXCEPTION_CHECK;

	ASSERT (size <= sizeof (uint64));
	/*   Hmmmmm.. woulda thought this was necessary, but doesn't seem to be
	if (ie) {
		switch(size) {
		case 1:
			break;
		case 2:
			value = SWAB16(value);
			break;
		case 4:
			value = SWAB32(value);
			break;
		case 8:
			value = SWAB64(value);
			break;
		default:
			FAIL;
		}
	}
	*/

	SIM_write_phys_memory (cpu, paddr, value, size);

//	ASSERT (SIM_read_phys_memory (cpu, paddr, size) == value);

    FE_EXCEPTION_CHECK;
}

void
v9_mem_xaction_t::write_blk_phys_memory (addr_t paddr, 
	uint8 *value, uint32 size, bool ie)
{
	ASSERT(size == 64);
	FE_EXCEPTION_CHECK;
	for (uint32 i = 0; i < size; i++) {
		// XXX double check endianess for these...
		SIM_write_phys_memory(cpu, paddr+i, value[i], 1);
	}
}

uint64 
v9_mem_xaction_t::get_int64_value () {
	ASSERT (mem_op);
    ASSERT (mem_op->size <= sizeof (uint64));
    uint64 value = SIM_get_mem_op_value_cpu (mem_op);

    FE_EXCEPTION_CHECK;
    return value;
}

void
v9_mem_xaction_t::get_blk_value (uint8 *value) {
	ASSERT (mem_op);
    ASSERT (mem_op->size == 64);

    SIM_c_get_mem_op_value_buf(mem_op, (char *)value);

    FE_EXCEPTION_CHECK;
}

uint32
v9_mem_xaction_t::get_size () {
	ASSERT (mem_op);
	return (mem_op->size);
}

addr_t
v9_mem_xaction_t::get_phys_addr () {
	ASSERT (mem_op);
	return (mem_op->physical_address);
}

addr_t
v9_mem_xaction_t::get_virt_addr () {
	ASSERT (mem_op);
	return (mem_op->logical_address);
}

void 
v9_mem_xaction_t::block_stc () {
	ASSERT (mem_op);
	mem_op->block_STC = 1;
}

void 
v9_mem_xaction_t::squashed_noreissue () {
	ASSERT (mem_op);
	mem_op->ma_no_reissue = 1;
}

bool 
v9_mem_xaction_t::is_cpu_xaction () {
	ASSERT (mem_op);
	bool cpu_mem_op = SIM_mem_op_is_from_cpu (mem_op);

	FE_EXCEPTION_CHECK;
	return cpu_mem_op;
}

bool 
v9_mem_xaction_t::is_icache_xaction () {
	ASSERT (mem_op);
	bool instr = SIM_mem_op_is_instruction (mem_op);

	FE_EXCEPTION_CHECK;
	return instr;
}

dynamic_instr_t*
v9_mem_xaction_t::get_dynamic_instr () {
	instruction_id_t instr_id;
	dynamic_instr_t *d_instr = 0;

	ASSERT (mem_op);
	instr_id = SIM_instruction_id_from_mem_op_id (cpu, mem_op->id);

//  XXX assert disabled to take care of simics bug
//	ASSERT (instr_id);

	if (instr_id)
		d_instr = static_cast <dynamic_instr_t *>
			( SIM_instruction_get_user_data (instr_id) );
	else
		SIM_clear_exception ();
	
	FE_EXCEPTION_CHECK;

	return d_instr;
}

bool 
v9_mem_xaction_t::is_store () {
	ASSERT (mem_op);
	return (mem_op->type == Sim_Trans_Store);
}

bool
v9_mem_xaction_t::is_atomic () {
	ASSERT (mem_op);
	return (mem_op->atomic);
}

void 
v9_mem_xaction_t::void_xaction () {
	ASSERT (mem_op);
	mem_op->ignore = 1;
}

generic_transaction_t*
v9_mem_xaction_t::get_mem_op () {
	return mem_op;
}

v9_memory_transaction_t*
v9_mem_xaction_t::get_v9_mem_op () {
	return ( (v9_memory_transaction_t *) (mem_op) );
}

bool
v9_mem_xaction_t::is_side_effect () {
	ASSERT (mem_op);
	return ( get_v9_mem_op ()->side_effect );
}

void
v9_mem_xaction_t::set_mem_op (generic_transaction_t *_mem_op) {
	mem_op = _mem_op;
}

bool
v9_mem_xaction_t::can_stall () {
	ASSERT (mem_op);
	return (mem_op->may_stall == 1);
}

void
v9_mem_xaction_t::set_user_ptr (void *ptr) {
	ASSERT (mem_op);

	mem_op->user_ptr = ptr;
}

void*
v9_mem_xaction_t::get_user_ptr () {
	ASSERT (mem_op);

	return (mem_op->user_ptr);
}

void
v9_mem_xaction_t::set_bypass_value (uint64 val) {
	ASSERT (mem_op);

	SIM_set_mem_op_value_cpu (mem_op, val);
	FE_EXCEPTION_CHECK;
}

void
v9_mem_xaction_t::set_bypass_blk_value (uint8 *val) {
	ASSERT (mem_op);
	ASSERT (mem_op->size == 64);

	SIM_c_set_mem_op_value_buf(mem_op, (char *)val);
	FE_EXCEPTION_CHECK;
}

bool
v9_mem_xaction_t::is_speculative () {
	ASSERT (mem_op);

	return (mem_op->speculative);
}

bool
v9_mem_xaction_t::unimpl_checks () {
	if (g_conf_asi_ie_safe)
		return mem_op->page_cross;
	else
		return (mem_op->inverse_endian || 
				mem_op->page_cross);
}


bool
v9_mem_xaction_t::is_unsafe_asi(int64 asi) {
	
	// TODO:  QUAD LDD
	
	switch (asi) {
	case -1:  // initialized value
		return true;
		
	case ASI_NUCLEUS:
	case ASI_NUCLEUS_LITTLE:
	case ASI_PRIMARY:
	case ASI_PRIMARY_NOFAULT:
	case ASI_PRIMARY_LITTLE:
	case ASI_PRIMARY_NOFAULT_LITTLE:
	case ASI_FL8_P:
	case ASI_FL16_P:
		return false;
		
	case ASI_AS_IF_USER_PRIMARY:
	case ASI_AS_IF_USER_SECONDARY:
	case ASI_AS_IF_USER_PRIMARY_LITTLE:
	case ASI_AS_IF_USER_SECONDARY_LITTLE:
	case ASI_SECONDARY:
	case ASI_SECONDARY_NOFAULT:
	case ASI_SECONDARY_LITTLE:
	case ASI_SECONDARY_NOFAULT_LITTLE:
	case ASI_FL8_S:
	case ASI_FL16_S:
	case ASI_PHYS_USE_EC:
		return !(g_conf_asi_sec_safe);

	case ASI_BLK_P:
		return !(g_conf_asi_blk_safe);

	case ASI_BLK_AIUP:
	case ASI_BLK_AIUS:
	case ASI_BLK_AIUPL:
	case ASI_BLK_AIUSL:
	case ASI_BLK_COMMIT_P:
	case ASI_BLK_COMMIT_S:
	case ASI_BLK_S:
	case ASI_BLK_PL:
	case ASI_BLK_SL:
		return !(g_conf_asi_sec_safe && g_conf_asi_blk_safe);
	
	default:
		return true;
	}
	
}

