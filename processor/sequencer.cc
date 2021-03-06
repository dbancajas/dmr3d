/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: sequencer.cc,v 1.12.2.56 2006/08/18 14:30:20 kchak Exp $");

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
#include "fetch_buffer.h"
#include "thread_scheduler.h"
#include "thread_context.h"
#include "yags.h"
#include "cascade.h"
#include "shb.h"
#include "mem_driver.h"
#include "mem_hier.h"
#include "profiles.h"

#ifdef POWER_COMPILE
#include "power_profile.h"
#include "power_defs.h"
#include "core_power.h"
#endif


void annotation_range::reinitialize(addr_t l, addr_t u)
{
    low = l;
    high = u;
}



// static initializations
uint64 sequencer_t::seq_id = 0;
dynamic_instr_t *sequencer_t::icache_d_instr = 0;
instruction_id_t sequencer_t::icache_instr_id = NULL;
dynamic_instr_t *sequencer_t::mmu_d_instr = 0;

sequencer_t::sequencer_t (chip_t *_p, uint32 _id, uint32 *mai_ids, uint32 _ctxts) 
    : num_hw_ctxt (_ctxts) 
{
	p = _p;
	id = _id;

    mai = new mai_t *[num_hw_ctxt];
    ctrl_flow = new ctrl_flow_t *[num_hw_ctxt];
    fetch_status = new uint32[num_hw_ctxt];
    icount      = new uint32[num_hw_ctxt];
    
    thread_ids = new uint32[num_hw_ctxt];
    
    check_thread_yield = new bool[num_hw_ctxt];
	thread_yield_reason = new ts_yield_reason_t[num_hw_ctxt];
	
	// Waiting to start new thread 
	wait_after_switch = new tick_t[num_hw_ctxt];
    last_thread_switch = new tick_t[num_hw_ctxt];
    wait_on_checkpoint = new bool[num_hw_ctxt];
	outstanding_state_loads = new uint32[num_hw_ctxt];
	outstanding_state_stores = new uint32[num_hw_ctxt];
	remaining_state_loads = new uint32[num_hw_ctxt];
	remaining_state_stores = new uint32[num_hw_ctxt];
	thread_switch_state = new thread_switch_state_t[num_hw_ctxt];
    mem_hier_seq = new sequencer_t *[num_hw_ctxt];
    prev_mh_seq = new sequencer_t *[num_hw_ctxt];
    early_interrupt_queue = new uint32[num_hw_ctxt];
    spin_heuristic = new spin_heuristic_t *[num_hw_ctxt];
    last_spinloop_commit = new uint64[num_hw_ctxt];
    
    current_function_start = new addr_t[num_hw_ctxt];
    current_function_end = new addr_t[num_hw_ctxt];
    current_function_index = new uint32[num_hw_ctxt];
    bzero (current_function_start, sizeof (addr_t) * num_hw_ctxt);
    bzero (current_function_end, sizeof (addr_t) * num_hw_ctxt);
    bzero (current_function_index, sizeof (uint32) * num_hw_ctxt);
    
    yags_ptr = new yags_t (g_conf_yags_choice_bits,
		g_conf_yags_exception_bits, g_conf_yags_tag_bits);
    cascade_ptr = new cascade_t(g_conf_cas_filter_bits,
		g_conf_cas_exception_bits, g_conf_cas_exception_shift);
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        mai[i] = p->get_mai_object(mai_ids[i]);
        if (mai[i]) {
            fetch_status[i] = 0x0;
            ctrl_flow[i] = new ctrl_flow_t (this, mai[i]->get_pc (), mai[i]->get_npc (), i, yags_ptr,
                cascade_ptr);
            uint32 mul_factor = (g_conf_narrow_core_switch) ? 2 : 1;
            mai[i]->set_reorder_buffer_size (g_conf_window_size * mul_factor);  
            fastsim = p->get_fastsim(mai_ids[i]);
        } else {
            fetch_status[i] = FETCH_GPC_UNKNOWN;
            ctrl_flow[i] = new ctrl_flow_t (this, 0, 0, i, yags_ptr, cascade_ptr);
        }
        
		mem_hier_seq[i] = this; 
		prev_mh_seq[i] = this; 
        thread_ids[i] = mai_ids[i];
        wait_after_switch[i] = 0;
        last_thread_switch[i] = 0;
        check_thread_yield[i] = false;
        wait_on_checkpoint[i] = false;
        icount[i] = 0;
		outstanding_state_loads[i] = 0;
		outstanding_state_stores[i] = 0;
		remaining_state_loads[i] = 0;
		remaining_state_stores[i] = 0;
		thread_switch_state[i] = THREAD_SWITCH_NONE;
        early_interrupt_queue[i] = 0;
        spin_heuristic[i] = new spin_heuristic_t(this, i);
        last_spinloop_commit[i] = 0;
        thread_yield_reason[i] = YIELD_NONE;
    
    }
	
	iwindow = new iwindow_t (g_conf_window_size);


	lsq = new lsq_t (this);
	st_buffer = new st_buffer_t (g_conf_stbuf_size, this);

    cout << "Number of Functional Unit Types = " << g_conf_fu_types[0] << endl;
	fus = new fu_t *[g_conf_fu_types[0]];
    for (uint32 i = 1; i < g_conf_fu_types[0]; i++ )
    {
        ASSERT(g_conf_fu_types[i] < FU_TYPES);
        fu_type_t unit = (fu_type_t) g_conf_fu_types[i];
        printf("Initializing fu_type:%d fu_count:%d fu_latencies:%d \n", (int)g_conf_fu_types[i], (int)g_conf_fu_count[i], (int)g_conf_fu_latencies[i]);
		fus [i - 1] = new fu_t (unit, g_conf_fu_latencies [i], 
			g_conf_fu_count [i]);
	}

	
	fetch_buffer = new fetch_buffer_t (this, num_hw_ctxt);
    
	eventq = new eventq_t (g_conf_eventq_size);
	dead_list = new wait_list_t (g_conf_dead_list_size);
	recycle_list = new wait_list_t (g_conf_recycle_list_size);
    print_mem_op = false;
    last_fetch_cycle = 0;
    
    register_count = 200;
    register_busy = new uint32[register_count];
    bzero(register_busy, register_count * sizeof(uint32));
    
	
#if 0
	if (g_conf_use_fastsim) 
		prepare_fastsim ();
#endif
	
	if (id < p->get_num_cpus())
		mmu_array = SIM_get_attribute(SIM_proc_no_2_ptr(id), "mmu").u.object;
	FE_EXCEPTION_CHECK;

	generate_pstats_list();
	last_mutex_try_lock = 0;
    spin_pc_index = 0;
    first_mutex_try_lock = new tick_t[g_conf_spin_loop_pcs[0] + 1];
    bzero(first_mutex_try_lock, (g_conf_spin_loop_pcs[0] + 1) * sizeof(tick_t));
    setup_sequencer_attributes();
    
    last_operand_dump_pc = 0;
    
#ifdef POWER_COMPILE    
    char seq_name[20];
    sprintf(seq_name, "sequencer-%d", id);
	profiles = new profiles_t(string(seq_name) + "-profiles");
	if (g_conf_power_profile) {
        core_power_profile = new core_power_t(seq_name, p->get_power_obj(), this);
        profiles->enqueue(core_power_profile);
    }
#endif    
	
}

sequencer_t::~sequencer_t (void) {
	delete iwindow;

	delete [] fus;
	delete ctrl_flow;
	delete lsq;
	delete st_buffer;

	delete eventq;
	delete dead_list;
	delete recycle_list;
}

void
sequencer_t::setup_sequencer_attributes()
{
    seq_max_issue = g_conf_vary_max_issue[0] ?
            g_conf_vary_max_issue[id + 1] : g_conf_max_issue;
            
    seq_max_commit = g_conf_vary_max_commit[0] ?
            g_conf_vary_max_commit[id + 1] : g_conf_max_commit;
            
    seq_max_fetch = g_conf_vary_max_fetch[0] ?
            g_conf_vary_max_fetch[id + 1] : g_conf_max_fetch;
        
    seq_issue_inorder = g_conf_vary_issue_inorder[0] ?
    g_conf_vary_issue_inorder[id + 1] : g_conf_issue_inorder;
    
    current_inactive_length = 0;
    
    datapath_width = g_conf_vary_datapath_width[0] ?
        g_conf_vary_datapath_width[id + 1] : 64;
    
            
}


uint64 
sequencer_t::generate_seq_id (void) {
	return seq_id++;
}

fu_t*
sequencer_t::get_fu_resource (fu_type_t unit) {
	if (unit == FU_TYPES) 
		return 0; 
	else 	
		return fus [unit];
}

chip_t *
sequencer_t::get_chip (void) {
	return p;
}

iwindow_t*
sequencer_t::get_iwindow (void) {
	return iwindow;
}

lsq_t*
sequencer_t::get_lsq (void) {
	return lsq;
}

st_buffer_t*
sequencer_t::get_st_buffer (void) {
	return st_buffer;
}

mai_t*
sequencer_t::get_mai_object (uint8 tid) {
	return mai[tid];
}

void 
sequencer_t::safety_checks (void) {
	if (SIM_get_pending_exception ()) FAIL;
}

