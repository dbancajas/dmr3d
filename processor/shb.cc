/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id $
 *
 * description:    Store History Buffer implementation
 * initial author: Koushik Chakraborty 
 *
 */
 
 

//  #include "simics/first.h"
// RCSID("$Id: shb.cc,v 1.1.2.14 2006/08/18 14:30:20 kchak Exp $");

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
#include "counter.h"
#include "stats.h"
#include "proc_stats.h"
#include "fu.h"
#include "eventq.h"
#include "v9_mem_xaction.h"
#include "mem_xaction.h"
#include "lsq.h"
#include "st_buffer.h"
#include "mem_hier_iface.h"
#include "transaction.h"
#include "mem_hier_handle.h"
#include "startup.h"
#include "config_extern.h"
#include "config.h"
#include "mem_driver.h"
#include "shb.h"

static chip_t *chip;

static void 
breakpoint_handler (void *obj, conf_object_t *cpu) {
    uint32 thread_id = SIM_get_proc_no(SIM_current_processor());
    addr_t PC = SIM_get_program_counter(SIM_current_processor());
    sequencer_t *seq = chip->get_sequencer_from_thread(thread_id);
    // Not sure why this can happen ... but some corner cases perhaps
    if (seq) {
        spin_heuristic_t *spin = seq->get_spin_heuristic();
        spin->detected_recognized_spin(PC, thread_id);
    } 
    
}


store_record_t::store_record_t(addr_t p_a, uint32 _s, int64 _v, uint32 _bmask)
{
    copy(p_a, _s, _v, _bmask);
}


void store_record_t::copy(addr_t p_a, uint32 _s, int64 _v, uint32 _bmask) 
{
    phys_addr = p_a;
    size = _s;
    value = _v;
    dw_bytemask = _bmask;
}

bool store_record_t::is_identical(addr_t p_a, uint32 _s, int64 _v, uint32 _bmask) 
{
    return (phys_addr == p_a &&
        size == _s &&
        value == _v &&
        dw_bytemask == _bmask);
}

void store_record_t::debug()
{
    DEBUG_OUT("PA %llx size %u bytemask %u value %llu\n",
    phys_addr, size, dw_bytemask, value);
}


spin_heuristic_t::spin_heuristic_t(sequencer_t *_seq, uint32 _tid) :
    seq(_seq), tid(_tid)
{
    
    store_history = new store_record_t *[g_conf_spinloop_threshold + 1];
    for (uint32 i = 0; i < g_conf_spinloop_threshold + 1; i++)
        store_history[i] = new store_record_t(0, 0, 0, 0);
    load_threshold = g_conf_load_count_factor * g_conf_spinloop_threshold;
    load_history = new addr_t[load_threshold + 1];
    atomic_pc = new addr_t[g_conf_spinloop_threshold + 1];
    
    if (seq->get_id() == 0 && g_conf_spin_loop_pcs[0] > 0) {
        SIM_hap_add_callback("Core_Breakpoint", breakpoint_handler, this);
        for (uint32 spin = 1; spin < g_conf_spin_loop_pcs[0] + 1; spin++) {
            SIM_breakpoint(SIM_get_object("primary_context"), Sim_Break_Virtual, Sim_Access_Execute,
                (addr_t) g_conf_spin_loop_pcs[spin], 1, Sim_Breakpoint_Simulation);
        }
        chip = seq->get_chip();
    }
    
    saturating_counter = 0;
    reset();
    
}

void spin_heuristic_t::observe_d_instr(dynamic_instr_t *d_instr)
{
    addr_t PC = d_instr->get_pc();
    mem_xaction_t *mem_xaction = d_instr->get_mem_transaction();
    addr_t pa = mem_xaction->get_phys_addr();
    int64 val = 0; 
    if (d_instr->is_store() && !d_instr->unsafe_store() && 
                !d_instr->is_atomic())
        val = mem_xaction->is_value_valid() ? mem_xaction->get_value() : 0;
    uint32 bytemask = mem_xaction->get_dw_bytemask();
    uint32 size = mem_xaction->get_size();
    mem_op_type mtype = MEMOP_TYPE_MAX;
    if (d_instr->is_atomic()) mtype = MEMOP_TYPE_ATOMIC;
    else if (d_instr->is_store()) mtype = MEMOP_TYPE_STORE;
    else if (d_instr->is_load()) mtype = MEMOP_TYPE_LOAD;
    observe_memop(PC, pa, val, bytemask, size, mtype);    
}


