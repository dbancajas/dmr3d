/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: mem_xaction.cc,v 1.4.2.15 2006/11/14 20:02:07 pwells Exp $");

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
#include "mem_hier_handle.h"
#include "transaction.h"
#include "config_extern.h"


const uint32 BYTEMASK[4][8] = {
	{0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80},
	{0x03, 0x00, 0x0c, 0x00, 0x30, 0x00, 0xc0, 0x00},
	{0x0f, 0x00, 0x00, 0x00, 0xf0, 0x00, 0x00, 0x00},
	{0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

const uint32 SIZEINDEX[9] = {
	0, GRAN_BYTE /* 1 */, GRAN_HALF /* 2 */, 0, GRAN_WORD /* 4 */, 0, 0, 0, GRAN_QUAD /* 8 */
};
  	

mem_xaction_t::mem_xaction_t (v9_mem_xaction_t *_v9, mem_trans_t *_trans) 
{
    v9_mem_xaction = _v9;

	v9_mem_xaction->set_user_ptr ( static_cast<void *> (this) );

    mem_hier_trans = _trans;
	second_ldd_memop = false;
    
	if (v9_mem_xaction->is_icache_xaction ()) {
		// icache
		set_phys_addr (v9_mem_xaction->get_phys_addr ());
		set_virt_addr (v9_mem_xaction->get_virt_addr());
#if 0
		set_phys_addr (0);
		set_virt_addr (0);
#endif
		set_size (0);

		dynamic_instr_t *d_instr = v9_mem_xaction->get_dynamic_instr ();
		//dynamic_instr_t *seq_d_instr = sequencer_t::icache_d_instr;
        //if (d_instr != seq_d_instr && seq_d_instr) d_instr = seq_d_instr;
        if (!d_instr) 
			d_instr = sequencer_t::icache_d_instr;

		ASSERT (d_instr);

		value.initialize ();
		bypass_value.initialize ();
		blk_bypass_valid = false;

		mem_hier_trans->copy(v9_mem_xaction->get_v9_mem_op ());
		mem_hier_trans->set_dinst(d_instr);
		
		void_write = false;
		dep_instr = 0;
		snoop_status = SNOOP_STATUS;

		dw_phys_addr = 0;
		blk_phys_addr = 0;
		dw_bytemask = 0;

	} else {
		// data
		set_phys_addr (v9_mem_xaction->get_phys_addr ());
		set_virt_addr (v9_mem_xaction->get_virt_addr ());
		set_size (v9_mem_xaction->get_size ());
		
		dynamic_instr_t *d_instr = v9_mem_xaction->get_dynamic_instr ();

		if (!d_instr) 
			d_instr = sequencer_t::icache_d_instr;

		if (d_instr->is_ldd_std() && !d_instr->is_unsafe_alt())
		{
			ASSERT(size == 4);
			set_size(8);
			// Need to be alligned, so must be second trans
			if (get_phys_addr() & 0x4) {
				ASSERT(!v9_mem_xaction->get_mem_op()->may_stall);
				set_phys_addr(get_phys_addr() - 4);
				set_virt_addr(get_virt_addr() - 4);
			}
		}

		value.initialize ();
		bypass_value.initialize ();
		blk_bypass_valid = false;

		if (d_instr->is_store ()) {
			if (get_size () <= sizeof (uint64))
				value.set (v9_mem_xaction->get_int64_value ());
			else {
				ASSERT(get_size() == 64);
				v9_mem_xaction->get_blk_value(blk_values);
			}
		}

		// mem hier transaction setup
		mem_hier_trans->copy(v9_mem_xaction->get_v9_mem_op ());
		mem_hier_trans->set_dinst(d_instr);
		
		void_write = false;	
		dep_instr = 0;
		snoop_status = SNOOP_STATUS;

		if (size <= sizeof (uint64)) {
			uint32 size_index = SIZEINDEX [size];
			dw_phys_addr = phys_addr >> 3;
			dw_bytemask = BYTEMASK [size_index][phys_addr & 0x7];
			blk_phys_addr = phys_addr >> 8;
	
			ASSERT (dw_bytemask != 0);
		} else {
			// greater than 8 bytes
			ASSERT(size == 64);
			blk_phys_addr = phys_addr >> 8;
			dw_phys_addr = phys_addr >> 3;
			dw_bytemask = 0;
		}
	}
    
    mem_hier_trans->mark_pending(MEM_XACTION_CREATE);
    mem_hier_trans->pending_messages++;
    
}

//XXX
void
mem_xaction_t::merge_changes (mem_xaction_t *m) {

	dep_instr = m->get_dependent ();

	if (m->is_bypass_valid ()) {
		if (get_size() <= sizeof(uint64))
			set_bypass_value (m->get_bypass_value ());
		else 
			set_bypass_blk_value (m->get_bypass_blk_value ());
	}			

	snoop_status = m->get_snoop_status ();
	if (m->void_write) void_xaction();
	
	dynamic_instr_t *d_instr = v9_mem_xaction->get_dynamic_instr ();
	if (g_conf_ldd_std_safe && d_instr && d_instr->is_store() &&
		!d_instr->is_unsafe_alt() && d_instr->is_ldd_std())
	{
		if (!m->value.is_valid()) {
			// First trans, set high 4 bytes
			ASSERT(v9_mem_xaction->get_mem_op()->may_stall);
			uint64 newval = value.get() << 32;
			value.set(newval);
		} else if (v9_mem_xaction->get_phys_addr() == get_phys_addr()) {
			// first trans reissued
			ASSERT(v9_mem_xaction->get_mem_op()->may_stall);
			//ASSERT(value.get() << 32 == m->value.get());
			value.set(m->value.get());
		} else {
			// Second trans, set low order bytes
			ASSERT(!v9_mem_xaction->get_mem_op()->may_stall);
			ASSERT(m->get_phys_addr() == get_phys_addr());  // set in constr.
			ASSERT(v9_mem_xaction->get_phys_addr() == get_phys_addr() + 4);
			ASSERT(size == 8);

			uint64 newval = m->value.get() | value.get();
			value.set(newval);
		}
	}
	
	second_ldd_memop = m->second_ldd_memop;

	if (snoop_status == SNOOP_BYPASS_COMPLETE ||
		snoop_status == SNOOP_BYPASS_EXTND) {
//		safety checks
		ASSERT (get_size () == m->get_size ());
		ASSERT (get_phys_addr () == m->get_phys_addr ());
	}
	
	mem_hier_trans->merge_changes(m->mem_hier_trans);
}

mem_xaction_t::~mem_xaction_t () {
    delete v9_mem_xaction;
    ASSERT(mem_hier_trans);
    mem_hier_trans->clear_pending_event(MEM_XACTION_CREATE);
	ASSERT(mem_hier_trans->pending_messages);
	mem_hier_trans->pending_messages--;
    if (!mem_hier_trans->pending_messages) mem_hier_trans->completed = true;
}

void 
mem_xaction_t::set_size (uint32 _s) {
	size = _s;
}

void 
mem_xaction_t::set_phys_addr (addr_t _a) {
	phys_addr = _a;
}

void 
mem_xaction_t::set_virt_addr (addr_t _v) {
	virt_addr = _v;
}

uint64
mem_xaction_t::get_value (void) {
	return ( value.get () );
}

uint8 *
mem_xaction_t::get_blk_value (void) {
	return ( blk_values );
}

void
mem_xaction_t::set_value (uint64 val) {
	value.set (val);
}

bool
mem_xaction_t::is_value_valid (void) {
	return ( value.is_valid () );
}


void
mem_xaction_t::set_bypass_value (uint64 val) {
	bypass_value.set (val);
}

void
mem_xaction_t::set_bypass_blk_value (uint8 *val) {
	blk_bypass_valid = true;
	memcpy(bypass_blk_values, val, get_size());
}

uint64
mem_xaction_t::get_bypass_value (void) {
	return ( bypass_value.get () );
}

uint8 *
mem_xaction_t::get_bypass_blk_value (void) {
	return ( bypass_blk_values );
}

bool
mem_xaction_t::is_bypass_valid (void) {
	if (get_size() > sizeof(uint64))
		return blk_bypass_valid;
	else
		return ( bypass_value.is_valid () );
}

uint32
mem_xaction_t::get_size (void) {
	return size;
}

uint32
mem_xaction_t::get_size_index (void) {
	uint32 index;

	if (size <= sizeof (uint64)) 
		index = SIZEINDEX [size];
	else
		index = 4;

	return index;
}

addr_t
mem_xaction_t::get_phys_addr (void) {
	return phys_addr;
}

addr_t
mem_xaction_t::get_virt_addr (void) {
	return virt_addr;
}

v9_mem_xaction_t*
mem_xaction_t::get_v9 (void) {
	return v9_mem_xaction;
}

void
mem_xaction_t::set_v9 (v9_mem_xaction_t *_v9) {
	if (_v9 != v9_mem_xaction)
		delete v9_mem_xaction;
	
	v9_mem_xaction = _v9;
	
	FAIL;
	// invalid function
}

void 
mem_xaction_t::release_stall (void) {
	v9_mem_xaction->release_stall ();	
}

void 
mem_xaction_t::apply_store_changes (void) {
	ASSERT (is_void_xaction () == true);

//	if (mem_hier_trans->inverse_endian)
//		WARNING;

	if (get_size() <= sizeof(uint64)) {
		v9_mem_xaction->write_int64_phys_memory (
			get_phys_addr (), get_value (), get_size (), 
			mem_hier_trans->inverse_endian);
	} else {
		ASSERT(get_size() == 64);
		v9_mem_xaction->write_blk_phys_memory (
			get_phys_addr (), get_blk_value (), get_size (), 
			mem_hier_trans->inverse_endian);
	}
}

void
mem_xaction_t::void_xaction (void) {
	v9_mem_xaction->void_xaction ();
	void_write = true;
}

bool
mem_xaction_t::is_void_xaction (void) {
	return void_write;
}

void
mem_xaction_t::safety_checks (dynamic_instr_t *d_instr) {
	ASSERT (v9_mem_xaction);
	ASSERT (v9_mem_xaction->get_mem_op ());

	// if may_stall = 0, the instruction should be immediate release store
	if (!v9_mem_xaction->can_stall () && d_instr->is_store ()) 
		ASSERT (d_instr->immediate_release_store () ||
			(g_conf_ldd_std_safe && d_instr->is_ldd_std()));
}

void
mem_xaction_t::set_dependent (dynamic_instr_t *d_instr) {
	dep_instr = d_instr;
}

dynamic_instr_t*
mem_xaction_t::get_dependent (void) {

	return dep_instr;
}


uint64
mem_xaction_t::munge_bypass (uint32 source_mask, 
	uint32 dest_mask, uint64 value) {

	while ( (source_mask & 0x80) == 0x0 ) {
		source_mask = source_mask << 1;
		dest_mask = dest_mask << 1;
	}

	ASSERT ( (source_mask & 0x80) == 0x80 );

	while ( (dest_mask & 0x80) == 0x0 ) {
		value = value >> 8; 
		dest_mask = dest_mask << 1;
		source_mask = source_mask << 1;
	}

	switch (get_size ()) {
	case sizeof (uint8):
		value = (uint8) value;
		break;
	case sizeof (uint16):
		value = (uint16) value; 
		break;
	case sizeof (uint32):
		value = (uint32) value;
		break;
	case sizeof (uint64):
		value = (uint64) value;
		break;
	default:
		FAIL;
	}
	

	return value;
}



void
mem_xaction_t::obtain_bypass_val (void) {
	ASSERT (dep_instr);
	ASSERT (dw_phys_addr != 0);
	
	mem_xaction_t *mem_st = dep_instr->get_mem_transaction ();
	// obtaining bypass value from unimpl st/ld dangerous
	if (mem_st->unimpl_checks () || unimpl_checks ()) 
		WARNING;
	
	// BLK ld/st
	if (get_size() > sizeof(uint64)) {
		ASSERT(get_size() == 64);
		set_snoop_status (SNOOP_BYPASS_COMPLETE);
		set_bypass_blk_value(mem_st->get_blk_value());
		return;
	}

	uint64       st_value = mem_st->get_value ();
	uint64 bypass;

	if (mem_st->get_dw_bytemask () != get_dw_bytemask ()) {
		// extended bypass 		
		bypass = munge_bypass (mem_st->get_dw_bytemask (), get_dw_bytemask (), st_value);
		set_snoop_status (SNOOP_BYPASS_EXTND);
	} else {
		// complete bypass
		bypass = st_value;
		set_snoop_status (SNOOP_BYPASS_COMPLETE);
	}

	set_bypass_value (bypass);		
}

mem_trans_t*
mem_xaction_t::get_mem_hier_trans (void) {
	return mem_hier_trans;
}

addr_t
mem_xaction_t::get_dw_addr (void) {
	return dw_phys_addr;
}

addr_t
mem_xaction_t::get_blk_addr (void) {
	return blk_phys_addr;
}

uint32
mem_xaction_t::get_dw_bytemask (void) {
	return dw_bytemask;
}

void
mem_xaction_t::apply_bypass_value (void) {
	ASSERT (is_bypass_valid ());
	
	if (mem_hier_trans->dinst->is_ldd_std()) {
		uint64 val;  
		if (!second_ldd_memop) // high bits
			val = get_bypass_value() >> 32;
		else // low bits 
			val = get_bypass_value() & 0xffffffffULL;
		v9_mem_xaction->set_bypass_value (val);
		
		//mem_hier_trans->dinst->print("apply_bypass_value");
		second_ldd_memop = true;
		return;
	}
	
	if (get_size() <= sizeof(uint64)) 
		v9_mem_xaction->set_bypass_value (get_bypass_value ());
	else
		v9_mem_xaction->set_bypass_blk_value (get_bypass_blk_value ());

//	ASSERT (v9_mem_xaction->get_int64_value () == get_bypass_value ());
}

void
mem_xaction_t::set_snoop_status (snoop_status_t status) {
	snoop_status = status;
}

snoop_status_t
mem_xaction_t::get_snoop_status () {
	return snoop_status;
}

bool
mem_xaction_t::unimpl_checks () {
	if (g_conf_asi_ie_safe)
		return mem_hier_trans->page_cross;
	else
		return (mem_hier_trans->inverse_endian || 
				mem_hier_trans->page_cross);
}

void
mem_xaction_t::setup_store_prefetch () {
	ASSERT (mem_hier_trans);

	mem_hier_trans->call_complete_request = 0;
	mem_hier_trans->sw_prefetch = 0;
	mem_hier_trans->hw_prefetch = 1;
    mem_hier_trans->prefetch_type = LDST_PREFETCH;
}

void
mem_xaction_t::setup_regular_access () {
	ASSERT (mem_hier_trans);

	mem_hier_trans->call_complete_request = 1;
	mem_hier_trans->sw_prefetch = 0;
	mem_hier_trans->hw_prefetch = 0;
    mem_hier_trans->prefetch_type = NO_PREFETCH;
}

