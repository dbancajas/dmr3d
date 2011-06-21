/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: mai.cc,v 1.7.2.42 2006/11/10 03:06:57 pwells Exp $");

#include "definitions.h"
#include "mai.h"
#include "mai_instr.h"
#include "isa.h"
#include "fu.h"
#include "dynamic.h"
#include "startup.h"
#include "window.h"
#include "iwindow.h"
#include "sequencer.h"
#include "chip.h"
#include "fu.h"
#include "eventq.h"
#include "ctrl_flow.h"
#include "ras.h"
#include "lsq.h"
#include "mem_xaction.h"
#include "st_buffer.h"
#include "config_extern.h"
#include "thread_scheduler.h"

#include "mem_hier_iface.h"
#include "transaction.h"
#include "proc_stats.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "verbose_level.h"

#include "cmp_mmu_iface.h"

mai_t::mai_t (conf_object_t *_cpu, chip_t *_p) {
	cpu = _cpu;
	p = _p;
	shadow_cpu = 0;
	idle_loop = false;
    	spin_loop = false;
	u2 = false;
    	kstack_region = 0;
	mmu_obj = SIM_get_attribute(cpu, "mmu").u.object;
	//FE_EXCEPTION_CHECK;
	mmu_iface = (cmp_mmu_iface_t *) SIM_get_interface(mmu_obj, "cmp_mmu");

	auto_probe_sim_parameters ();

	ivec.initialize ();

	v9_interface = (sparc_v9_interface_t *) 
		SIM_get_interface(cpu, SPARC_V9_INTERFACE);
	ASSERT (v9_interface);
    	syscall = 0x106; // Random assignment to keep aseets happy
	tl_reg = SIM_get_register_number (cpu, "tl");
	tick_reg = SIM_get_register_number (cpu, "tick");
    	pstate_reg = SIM_get_register_number (cpu, "pstate");
    	pc_reg = SIM_get_register_number (cpu, "pc");
	npc_reg = SIM_get_register_number (cpu, "npc");
    	stick_reg = SIM_get_register_number (cpu, "stick");
	//FE_EXCEPTION_CHECK;

	if (SIM_get_pending_exception ()) {
		DEBUG_OUT("u2?\n");
		SIM_clear_exception();
		FE_EXCEPTION_CHECK;
		u2 = true;
	}
		
	syscall = 0;
    	syscall_b4_interrupt = 0;
	/*if (g_conf_csp_use_mmu || g_conf_override_idsr_read) {
		v9_interface->install_user_class_asi_handler(cpu, static_demap_asi,
			ASI_DMMU_DEMAP);
		v9_interface->install_user_class_asi_handler(cpu, static_demap_asi,
			ASI_IMMU_DEMAP);
	}*/
    
    cpu_id = SIM_get_proc_no(cpu);
    supervisor_entry_step = 0;
    last_user_pc = 0;
    bpred_state = new bpred_state_t;
    last_eviction = 0;
    last_8bit_commit = 0;
    last_16bit_commit = 0;
    last_24bit_commit = 0;
    
    register_count = 96; // ??
    register_read_first = new bool[register_count];
    register_write_first = new bool[register_count];
    
    bzero(register_read_first, sizeof(bool) * register_count);
    bzero(register_write_first, sizeof(bool) * register_count);
    
}

mai_t::~mai_t () {
}

conf_object_t*
mai_t::get_cpu_obj () {
	return cpu;
}

void 
mai_t::auto_probe_sim_parameters () {

	simics_lsq_enable (cpu, g_conf_use_internal_lsq);
	if (g_conf_memory_image_size==0){
		g_conf_memory_image_size = get_memory_image_size ();
	}
	//	cout<<endl<<"Mem Image Size: "<<g_conf_memory_image_size<<endl;
	
	
}