void
sequencer_t::front_end_status () {
	memq_t *ldq = lsq->get_ldq ();
	memq_t *stq = lsq->get_stq ();

	// iwindow
    
    if (num_hw_ctxt == 1) {
        if (iwindow->empty ()) {
            reset_fu_status (0, FETCH_STALL_UNTIL_EMPTY); 
            reset_fu_status (0, FETCH_WINDOW_FULL);
            ASSERT (ldq->empty ());
            ASSERT (stq->empty ());
            
        } else if (iwindow->full ()) {
            set_fu_status (0, FETCH_WINDOW_FULL);
            proc_stats_t *pstat = get_pstats(0);
            dynamic_instr_t *block_instr = iwindow->head();
            if (block_instr->is_atomic())
                pstat->stat_iwindow_full_cause->inc_total(1, 0);
            else if (block_instr->is_store()) {
				if (block_instr->unsafe_store()) {
					pstat->stat_iwindow_full_cause->inc_total(1, 1);
					pstat->stat_iwindow_full_unsafe_st->inc_total(1, 
						block_instr->get_opcode() - i_add);
					//block_instr->print("iwin_full");
				}
				else 
					pstat->stat_iwindow_full_cause->inc_total(1, 2);
			}
            else if (block_instr->is_load()) {
				if (block_instr->unsafe_load()) {
					pstat->stat_iwindow_full_cause->inc_total(1, 3);
					//block_instr->print("iwin_full");
				}
				else 
					pstat->stat_iwindow_full_cause->inc_total(1, 4);
			}			
            else
                pstat->stat_iwindow_full_cause->inc_total(1, 5);
                
        } else {
            ASSERT (!iwindow->empty ());
            ASSERT (!iwindow->full ());
            
            reset_fu_status (0, FETCH_WINDOW_FULL);
        }
        
    } else {
        // SMT mode assume 2-way
        bool all_empty = true;
        for (uint32 i = 0; i < num_hw_ctxt; i++)
        {
            if (iwindow->empty_ctxt(i)) {
                reset_fu_status (i, FETCH_STALL_UNTIL_EMPTY); 
                reset_fu_status (i, FETCH_WINDOW_FULL);
            } else 
                all_empty = false;
        }
        if (all_empty) {
            ASSERT (ldq->empty ());
            ASSERT (stq->empty ());
        }
        
            
        if (iwindow->full()) {
            for (uint32 i = 0; i < num_hw_ctxt; i++)
                set_fu_status (i, FETCH_WINDOW_FULL);
        } else {
            for (uint32 i = 0; i < num_hw_ctxt; i++)
                reset_fu_status (i, FETCH_WINDOW_FULL);
        }
        
    }
        

	// ldq
	if (!ldq->full ()) {
        for (uint32 i = 0; i < num_hw_ctxt; i++)
            reset_fu_status (i, FETCH_LDQ_FULL);
	}

	// stq
	if (!stq->full ())  {
        for (uint32 i = 0; i < num_hw_ctxt; i++)
            reset_fu_status (i, FETCH_STQ_FULL);
	}
}

void 
sequencer_t::advance_cycle (void) {
    
#ifdef POWER_COMPILE    
    // Check to see if it is an active core or not
    // Optimistically assume full power savings
    if (g_conf_power_profile && mai[0]) {
        core_power_profile->update_power_stats();
        core_power_profile->clear_access_stats();
    }
    
#endif    
    
    bool early_return = true;
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        if (mai[i]) {
			early_return = false;
            break;
		}
    }
    
    if (early_return) {
        current_inactive_length++;
        return;
    }
    
    if (current_inactive_length) {
        proc_stats_t *pstats = get_pstats(0);
        pstats->stat_inactive_period_length->inc_total(1, current_inactive_length);
        current_inactive_length = 0;
    }
    
	if (fastsim && fastsim->fastsim_mode ()) {
		if (iwindow->empty ()) 
			if (fastsim->sim ())
				finish_fastsim ();

		return;	
	}

    update_register_busy ();
    
	handle_interrupt ();

	start ();
	finish ();

	eventq->advance_cycle ();

	schedule ();
	
	cleanup_dead ();

	front_end_status ();
	if (!g_conf_disable_stbuffer) st_buffer->advance_cycle ();

	safety_checks ();

    // For window do with 0th context
    proc_stats_t *pstats = get_pstats(0);
	if (pstats) pstats->iwindow_stats (iwindow);
	
	handle_simulation ();

	forward_progress_check ();

	check_for_thread_switch ();
}

void
sequencer_t::check_for_thread_switch()
{
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        proc_stats_t *pstats = get_pstats (i);
        if (mai[i]) {
			STAT_INC (pstats->stat_cycles);  // only when ctxt not idle
		}
        else
			STAT_INC(pstats->stat_idle_context_cycles);
    }
    
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
		// These updates, despite thread_switch_state, ensures that
		// thread_yield() is called every PREEMPT_CYCLES reguardless of how 
		// long it takes to move state in and out
        if (g_conf_scheduling_algorithm == VANILLA_GANG_SCHEDULING_POLICY)
            last_thread_switch[i]++;
        else if (g_conf_scheduling_algorithm == BANK_SCHEDULING_POLICY)
            last_thread_switch[i]++;
		
		else if (thread_switch_state[i] == THREAD_SWITCH_NONE)
                last_thread_switch[i]++;
	
        if (!mai[i]) continue;
        switch (thread_switch_state[i]) {
		case THREAD_SWITCH_NONE:
		
			// if (g_conf_scheduling_algorithm != VANILLA_GANG_SCHEDULING_POLICY)
            //     last_thread_switch[i]++;
			
            if (g_conf_paused_vcpu_interrupt && mai[i]->is_pending_interrupt())
            {
                thread_switch_state[i] = THREAD_SWITCH_HANDLING_INTERRUPT;
                break;
            }
            
            if (g_conf_no_os_pause && mai[i]->can_preempt())
            {
                if (early_interrupt_queue[i] && 
                    p->get_scheduler()->waiting_interrupt_threads())
                {
                    early_interrupt_queue[i]--;
                    potential_thread_switch(i, YIELD_PAUSED_THREAD_INTERRUPT);
                    
                } else if (last_thread_switch[i] > (tick_t) g_conf_thread_preempt)
                    potential_thread_switch(i, YIELD_PROC_MODE_CHANGE);
            }
            
            
			// Check for long running
			if (g_conf_thread_preempt && 
				last_thread_switch[i] == (tick_t) g_conf_thread_preempt)
			{
				potential_thread_switch(i, YIELD_LONG_RUNNING);
			}
            else if (g_conf_long_thread_preempt &&
                last_thread_switch[i] >= (tick_t) g_conf_long_thread_preempt)
            {
                potential_thread_switch(i, YIELD_EXTRA_LONG_RUNNING);
            }
            
			break;
		
		case THREAD_SWITCH_CHECK_YIELD:
			ASSERT(thread_yield_reason[i] != YIELD_NONE);
            //ASSERT(last_thread_switch[i] <= (tick_t)g_conf_long_thread_preempt);
			if (p->get_scheduler()->thread_yield(this , i, mai[i], 
				thread_yield_reason[i]))
			{
                
				set_fu_status(i, FETCH_PENDING_SWITCH);
                // Mark spin loop if necessary
                if (thread_yield_reason[i] == YIELD_MUTEX_LOCKED)
                    mai[i]->set_spin_loop(true);
                else
                    mai[i]->set_spin_loop(false);
                
				if (!g_conf_use_processor)
					p->get_mem_driver()->stall_thread(mai[i]->get_id());
                proc_stats_t *pstats = get_pstats (0);
                pstats->stat_yield_reason->inc_total(1, thread_yield_reason[i]);
                if (g_conf_narrow_core_switch) {
                    p->narrow_core_switch(this);
                    thread_switch_state[i] = THREAD_SWITCH_NARROW_CORE;
                } else {
                    thread_switch_state[i] = THREAD_SWITCH_OUT_START;
                }
				break;
			}
			// If not deferring on switch due to OS mode then
            // do not try for a while. Otherwise, we will either
            // switch on PROC_MODE_CHANGE or when the second timer
            // expires
			else if (!g_conf_no_os_pause && thread_yield_reason[i] == YIELD_LONG_RUNNING)
				last_thread_switch[i] = 0;
			
			thread_yield_reason[i] = YIELD_NONE;
			thread_switch_state[i] = THREAD_SWITCH_NONE;
			break;
	
        case THREAD_SWITCH_OUT_START:
			STAT_SET(get_pstats(i)->stat_thread_switch_start,
				p->get_g_cycles());
			// Initiated switch; Waiting for pipe to drain
            
                
			if (iwindow->empty_ctxt(i) && st_buffer->empty_ctxt(i)) {
				if (g_conf_cache_vcpu_state) {
					store_vcpu_state(i);
					thread_switch_state[i] = THREAD_SWITCH_OUT_WAIT_CACHE;
				} else { 
					wait_after_switch[i] = g_conf_thread_switch_latency;
					thread_switch_state[i] = THREAD_SWITCH_OUT_WAIT_TIMER;
				}
			}
			break;

		case THREAD_SWITCH_OUT_WAIT_CACHE:
			if (!outstanding_state_stores[i]) {
				switch_out_epilogue(i);
			}
			break;
			
		case THREAD_SWITCH_OUT_WAIT_TIMER:
			wait_after_switch[i]--;
			if (wait_after_switch[i] == 0) {
				switch_out_epilogue(i);
			}
			break;
			
		case THREAD_SWITCH_IN_START:
			STAT_SET(get_pstats(i)->stat_thread_switch_start,
				p->get_g_cycles());
			// Initiating switch in
            ASSERT(iwindow->empty_ctxt(i) && st_buffer->empty_ctxt(i));
			if (g_conf_cache_vcpu_state) {
				fetch_vcpu_state(i);
				thread_switch_state[i] = THREAD_SWITCH_IN_WAIT_CACHE;
			} else { 
				wait_after_switch[i] = g_conf_thread_switch_latency;
				thread_switch_state[i] = THREAD_SWITCH_IN_WAIT_TIMER;
			}
			break;

		case THREAD_SWITCH_IN_WAIT_CACHE:
			if (!outstanding_state_loads[i]) {
				switch_in_epilogue(i);
			}
			break;
				
		case THREAD_SWITCH_IN_WAIT_TIMER:
			wait_after_switch[i]--;
			if (wait_after_switch[i] == 0) {
				switch_in_epilogue(i);
			}
			break;
            
        case THREAD_SWITCH_HANDLING_INTERRUPT:
            //wait_after_switch[i]--;
            last_thread_switch[i]++;
            ASSERT(g_conf_use_processor || !p->get_mem_driver()->is_paused(mai[i]->get_id()));
            if (outstanding_state_stores[i] || remaining_state_stores[i] ||
                outstanding_state_loads[i] || remaining_state_loads[i])
                break;
            if (mai[i]->can_preempt()) {
                if (thread_yield_reason[i] == YIELD_NONE) {
                    thread_switch_state[i] = THREAD_SWITCH_NONE;
                    if (last_thread_switch[i] >= (tick_t)g_conf_thread_preempt)
                        potential_thread_switch(i, YIELD_LONG_RUNNING);
                    else if (early_interrupt_queue[i]) {
                        if (!g_conf_no_os_pause || 
                            p->get_scheduler()->waiting_interrupt_threads()) {
                        
                            early_interrupt_queue[i]--;
                            potential_thread_switch(i, YIELD_PAUSED_THREAD_INTERRUPT);
                        }
                    }
                } else {
                    thread_switch_state[i] = THREAD_SWITCH_CHECK_YIELD;
                }
            } else if (last_thread_switch[i] >= (tick_t)g_conf_long_thread_preempt) {
                // MUST honour the long preempt
                thread_switch_state[i] = THREAD_SWITCH_CHECK_YIELD;
                thread_yield_reason[i] = YIELD_EXTRA_LONG_RUNNING;
            }
            break;
		
        case THREAD_SWITCH_NARROW_CORE:
            if (iwindow->empty_ctxt(i) && st_buffer->empty_ctxt(i)) {
                //p->narrow_core_switch(this);
                sequencer_t *r_seq = p->get_remote_sequencer(this);
                if (r_seq->get_fu_status(i, FETCH_STALL_UNTIL_EMPTY))
                    r_seq->reset_fu_status(i, FETCH_STALL_UNTIL_EMPTY);
                thread_switch_state[i] = THREAD_SWITCH_NONE;
                thread_yield_reason[i] = YIELD_NONE;
                reset_fu_status(i, FETCH_PENDING_SWITCH);
                set_fu_status(i, FETCH_GPC_UNKNOWN);
                mai[i] = 0;
            }
            break;
		default:
			FAIL;
		}
	}
}

