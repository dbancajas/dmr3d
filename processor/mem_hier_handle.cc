/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: mem_hier_handle.cc,v 1.8.2.17 2006/08/18 14:30:20 kchak Exp $");

#include "definitions.h"
#include "mai.h"
#include "mai_instr.h"
#include "isa.h"
#include "fu.h"
#include "dynamic.h"
#include "window.h"
#include "iwindow.h"
#include "sequencer.h"
#include "chip.h"
#include "fu.h"
#include "eventq.h"
#include "v9_mem_xaction.h"
#include "mem_xaction.h"
#include "lsq.h"
#include "st_buffer.h"
#include "fetch_buffer.h"
#include "mem_hier_iface.h"
#include "transaction.h"
#include "mem_hier_handle.h"
#include "startup.h"
#include "config_extern.h"
#include "mem_hier.h"
#include "mem_driver.h"
#include "fastsim.h"
#include "external.h"

mem_hier_handle_t::mem_hier_handle_t (conf_object_t *_obj, 
	mem_hier_interface_t *_iface, proc_object_t *_proc)
	: obj (_obj), iface (_iface) {
	
	mem_hier = new mem_hier_t(_proc);
	mem_hier->set_ptr(mem_hier);	
	mem_hier->set_handler(this);
    chip = _proc->chip;
}

mem_hier_handle_t::~mem_hier_handle_t () {
}

cycles_t
mem_hier_handle_t::lsq_operate (conf_object_t *proc_obj, conf_object_t *space, 
	map_list_t *map, generic_transaction_t *mem_op) {

	if (!SIM_mem_op_is_from_cpu (mem_op)) {
		return 0;
	}

	FE_EXCEPTION_CHECK;

	// cpu mem op
	proc_object_t *proc = (proc_object_t *) proc_obj;

	bool is_shadow = false;
    
    
    instruction_id_t instr_id =
		SIM_instruction_id_from_mem_op_id(mem_op->ini_ptr, mem_op->id);
	
	if (!instr_id) {
        
		instr_id = sequencer_t::icache_instr_id;

		if (!instr_id) {
		 	ASSERT(SIM_mem_op_is_prefetch(mem_op) || SIM_mem_op_is_control(mem_op));
		 	return 0;
		 } else {
		 	ASSERT(SIM_mem_op_is_instruction(mem_op));
		 }
	}
	
    ASSERT(instr_id);
	dynamic_instr_t *d_instr = static_cast <dynamic_instr_t *> 
		(SIM_instruction_get_user_data (instr_id));
	
	if (!d_instr) {
		// shadow instr
		//ASSERT(instr_id == sequencer_t::shadow_instr_id);
		//return 0;
        FAIL;
	}
		
	FE_EXCEPTION_CHECK;
	
    mai_instruction_t *mai_instr = d_instr->get_mai_instruction ();
	

	// Shouldn't execute any sync instructions
	// ASSERT(SIM_mem_op_is_instruction(mem_op) ||
	// 	!mai_instr->is_sync(TF_SYNC_CANT_EXEC));
	
	// FE_EXCEPTION_CHECK;

	if (SIM_mem_op_is_instruction(mem_op) && mai_instr->done_fetch()) {
		//This happens when re-executing a newly-inserted instruction
		//(that had previously been Simics-squashed)
        cerr << "You have set instruction-fetch-mode (ifm) to instruction-cache-access-trace in your SIMICS script" << endl;
	cerr << "This is incompatible in SIMICS 3.0.X" << endl;
	cerr << "Set ifm to instruction-fetch-trace" << endl;
	d_instr->set_fetch_entry(FETCH_ENTRY_RELEASE);
	//	return 0;
	}

	sequencer_t *seq = d_instr->get_sequencer();
	ASSERT (seq);
    

	//sequencer_t *seq = proc->chip->get_sequencer_from_thread(SIM_get_proc_no(mem_op->ini_ptr));
	//ASSERT (seq);

	if (is_shadow) {
		return 0;
	}

	mai_t         *mai  = proc->chip->get_mai_from_thread(SIM_get_proc_no(mem_op->ini_ptr));
	fastsim_t  *fastsim = seq->get_fastsim ();

	if (fastsim->fastsim_mode ()) return 0;

    mem_trans_t *mem_trans = mem_hier->get_mem_trans();
    v9_mem_xaction_t *v9_mem_xaction = new v9_mem_xaction_t (mai, mem_op, space, map);
    mem_xaction_t       *mem_xaction = new mem_xaction_t (v9_mem_xaction,mem_trans); 
                                     
	if (!v9_mem_xaction->is_cpu_xaction ()) {
		return 0;	
	}

	int64 lat = 0; 

	if (v9_mem_xaction->is_icache_xaction ()) {
		lat = handle_icache (mem_xaction);
	} else {
		// data
		lsq_t       *lsq = seq->get_lsq ();

		lat = lsq->operate (mem_xaction); 

		if (mem_xaction->is_void_xaction ()) 
			ASSERT (mem_op->ignore == 1);
	}
	return lat;
}