uint64
mai_t::get_memory_image_size () {

	conf_object_t *obj = 0; 
	addr_t mem_size = 0;
	char num[16];
	int i = 0;
	
	do {
		sprintf(num, "%u", i);
		obj = SIM_get_object (("memory"+string(num)).c_str());
		if (!obj) {
			SIM_clear_exception ();
			obj = SIM_get_object ("memory");
		}
		if (!obj) {
			SIM_clear_exception ();
			obj = SIM_get_object (("server_memory"+string(num)).c_str());
		}
		if (!obj) {
			SIM_clear_exception ();
			obj = SIM_get_object (("m0_memory" + string(num)).c_str());
		}
	
		if (!obj) {
			if (i == 0) FAIL;
			SIM_clear_exception();
			continue;
		}

		attr_value_t mem_obj_attr = SIM_get_attribute (obj, "image");
		ASSERT (mem_obj_attr.kind == Sim_Val_Object);

		conf_object_t *mem_obj = mem_obj_attr.u.object;

		attr_value_t mem_image_size_attr = SIM_get_attribute (mem_obj, "size");
		ASSERT (mem_image_size_attr.kind == Sim_Val_Integer);

		mem_size += mem_image_size_attr.u.integer;
		i++;
	} while (obj);
	
	DEBUG_OUT("Memory image size: %llx\n", mem_size);
	
	return mem_size;
}

void 
mai_t::simics_lsq_enable (conf_object_t *_cpu, bool b) {
	attr_value_t *lsq_enabled = new attr_value_t ();

	lsq_enabled->kind = Sim_Val_Integer;
	lsq_enabled->u.integer = static_cast <int32> (b);
	SIM_set_attribute (_cpu, "lsq-enabled", lsq_enabled);
}

void
mai_t::set_reorder_buffer_size (uint64 entries) {
	attr_value_t *rob_size = new attr_value_t ();

    DEBUG_OUT("Setting re-order buffer to %llu\n", entries);
	rob_size->kind = Sim_Val_Integer;
	rob_size->u.integer = entries;
	
	set_error_t e = 
		SIM_set_attribute (cpu, "reorder-buffer-size", rob_size);


	ASSERT (e == Sim_Set_Ok);	

	delete rob_size;
}

bool
mai_t::is_supervisor () {
	
	uint32 level = SIM_cpu_privilege_level (cpu);
	
	FE_EXCEPTION_CHECK;
	if (level) return true; else return false;
}

void
mai_t::setup_trap_context (bool wrpr) {
    proc_stats_t *tstats0 = p->get_tstats_list (get_id())[0];

	uint64 tl = get_tl ();
	ASSERT (tl > 0);

	if (tl == STAT_GET (tstats0->stat_tl)) {
//		DEBUG_OUT ("skipping setup_trap_context ()\n");
		return;
	}

	STAT_INC (tstats0->stat_tl);
	if (STAT_GET (tstats0->stat_tl) != tl) {
		WARNING;
		STAT_SET (tstats0->stat_tl, tl);
	}
	
	uint64 tt;
	if (wrpr) tt = 0;
	else tt = get_tt (tl);
	STAT_SET (tstats0->stat_tt [tl], tt);
	setup_syscall_interrupt(tt);
}

void
mai_t::setup_syscall_interrupt(uint64 tt)
{
    proc_stats_t *tstats0 = p->get_tstats_list (get_id())[0];
    tick_t curr_cycle = p->get_g_cycles();
	// Solaris syscall
	if (is_syscall_trap(tt)) {
		ASSERT(STAT_GET (tstats0->stat_syscall_num) == 0);
		ASSERT(STAT_GET (tstats0->stat_syscall_start) == 0);
		set_syscall_num();
        // DEBUG_OUT("cpu%u making syscall %llu\n", get_id(), syscall);
		STAT_SET (tstats0->stat_syscall_num, syscall);
		STAT_SET (tstats0->stat_syscall_start, curr_cycle);
        syscall_b4_interrupt = 0;
	}
    
	else if (is_interrupt_trap(tt)) {
		
		// if currently in syscall, finish it up and change to interrupt type
		if (STAT_GET (tstats0->stat_syscall_num) != 0) {
            // stat for OS entry only
            if (g_conf_stat_os_entry_only) return;
			
            ASSERT(STAT_GET (tstats0->stat_syscall_start) != 0);
			uint64 syscall_len = curr_cycle - 
				STAT_GET (tstats0->stat_syscall_start);
			uint64 syscall = STAT_GET (tstats0->stat_syscall_num);

			STAT_ADD (tstats0->stat_syscall_len, syscall_len, syscall);
			STAT_ADD (tstats0->stat_syscall_hit, 1, syscall);
			tstats0->stat_syscall_len_histo->inc_total(1, syscall_len);
		}
		
		// rest of code like syscall above
        if (syscall != 0 && syscall_b4_interrupt == 0)
            syscall_b4_interrupt = syscall;
		set_syscall_num();
        // DEBUG_OUT("cpu%u got interrupt %llu prev_syscall %llu @ %llu\n", get_id(),
        //     syscall, syscall_b4_interrupt, curr_cycle);
        STAT_SET (tstats0->stat_syscall_num, syscall);
		STAT_SET (tstats0->stat_syscall_start, curr_cycle);
	}
}