void
sequencer_t::forward_progress_check () {
    
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        proc_stats_t *pstats = get_pstats (i);
        if (pstats) {
            uint64 commits = STAT_GET (pstats->stat_commits);
            if (g_conf_spin_check_interval && 
                commits - last_spinloop_commit[i] > (uint64)g_conf_spin_check_interval
                && mai[i])
            {
                bool spin = spin_heuristic[i]->check_spinloop();
                if (spin) {
                    //DEBUG_OUT("seq%u - vcpu%u : spinloop ?? last_pc_commit %llx @ %llu\n",
                    //    id, mai[i]->get_id(), last_pc_commit, p->get_g_cycles());
                    potential_thread_switch(i, YIELD_MUTEX_LOCKED);
                    STAT_INC (pstats->stat_detected_spins);
                }
                last_spinloop_commit[i] = commits;
            }
            
            uint64 check_cycle = STAT_GET (pstats->stat_check_cycle);
            if (check_cycle == p->get_g_cycles ()) {
                uint64 last_commits = STAT_GET (pstats->stat_last_commits);
                check_cycle += g_conf_check_freq;
                STAT_SET (pstats->stat_check_cycle, check_cycle);
                STAT_SET (pstats->stat_last_commits, commits);
                if (last_commits == commits && mai[i] 
                    && last_thread_switch[i] >= 10000) {
                    mai[i]->piq();
                    DEBUG_OUT ("No forward progress for Seq %d ctxt %d at %llu \n",
                    get_id(),i, p->get_g_cycles()); 
                    FAIL;
                }
            }
        }
        
    }
}

void 
sequencer_t::handle_simulation () {

    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        if (wait_on_checkpoint[i]) {
            ASSERT(mai[i]);
            if (mai[i]->get_tl() == 0) {
                wait_on_checkpoint[i] = false;
                set_fu_status(i, FETCH_PENDING_CHECKPOINT);
            }
            STAT_INC(get_pstats(i)->stat_checkpoint_penalty);
        } else if (get_fu_status(i, FETCH_PENDING_CHECKPOINT)) {
            if (iwindow->empty_ctxt(i) && mai[i] && mai[i]->get_tl() != 0) {
                wait_on_checkpoint[i] = true;
                reset_fu_status(i, FETCH_PENDING_CHECKPOINT);
            }
            STAT_INC(get_pstats(i)->stat_checkpoint_penalty);
        }
        
    }

	//ASSERT(mai[0]);

    
	uint64 total_commits = 0;
	uint64 total_cycles = 0;

    for (uint32 c = 0; c < num_hw_ctxt; c++)
    {
        if (mai[c])
        {
            
			uint32 i = 0;
            while (pstats_list[c][i] != 0) {
                total_commits += (uint64) pstats_list[c][i]->stat_commits->get_total ();
                total_cycles += (uint64) pstats_list[c][i]->stat_cycles->get_total ();
                
                i++;
            }
            
            /*
            if (g_conf_run_cycles && 
            p->get_g_cycles () == (uint64) g_conf_run_cycles) {
                
                structure_stats ();
                p->print_stats ();
                p->get_mem_hier()->print_stats();
                mai[c]->break_sim (0);
            }
            
            if (g_conf_run_commits && 
            total_commits >= (uint64) g_conf_run_commits) {
                
                structure_stats ();
                p->print_stats ();
                p->get_mem_hier()->print_stats();
                mai[c]->break_sim (0);
            }
            */
        }
        
    }
    
	if ( g_conf_heart_beat && ((p->get_g_cycles () % g_conf_heart_beat_cycles) == 0) ) 
		DEBUG_OUT ("seq%d committed %lld instructions in %lld cycles\n", 
			get_id (), total_commits, total_cycles);
}

void
sequencer_t::structure_stats () {
	proc_stats_t *pstats = get_pstats (0);

	STAT_SET (pstats->stat_dead_list_size, dead_list->get_size ());
	STAT_SET (pstats->stat_recycle_list_size, recycle_list->get_size ());
}


void 
sequencer_t::prepare_fastsim () {
    
    // Not handle fastsim in SMT
	squash_inflight_instructions (0);	
	ASSERT (!fastsim->fastsim_mode ());

	fastsim->sim_icount (g_conf_fastsim_icount);

    /* FastSIM won't work with SMT */
	if (mai[0] && mai[0]->pending_interrupt())
		fastsim->set_interrupt (mai[0]->get_interrupt ());
}

void 
sequencer_t::finish_fastsim () {
	ASSERT (!fastsim->fastsim_mode ());
    // TODO SMT with fastsim
	mai[0]->set_interrupt (fastsim->get_interrupt ());
}


uint8
sequencer_t::mai_to_tid(mai_t *_ma)
{
    uint8 tid ;
    for (tid = 0 ; tid < num_hw_ctxt; tid++)
    {
        if (mai[tid] == _ma) return tid;
    }
	FAIL;
}

void 
sequencer_t::prepare_for_interrupt (mai_t *_ma) {
    uint8 tid = mai_to_tid(_ma);
	ASSERT(tid < num_hw_ctxt && mai[tid]);
	
	if (mai[tid]->pending_interrupt ()) 
	{
		if (!get_fu_status (tid, FETCH_STALL_UNTIL_EMPTY)) 
			set_fu_status (tid, FETCH_STALL_UNTIL_EMPTY);
		
		ASSERT (st_buffer->empty_ctxt (tid));

		if (g_conf_handle_interrupt_early)
			squash_inflight_instructions (tid);
	}
}

void
sequencer_t::squash_inflight_instructions (uint8 tid, bool force_squash) {

	dynamic_instr_t *d_instr = iwindow->head ();
	if (!d_instr) return;

	if (d_instr->get_tid() == tid && 
        (d_instr->get_pipe_stage () <= PIPE_EXECUTE || force_squash)) {
		d_instr->kill_all ();
		return;
	}

	uint32 index = iwindow->get_last_created ();
	uint32  head = iwindow->get_last_committed ();

	do {
		d_instr      = iwindow->peek (index);
		index        = iwindow->window_decrement (index);

		if (d_instr->get_tid() == tid && 
            d_instr->get_sync () && d_instr->get_pipe_stage () > PIPE_EXECUTE) break;

	} while (index != head);

	if (d_instr->get_next_d_instr ())
		d_instr->get_next_d_instr ()->kill_all ();
}

uint8 sequencer_t::select_thread_for_fetch(list<uint8> tlist)
{
       if (tlist.size() == 1) return tlist.front();
       uint32 selected_tid = tlist.front();
       list<uint8>::iterator it = tlist.begin();
       while (it != tlist.end())
       {
           if (icount[*it] < icount[selected_tid])
               selected_tid = *it;
           it++;
       }
       return selected_tid;
       //return p->get_g_cycles() % num_hw_ctxt;
}

void
sequencer_t::start (void) {

	uint32 fetch_avail = g_conf_max_fetch;
    // Need to select thread to fetch from
    list<uint8> tid_list;
    for (uint32 i = 0; i < num_hw_ctxt; i++)
        if (fu_ready(i)) tid_list.push_back(i);
	
    while (tid_list.size()  && fetch_avail > 0 && iwindow->slot_avail ()) {
        uint8 tid = select_thread_for_fetch (tid_list);
        if (!fu_ready(tid) || thread_switch_state[tid] != THREAD_SWITCH_NONE) {
			//ASSERT(get_fu_status(tid, FETCH_PENDING_CHECKPOINT));
			tid_list.remove(tid);
			continue;
		}
        
        if (p->get_scheduler()->thread_yield(ctrl_flow[tid]->get_pc(), this, tid))
        {
            potential_thread_switch(tid, YIELD_FUNCTION);
            break;
        }
        if (g_conf_narrow_core_switch && determine_narrow_core_switch(tid))
        {
            potential_thread_switch(tid, YIELD_NARROW_CORE);
            break;
        }
        
        dynamic_instr_t *d_instr = 0;
    	if (g_conf_recycle_instr)
			d_instr = recycle (tid);
		if (!d_instr)
			d_instr = new dynamic_instr_t (this, tid);
        icount[tid]++;
		d_instr->set_pipe_stage (PIPE_INSERT);
		d_instr->wakeup ();
        
        if (!fu_ready(tid)) tid_list.remove(tid);
		fetch_avail--;
	}

    // Handle Fetch stat ... no support for SMT
    proc_stats_t *pstat = get_pstats(0);
    proc_stats_t *tstat = get_tstats(0);
    // SMT handling should be handled separately
    if (fetch_avail < g_conf_max_fetch) {
        if (pstat) STAT_INC(pstat->stat_fetch_cycles);       
        if (tstat) STAT_INC(tstat->stat_fetch_cycles);
    } else {
        // COULD not fetch
        uint32 fetch_stall_cause = 0;
        if (get_fu_status(0, FETCH_LDQ_FULL)) 
            fetch_stall_cause = 1;
        else if (get_fu_status(0, FETCH_STQ_FULL))
            fetch_stall_cause = 2;
        else if (get_fu_status(0, FETCH_STALL_UNTIL_EMPTY)) {
			// check for exception or sync2
			dynamic_instr_t *tail = iwindow->tail(0);
			if (tail && tail->get_sync())
				fetch_stall_cause = 5;
			else
				fetch_stall_cause = 4;
		}
        else if (get_fu_status(0, FETCH_GPC_UNKNOWN))
            fetch_stall_cause = 3;
        else if ((get_fu_status(0, FETCH_WINDOW_FULL) && !iwindow->full()) ||
                get_fu_status(0, FETCH_PENDING_SWITCH) || get_fu_status(0, FETCH_PENDING_CHECKPOINT))
            fetch_stall_cause = 6;
        else if (!iwindow->full() && thread_switch_state[0] == THREAD_SWITCH_NONE)
            ASSERT_MSG(false, "FAILED AS COULD NOT JUSTIFY REASON FOR FETCH STALL");
        if (pstat) pstat->stat_fetch_stall_breakdown->inc_total(1, fetch_stall_cause);
        if (tstat) tstat->stat_fetch_stall_breakdown->inc_total(1, fetch_stall_cause);
    }
        
}