cycles_t
mem_hier_handle_t::handle_icache (mem_xaction_t *mem_xaction) {
	uint64 latency;
	v9_mem_xaction_t *v9_mem_xaction = mem_xaction->get_v9 ();

	ASSERT (v9_mem_xaction->is_icache_xaction ());
	v9_mem_xaction->squashed_noreissue ();
	v9_mem_xaction->block_stc ();

//	with Simics 1.8.x
	dynamic_instr_t *seq_d_instr = sequencer_t::icache_d_instr;

//	with Simics 2.0	
	dynamic_instr_t *d_instr = v9_mem_xaction->get_dynamic_instr ();
    if (d_instr != seq_d_instr) d_instr = seq_d_instr;

	ASSERT(d_instr == sequencer_t::icache_d_instr);
	
    //addr_t fetch_addr = v9_mem_xaction->get_phys_addr();
    addr_t fetch_addr = mem_xaction->get_mem_hier_trans()->phys_addr;
    addr_t line_addr = fetch_addr - (fetch_addr % g_conf_l1i_lsize);
	d_instr->set_mem_transaction (mem_xaction);
	switch (d_instr->get_fetch_entry ()) {

	case FETCH_ENTRY_CREATE:
        if (g_conf_use_fetch_buffer) {
			fetch_buffer_t *fb = d_instr->get_sequencer()->get_fetch_buffer();
            latency = fb->initiate_fetch (line_addr, d_instr, 
                                mem_xaction->get_mem_hier_trans() );
            if (latency > 0)
                d_instr->set_fetch_entry(FETCH_ENTRY_HOLD);
            else
                d_instr->set_fetch_entry(FETCH_ENTRY_RELEASE);
        } else {
            d_instr->set_initiate_fetch(true);
            d_instr->set_fetch_entry (FETCH_ENTRY_HOLD);
            latency = g_conf_max_latency;
        }
		
		break;
		
	case FETCH_ENTRY_HOLD:
		FAIL;

		break;

	case FETCH_ENTRY_RELEASE:
        d_instr->set_fetch_entry (FETCH_ENTRY_DONE);
        latency = 0;

		break;

	default:
		FAIL;

	}
	
	return latency;
}

cycles_t
mem_hier_handle_t::snoop_operate (conf_object_t *proc_obj, conf_object_t *space, 
	map_list_t *map, generic_transaction_t *mem_op) {

	if (!g_conf_use_processor) {
		// TODO: implement
		//mem_driver->snoop_operate(proc_obj, space, map, mem_op);
	}
		
	if (!SIM_mem_op_is_from_cpu (mem_op)) return 0;

	// cpu mem op
	proc_object_t *proc = (proc_object_t *) proc_obj;

	bool is_shadow = false;

	sequencer_t *seq = proc->chip->get_sequencer_from_thread(SIM_get_proc_no(mem_op->ini_ptr));
	ASSERT (seq);

	if (is_shadow) return 0;

	fastsim_t  *fastsim = seq->get_fastsim ();

	if (fastsim->fastsim_mode ()) return 0;
	
	lsq_t         *lsq  = seq->get_lsq ();

	mem_xaction_t *mem_xaction = (mem_xaction_t *) mem_op->user_ptr;
	//ASSERT_WARN (mem_xaction);

	if (mem_xaction) lsq->apply_bypass_value (mem_xaction);

	return 0;
}