void
mai_t::ino_setup_trap_context (uint64 tt, bool wrpr, int64 wrpr_val) {
	
	if (g_conf_use_processor)
		return;
	
    proc_stats_t *tstats0 = p->get_tstats_list (get_id())[0];

	if (STAT_GET (tstats0->stat_tl) != get_tl()) {
		WARNING;
		DEBUG_OUT("trap cpu%d @ %llu:  %lld %lld %llx\n", get_id(),
			p->get_g_cycles(),
			STAT_GET (tstats0->stat_tl), get_tl(), tt);
		STAT_SET (tstats0->stat_tl, get_tl());
	}

	if (!wrpr)
		STAT_INC(tstats0->stat_tl);
	else
		STAT_SET(tstats0->stat_tl, wrpr_val);
		
	uint64 tl = STAT_GET (tstats0->stat_tl);
	
	STAT_SET (tstats0->stat_tt [tl], tt);
	STAT_SET (tstats0->stat_trap_length[tl], p->get_g_cycles());
	
//	setup_syscall_interrupt(tt);
}

void
mai_t::ino_finish_trap_context() {

	if (g_conf_use_processor)
		return;
	
    proc_stats_t *tstats0 = p->get_tstats_list (get_id())[0];
    proc_stats_t *tstats = get_tstats();

	uint64 tl = STAT_GET (tstats0->stat_tl);
	uint64 mai_tl = get_tl() - 1;
	
//	DEBUG_OUT("return %lld %lld\n", STAT_GET (tstats0->stat_tl), get_tl());
	
	//ASSERT_WARN (tl > 0);
	ASSERT (mai_tl >= 0);
	for (uint32 i = tl; i > mai_tl; i--) {

		int64 tt = STAT_GET (tstats0->stat_tt [i]);
		int64 trap_len = p->get_g_cycles() - 
			STAT_GET (tstats0->stat_trap_length [i]);

		// Apropos tstats for user/os
		STAT_ADD (tstats->stat_trap_len, trap_len, tt);
		STAT_INC (tstats->stat_trap_hit, tt);

		// 
		STAT_DEC (tstats0->stat_tl);
		STAT_SET (tstats0->stat_tt [i], 0);
		STAT_SET (tstats0->stat_trap_length [i], 0);
	}

}

void
mai_t::ino_pause_trap_context() {
	if (g_conf_use_processor)
		return;
	
    proc_stats_t *tstats0 = p->get_tstats_list (get_id())[0];
    proc_stats_t *tstats = get_tstats();

	uint64 tl = STAT_GET (tstats0->stat_tl);
	
	for (uint32 i = tl; i > 0; i--) {

		int64 tt = STAT_GET (tstats0->stat_tt [i]);
		int64 trap_len = p->get_g_cycles() - 
			STAT_GET (tstats0->stat_trap_length [i]);

		STAT_ADD (tstats->stat_trap_len, trap_len, tt);
	}
}

void
mai_t::ino_resume_trap_context() {
	if (g_conf_use_processor)
		return;
	
    proc_stats_t *tstats0 = p->get_tstats_list (get_id())[0];
	uint64 tl = STAT_GET (tstats0->stat_tl);
	
	for (uint32 i = tl; i > 0; i--) {
		STAT_SET (tstats0->stat_trap_length[i], p->get_g_cycles());
	}
}