bool sequencer_t::determine_narrow_core_switch(uint8 tid)
{
    addr_t PC = ctrl_flow[tid]->get_pc();
    if (curr_annotation_range.low <= PC && curr_annotation_range.high > PC)
        return false;
    uint32 width = query_annotation(PC);
    if (!width) return false;
    if (width == datapath_width) return false;
    // Assume 2-way narrow core
    if (id == 0) {
        if (width <= g_conf_vary_datapath_width[2]) return true;
        return false;
    } else {
        if (width > datapath_width) return true;
        return false;
    }
        
}


void sequencer_t::smt_commit (void) {
    uint32 commit_avail = g_conf_max_commit;
    
    set<uint8> tids_processed;
    uint32 tid;
    
    while (commit_avail > 0 && !iwindow->empty() && 
           tids_processed.size() < num_hw_ctxt)
    {
        dynamic_instr_t *d_instr = iwindow->peek_instr_for_commit(tids_processed);
        if (d_instr) tid = d_instr->get_tid();
        if (d_instr && process_instr_for_commit (d_instr)) {
            proc_stats_t *pstats = get_pstats (d_instr->get_tid());
            proc_stats_t *tstats = get_tstats (d_instr->get_tid());
            STAT_INC (pstats->stat_commits);
            if (tstats)
				STAT_INC (tstats->stat_commits);
            if (!mai[tid]->is_supervisor()) {
                STAT_INC(pstats->stat_user_commits);
                if (tstats) STAT_INC(tstats->stat_user_commits);
            }
          
            commit_avail--;
            
        } else {
            if (d_instr) tids_processed.insert(d_instr->get_tid());
            else break;
        }
    }
    
    
}

bool sequencer_t::process_instr_for_commit (dynamic_instr_t *d_instr)
{
    bool retire = false;
    ASSERT(d_instr);
    
    uint32 tid = d_instr->get_tid();
    proc_stats_t *pstats = get_pstats(tid);
    proc_stats_t *tstats = get_tstats(tid);
    if (d_instr->speculative())
        d_instr->get_mai_instruction()->correct_instruction();
    
    if (d_instr->speculative ()) {
        // DEBUG_OUT ("seq%u : speculative head @ %llu\n", id, p->get_g_cycles ());
        if (d_instr->get_pipe_stage () <= PIPE_EXECUTE)
            return false;
        
        STAT_INC(pstats->stat_speculative_head);
        STAT_INC(tstats->stat_speculative_head); 
        if (g_conf_trace_pipe) d_instr->print("spec_head->kill"); 
        d_instr->kill_all ();
        
        
        
    } else if (d_instr->get_pipe_stage () == PIPE_MEM_ACCESS) {
        if (d_instr->is_load ()) {
            d_instr->ld_mem_access ();
        } else if (d_instr->is_store ()) {
            if (d_instr->immediate_release_store () && !st_buffer->empty ())
               return false;
            d_instr->st_mem_access ();
        } else FAIL;
        
        
    } else if (d_instr->get_pipe_stage () == PIPE_MEM_ACCESS_SAFE) {
        ASSERT (d_instr->is_load ());
        if (!st_buffer->empty ()) return false;
        
        d_instr->safe_ld_mem_access (); 
        
    } else if (d_instr->retire_ready ()) {
        if (d_instr->is_store () && 
            !d_instr->immediate_release_store () &&
            st_buffer->full ()) 
            return false;
        
        if (d_instr->is_store() && d_instr->immediate_release_store() &&
            !st_buffer->empty_ctxt(tid))
            return false;
        if (d_instr->sequencing_membar() && !st_buffer->empty_ctxt(tid))
            return false;
        //if (d_instr->is_load() && d_instr->unsafe_load() && !st_buffer->empty_ctxt())
        //    return false;
        last_pc_commit = d_instr->get_pc();
        if (!mai[tid]->is_supervisor()) {
            if (last_pc_commit < 0xff00000 && (last_pc_commit & 0xff000000) == 0)
               mai[tid]->set_last_user_pc(last_pc_commit);
        }
        
        d_instr->retire ();
        retire = true;
    }
    
    return retire;
    
}

void 
sequencer_t::finish (void) {
    
    if (num_hw_ctxt > 1) {
        smt_commit();
        return;
    }
    
    if (g_conf_narrow_core_switch && thread_switch_state[0] == THREAD_SWITCH_NONE)
    {
        sequencer_t *seq = p->get_remote_sequencer(this);
        if (seq->get_mai_object(0) == mai[0])
            return;
    }
        
    
	proc_stats_t *pstats = get_pstats (0);
	uint32 commit_avail = g_conf_max_commit;
    uint32 tid;
    
	while (commit_avail > 0 && !iwindow->empty ()) {
		dynamic_instr_t *d_instr = iwindow->head ();
		ASSERT (d_instr);
        tid = d_instr->get_tid();
        tick_t fetch_cycle = d_instr->get_fetch_cycle();
		if (process_instr_for_commit (d_instr)) {

            proc_stats_t *tstats = get_tstats (tid);
			STAT_INC (pstats->stat_commits);
			STAT_INC (tstats->stat_commits);
            last_commit_pc = d_instr->get_pc();
            if (!mai[tid]->is_supervisor()) {
                STAT_INC(pstats->stat_user_commits);
                STAT_INC(tstats->stat_user_commits);
            }
            if (fetch_cycle > last_fetch_cycle)
            {
                last_fetch_cycle = fetch_cycle;
                STAT_INC(pstats->stat_useful_fetch_cycles);
                STAT_INC(tstats->stat_useful_fetch_cycles);
            }
			commit_avail--;

		} else 
			break;
	}
}

void
sequencer_t::insert_dead (dynamic_instr_t *d_instr) {

	wait_list_t *wl = dead_list->get_end_wait_list ();
	if (wl->full ()) wl = dead_list->create_wait_list ();

	wl->insert (d_instr);

//	ASSERT (!dead_list->full ());
}

void
sequencer_t::insert_recycle (dynamic_instr_t *d_instr) {

	wait_list_t *wl = recycle_list->get_end_wait_list ();
	if (wl->full ()) wl = recycle_list->create_wait_list ();

	wl->insert (d_instr);

//	ASSERT (!recycle_list->full ());
}

dynamic_instr_t*
sequencer_t::recycle (uint8 tid) {
	dynamic_instr_t *d_instr = recycle_list->head ();

	if (d_instr) {
		recycle_list->pop_head (d_instr);
		adjust_wait_list (recycle_list);

		d_instr->recycle (tid);
	}

	return d_instr;
}

wait_list_t*
sequencer_t::adjust_wait_list (wait_list_t* & start_wl) {
	wait_list_t *next_wl = 0, *wl;

	if (!start_wl->empty ()) return start_wl;

	wait_list_t *end_wl = start_wl->get_end_wait_list ();

	for (wl = start_wl; 
		wl && wl->get_next_wl () && wl->empty (); 
		wl = next_wl) {

		next_wl = wl->get_next_wl ();

		ASSERT (wl->empty ());
		wl->set_end_wait_list (0);
		wl->set_next_wait_list (0);

		delete wl;
	}

//	ASSERT (!wl->empty ());
	wl->set_end_wait_list (end_wl);

	start_wl = wl;
	return start_wl;
}

void
sequencer_t::cleanup_dead (void) {
	wait_list_t *wl = dead_list;

	for (; wl; wl = wl->get_next_wl ()) {
		while (wl->head ()) {
			dynamic_instr_t *d_instr = wl->head ();
			if (d_instr->get_outstanding () == 0x0) {
				wl->pop_head (d_instr);

				if (g_conf_recycle_instr)
					insert_recycle (d_instr);
				else	
					delete d_instr;

			} else 
				break;
		}
	}

	adjust_wait_list (dead_list);
}

bool
sequencer_t::get_fu_status (uint8 tid, uint32 s) {
	return ((fetch_status[tid] & s) == s);	
}

uint32
sequencer_t::get_fu_status (uint8 tid) {
	return fetch_status[tid];
}

void
sequencer_t::set_fu_status (uint8 tid, uint32 s) {
	fetch_status[tid] = (fetch_status[tid] | s);
}

void
sequencer_t::reset_fu_status (uint8 tid, uint32 s) {
	fetch_status[tid] = (fetch_status[tid] & (~s));
}

bool
sequencer_t::fu_ready (uint8 tid) {
	return (fetch_status[tid] == 0x0);
}

addr_t
sequencer_t::get_pc (uint8 tid) {
	return (ctrl_flow[tid]->get_pc ());
}