void
mem_hier_handle_t::stall_mem_hier()
{
    if (g_conf_use_mem_hier)
        mem_hier->checkpoint_and_quit();
}

void
mem_hier_handle_t::make_request (mem_trans_t *trans) {
//	conf_object_t *cpu = trans->ini_ptr;
	// Hack to tie memhier to sequencers not Simics cpus -- memhier doesn't really
	// use cpu ptrs so this is sortof OK for now.
	conf_object_t *cpu = (conf_object_t *) trans->mem_hier_seq;
	ASSERT (cpu);

	// If in a syscall and a regwin trap to user stack, send to previous
	// user cache, not current cache
	if (g_conf_optimize_os_user_spills) {
		uint32 tid = trans->dinst ? trans->dinst->get_tid() : 0; // FIX for SMT
		// uint32 old_cpu_id = trans->mem_hier_seq->get_id();
		uint32 cpu_id = trans->sequencer->get_prev_mh_seq(tid)->get_id();

		mai_t *mai = trans->sequencer->get_mai_object(tid);
		if (trans->is_as_user_asi() && mai->get_syscall_num() &&
			trans->sequencer->is_spill_trap(tid))
		{
			trans->cpu_id = cpu_id;
			ASSERT(cpu_id < g_conf_num_user_seq);
			// DEBUG_OUT("change from %u to %u -> \n", old_cpu_id, cpu_id);
			cpu = (conf_object_t *) trans->sequencer->get_prev_mh_seq(tid);
		}
	}
	// If a lpw access, send to this LWP's 'home' node
	if (g_conf_optimize_os_lwp) {
		uint32 tid = trans->dinst ? trans->dinst->get_tid() : 0; // FIX for SMT
		mai_t *mai = trans->sequencer->get_mai_object(tid);
		uint32 old_cpu_id = trans->mem_hier_seq->get_id();

		// If LWP access, or kernel spill fill that we don't mark as LWP
		if ((trans->get_access_type() == LWP_STACK_ACCESS) ||
			(mai->is_supervisor() && !trans->is_as_user_asi() &&
			 (trans->sequencer->is_spill_trap(tid) ||
			 trans->sequencer->is_fill_trap(tid))))
		{
			uint32 cpu_id = mai->get_id();
			trans->cpu_id = cpu_id;
			if (cpu_id != old_cpu_id) {
				DEBUG_OUT("LWP from %u to %u\n", old_cpu_id, cpu_id);
			}
			cpu = (conf_object_t *) chip->get_sequencer(cpu_id);
		}
	}
    
    if (trans->call_complete_request)
        trans->mark_pending(PROC_REQUEST);
    
	if (g_conf_use_mem_hier)
		mem_hier->make_request (cpu, trans);
	else if (iface) 
		iface->make_request (obj, cpu, trans);
	else
		complete_request (0, cpu, trans);
}

bool
mem_hier_handle_t::is_mem_hier_quiet()
{
    return (!g_conf_use_mem_hier || mem_hier->quiet_and_ready());
}

tick_t
mem_hier_handle_t::get_mem_g_cycles()
{
	return mem_hier->get_g_cycles();
}

void
mem_hier_handle_t::profile_commits(dynamic_instr_t *dinst)
{
	mem_hier->profile_commits(dinst);
}