void
mai_t::handle_interrupt (sequencer_t *seq, proc_stats_t *pstats) {

	ASSERT(seq == p->get_sequencer_from_thread(SIM_get_proc_no(cpu)));

    // TODO -- get the right stat
	proc_stats_t *tstats = p->get_tstats (cpu_id);
	instruction_error_t    ie;
//	instruction_error_t    shadow_ie;
	st_buffer_t           *st_buffer = seq->get_st_buffer ();

	// st buffer needs to be empty
    uint8 tid = seq->mai_to_tid(this);
    
	if (!st_buffer->empty_ctxt(tid)) {
  		return;
	}

    bool change_core = g_conf_interrupt_core_change ? true : (syscall == 0);
	
	switch ((ie = SIM_instruction_handle_interrupt (cpu, ivec.get() ))) {
	case Sim_IE_OK:
		STAT_INC (pstats->stat_interrupts);
		STAT_INC (tstats->stat_interrupts);
		// after mai call
		ASSERT(is_interrupt_trap(get_tt(get_tl())));
        setup_trap_context ();
		reset_interrupt ();
        if (change_core)
            seq->potential_thread_switch(tid, YIELD_EXCEPTION);

        if (seq->get_pc(tid) != get_pc())
            seq->get_ctrl_flow(tid)->fix_spec_state((dynamic_instr_t *) 0);
		FE_EXCEPTION_CHECK;

		break;
	case Sim_IE_Illegal_Interrupt_Point:
		seq->prepare_for_interrupt (this);
		break;
	case Sim_IE_Interrupts_Disabled:
		break;

	default:
		FAIL;
	}
}


uint64
mai_t::get_tl () {
	return (static_cast <uint64> (SIM_read_register (cpu, tl_reg)));
}

uint64
mai_t::get_tt (uint64 tl) {

	char tt_s [5];

	sprintf (tt_s, "tt%lld", tl);
	int64 tt = SIM_get_register_number (cpu, tt_s);
	return (static_cast <uint64> (SIM_read_register (cpu, tt)));
}

uint64
mai_t::get_tstate (uint64 tl) {

	char ts_s [5];

	sprintf (ts_s, "tstate%lld", tl);
	int64 ts = SIM_get_register_number (cpu, ts_s);
	return (static_cast <uint64> (SIM_read_register (cpu, ts)));
}

bool
mai_t::is_fill_spill (uint64 tt) {
	// spill, fill or clean win
	return ((tt >= 0x80 && tt < 0x100) || tt == 0x24);
}

bool
mai_t::is_tlb_trap (uint64 tt) {
	return (tt == 0x64 || tt == 0x68 || tt == 0x6c || tt == 0x34);
}


bool
mai_t::is_user_trap() {
	uint64 tl = get_tl();
	if (tl != 1) return false;
	
	uint64 tstate = get_tstate(1);
	// bit 10 is pstate copy's priv
	return (((tstate >> 10) & 1) == 0);
}

addr_t
mai_t::get_pc () {
	FE_EXCEPTION_CHECK;
	addr_t pc = static_cast <addr_t> (SIM_read_register (cpu, pc_reg));
	if (is_32bit())
		pc &= 0xffffffffULL;
	FE_EXCEPTION_CHECK;
	return pc;
}


addr_t
mai_t::get_npc () {
	addr_t npc = static_cast <addr_t> (SIM_read_register (cpu, npc_reg));
	if (is_32bit())
		npc &= 0xffffffffULL;
	FE_EXCEPTION_CHECK;
	return npc;
}

void mai_t::break_sim (tick_t when) {
	ASSERT (when >= 0);
	SIM_break_cycle (cpu, when);	
}

void
mai_t::piq () {
	int              i;
	instruction_id_t id;

	attr_value_t     ipc;
	attr_value_t     inpc;
	attr_value_t     opc;
	attr_value_t     onpc;

	i = 0; 
	while ((id = SIM_instruction_nth_id (cpu, i))) {

		dynamic_instr_t *d_instr = static_cast <dynamic_instr_t *> 
	          	(SIM_instruction_get_user_data (id));

		mai_instruction_t *mai_instr = d_instr->get_mai_instruction (); 
		
		instruction_id_t shadow_instr_id = 
			mai_instr->get_shadow_instr_id ();

do_again:	
		ipc = SIM_instruction_read_input_reg (id, V9_Reg_Id_PC);
		inpc= SIM_instruction_read_input_reg (id, V9_Reg_Id_NPC);
		opc = SIM_instruction_read_output_reg (id, V9_Reg_Id_PC);
		onpc= SIM_instruction_read_output_reg (id, V9_Reg_Id_NPC);

		DEBUG_OUT ("%d, i: %d, phase: %d sync: %d spec: %d, status: %d exception: %d\n", 
		i,  id,
		SIM_instruction_phase (id), 
		SIM_instruction_is_sync (id), 
		SIM_instruction_speculative (id), 
		SIM_instruction_status (id) & Sim_IS_Faulting, 
		SIM_instruction_status (id));

		DEBUG_OUT ("PC (0x%llx, 0x%llx) (0x%llx, 0x%llx)\n", 
		ipc.u.integer, inpc.u.integer, opc.u.integer, onpc.u.integer);

		FE_EXCEPTION_CHECK;

		if (id == mai_instr->get_instr_id ()) {
			mai_instr->debug (1);

			id = shadow_instr_id;

			DEBUG_OUT ("SHADOW---\n");
			if (id) {
				ASSERT (shadow_instr_id == SIM_instruction_nth_id (shadow_cpu, i));	

				FE_EXCEPTION_CHECK;
				goto do_again;
			} else 
				DEBUG_OUT ("none.\n");
		} else { 
			mai_instr->shadow_debug (1);
		}

		i++;	
	}
}