addr_t
sequencer_t::get_npc (uint8 tid) {
	return (ctrl_flow[tid]->get_npc ());
}

ctrl_flow_t*
sequencer_t::get_ctrl_flow (uint8 tid) {
	return ctrl_flow[tid];
}

void
sequencer_t::handle_interrupt () {
    
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        
        if (mai[i] && mai[i]->pending_interrupt() && thread_switch_state[i] == THREAD_SWITCH_NONE)
            mai[i]->handle_interrupt (this, get_pstats(i));
        
    }
}


bool
sequencer_t::st_buffer_empty()
{
    return st_buffer->empty();
}

int64
sequencer_t::get_interrupt (uint8 tid) {
	return mai[tid]->get_interrupt ();
}

int64
sequencer_t::get_shadow_interrupt (uint8 tid) {
	return 0; //shadow_ivec.get ();
}


mem_hier_handle_t*
sequencer_t::get_mem_hier () {
	return (p->get_mem_hier ());
}

eventq_t*
sequencer_t::get_eventq () {
	return eventq;
}

void 
sequencer_t::schedule () {
	uint32 execute_avail = seq_max_issue;
	dynamic_instr_t *d_instr;

	if (iwindow->empty ()) return;
	
	uint32 last_created = iwindow->get_last_created ();
	uint32 index = iwindow->get_last_committed ();

	index = iwindow->window_increment (index);
	d_instr = iwindow->peek (index);

	while (execute_avail > 0 && d_instr) {
		mai_instruction_t *mai_instr = d_instr->get_mai_instruction ();
		fu_t *fu = d_instr->get_fu ();
		bool b = true;
        uint32 wait_reason = 0;

		if (d_instr->get_sync () && 
            ((iwindow->head_per_thread(d_instr->get_tid()) !=  d_instr) ||
            !st_buffer->empty_ctxt(d_instr->get_tid()))) 
			b = false;

        wait_reason = b ? wait_reason + 1 : wait_reason ;    
		if (b && d_instr->is_load () && g_conf_issue_load_commit && 
			iwindow->head () != d_instr) 
			b = false;

        wait_reason = b ? wait_reason + 1 : wait_reason ;    
		if (b && d_instr->is_store () && g_conf_issue_store_commit && 
			iwindow->head () != d_instr)
			b = false;

        wait_reason = b ? wait_reason + 1 : wait_reason ;    
		if (b && g_conf_issue_non_spec && d_instr->speculative ())
			b = false;

        wait_reason = b ? wait_reason + 1 : wait_reason ;
		if (b && d_instr->is_load () && g_conf_issue_loads_non_spec &&
			d_instr->speculative ()) 
			b = false;

        wait_reason = b ? wait_reason + 1 : wait_reason ;    
        // if (b && d_instr->is_load() && d_instr->unsafe_load() && (!st_buffer->empty_ctxt() ))
        //     b = false;
        
        wait_reason = b ? wait_reason + 1 : wait_reason ;
        // if (b && d_instr->is_store() && d_instr->unsafe_store() && !st_buffer->empty_ctxt() )
            // b = false;
        
        wait_reason = b ? wait_reason + 1 : wait_reason ;
		if (b && d_instr->is_store () && g_conf_issue_stores_non_spec &&
			d_instr->speculative ())
			b = false;

        wait_reason = b ? wait_reason + 1 : wait_reason ;    
		if (b && d_instr->get_pipe_stage () == PIPE_WAIT &&
			!d_instr->check_register_busy() && mai_instr->readyto_execute () && fu->get_fu ()) {
            execute_avail--;
			d_instr->set_pipe_stage (PIPE_EXECUTE);

            ASSERT(!d_instr->get_sync() ||
                iwindow->head_per_thread(d_instr->get_tid()) == d_instr);
			eventq->insert (d_instr, fu->get_latency ());
            
        } else {
            b = false;
            if (seq_issue_inorder) 
                break;
        }	

        if (!b && d_instr->get_pipe_stage () == PIPE_WAIT) {
            // Instruction not issued
            proc_stats_t *pstats = get_pstats (d_instr->get_tid ());
            proc_stats_t *tstats = get_tstats (d_instr->get_tid ());
            STAT_INC (pstats->stat_wait_reason, wait_reason);
            STAT_INC (tstats->stat_wait_reason, wait_reason);
        }
		// need to reach here.
		index = iwindow->window_increment (index);
		if (index == iwindow->window_increment (last_created))
			d_instr = 0;
		else 
			d_instr = iwindow->peek (index);
	}
}

fastsim_t*
sequencer_t::get_fastsim (void) {
	return fastsim;
}


void
sequencer_t::write_checkpoint(FILE *file) 
{
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        fprintf(file, "%llu\n", last_spinloop_commit[i]);
        fprintf(file, "%u\n", (uint32)thread_ids[i]);
        ctrl_flow[i]->write_checkpoint(file);

		uint32 j = 0;
		while (pstats_list[i][j] != 0) {
			pstats_list[i][j]->stats->to_file(file);
			j++;
		}
    }
	// Put in the mem_hier_seq too
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        fprintf(file, "%u\n", mem_hier_seq[i] ? mem_hier_seq[i]->get_id(): p->get_num_sequencers());   
        fprintf(file, "%u\n", prev_mh_seq[i] ? prev_mh_seq[i]->get_id() : p->get_num_sequencers());   
    }
	lsq->write_checkpoint(file);
    yags_ptr->write_checkpoint(file);
    cascade_ptr->write_checkpoint(file);
    
#ifdef POWER_COMPILE
    if (g_conf_power_profile) 
        core_power_profile->to_file(file);
#endif        
}

void
sequencer_t::read_checkpoint(FILE *file)
{
    
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        fscanf(file, "%llu\n", &last_spinloop_commit[i]);
   
        fscanf(file, "%u\n", &thread_ids[i]);
        mai[i] = p->get_mai_object(thread_ids[i]);
        ctrl_flow[i]->read_checkpoint(file);
		
		uint32 j = 0;
		while (pstats_list[i][j] != 0) {
			pstats_list[i][j]->stats->from_file(file);
			j++;
		}
        if (!mai[i]) set_fu_status(i, FETCH_GPC_UNKNOWN);
        else reset_fu_status(i, FETCH_GPC_UNKNOWN);
    }

    uint32 mem_seq_id, prev_mh_seq_id;
    uint32 num_seqs = p->get_num_sequencers();
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        fscanf(file, "%u\n", &mem_seq_id);
        mem_hier_seq[i] = mem_seq_id < num_seqs ? p->get_sequencer(mem_seq_id) : 0;
        fscanf(file, "%u\n", &prev_mh_seq_id);
		if (g_conf_csp_use_mmu && mai[i])
			mai[i]->switch_mmu_array(mem_hier_seq[i]->mmu_array);
        prev_mh_seq[i] = prev_mh_seq_id < num_seqs ? p->get_sequencer(prev_mh_seq_id) : 0;
        if (mem_hier_seq[i]) {
            ctrl_flow[i]->set_direct_bp(mem_hier_seq[i]->get_yags_ptr());
            ctrl_flow[i]->set_indirect_bp(mem_hier_seq[i]->get_cascade_ptr());
        }
    }
   
	lsq->read_checkpoint(file);
    yags_ptr->read_checkpoint(file);
    cascade_ptr->read_checkpoint(file);
    
    
#ifdef POWER_COMPILE
    if (g_conf_power_profile) 
        core_power_profile->from_file(file);  
#endif  
    
}

// Switch sequencer to a new Simics/OS visible CPU
//    Don't charge for switch time if initial setup from checkpoints
void
sequencer_t::switch_to_thread(mai_t *_mai, uint8 t_ctxt, bool checkpoint) {
	
	// Previous thread
    if (mai[t_ctxt]) {
        if (g_conf_csp_stop_tick)
            mai[t_ctxt]->save_tick_reg();
        
        if (!g_conf_use_processor)
            mai[t_ctxt]->ino_pause_trap_context();
    }
	// Switch in new thread
	mai[t_ctxt] = _mai;
	if (!mai[t_ctxt]) {
        set_fu_status(t_ctxt, FETCH_GPC_UNKNOWN);
        thread_ids[t_ctxt] = p->get_num_cpus();
		return;
	}

    uint32 mul_factor = (g_conf_narrow_core_switch) ? 2 : 1;
	mai[t_ctxt]->set_reorder_buffer_size (g_conf_window_size * mul_factor);
	if (!g_conf_use_processor)
		mai[t_ctxt]->ino_resume_trap_context();
	if (g_conf_csp_use_mmu)
		mai[t_ctxt]->switch_mmu_array(mmu_array);

    thread_ids[t_ctxt] = _mai->get_id();
	// Copies new mai pc/npc
    ctrl_flow[t_ctxt]->set_ctxt(t_ctxt);
    if (g_conf_copy_brpred_state) {
        ctrl_flow[t_ctxt]->get_bpred_state(true)->copy_state(_mai->get_bpred_state(), false);
        ctrl_flow[t_ctxt]->get_bpred_state(true)->ras_state->clear();
    } else
        ctrl_flow[t_ctxt]->get_bpred_state(true)->clear();
				
	ctrl_flow[t_ctxt]->fix_spec_state(false);

    fastsim = p->get_fastsim(thread_ids[t_ctxt]);
	
	reset_fu_status(t_ctxt, FETCH_GPC_UNKNOWN);
	
	thread_yield_reason[t_ctxt] = YIELD_NONE;
	thread_switch_state[t_ctxt] = THREAD_SWITCH_NONE;
    
    // For successive paused VCPU interrupt
    last_thread_switch[t_ctxt] = 0;

    if (early_interrupt_queue[t_ctxt] >= 1) {
        early_interrupt_queue[t_ctxt]--;
        /*
        if (early_interrupt_queue[t_ctxt]) {
            if (g_conf_no_os_pause)
                last_thread_switch[t_ctxt] = g_conf_long_thread_preempt - g_conf_interrupt_handle_cycle;
            else
                last_thread_switch[t_ctxt] = g_conf_thread_preempt - g_conf_interrupt_handle_cycle;
        }
        */
    }
    
	// Don't charge for initial thread switch when loading checkpt
	if (!checkpoint) {
		thread_switch_state[t_ctxt] = THREAD_SWITCH_IN_START;
		set_fu_status(t_ctxt, FETCH_PENDING_SWITCH);
	}
	
    ASSERT(icount[t_ctxt] == 0);
    
    spin_heuristic[t_ctxt]->reset();
    proc_stats_t *pstats = get_pstats (t_ctxt);
    if (pstats) last_spinloop_commit[t_ctxt] = STAT_GET (pstats->stat_commits); 
    
    
}

