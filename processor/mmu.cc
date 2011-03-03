/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    mmu
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: mmu.cc,v 1.1.2.3 2006/03/20 22:50:32 pwells Exp $");

// Needed for ASI #defs
// TODO: Probably a better way to do this
#ifndef TARGET_ULTRAII
	#define TARGET_ULTRAII
 	#define TARGET_ULTRA
#endif

#include "definitions.h"
#include "mmu.h"
#include "chip.h"
#include "mai.h"
#include "startup.h"
#include "verbose_level.h"
#include "ms2_mmu_iface.h"
#include "mai_instr.h"
#include "isa.h"
#include "dynamic.h"
#include "sequencer.h"
#include "proc_stats.h"
#include "stats.h"
#include "counter.h"
#include "config_extern.h"

#define VERBOSE_MMU(_fname, _memop, ...) \
    VERBOSE_OUT(verb_t::mmu, "0x%016llx: " _fname "\n", \
    (_memop) ? _memop->s.logical_address : 0x0, \
	##__VA_ARGS__)

mmu_info_t::mmu_info_t () :
	virt_addr (0), phys_addr (0), cache_phys (false), cache_virt (false),
	side_effect (false), inv_endian_flip (false), except (0)
{ }
	
mmu_info_t::mmu_info_t (v9_memory_transaction_t *mem_op, bool orig_inv_endian,
	exception_type_t ex)
	: virt_addr (mem_op->s.logical_address),
	  phys_addr (mem_op->s.physical_address),
	  cache_phys (mem_op->cache_physical),
	  cache_virt (mem_op->cache_virtual),
	  side_effect (mem_op->side_effect),
	  inv_endian_flip (mem_op->s.inverse_endian ^ orig_inv_endian),
	  except (ex)
{ }

mmu_t::mmu_t (sequencer_t *_seq, uint32 _id) 
	: seq (_seq), thread_id(_id)
{ 
	int mm_id = -1; 
	mmu_fill_op = MM_ZALLOC (1, v9_memory_transaction_t);
	mmu_inq_op = MM_ZALLOC (1, v9_memory_transaction_t);
}

void
mmu_t::fill_transaction (v9_memory_transaction_t *mem_op, mmu_info_t *mmui)
{
	if (mmui->virt_addr == mem_op->s.logical_address) { 
		mem_op->s.physical_address = mmui->phys_addr;
	} else {
		// handle ldd/std
		// same 64 bytes
		ASSERT (mem_op->s.type != Sim_Trans_Instr_Fetch);
		ASSERT ((mem_op->s.logical_address >> 6) == (mmui->virt_addr >> 6));
		
		mem_op->s.physical_address = mmui->phys_addr +
			(mem_op->s.logical_address - mmui->virt_addr);
	}
	mem_op->cache_physical = mmui->cache_phys;
	mem_op->cache_virtual = mmui->cache_virt;
	mem_op->side_effect = mmui->side_effect;
	mem_op->s.inverse_endian ^= mmui->inv_endian_flip;
}

void
mmu_t::set_simics_mmu (conf_object_t *_simics_mmu) {
	simics_mmu_obj = _simics_mmu;
	simics_mmu_iface = 
		(mmu_interface_t *) SIM_get_interface (simics_mmu_obj, "mmu");
	simics_mmu_asi_iface = 
		(mmu_asi_interface_t *) SIM_get_interface (simics_mmu_obj, "asi");
	cpu_public_iq_iface = (public_iq_interface_t *)
		SIM_get_interface (seq->get_mai_object (thread_id)->get_cpu_obj (),
			PUBLIC_IQ_INTERFACE);
}
	

exception_type_t
mmu_t::do_access (v9_memory_transaction_t *mem_op) {
	VERBOSE_MMU("mmu do access", mem_op);
	ASSERT (mem_op->s.ini_ptr == seq->get_mai_object (thread_id)->get_cpu_obj ());
	
	exception_type_t ex;
	dynamic_instr_t *dinst = sequencer_t::mmu_d_instr;
	mai_instruction_t *mai_inst = (dinst) ?
		dinst->get_mai_instruction () : NULL;
	
	bool ifetch = (mem_op->s.type == Sim_Trans_Instr_Fetch);
	bool inv_endian_orig = mem_op->s.inverse_endian;
	
	proc_stats_t *pstats = seq->get_pstats (0);

	if (!dinst) {
		ASSERT (mem_op->s.inquiry);
		// Don't do anything special
		ex = simics_mmu_iface->logical_to_physical (simics_mmu_obj, mem_op);
	}
	else if ((ifetch && mai_inst->fetch_mmu_info) || mai_inst->ldst_mmu_info) {
		// We've already done this transaction
		ex = handle_mmu_replay (mem_op, dinst);
	} 
	else {
		// First translation for this transaction
		ex = simics_mmu_iface->logical_to_physical (simics_mmu_obj, mem_op);
		
		// If miss, try HW fill and translate again
		if (g_conf_mmu_hw_fill) {
			if (ex == Fast_Instruction_Access_MMU_Miss ||
				ex == Fast_Data_Access_MMU_Miss)
			{
				hw_fill_from_linux_pt (mem_op);
				ex = simics_mmu_iface->logical_to_physical (simics_mmu_obj,
					mem_op);
				
				if (ex == Sim_PE_No_Exception) {
					if (ifetch) STAT_INC (pstats->stat_immu_hw_fill);
					else STAT_INC (pstats->stat_dmmu_hw_fill);
				}
			}
		}
		
		// Save translation for this instruction
		if (ifetch) {
			ASSERT (!mai_inst->fetch_mmu_info);
			mai_inst->fetch_mmu_info = new mmu_info_t (mem_op, inv_endian_orig,
				ex);

			ASSERT ((mai_inst->fetch_mmu_info->virt_addr & 0xffffffff) == 
				(dinst->get_pc () & 0xffffffff));

			if (ex == Sim_PE_No_Exception) STAT_INC (pstats->stat_immu_hit);
			else STAT_INC (pstats->stat_immu_miss);
		}
		else {
			ASSERT (!mai_inst->ldst_mmu_info);
			ASSERT (mai_inst->fetch_mmu_info);

			mai_inst->ldst_mmu_info = new mmu_info_t (mem_op, inv_endian_orig,
				ex);
			
			if (ex == Sim_PE_No_Exception) STAT_INC (pstats->stat_dmmu_hit);
			else STAT_INC (pstats->stat_dmmu_miss);
		}
	}
	
	ASSERT (ex != Sim_PE_No_Exception || 
		(mem_op->s.logical_address & ~(~0<<13)) ==
		(mem_op->s.physical_address & ~(~0<<13)));
	
	return ex;
}

exception_type_t
mmu_t::handle_mmu_replay (v9_memory_transaction_t *mem_op,
	dynamic_instr_t *dinst)
{
	proc_stats_t *pstats = seq->get_pstats (0);

	exception_type_t ex;
	bool ifetch = SIM_mem_op_is_instruction(&mem_op->s);
	mai_instruction_t *mai_inst = dinst->get_mai_instruction ();

	if (ifetch) {
		ASSERT (mai_inst->fetch_mmu_info);
		ASSERT (mai_inst->fetch_mmu_info->virt_addr ==
			mem_op->s.logical_address);
		ASSERT ((mai_inst->fetch_mmu_info->virt_addr & 0xffffffff) == 
			(dinst->get_pc () & 0xffffffff));
		
		ex = mai_inst->fetch_mmu_info->except;
		if (ex == Sim_PE_No_Exception || dinst->speculative ()) {
			fill_transaction (mem_op, mai_inst->fetch_mmu_info);
			STAT_INC (pstats->stat_immu_bypass);
		} else {
			// If an exception first time, call the MMU again if we are now
			// non-spec to set fault regs
			ex = simics_mmu_iface->logical_to_physical (simics_mmu_obj, mem_op);
			ASSERT (ex == mai_inst->fetch_mmu_info->except);
		}
	}
	else { 
		ASSERT (mai_inst->fetch_mmu_info);
		ASSERT (mai_inst->ldst_mmu_info);
		ASSERT (mai_inst->ldst_mmu_info->virt_addr ==
			mem_op->s.logical_address ||
			dinst->get_opcode () == i_ldd ||
			dinst->get_opcode () == i_ldda ||
			dinst->get_opcode () == i_std ||
			dinst->get_opcode () == i_stda);
			
		ex = mai_inst->ldst_mmu_info->except;
		if (ex == Sim_PE_No_Exception || dinst->speculative ()) {
			fill_transaction (mem_op, mai_inst->ldst_mmu_info);
			STAT_INC (pstats->stat_dmmu_bypass);
		} else {
			// If an exception first time, call the MMU again if we are now
			// non-spec to set fault regs
			ex = simics_mmu_iface->logical_to_physical (simics_mmu_obj, mem_op);
			ASSERT (ex == mai_inst->ldst_mmu_info->except);
		}
	}
	return ex;
}

inline int64
sign_ext64 (int64 w, int high_bit) {
	int msb = (w >> high_bit) & 0x1;
	int64 result = 0;

	if (msb) 
		result = ((~result << (high_bit + 1)) | w);
	else 
		result = w;
	
	return result;
}

void
mmu_t::hw_fill_from_linux_pt (v9_memory_transaction_t *mem_op)
{
	static const addr_t linux_pt_pointer = 0xfffffffe00000000ULL;

	addr_t pt_offset;
	addr_t pte_addr;
	v9_memory_transaction_t *pte_mem_op;
	
	pt_offset = (mem_op->s.logical_address >> 13) << 3;
	pt_offset = sign_ext64 (pt_offset, 53);
	pte_addr = linux_pt_pointer + pt_offset;

	VERBOSE_OUT (verb_t::mmu, "PTE: 0x%llx ", pte_addr);
	
	exception_type_t ex;
	uint8 asi = SIM_mem_op_is_instruction(&mem_op->s) ? 
		ASI_PRIMARY : ASI_SECONDARY;

	pte_mem_op = fill_inquiry_transaction (pte_addr, asi);
	ex = simics_mmu_iface->logical_to_physical (simics_mmu_obj, pte_mem_op);
	
	if (ex != Sim_PE_No_Exception) {
		VERBOSE_OUT (verb_t::mmu, "PTE miss 0x%x\n", ex);
		return;
	}
	
	// Read phys mem and check if valid
	uint64 pte = SIM_read_phys_memory (seq->get_mai_object (thread_id)->get_cpu_obj (),
		pte_mem_op->s.physical_address, 8);

	if (!(pte & 0x8000000000000000ULL)) {
		VERBOSE_OUT (verb_t::mmu, "Invalid PTE\n");
		return;
	}

	VERBOSE_OUT (verb_t::mmu, "Valid PTE PA: 0x%llx PTE: 0x%llx\n",
		pte_mem_op->s.physical_address, pte);
	
	// Store to TLB
	insert_tlb_entry (mem_op, pte);
}

v9_memory_transaction_t *
mmu_t::fill_inquiry_transaction (addr_t addr, uint8 asi)
{
	mmu_inq_op->address_space = asi;
	mmu_inq_op->s.logical_address = addr;
	mmu_inq_op->s.size = 1;
	mmu_inq_op->s.inquiry = 1;
	mmu_inq_op->s.type = Sim_Trans_Load;
	mmu_inq_op->s.ini_type = Sim_Initiator_Other;
	mmu_inq_op->priv = 1;

	return mmu_inq_op;
}

// Emulate a SW TLB fill
//   mem_op is transaction of missing address
//   pte is data to be stored into tlb for that address
void
mmu_t::insert_tlb_entry (v9_memory_transaction *mem_op, uint64 pte)
{
	// Run original miss through non-spec to set MMU fault regs
	exception_type_t ex;
	bool orig_spec = mem_op->s.speculative;
	mem_op->s.speculative = 0;

	// Hack to get mmu to actually set the registers
	void (*orig_mmu_save_error_info)(public_instruction_t *, lang_void *);
	orig_mmu_save_error_info = cpu_public_iq_iface->mmu_save_error_info;
	cpu_public_iq_iface->mmu_save_error_info = 0;
	
	ex = simics_mmu_iface->logical_to_physical (simics_mmu_obj, mem_op);
	ASSERT (ex == Fast_Instruction_Access_MMU_Miss ||
		ex == Fast_Data_Access_MMU_Miss);
	
	mem_op->s.speculative = orig_spec;
	cpu_public_iq_iface->mmu_save_error_info = orig_mmu_save_error_info;

	// Emulate SW fill transaction 
	uint8 asi = SIM_mem_op_is_instruction(&mem_op->s) ? 
		ASI_ITLB_DATA_IN_REG : ASI_DTLB_DATA_IN_REG;
	
	// VA is supposed to be 0x0 and data is the op's store data
	// But can't assign data to a mem_op without an instruction,
	// so flag TLB to look in user_ptr for data instead
	mmu_fill_op->s.logical_address = 0x1;
	mmu_fill_op->address_space = asi;
	mmu_fill_op->s.size = 8;
	mmu_fill_op->s.type = Sim_Trans_Store;
	mmu_fill_op->s.inverse_endian = 0;
	mmu_fill_op->s.ini_type = Sim_Initiator_Other;
	mmu_fill_op->priv = 1;
	mmu_fill_op->s.user_ptr = (void *) &pte;
	
	handle_asi (mmu_fill_op);
}

int
mmu_t::current_context (conf_object_t *cpu) {
	ASSERT (cpu == seq->get_mai_object (thread_id)->get_cpu_obj ());
	return simics_mmu_iface->get_current_context (simics_mmu_obj, cpu);
}

void
mmu_t::reset (conf_object_t *cpu, exception_type_t exc_no) {
	ASSERT (cpu == seq->get_mai_object (thread_id)->get_cpu_obj ());
	simics_mmu_iface->cpu_reset (simics_mmu_obj, cpu, exc_no);
}

exception_type_t
mmu_t::undefined_asi (v9_memory_transaction_t *mem_op) {
	ASSERT (mem_op->s.ini_ptr == seq->get_mai_object (thread_id)->get_cpu_obj ());
	return simics_mmu_iface->undefined_asi (simics_mmu_obj, mem_op);
}

exception_type_t
mmu_t::set_error (conf_object_t *cpu_obj, exception_type_t ex, uint64 addr,
	int ft, int asi, read_or_write_t r_or_w, data_or_instr_t d_or_i, int atomic,
	int size, int priv)
{
	ASSERT (cpu_obj == seq->get_mai_object (thread_id)->get_cpu_obj ());
	return simics_mmu_iface->set_error (simics_mmu_obj, cpu_obj, ex, addr, ft,
		asi, r_or_w, d_or_i, atomic, size, priv);
}		

void
mmu_t::set_error_info (mmu_error_info_t *ei) {
	simics_mmu_iface->set_error_info (ei);
}

exception_type_t
mmu_t::handle_asi (v9_memory_transaction_t *mem_op) {
	switch (mem_op->address_space) {

	case ASI_LSU_CONTROL_REG:
	case ASI_DMMU:
	case ASI_IMMU:
	case ASI_IMMU_TSB_8KB_PTR_REG:
	case ASI_IMMU_TSB_64KB_PTR_REG:
	case ASI_DMMU_TSB_8KB_PTR_REG:
	case ASI_DMMU_TSB_64KB_PTR_REG:
	case ASI_DMMU_TSB_DIRECT_PTR_REG:
	case ASI_ECACHE_R:
	case ASI_ECACHE_TAG:
	case ASI_DCACHE_TAG:
	case ASI_DCACHE_DATA:
	case ASI_ICACHE_TAG:
	case ASI_ICACHE_INSTR:
	case ASI_ECACHE_W:
	case ASI_ICACHE_PRE_DECODE:
		return simics_mmu_asi_iface->mmu_handle_asi (simics_mmu_obj,
			&mem_op->s);
		
	case ASI_ITLB_DATA_IN_REG:
	case ASI_DTLB_DATA_IN_REG:
		return simics_mmu_asi_iface->mmu_data_in_asi (simics_mmu_obj,
			&mem_op->s);

	case ASI_ITLB_DATA_ACCESS_REG:
	case ASI_DTLB_DATA_ACCESS_REG:
		return simics_mmu_asi_iface->mmu_data_access_asi (simics_mmu_obj,
			&mem_op->s);

	case ASI_ITLB_TAG_READ_REG:
	case ASI_DTLB_TAG_READ_REG:
		return simics_mmu_asi_iface->mmu_tag_read_asi (simics_mmu_obj,
			&mem_op->s);

	case ASI_DMMU_DEMAP:
	case ASI_IMMU_DEMAP:
		return simics_mmu_asi_iface->mmu_demap_asi (simics_mmu_obj, &mem_op->s);
		
	default:
		FAIL;
		return Sim_PE_Default_Semantics;
	}
	
}

exception_type_t
mmu_t::static_do_access (conf_object_t *obj, v9_memory_transaction_t *mem_op) {
	
	proc_object_t *proc = (proc_object_t *) obj;
	uint32 proc_no = SIM_get_proc_no((conf_object_t *) mem_op->s.ini_ptr);
	sequencer_t *seq = proc->chip->get_sequencer_from_thread(proc_no);
	
	return proc->chip->get_mmu(seq->get_id())->do_access(mem_op);
}	

int
mmu_t::static_current_context (conf_object_t *obj, conf_object_t *cpu) {
	
	proc_object_t *proc = (proc_object_t *) obj;
	uint32 proc_no = SIM_get_proc_no(cpu);
	sequencer_t *seq = proc->chip->get_sequencer_from_thread(proc_no);
	
	return proc->chip->get_mmu(seq->get_id())->current_context (cpu);
}
	
void
mmu_t::static_reset (conf_object_t *obj, conf_object_t *cpu,
	exception_type_t exc_no)
{
	
	proc_object_t *proc = (proc_object_t *) obj;
	uint32 proc_no = SIM_get_proc_no(cpu);
	sequencer_t *seq = proc->chip->get_sequencer_from_thread(proc_no);
	
	return proc->chip->get_mmu(seq->get_id())->reset (cpu, exc_no);
}
	
exception_type_t
mmu_t::static_undefined_asi (conf_object_t *obj,
	v9_memory_transaction_t *mem_op)
{

	proc_object_t *proc = (proc_object_t *) obj;
	uint32 proc_no = SIM_get_proc_no((conf_object_t *) mem_op->s.ini_ptr);
	sequencer_t *seq = proc->chip->get_sequencer_from_thread(proc_no);
	
	return proc->chip->get_mmu(seq->get_id())->undefined_asi (mem_op);
}

exception_type_t
mmu_t::static_set_error (conf_object_t *obj,
	conf_object_t *cpu, exception_type_t ex, uint64 addr, int ft, 
	int asi, read_or_write_t r_or_w, data_or_instr_t d_or_i, int atomic,
	int size, int priv)
{
	
	proc_object_t *proc = (proc_object_t *) obj;
	uint32 proc_no = SIM_get_proc_no(cpu);
	sequencer_t *seq = proc->chip->get_sequencer_from_thread(proc_no);
	
	return proc->chip->get_mmu(seq->get_id())->set_error (cpu, ex, addr, ft,
		asi, r_or_w, d_or_i, atomic, size, priv);
}
	
void
mmu_t::static_set_error_info (mmu_error_info_t *ei) {
	
	// Real mmu set mmu_obj of ei to itself, need a pointer to ms2
	conf_object_t *cpu = SIM_proc_no_2_ptr(0);
	attr_value_t ms2 = SIM_get_attribute (cpu, "mmu");
	ASSERT (ms2.kind == Sim_Val_Object);

	conf_object_t *ms2_obj = ms2.u.object;
	
	static_set_error(ms2_obj,
		ei->cpu_obj,
		ei->ex,
		ei->addr,
		ei->ft,
		ei->asi,
		ei->r_or_w,
		ei->d_or_i,
		ei->atomic,
		ei->tl,
		ei->priv);
	
	MM_FREE(ei);
}

exception_type_t
mmu_t::static_handle_asi (conf_object_t *obj, generic_transaction_t *mem_op) {

	proc_object_t *proc = (proc_object_t *) obj;
	uint32 proc_no = SIM_get_proc_no((conf_object_t *) mem_op->ini_ptr);
	sequencer_t *seq = proc->chip->get_sequencer_from_thread(proc_no);
	
	return proc->chip->get_mmu(seq->get_id())->handle_asi ((v9_memory_transaction_t *) mem_op);
}

void
mmu_t::register_asi_handlers (conf_object_t *cpu) {
	
	sparc_v9_interface_t *v9_interface;
	
	v9_interface = (sparc_v9_interface_t *) 
		SIM_get_interface(cpu, SPARC_V9_INTERFACE);
	ASSERT (v9_interface);
	
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_LSU_CONTROL_REG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_DMMU);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_IMMU);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_IMMU_TSB_8KB_PTR_REG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_IMMU_TSB_64KB_PTR_REG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_DMMU_TSB_8KB_PTR_REG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_DMMU_TSB_64KB_PTR_REG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_DMMU_TSB_DIRECT_PTR_REG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_ITLB_DATA_IN_REG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_DTLB_DATA_IN_REG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_ITLB_DATA_ACCESS_REG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_DTLB_DATA_ACCESS_REG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_ITLB_TAG_READ_REG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_DTLB_TAG_READ_REG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_DMMU_DEMAP);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_IMMU_DEMAP);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_ECACHE_R);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_ECACHE_TAG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_DCACHE_TAG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_DCACHE_DATA);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_ICACHE_TAG);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_ICACHE_INSTR);
	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_ECACHE_W);

	v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, ASI_ICACHE_PRE_DECODE);
}