void 
mai_t::set_shadow_cpu_obj (conf_object_t *_cpu) {
	
	shadow_cpu = _cpu;
	copy_master_simics_attributes ();

	simics_lsq_enable (shadow_cpu, g_conf_use_internal_lsq);
}

void 
mai_t::copy_master_simics_attributes () {

	attr_value_t temp = SIM_get_attribute (cpu, "reorder-buffer-size");
	attr_value_t *rob_size = new attr_value_t ();

	rob_size->kind = Sim_Val_Integer;
	rob_size->u.integer = temp.u.integer;
	
	set_error_t e = SIM_set_attribute (shadow_cpu, "reorder-buffer-size", rob_size);
	ASSERT (e == Sim_Set_Ok);	

}

conf_object_t *
mai_t::get_shadow_cpu_obj (void) {
	return shadow_cpu;
}

bool 
mai_t::check_shadow (void) {
	if (shadow_cpu) return true; else return false;

}

int32
mai_t::get_id() {
	return cpu_id;
}

void
mai_t::set_interrupt(uint64 vector) {
	ivec.set(vector);
}

void
mai_t::reset_interrupt() {
	ivec.invalidate();
}

bool
mai_t::pending_interrupt() {
	return ivec.is_valid ();
}

uint64
mai_t::get_interrupt() {
	return ivec.get ();
}

void
mai_t::write_checkpoint(FILE *file) 
{
	fprintf(file, "%llu %d\n", ivec.is_valid() ? ivec.get() : 0, ivec.is_valid());
    fprintf(file, "%llu\n", syscall);
}

void
mai_t::read_checkpoint(FILE *file)
{
	uint64 temp_ivec;
	uint32 temp_valid;
	fscanf(file, "%llu %d\n", &temp_ivec, &temp_valid);
	if (temp_valid)
		ivec.set(temp_ivec);
	else
		ivec.invalidate();
    fscanf(file, "%llu\n", &syscall);
}



void
mai_t::set_syscall_num(bool provided, uint64 val)
{
	if (provided) {
		syscall = val;
		return;
	}
    uint64 tt = get_tt(get_tl());
    if (tt == 0x108 || tt == 0x110)
        syscall = get_user_g1();
    else if (is_interrupt_trap(tt)) {
		if (tt == 0x60)
			syscall = SYS_NUM_SW_INTR;
		else
			syscall = SYS_NUM_HW_INTR;
//		syscall = 300 - 0x40 + tt;
	}
	else 
        syscall = tt;
}

uint64
mai_t::get_syscall_num()
{
    return syscall;
}

uint64
mai_t::get_user_g1 () {
	uint64 g1 = v9_interface->read_global_register(cpu, 0 /* normal */,
		1 /* %g1 */);
	FE_EXCEPTION_CHECK;
	return g1;
}

addr_t
mai_t::get_register (string reg) {
	int64 reg_num = SIM_get_register_number (cpu, reg.c_str());
	uint64 reg_val = static_cast <addr_t> (SIM_read_register (cpu, reg_num));
	FE_EXCEPTION_CHECK;
	return reg_val;
}

void
mai_t::set_register (string reg, uint64 val) {
	int64 reg_num = SIM_get_register_number (cpu, reg.c_str());
	SIM_write_register (cpu, reg_num, val);
	FE_EXCEPTION_CHECK;
}

proc_stats_t *
mai_t::get_tstats() {
	return p->get_tstats(get_id());
}