proc_stats_t *
sequencer_t::get_pstats(uint8 seq_ctxt)
{
	if (g_conf_kernel_stats && mai[seq_ctxt] && mai[seq_ctxt]->is_supervisor ()) 
		return pstats_list[seq_ctxt][1];
	else
		return pstats_list[seq_ctxt][0];
}

proc_stats_t **
sequencer_t::get_pstats_list(uint8 seq_ctxt)
{
	return pstats_list[seq_ctxt];
}

proc_stats_t *
sequencer_t::get_tstats(uint8 seq_ctxt)
{
	return p->get_tstats(thread_ids[seq_ctxt]);	
}

proc_stats_t **
sequencer_t::get_tstats_list(uint8 seq_ctxt)
{
	return p->get_tstats_list(thread_ids[seq_ctxt]);	
}

uint32
sequencer_t::get_id()
{
	return id;
}

fetch_buffer_t *
sequencer_t::get_fetch_buffer()
{
	return fetch_buffer;
}

void
sequencer_t::prepare_for_checkpoint(uint32 tid)
{
	ASSERT(!wait_on_checkpoint[tid]);
	
	if (mai[tid] && mai[tid]->get_tl() > 0)
		wait_on_checkpoint[tid] = true;
	else
		set_fu_status(tid, FETCH_PENDING_CHECKPOINT);
	
	STAT_INC(get_pstats(tid)->stat_intermediate_checkpoint);
	if (get_tstats(tid)) 
		STAT_INC(get_tstats(tid)->stat_intermediate_checkpoint);
}

bool
sequencer_t::ready_for_checkpoint()
{
    
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        //ASSERT(mai[i] == NULL || get_fu_status(i, FETCH_PENDING_CHECKPOINT) || wait_on_checkpoint[i]);
        
        if (wait_on_checkpoint[i]) 
            return false;
        if (mai[i] && mai[i]->get_tl() > 0)
            return false;
	}
    
    if (!st_buffer_empty())
            return false;
    if (!iwindow->empty())
            return false;
    
	return true;
}

void
sequencer_t::potential_thread_switch(uint8 t_ctxt, ts_yield_reason_t why) {
        
	if (thread_yield_reason[t_ctxt] == YIELD_NONE)
		thread_yield_reason[t_ctxt] = why;
	else {
		thread_yield_reason[t_ctxt] = 
			p->get_scheduler()->prioritize_yield_reason(why,
				thread_yield_reason[t_ctxt]);
    }

    
	// Only change state when here, otherwise just update prioritized reason
	if (thread_switch_state[t_ctxt] == THREAD_SWITCH_NONE) 
		thread_switch_state[t_ctxt] = THREAD_SWITCH_CHECK_YIELD;
   
    if (why == YIELD_PAUSED_THREAD_INTERRUPT) {
        early_interrupt_queue[t_ctxt]++;
        //DEBUG_OUT("seq%u getting paused thread interrupt current_state : %u\n", id,
        //    thread_switch_state[t_ctxt]);
        
    }
    
    
}

void
sequencer_t::generate_pstats_list () {

	pstats_list = new proc_stats_t ** [num_hw_ctxt];

	for (uint32 s = 0; s < num_hw_ctxt; s++) {

		if (g_conf_kernel_stats) 
			pstats_list[s] = new proc_stats_t * [3];
		else
			pstats_list[s] = new proc_stats_t * [2];

		char name[32];
		sprintf(name, "ctxt_stats_%u_%u", get_id(), s);

		uint32 p = 0;
		pstats_list[s][p++] = new proc_stats_t (string(name));
		
		if (g_conf_kernel_stats) {
			sprintf(name, "kctxt_stats_%u_%u", get_id(), s);
			pstats_list[s][p++] = new proc_stats_t (string(name));
		}
		
		pstats_list[s][p] = 0;
	}
	
}

void
sequencer_t::print_stats() {
	
   
	for (uint32 s = 0; s < num_hw_ctxt; s++) {
        base_counter_t *stat_elapsed_sim = pstats_list[s][0]->stat_elapsed_sim;
        base_counter_t *stat_start_sim = pstats_list[s][0]->stat_start_sim;
        stat_elapsed_sim->set (static_cast <int64> (time (0)) - 
			(int64) stat_start_sim->get_total ());
		
		uint32 i = 0;
		uint64 total_commits = 0;
		uint64 total_cycles = 0;
		while (pstats_list[s][i] != 0) {
			total_commits += (uint64) pstats_list[s][i]->stat_commits->get_total ();
			total_cycles += (uint64) pstats_list[s][i]->stat_cycles->get_total ();
			i++;
		}
		STAT_SET(pstats_list[s][0]->stat_total_commits, total_commits);
		STAT_SET(pstats_list[s][0]->stat_total_cycles, total_cycles);
			
		i = 0;
		while (pstats_list[s][i] != 0) {
			pstats_list[s][i]->print ();
            
			i++;
		}
	}
	
    profiles->print();

	
}

uint32 sequencer_t::get_thread(uint32 i)
{
    return thread_ids[i];
}

void sequencer_t::update_icount(uint32 tid, uint32 num)
{
    ASSERT(icount[tid] >= num);
    if (icount[tid]) icount[tid] -= num;
}

void sequencer_t::decrement_icount(uint32 tid)
{
    icount[tid]--;
}

void sequencer_t::set_mem_hier_seq(sequencer_t *_s, uint32 tid)
{
    ASSERT(iwindow->empty_ctxt(tid));
	if (mem_hier_seq[tid]) prev_mh_seq[tid] = mem_hier_seq[tid];
    mem_hier_seq[tid] = _s; 
    last_thread_switch[tid] = 0;
    // SMT context won't work now ----
    if (!_s) return;
    if (g_conf_share_br_pred) {
        ctrl_flow[tid]->set_direct_bp(_s->get_yags_ptr());
        ctrl_flow[tid]->set_indirect_bp(_s->get_cascade_ptr());
    }
	if (g_conf_csp_use_mmu)
		mai[tid]->switch_mmu_array(_s->mmu_array);
    //DEBUG_OUT("%8llu: %d ctxt %d is now %d\n", p->get_g_cycles(), id, tid, _s->get_id());
	// Don't charge for initial thread switch when loading checkpt
	if (!p->get_g_cycles() < 10) {
		thread_switch_state[tid] = THREAD_SWITCH_IN_START;
		set_fu_status(tid, FETCH_PENDING_SWITCH);
	}
}

sequencer_t *sequencer_t::get_mem_hier_seq(uint32 tid)
{
    return mem_hier_seq[tid];
}

sequencer_t *sequencer_t::get_prev_mh_seq(uint32 tid)
{
    return prev_mh_seq[tid];
}

void sequencer_t::invalidate_address(sequencer_t *_s, invalidate_addr_t *invalid_addr)
{
    bool req_invalidate = false;
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        if (mem_hier_seq[i] == _s) 
        {
            req_invalidate = true;
            break;
        }
    }
    if (req_invalidate) lsq->invalidate_address(invalid_addr);
}

yags_t *sequencer_t::get_yags_ptr()
{
    return yags_ptr;   
}

cascade_t *sequencer_t::get_cascade_ptr()
{
    return cascade_ptr;
}

tick_t
sequencer_t::get_last_mutex_lock() {
	return last_mutex_try_lock;
}

void
sequencer_t::set_last_mutex_lock() {
	last_mutex_try_lock = p->get_g_cycles();
}

void sequencer_t::set_first_mutex_try(uint32 spin_index)
{
    spin_pc_index = spin_index;
    first_mutex_try_lock[spin_index] = p->get_g_cycles();
}


uint32 sequencer_t::get_last_spin_index()
{
    return spin_pc_index;
}


void sequencer_t::thread_switch_stats(uint32 ctxt, bool on_off)
{
	proc_stats_t *pstats = get_pstats (ctxt);
	proc_stats_t *tstats = get_tstats (ctxt);

	tick_t start = STAT_GET(get_pstats(ctxt)->stat_thread_switch_start);
	tick_t latency = p->get_g_cycles() - start;
	
	if (on_off) 
		pstats->stat_thread_switch_on->inc_total(1, latency);
	else
		pstats->stat_thread_switch_off->inc_total(1, latency);
	
	if (tstats) {
		if (on_off) 
			tstats->stat_thread_switch_on->inc_total(1, latency);
		else
			tstats->stat_thread_switch_off->inc_total(1, latency);
	}
}


void
sequencer_t::fetch_vcpu_state(uint32 ctxt)
{
	//DEBUG_OUT("fetch_vcpu_state() @ %llu seq: %d  thread: %d\n", 
	//	p->get_g_cycles(), id, mai[ctxt]->get_id());
		
	ASSERT(outstanding_state_loads[ctxt] == 0);
	ASSERT(outstanding_state_stores[ctxt] == 0);
	
	remaining_state_loads[ctxt] = g_conf_state_size;

	for (uint32 i = 0; i < g_conf_max_outstanding_thread_state && i < g_conf_state_size; i++)
		send_vcpu_state_one(ctxt, true);
}

void
sequencer_t::store_vcpu_state(uint32 ctxt)
{
	//DEBUG_OUT("store_vcpu_state() @ %llu seq: %d  thread: %d\n", 
	//	p->get_g_cycles(), id, mai[ctxt]->get_id());

	ASSERT(outstanding_state_loads[ctxt] == 0);
	ASSERT(outstanding_state_stores[ctxt] == 0);
	
	remaining_state_stores[ctxt] = g_conf_state_size;

	for (uint32 i = 0; i < g_conf_max_outstanding_thread_state && i < g_conf_state_size
        && remaining_state_stores[ctxt]; i++)
		send_vcpu_state_one(ctxt, false);
}