void
spin_heuristic_t::observe_memop(addr_t PC, addr_t pa, int64 val, uint32 bytemask, 
    uint32 size, mem_op_type mtype)
{
    switch (g_conf_spin_detection_algorithm) {
        case STORE_HISTORY_BUFFER:
        
            if (mtype == MEMOP_TYPE_STORE &&
                store_index < g_conf_spinloop_threshold) {
                    search_and_insert_store(pa, val, bytemask, size);
            }
            break;
        
        case LOAD_HISTORY_BUFFER:
            
            if (mtype == MEMOP_TYPE_STORE) {
                store_count++;
                if (store_index < g_conf_spinloop_threshold) 
                    search_and_insert_store(pa, val, bytemask, size);
            } else if (mtype == MEMOP_TYPE_LOAD && load_index < load_threshold) {
                search_and_insert_load(pa);
            }
            break;
        case ATOMIC_HISTORY_BUFFER:
            if (mtype == MEMOP_TYPE_ATOMIC && atomic_index < g_conf_spinloop_threshold) {
                 search_and_insert_atomic(pa, PC);
                 atomic_count++;
            }
            break;
        default:
            FAIL;
    }
    
}


void spin_heuristic_t::search_and_insert_store(addr_t pa, int64 val, uint32 bytemask,
    uint32 size)
{
    uint32 i;
    for (i = 0; i < store_index; i++)
        if (store_history[i]->is_identical(pa, size, val, bytemask))
            break;
    if (i == store_index) 
        store_history[store_index++]->copy(pa, size, val, bytemask);
    
    
}

void spin_heuristic_t::search_and_insert_load(addr_t pa)
{
    uint32 i ;
    for (i = 0; i < load_index; i++)
        if (load_history[i]== pa) break;
    if (i == load_index) load_history[load_index++] = pa;
}


void spin_heuristic_t::search_and_insert_atomic(addr_t pa, addr_t PC)
{
    uint32 i;
    for (i = 0; i < atomic_index; i++)
        if (atomic_pc[i] == PC) break;
    if (i == atomic_index) atomic_pc[atomic_index++] = PC;
    
}

bool spin_heuristic_t::check_spinloop()
{
    bool spinloop = false;
    bool supervisor = seq->get_mai_object(tid)->is_supervisor();
    switch (g_conf_spin_detection_algorithm) {
        case STORE_HISTORY_BUFFER:
            if (g_conf_user_spin_factor == 0) {
                spinloop = (store_index < g_conf_spinloop_threshold);
                break;
            }
            if (supervisor) {
                spinloop = (store_index < g_conf_spinloop_threshold && !proc_mode_change);
                saturating_counter = 0;
            }
            else {
                if (store_index < g_conf_spinloop_threshold)
                    saturating_counter++;
                else
                    saturating_counter = 0;
                if (!proc_mode_change && saturating_counter == g_conf_user_spin_factor) {
                    spinloop = true;
                    saturating_counter = 0;
                } else if (proc_mode_change) {
                    saturating_counter = 0;
                }
            }
            if (spinloop) DEBUG_OUT("store count: %u ", store_index);
            break;
        case LOAD_HISTORY_BUFFER:
            if (!g_conf_spin_proc_mode || !proc_mode_change) {
                if (supervisor)
                    spinloop = (store_index < g_conf_spinloop_threshold);
                else
                    spinloop = (store_index < g_conf_spinloop_threshold) &&
                                (load_index < load_threshold);
            }
            /*      
            spinloop = (store_index < g_conf_spinloop_threshold) && 
                        ((store_count > g_conf_store_count_factor * g_conf_spinloop_threshold) ||
                        (load_index < load_threshold));
            */            
            break;
        case ATOMIC_HISTORY_BUFFER:
            if (atomic_count)
                DEBUG_OUT("cpu%u : Atomic Index %u Atomic Count %u\n",
                seq->get_mai_object(0)->get_id(),atomic_index, atomic_count);
            break;
        default:
            FAIL;
    }
    
    reset();
    return spinloop;
            
}