bool
mai_t::is_idle_loop() {
	return idle_loop;
}

void
mai_t::set_idle_loop(bool idle) {
	idle_loop = idle;
}



bool
mai_t::is_syscall_trap(uint64 tt)
{
	return (
		               //   LINUX TT                        OPEN SOLARIS TT DOC
		tt == 0x100 || // SunOS Syscall                         old system call
//		tt == 0x106 || //                                                nfssys
		tt == 0x108 || // Solaris Syscall             ILP32 system call on LP64
		tt == 0x110 || // Linux 32bit Syscall             V9 user trap handlers
		tt == 0x111 || // Old Linux 64bit Syscall                     "
// short execution, don't count it:
//		tt == 0x124 || //                                         get timestamp
//		tt == 0x126 || //                                            self xcall
// short execution, don't count it:
//		tt == 0x127 || // indirect Solaris Syscall                 get hrestime
		
		tt == 0x140 || //                                      LP64 system call
		tt == 0x16d  // Linux 64bit Syscall
		);
}

bool
mai_t::is_interrupt_trap(uint64 tt)
{
	return (tt >= 0x41 && tt <= 0x4f) || tt == 0x60;
}

void
mai_t::set_kstack_region()
{
    uint64 stack_ptr = get_register("sp");
    kstack_region = stack_ptr - stack_ptr % 0xc000;
}

uint64
mai_t::get_kstack_region()
{
    return kstack_region;
}

void
mai_t::switch_mmu_array(conf_object_t *mmu_array)
{
	attr_value_t temp_attr = SIM_make_attr_object(mmu_array);
	SIM_set_attribute(mmu_obj, "tlb_array", &temp_attr);
}



exception_type_t
mai_t::demap_asi (v9_memory_transaction_t *mem_op) {
	ASSERT(mem_op->address_space == ASI_DMMU_DEMAP ||
		mem_op->address_space == ASI_IMMU_DEMAP);
	
	VERBOSE_OUT(verb_t::mmu, "demap_asi: cpu:%d va:0x%016llx PC:%llx @ %llu\n", 
		SIM_get_proc_no(cpu), mem_op->s.logical_address, SIM_get_program_counter(cpu), p->get_g_cycles());


	// These are demapping entries pointing to the TSB, typically when the OS makes
	// a context switch.  It is ok to demap the initiating CPU, but not other CPUs
	// on the CMP.  This is currently done for all locked entries in the tlb code
	// so the below is unecessary for now
	/*	
	addr_t pc = SIM_get_program_counter(cpu);
	if (pc == 0x1007dc4 || pc == 0x1008010 || pc == 0x1008098)
	{
		// ignore demap for TSB entries demapped from these PCs  
		VERBOSE_OUT(verb_t::mmu, "ignoring dmap from pc: 0x%08llx\n", 
			SIM_get_program_counter(cpu));
		return Sim_PE_No_Exception;
	}
	*/

	// TODO: currently demaps all mmus -- only needs to demap mmus
	// from sequencers that this VCPU potentially executed on 
	exception_type_t ret;
	uint32 machine_id = p->get_machine_id(get_id());
    if (mmu_iface) {
        for (uint32 i = 0; i < p->get_num_cpus(); i++) {
            // Only send to cpus of the same VM
            if (machine_id != p->get_machine_id(i))
                continue;
            conf_object_t *mmu_ptr = p->get_mai_object(i)->mmu_obj;
            if (i == (uint32) get_id())
                mem_op->s.user_ptr = (void *) 0;
            else
                mem_op->s.user_ptr = (void *) 3333; // cmp_demap marker
            
            ret = mmu_iface->demap_asi(mmu_ptr, &mem_op->s);
        }
        return Sim_PE_No_Exception;
    } else {
        FAIL;
    }
    
	return Sim_PE_Default_Semantics;
	
}


exception_type_t
mai_t::static_demap_asi(conf_object_t *obj, generic_transaction_t *mem_op) {

// Huh? this returns cheetah-mmu obj here, but ms2 proc_object not in mmu.cc...
//	proc_object_t *proc = (proc_object_t *) obj;
	proc_object_t *proc = (proc_object_t *) SIM_get_object("ms2");

	uint32 proc_no = SIM_get_proc_no((conf_object_t *) mem_op->ini_ptr);
	
	return proc->chip->get_mai_object(proc_no)->demap_asi((v9_memory_transaction_t *) mem_op);
}