void
sequencer_t::send_vcpu_state_one(uint32 ctxt, bool load)
{
	
	uint32 idx;
	if (load) {
        if (remaining_state_loads[ctxt] == 0) return;
		outstanding_state_loads[ctxt]++;
		idx = g_conf_state_size - remaining_state_loads[ctxt]--;
	} else {
        if (remaining_state_stores[ctxt] == 0) return;
		outstanding_state_stores[ctxt]++;
		idx = g_conf_state_size - remaining_state_stores[ctxt]--;
	}
	
	uint32 vcpu_id = mai[ctxt]->get_id();
	uint32 reqs_line = (g_conf_l1d_lsize / 8);
	
	uint32 num_lines = g_conf_state_size / reqs_line;
	if (g_conf_state_size % reqs_line > 0) 
		num_lines++;
	
	addr_t pa;
	if (g_conf_optimize_thread_state_addr)
		pa = THREAD_STATE_BASE + (vcpu_id * g_conf_thread_state_offset) +
			(idx % num_lines) * g_conf_l1d_lsize + 
			(idx / num_lines) * 8;
	else 
		pa = THREAD_STATE_BASE + (vcpu_id * g_conf_thread_state_offset) +
			(idx * 8);

	send_state_trans(ctxt, pa, load);    
}

void
sequencer_t::send_state_trans(uint32 ctxt, addr_t pa, bool load)
{
    mem_trans_t *trans;
	trans = mem_hier_t::ptr()->get_mem_trans();
    trans->phys_addr = pa;
    trans->virt_addr = 0;
	
	trans->sequencer = this;
    trans->mem_hier_seq = get_mem_hier_seq(ctxt);
    trans->cpu_id = trans->mem_hier_seq->get_id();
    trans->vcpu_state_transfer = true;
	trans->sequencer_ctxt = ctxt;
	// No need to ever have these lines in shared state -- just get write
	// permission for state loads as well to save b/w when we go to write
    trans->read = false;
	trans->write = true;
	// trans->read = load;
	// trans->write = !load;
	trans->size = 8;
	trans->ini_ptr = mai[ctxt]->get_cpu_obj();
	trans->may_stall = 1;
	trans->call_complete_request = 1;
	trans->mark_pending(PROC_REQUEST);
	trans->cache_phys = true;
	
    if (g_conf_use_processor)
        p->get_mem_hier()->make_request(trans);
    else
        p->get_mem_driver()->get_mem_hier()->make_request(
			(conf_object_t *) trans->mem_hier_seq, trans);
}

void
sequencer_t::complete_switch_request(mem_trans_t *trans)
{
    ASSERT(trans->vcpu_state_transfer);

	// DEBUG_OUT("complete_switch_request() %s: %u\n", 
		// trans->read ? "read" : "write", 
		// trans->read ? outstanding_state_loads[trans->sequencer_ctxt] :
			// outstanding_state_stores[trans->sequencer_ctxt]);
	
    // Can't distinguish based on trans->read and write
    ASSERT( (!outstanding_state_loads[trans->sequencer_ctxt] &&
            outstanding_state_stores[trans->sequencer_ctxt]) ||
            (!outstanding_state_stores[trans->sequencer_ctxt] &&
            outstanding_state_loads[trans->sequencer_ctxt])); 
            
	if (outstanding_state_loads[trans->sequencer_ctxt]) {
		outstanding_state_loads[trans->sequencer_ctxt]--;
		if (remaining_state_loads[trans->sequencer_ctxt] > 0)
			send_vcpu_state_one(trans->sequencer_ctxt, true);

	} else { 
		outstanding_state_stores[trans->sequencer_ctxt]--;
		if (remaining_state_stores[trans->sequencer_ctxt] > 0)
			send_vcpu_state_one(trans->sequencer_ctxt, false);
	}
}

spin_heuristic_t *sequencer_t::get_spin_heuristic(uint32 tid)
{
    return spin_heuristic[tid];
}

bool sequencer_t::is_spill_trap(uint32 tid)
{
    bool ret = false;
    if (mai[tid] && mai[tid]->get_tl()) {
        uint64 tt = mai[tid]->get_tt(mai[tid]->get_tl());
        if ((tt >= 0x080 && tt <= 0x09f) ||
            (tt >= 0x0a0 && tt <= 0x0bf))
            ret = true;
    }
    return ret;
}

bool sequencer_t::is_fill_trap(uint32 tid)
{
    bool ret = false;
    if (mai[tid] && mai[tid]->get_tl()) {
        uint64 tt = mai[tid]->get_tt(mai[tid]->get_tl());
        if ((tt >= 0x0c0 && tt <= 0x0df) ||
            (tt >= 0x0e0 && tt <= 0x0ff))
            ret = true;
    }
    return ret;
}

void sequencer_t::interrupt_on_running_thread(uint32 t_ctxt)
{
    DEBUG_OUT("seq%u: Interrupt on running vcpu%u @ %llu\n", id, 
        thread_ids[t_ctxt], p->get_g_cycles());
    thread_switch_state[t_ctxt] = THREAD_SWITCH_HANDLING_INTERRUPT;
    wait_after_switch[t_ctxt] = g_conf_interrupt_handle_cycle;
    if (p->get_mem_driver()->is_paused(mai[t_ctxt]->get_id()))
        p->get_mem_driver()->release_proc_stall(mai[t_ctxt]->get_id());
    
}

uint32 sequencer_t::get_num_hw_ctxt()
{
	return num_hw_ctxt;
}

void sequencer_t::debug_cache_miss(uint32 ctxt)
{
    proc_stats_t *pstats = get_pstats (ctxt);
    DEBUG_OUT("%u -> L1 load : %llu L1 instr %llu L2 load %llu L2 instr %llu commits %llu\n",
        id, STAT_GET(pstats->stat_load_l1_miss), STAT_GET(pstats->stat_instr_l1_miss),
        STAT_GET(pstats->stat_load_l2_miss), STAT_GET(pstats->stat_instr_l2_miss),
        STAT_GET(pstats->stat_commits));
    
}

tick_t sequencer_t::get_last_thread_switch(uint32 tid)
{
    return last_thread_switch[tid];
}

addr_t sequencer_t::get_return_top(uint32 tid)
{
    return ctrl_flow[tid]->get_return_top();
}

void sequencer_t::ctrl_flow_redirect(uint32 tid)
{
    //DEBUG_OUT("ctrl_flow_redirect @ %llu\n", p->get_g_cycles());
    // FIXME -------
    if (thread_switch_state[tid] == THREAD_SWITCH_NARROW_CORE) {
        DEBUG_OUT("squashing instructions from the remote core %llu\n", p->get_g_cycles());
        sequencer_t *r_seq = p->get_remote_sequencer(this);
        r_seq->squash_inflight_instructions(tid, true);
        if (get_fu_status(tid, FETCH_STALL_UNTIL_EMPTY))
            r_seq->set_fu_status(tid, fetch_status[tid]);
    }
    
    
    return;
    
    if (thread_switch_state[tid] != THREAD_SWITCH_NONE) 
    {
        thread_switch_state[tid] = THREAD_SWITCH_NONE;
        thread_yield_reason[tid] = YIELD_NONE;
        if (get_fu_status(tid, FETCH_PENDING_SWITCH))
            reset_fu_status(tid, FETCH_PENDING_SWITCH);
        // DEBUG_OUT("Cancelling Core Change @ %llu\n", p->get_g_cycles ());
        p->get_scheduler()->ctrl_flow_redirection(thread_ids[tid]);
    }
        
    
}

void sequencer_t::push_return_top(addr_t r_addr, uint32 t_ctxt)
{
    ctrl_flow[t_ctxt]->push_return_top(r_addr);
}

void sequencer_t::set_mai_thread(uint32 _ctxt, mai_t *_mai)
{
    mai[_ctxt] = _mai;
    if (_mai) thread_ids[_ctxt] = _mai->get_id();
    else thread_ids[_ctxt] = p->get_num_cpus();
}

bool sequencer_t::active_core()
{
    for (uint32 i = 0; i < num_hw_ctxt; i++)
    {
        if (mai[i]) return true;
    }
    return false;
}