void
mem_hier_handle_t::complete_request (conf_object_t *proc_obj, conf_object_t *cpu, 
	mem_trans_t *trans) {

    
//	ASSERT (trans->ini_ptr == cpu);
	ASSERT (g_conf_optimize_os_user_spills || g_conf_optimize_os_lwp ||
		trans->mem_hier_seq == (sequencer_t *) cpu);
	ASSERT(!trans->dinst || trans->dinst->get_sequencer() == trans->sequencer);

	ASSERT (trans->completed && trans->get_pending_events());
	ASSERT (!trans->dinst || trans->dinst->get_outstanding (PENDING_MEM_HIER));

    bool fetch = trans->ifetch;
    addr_t fetch_addr = 0;
    addr_t line_addr = 0;
    if (trans->ifetch) {
        fetch_addr = trans->phys_addr;
        line_addr = fetch_addr - (fetch_addr % g_conf_l1i_lsize);
    }
            
	if (trans->dinst) trans->dinst->wakeup ();
	
	if (trans->vcpu_state_transfer)
		trans->sequencer->complete_switch_request(trans);

    if (g_conf_use_fetch_buffer && fetch) { 

		trans->sequencer->get_fetch_buffer()->complete_fetch(line_addr, trans);
    }
    
    trans->clear_pending_event(PROC_REQUEST);
    
	FE_EXCEPTION_CHECK;
}

void
mem_hier_handle_t::invalidate_address (conf_object_t *proc_obj, 
	conf_object_t *cpu, invalidate_addr_t *invalid_addr) {
    // Huh ?    
	//bool is_shadow = false;
	//if (is_shadow) return;
	sequencer_t *seq = (sequencer_t *) cpu;
    chip->invalidate_address(seq, invalid_addr);
}

void
mem_hier_handle_t::static_invalidate_address (conf_object_t *proc_obj, 
	conf_object_t *cpu, invalidate_addr_t *invalid_addr) {

	proc_object_t *proc = (proc_object_t *) proc_obj;

	mem_hier_handle_t *mem_hier = proc->get_mem_hier ();
	mem_hier->invalidate_address (proc_obj, cpu, invalid_addr);
}


void
mem_hier_handle_t::static_complete_request (conf_object_t *proc_obj, 
	conf_object_t *cpu, mem_trans_t *trans) {

	proc_object_t *proc = (proc_object_t *) proc_obj;
	mem_hier_handle_t *mem_hier = proc->get_mem_hier ();
	mem_hier->complete_request (proc_obj, cpu, trans);
}


cycles_t
mem_hier_handle_t::static_snoop_operate (conf_object_t *proc_obj, conf_object_t *space, 
	map_list_t *map, generic_transaction_t *mem_op) {

	if (!g_conf_use_processor) {
		proc_object_t *proc = (proc_object_t *)proc_obj;
		return proc->mem_driver->snoop_operate(proc_obj, space, map, mem_op);
	}
		
	proc_object_t *proc = (proc_object_t *) proc_obj;
	mem_hier_handle_t *mem_hier = proc->get_mem_hier ();

	return ( mem_hier->snoop_operate (proc_obj, space, map, mem_op) );
}


cycles_t
mem_hier_handle_t::static_lsq_operate (conf_object_t *proc_obj, conf_object_t *space, 
	map_list_t *map, generic_transaction_t *mem_op) {

	if (!g_conf_use_processor) {
		proc_object_t *proc = (proc_object_t *)proc_obj;
		return proc->mem_driver->timing_operate(proc_obj, space, map, mem_op);
	}
		
	proc_object_t *proc = (proc_object_t *) proc_obj;
	mem_hier_handle_t *mem_hier = proc->get_mem_hier ();
	return ( mem_hier->lsq_operate (proc_obj, space, map, mem_op) );
}
void 
mem_hier_handle_t::printStats()
{
	mem_hier->printStats(); //dump DRAMSim2 stats in stdout
}
void
mem_hier_handle_t::print_stats (void)
{
	mem_hier->print_stats ();
}

void 
mem_hier_handle_t::clear_stats()
{
    mem_hier->clear_stats();
}

void
mem_hier_handle_t::set_cpus (conf_object_t **cpus, uint32 num_cpus)
{
	mem_hier->set_cpus(cpus, num_cpus);
}

void
mem_hier_handle_t::set_runtime_config (string config)
{
	// mh_runtime_config_file = config; 
	// mem_hier->read_runtime_config_file(config);
}

string
mem_hier_handle_t::get_runtime_config ()
{
	return mh_runtime_config_file; 
}