void spin_heuristic_t::emit_store_history(mai_t *mai)
{
    uint32 store_count = 0;
    for (uint32 i = 0; i < store_index; i++) {
        DEBUG_OUT("cpu%u store %u: ", mai->get_id(), store_count );
        store_history[i]->debug();
    }
    
    
}

void spin_heuristic_t::reset()
{
    load_index = store_index = store_count = 0;
    atomic_index = atomic_count = 0;
    spin_count = 0;
    last_spin_index = 0;
    proc_mode_change = false;
    
}


void spin_heuristic_t::emit_load_history(mai_t *mai)
{
    uint32 cpu_id = mai->get_id();
    for (uint32 i = 0; i < load_index; i++) 
        DEBUG_OUT("cpu%u: %u %llx\n",cpu_id, i, load_history[i]); 
}


void spin_heuristic_t::detected_recognized_spin(addr_t PC, uint32 thread_id)
{
    uint32 spin;
    tick_t current_cycle = chip->get_g_cycles();
    uint32 spin_cycle = current_cycle - last_spin_cycle;
    for (spin = 1; spin < (uint32) g_conf_spin_loop_pcs[0] + 1; spin++)
        if (PC == (uint32)g_conf_spin_loop_pcs[spin]) break;
    ASSERT(spin < (uint32) g_conf_spin_loop_pcs[0] + 1);
    uint64 current_step = SIM_step_count(SIM_proc_no_2_ptr(thread_id));
    uint64 spin_steps = current_step - last_spin_step;
    if (last_spin_index == spin && spin_cycle < 1000)
    {
        spin_count++;
        proc_stats_t *pstats = seq->get_pstats (0); // TODO SMT
        proc_stats_t *tstats = seq->get_tstats (0); // TODO SMT
        if (pstats && tstats) {
            pstats->stat_spins->inc_total(spin_cycle, spin-1);
            tstats->stat_spins->inc_total(spin_cycle, spin-1);
            pstats->stat_spins_instr->inc_total(spin_steps, spin-1);
            tstats->stat_spins_instr->inc_total(spin_steps, spin-1);
            pstats->stat_spin_cycles->inc_total(spin_cycle);
            tstats->stat_spin_cycles->inc_total(spin_cycle);
        }
        if (g_conf_spinloop_threshold == 0 && 
            (!g_conf_adaptive_pc_threshold || spin_count > g_conf_adaptive_pc_threshold))
            seq->potential_thread_switch(0, YIELD_MUTEX_LOCKED);
        // Potential thread_switch
        
        
    } else {
        spin_count = 0;
    }
    last_spin_index = spin;
    last_spin_cycle = current_cycle;
    last_spin_step = current_step;
}


void spin_heuristic_t::observe_mem_trans(mem_trans_t *trans, 
    generic_transaction_t *mem_op)
{
    uint32 bytemask = 0;
    int64 val = 0;
    mem_op_type mtype = MEMOP_TYPE_MAX;
    if (trans->atomic) mtype = MEMOP_TYPE_ATOMIC;
    else if (trans->write) mtype = MEMOP_TYPE_STORE;
    else if (trans->read) mtype = MEMOP_TYPE_LOAD;
    if (mtype == MEMOP_TYPE_STORE && trans->asi == ASI_PRIMARY) {
        ASSERT(SIM_mem_op_is_write(mem_op));
        val = SIM_get_mem_op_value_cpu(mem_op);
    }
    observe_memop(SIM_get_program_counter(SIM_current_processor()),
        trans->phys_addr, val, bytemask, trans->size, mtype);
    
}

void spin_heuristic_t::register_proc_mode_change()
{
    proc_mode_change = true;
    saturating_counter = 0;
}

void spin_heuristic_t::observe_commit_pc(dynamic_instr_t *d_instr)
{
    addr_t PC = d_instr->get_pc();
    uint32 spin;
    for (spin = 1; spin < (uint32) g_conf_spin_loop_pcs[0] + 1; spin++)
        if (PC == (uint32)g_conf_spin_loop_pcs[spin]) break;
    if (spin < (uint32) (g_conf_spin_loop_pcs[0] + 1))
        detected_recognized_spin(PC, d_instr->get_sequencer()->get_thread(d_instr->get_tid()));
}
    
    