void
mai_t::save_tick_reg()
{
	saved_tick = (static_cast <uint64> (SIM_read_register(cpu, tick_reg)));
	if (!u2)
		saved_stick = (static_cast <uint64> (SIM_read_register(cpu, stick_reg)));
}

void
mai_t::restore_tick_reg()
{
	SIM_write_register (cpu, tick_reg, saved_tick);
	if (!u2)
		SIM_write_register (cpu, stick_reg, saved_stick);
}


bool mai_t::is_spin_loop()
{
    return spin_loop;
}


void mai_t::set_spin_loop(bool _spin)
{
    spin_loop = _spin;
}

uint64 mai_t::get_syscall_b4_interrupt()
{
    return syscall_b4_interrupt;
}

void mai_t::reset_syscall_num()
{
    syscall = 0;
    syscall_b4_interrupt = 0;
}

bool mai_t::is_pending_interrupt()
{
    attr_value_t val = SIM_get_attribute(cpu, "pending_interrupt");
    return (val.u.integer > 0);
}

bool mai_t::can_preempt()
{
    if (g_conf_no_os_pause) {
        if(g_conf_safe_trap) {
            if (is_supervisor()) {
                if (get_tl()) {
                    uint64 current_step = SIM_step_count(cpu);
                    if (supervisor_entry_step && 
                       (current_step - supervisor_entry_step < 60))
                        return true;
                }
                return false;
            }
            return !(is_pending_interrupt());
        } else {
            return (is_supervisor() == false &&
                is_pending_interrupt() == false);
        }        
    }
    return true;            
}

void mai_t::mode_change()
{
    if (is_supervisor())
        supervisor_entry_step = SIM_step_count(cpu);
    else
        supervisor_entry_step = 0;
}

void mai_t::set_last_user_pc(addr_t _pc)
{
    last_user_pc = _pc;
}

addr_t mai_t::get_last_user_pc()
{
    return last_user_pc;
}

bpred_state_t * mai_t::get_bpred_state()
{
    return bpred_state;
}

tick_t mai_t::get_last_eviction()
{
    return last_eviction;
}

void mai_t::set_last_eviction(tick_t cycle)
{
    last_eviction = cycle;
}


void mai_t::update_register_read(int32 idx)
{
    if (!register_write_first[idx] && !register_read_first[idx])
        register_read_first[idx] = true;
}

void mai_t::update_register_write(int32 idx)
{
    if (!register_read_first[idx] && !register_write_first[idx])
        register_write_first[idx] = true;
}

uint32 mai_t::get_register_read_first()
{
    uint32 ret = 0;
    for (uint32 i = 0; i < register_count; i++)
        if (register_read_first[i]) ret++;
    return ret;
}

uint32 mai_t::get_register_write_first()
{
    uint32 ret = 0;
    for (uint32 i = 0; i < register_count; i++)
        if (register_write_first[i]) ret++;
    return ret;
}

void mai_t::reset_register_read_write()
{
    bzero(register_read_first, sizeof(bool) * register_count);
    bzero(register_write_first, sizeof(bool) * register_count);
}

void mai_t::stat_register_read_write(proc_stats_t *pstats, proc_stats_t *tstats)
{
    uint32 ret = 0;
    for (uint32 i = 0; i < register_count; i++)
    {
        if (register_read_first[i]) {
            ret++;
            pstats->stat_register_read_group->inc_total(1, i);
            tstats->stat_register_read_group->inc_total(1, i);
        }
    }
    
    pstats->stat_register_read_first->inc_total(1, ret);
    tstats->stat_register_read_first->inc_total(1, ret);
    
    ret = 0;
    
    
    for (uint32 i = 0; i < register_count; i++)
    {
        if (register_write_first[i]) {
            ret++;
            pstats->stat_register_write_group->inc_total(1, i);
            tstats->stat_register_write_group->inc_total(1, i);
        }
    }
    
    pstats->stat_register_write_first->inc_total(1, ret);
    tstats->stat_register_write_first->inc_total(1, ret);
}



bool
mai_t::is_32bit() {
	uint64 pstate = static_cast <uint64> (SIM_read_register (cpu, pstate_reg));
	return ((pstate & (1<<3)) > 0);
}