void sequencer_t::initialize_function_records ()
{
    if (g_conf_function_file == "") return;
    
    FILE *file = fopen (g_conf_function_file.c_str(), "r");
    if (!file) {
        FAIL_MSG("Failed to open file %s", g_conf_function_file.c_str());
    }
    ASSERT(file);
    char name[5000];
    addr_t start, end;
    uint32 f_count = 0;
    while (fscanf (file, "%s %llx %llx\n", name, &start, &end) != EOF)
    {
        function_map_index [start] = f_count;
        function_name [f_count] = string (name);
        function_start_end [start] = end;
        binary_high_end = end;
        f_count++;
    }
    
    total_functions = f_count;
    string name_str[f_count + 1];
    for (uint32 i = 0; i < f_count; i++) 
        name_str [i] = function_name [i];   
    name_str[f_count] = "Uknown_function";
    
    for (uint32 i = 0; i < num_hw_ctxt; i++) {
        proc_stats_t *pstats = get_pstats (i);
        
        if (pstats) {
            pstats->stat_except16_PC = pstats->stats->COUNTER_BREAKDOWN ("exceptt16_func", "Except 16 Functions", 
                f_count + 1, name_str, name_str);
            pstats->stat_except16_PC->set_void_zero_entries ();
            
            pstats->stat_except24_PC = pstats->stats->COUNTER_BREAKDOWN ("exceptt24_func", "Except 24 Functions", 
                f_count + 1, name_str, name_str);
            pstats->stat_except24_PC->set_void_zero_entries ();
            
            pstats->stat_except32_PC = pstats->stats->COUNTER_BREAKDOWN ("exceptt32_func", "Except 32 Functions", 
                f_count + 1, name_str, name_str);
            pstats->stat_except32_PC->set_void_zero_entries ();
            
            pstats->stat_function_execution = pstats->stats->COUNTER_BREAKDOWN ("stat_function_execute", "Contribution from each function",
                f_count + 1, name_str, name_str);
            pstats->stat_function_execution->set_void_zero_entries ();
            
            pstats->stat_float_exception = pstats->stats->COUNTER_BREAKDOWN ("stat_float_execute", "Contribution from each function",
                f_count + 1, name_str, name_str);
            pstats->stat_float_exception->set_void_zero_entries ();
        }
        proc_stats_t *tstats = get_tstats (i);
        
        if (!tstats) continue;
        
        tstats->stat_except16_PC = tstats->stats->COUNTER_BREAKDOWN ("exceptt16_func", "Except 16 Functions", 
            f_count + 1, name_str, name_str);
        tstats->stat_except16_PC->set_void_zero_entries ();
        
        tstats->stat_except24_PC = tstats->stats->COUNTER_BREAKDOWN ("exceptt24_func", "Except 24 Functions", 
            f_count + 1, name_str, name_str);
        tstats->stat_except24_PC->set_void_zero_entries ();
        
        tstats->stat_except32_PC = tstats->stats->COUNTER_BREAKDOWN ("exceptt32_func", "Except 32 Functions", 
            f_count + 1, name_str, name_str);
        tstats->stat_except32_PC->set_void_zero_entries ();
        
        tstats->stat_function_execution = tstats->stats->COUNTER_BREAKDOWN ("stat_function_execute", "Contribution from each function",
                f_count + 1, name_str, name_str);
        tstats->stat_function_execution->set_void_zero_entries ();
        
        tstats->stat_float_exception = tstats->stats->COUNTER_BREAKDOWN ("stat_float_execute", "Contribution from each function",
                f_count + 1, name_str, name_str);
        tstats->stat_float_exception->set_void_zero_entries ();
        
    }  
    
    if (g_conf_operand_width_analysis && g_conf_operand_trace) {
        string stat_out = string(getenv("OUTPUT_FILE"));
        string::size_type pos = stat_out.find("stats", 0);
        string otrace_file = stat_out.substr(0, pos) + "op_width";
        char ptrace_filename[100];
        sprintf(ptrace_filename, "%s%d", otrace_file.c_str(), id);
        operand_trace_file = fopen(ptrace_filename, "a");
        
        except_32_prev = 0;
        except_16_prev = 0;
        except_24_prev = 0;
        except16_negative_prev = except24_negative_prev = except32_negative_prev = 0;
        except_negative_prev = 0;
        floating_point_op = 0;
        operand_interval_mask = (1 << g_conf_operand_dump_interval) - 1;
        most_recent_width_dump = 0;
    }
        
}
    

void sequencer_t::initialize_annotated_pc()
{
    string filename = g_conf_nc_migration_root + getenv("BENCHMARK") + ".pclist";
    FILE *file = fopen(filename.c_str(), "r");
    ASSERT(file);
    addr_t pc;
    uint32 type;
    while (fscanf (file, "%llx %u\n", &pc, &type) != EOF)
    {
        profile_pc_type[pc] = type;
        annotated_pc_map[pc] = true;
        pc_map_iterdb[pc] = annotated_pc_map.find(pc);
    }
    
    
}

uint32 sequencer_t::query_annotation(addr_t PC)
{
    uint32 ret = 0;
    
    if (profile_pc_type.find(PC) != profile_pc_type.end()) {
        ret = profile_pc_type[PC];
        prev_annotation_range = curr_annotation_range;
        map<addr_t, bool>::iterator c_iter = pc_map_iterdb[PC];
        //curr_annotation_range.reinitialize(PC, (c_iter + 1)->first);
        curr_pc_map_iter = c_iter;
    } else {
        // addr_t l_val = curr_range.low;
        // map<addr_t, bool>::iterator c_iter = current_pc_map_iter;
        // while (c_iter != annotated_pc_map.end())
        // {
        //     if (c_iter->first > l_val) 
        //         break;
        // }
        // current_pc_map_iter = c_iter - 1;
        // if (c_iter != annotated_pc_map.end())
        // {
        //     prev_annotation_range = curr_annotation_range;
        //     curr_annotation_range.reinitialize(current_pc_map_iter->first, c_iter->second);
        // }
    }
        
    return ret;
    
    
    
}

void sequencer_t::operand_trace_dump(uint32 tid)
{
    proc_stats_t *tstats = get_tstats(tid);
    uint64 current_commit = STAT_GET (tstats->stat_commits);
    if (!last_operand_dump_pc) last_operand_dump_pc = last_commit_pc;
    if (current_commit != most_recent_width_dump &&
        (current_commit - most_recent_width_dump > operand_interval_mask)) {
        fprintf(operand_trace_file, "%u %u %u %u %u %u %u %u %llx\n",
            (uint32) ((uint64) tstats->stat_except_opcode32->get_total() - except_32_prev),
            (uint32) ((uint64) tstats->stat_except_opcode24->get_total() - except_24_prev),
            (uint32) ((uint64) tstats->stat_except_opcode16->get_total() - except_16_prev),
            (uint32) ((uint64) tstats->stat_negative_except32->get_total() - except32_negative_prev),
            (uint32) ((uint64) tstats->stat_negative_except24->get_total() - except24_negative_prev),
            (uint32) ((uint64) tstats->stat_negative_except16->get_total() - except16_negative_prev),
            (uint32) (STAT_GET (tstats->stat_negative_inpoutp) - except_negative_prev),
        (uint32) ((uint64)tstats->stat_float_exception->get_total() - floating_point_op), 
        last_operand_dump_pc);
        except_32_prev = (uint64) tstats->stat_except_opcode32->get_total();
        except_24_prev = (uint64) tstats->stat_except_opcode24->get_total();
        except_16_prev = (uint64) tstats->stat_except_opcode16->get_total();
        except32_negative_prev = (uint64) tstats->stat_negative_except32->get_total();
        except24_negative_prev = (uint64) tstats->stat_negative_except24->get_total();
        except16_negative_prev = (uint64) tstats->stat_negative_except16->get_total();
        except_negative_prev =   STAT_GET (tstats->stat_negative_inpoutp);
        floating_point_op = (uint64) tstats->stat_float_exception->get_total() ;
        most_recent_width_dump = current_commit;
        last_operand_dump_pc = last_commit_pc;
        fflush(operand_trace_file);
    }
    
}


void sequencer_t::update_current_function (uint32 tid, addr_t PC)
{
    
    if (g_conf_operand_trace) {
        operand_trace_dump(tid);
    }
    
    if (current_function_start [tid] <= PC &&
        current_function_end [tid] >= PC)
        return;
        
    // Have to update
    map<addr_t, addr_t>::iterator it = function_start_end.begin ();
    // Avoid checking if the PC is out of the range
    if (PC > binary_high_end) it = function_start_end.end();
    for (; it != function_start_end.end (); it++)
    {
        if (PC >= it->first && PC <= it->second)
            break;
        
    }
 
    if (it == function_start_end.end ()) {
        current_function_start [tid] = 0;
        current_function_end [tid] = 0;
        current_function_index [tid] = 0;
        
    } else {
        current_function_start [tid] = it->first;
        current_function_end [tid] = it->second;
        current_function_index[tid] = function_map_index [it->first];
    }
    
}

uint32 sequencer_t::get_function_index (uint32 tid)
{
    if (current_function_start [tid])
        return current_function_index [tid];
    else
        return total_functions;
}

core_power_t *sequencer_t::get_core_power_profile()
{
    return core_power_profile;
}


void sequencer_t::switch_out_epilogue(uint32 ctxt)
{
    thread_switch_stats(ctxt, false);
    thread_switch_state[ctxt] = THREAD_SWITCH_NONE;
    reset_fu_status(ctxt, FETCH_PENDING_SWITCH);
    mai[ctxt]->get_bpred_state()->copy_state(ctrl_flow[ctxt]->get_bpred_state(true));
    proc_stats_t *pstats = get_pstats (ctxt);
    proc_stats_t *tstats = get_tstats (ctxt);
    mai[ctxt]->stat_register_read_write(pstats, tstats);
    
    p->get_scheduler()->ready_for_switch(this, ctxt, mai[ctxt], thread_yield_reason[ctxt]);
    thread_yield_reason[ctxt] = YIELD_NONE;
    
    
    
    
}

void sequencer_t::switch_in_epilogue(uint32 ctxt)
{
    
    thread_switch_stats(ctxt, true);
    reset_fu_status(ctxt, FETCH_PENDING_SWITCH);
    thread_switch_state[ctxt] = THREAD_SWITCH_NONE;
    mai[ctxt]->reset_register_read_write();
    if (!g_conf_use_processor)
        p->get_mem_driver()->release_proc_stall(mai[ctxt]->get_id());
    
}

void sequencer_t::update_register_busy()
{
    for (uint32 i = 0; i < register_count; i++)
    {
        if (register_busy[i]) register_busy[i]--;
    }
}

void sequencer_t::set_register_busy(uint32 index, uint32 datawidth)
{
    ASSERT(index < register_count);
    if (datawidth <= datapath_width) {
        register_busy[index] = 0;
    } else {
        uint32 latency = datawidth/datapath_width + g_conf_wide_operand_penalty;
        register_busy[index] = latency;
    }
}

uint32 sequencer_t::get_register_busy(uint32 index)
{
    if (register_busy[index]) {
        proc_stats_t *pstats = get_pstats(0);
        proc_stats_t *tstats = get_tstats(0);
        STAT_INC (pstats->stat_narrow_penalty);
        STAT_INC (tstats->stat_narrow_penalty);
    }
    
    return register_busy[index];
}

void sequencer_t::narrow_core_switch(sequencer_t *src_seq)
{
    // Assume non-SMT
    // copy ctrl_flow state, mai object from src_seq
    // start fetching down the path
    
    uint32 t_ctxt = 0;
    reset_fu_status(t_ctxt, FETCH_GPC_UNKNOWN);
	thread_yield_reason[t_ctxt] = YIELD_NONE;
	thread_switch_state[t_ctxt] = THREAD_SWITCH_NONE;
    mai[t_ctxt] = src_seq->get_mai_object(t_ctxt);
    ctrl_flow[t_ctxt]->copy_state(src_seq->get_ctrl_flow(t_ctxt));
    thread_ids[t_ctxt] = mai[t_ctxt]->get_id();
    if (id % 2 == 1) mem_hier_seq[t_ctxt] = src_seq->get_mem_hier_seq(t_ctxt);
    
}
