/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: lsq.cc,v 1.6.2.11 2006/08/18 14:30:19 kchak Exp $");

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
#include "v9_mem_xaction.h"
#include "mem_xaction.h"
#include "sequencer.h"
#include "st_buffer.h"
#include "transaction.h"
#include "proc_stats.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "config_extern.h"
#include "disambig.h"

memq_t::memq_t (const uint32 size) : 
	window_t <dynamic_instr_t>::window_t (size) {}

instr_event_t
memq_t::insert (dynamic_instr_t *d_instr) {

	bool ret = window_t <dynamic_instr_t>::insert (d_instr);
	if (ret) {
		d_instr->set_lsq_slot (last_created);
		return EVENT_OK;
	} else {
		return EVENT_FAILED;
	}
}

void
memq_t::squash (dynamic_instr_t *d_instr) {
	uint32 slot = d_instr->get_lsq_slot ();
    
//    bool need_collapse = (slot != window_increment(last_committed));

    bool need_collapse = false;

	ASSERT (window [slot] == d_instr);

	window [slot] = 0;
	slot = window_increment (slot);
    
	while (slot != window_increment (last_created)) {
		dynamic_instr_t *squash_instr = window [slot];
		ASSERT (squash_instr);

        if (squash_instr->get_tid() == d_instr->get_tid())
            window [slot] = 0;
        else 
            need_collapse = true;
        
		slot = window_increment (slot);
	}

    if (need_collapse) 
    {
        collapse();
        reassign_lsq_slot();
    }
    
	else last_created = window_decrement (d_instr->get_lsq_slot ());
}

void 
memq_t::reassign_lsq_slot()
{
    for (uint32 i = 0; i < window_size; i++)
    {
        if (window[i]) window[i]->set_lsq_slot(i);
        //else break;
    }
    
}

lsq_t::lsq_t (sequencer_t *_seq) {
	seq = _seq;

	ldq = new memq_t (g_conf_lsq_load_size);
	stq = new memq_t (g_conf_lsq_store_size);

	disambig = new disambig_t (g_conf_disambig_size);
	store_count = 0;
}

instr_event_t
lsq_t::insert_load (dynamic_instr_t *d_instr) {

	ASSERT (d_instr->get_lsq_entry () == LSQ_ENTRY_NONE);

	instr_event_t e = ldq->insert (d_instr);
	if (e == EVENT_OK) 
		d_instr->mark_lsq_entry (LSQ_ENTRY_CREATE);
	d_instr->set_stq_ptr(store_count);

	return e;	
}

instr_event_t
lsq_t::insert_store (dynamic_instr_t *d_instr) {

	ASSERT (d_instr->get_lsq_entry () == LSQ_ENTRY_NONE);

	instr_event_t e = stq->insert (d_instr);
	if (e == EVENT_OK) 
		d_instr->mark_lsq_entry (LSQ_ENTRY_CREATE);
	d_instr->set_stq_ptr(store_count++);

	return e;
}

void 
lsq_t::pop_load (dynamic_instr_t *d_instr) {
	if (ldq->pop_head (d_instr))
        ldq->reassign_lsq_slot();
}

void 
lsq_t::pop_store (dynamic_instr_t *d_instr) {
	if (stq->pop_head (d_instr))
        stq->reassign_lsq_slot();
}

void
lsq_t::squash_loads (dynamic_instr_t *d_instr) {
	ASSERT (d_instr->is_load ());
	ldq->squash (d_instr);
}

void 
lsq_t::squash_stores (dynamic_instr_t *d_instr) {
	ASSERT (d_instr->is_store ());
	stq->squash (d_instr);
	store_count = d_instr->get_stq_ptr();
}

lsq_t::~lsq_t () {
	delete ldq;
	delete stq;
}

// called from memory hierarchy side

void 
lsq_t::hold (dynamic_instr_t *d_instr) {

	ASSERT (d_instr->is_load () || d_instr->is_store ());
	ASSERT (d_instr->get_lsq_entry () == LSQ_ENTRY_CREATE);

	uint32 lsq_slot = d_instr->get_lsq_slot ();
	if (d_instr->is_store ()) ASSERT (stq->peek (lsq_slot) == d_instr);
	if (d_instr->is_load ())  ASSERT (ldq->peek (lsq_slot) == d_instr);

	d_instr->mark_lsq_entry (LSQ_ENTRY_HOLD);
}

void 
lsq_t::release_hold (dynamic_instr_t *d_instr) {
	ASSERT (d_instr->get_lsq_entry () == LSQ_ENTRY_HOLD);
	
	d_instr->mark_lsq_entry (LSQ_ENTRY_RELEASE);

	mem_xaction_t *mem_xaction = d_instr->get_mem_transaction ();
	v9_mem_xaction_t *v9_mem_xaction = mem_xaction->get_v9 ();

	mai_instruction_t *mai_instr = d_instr->get_mai_instruction ();
	v9_mem_xaction->set_mem_op (mai_instr->get_mem_transaction ());

	ASSERT (d_instr->get_mai_instruction ()->finished_stalling () ||
			d_instr->get_mai_instruction ()->is_stalling ());

//	ASSERT (d_instr->get_mai_instruction ()->stall_cycles () != 0);

	mem_xaction->release_stall ();

	if (
	d_instr->get_mai_instruction ()->get_mem_transaction () != 
	mem_xaction->get_v9 ()->get_mem_op ())
		WARNING;

	if (d_instr->is_store ()) {
		ASSERT (!d_instr->get_mai_instruction ()->speculative ());
		ASSERT (!v9_mem_xaction->is_speculative ());
	}
	ASSERT (d_instr->get_mai_instruction ()->stall_cycles () == 0);
}

bool
lsq_t::prev_stores_executed (dynamic_instr_t *d_instr) {

	ASSERT (d_instr->get_lsq_entry () == LSQ_ENTRY_CREATE);
	uint32 lsq_slot = d_instr->get_lsq_slot ();
	
	uint32 slot = stq->window_increment (stq->get_last_committed ());
	bool b = true;

	while (b && (slot != lsq_slot)) {
		if (stq->peek(slot)->get_tid() == d_instr->get_tid() && 
            stq->peek (slot)->get_lsq_entry () == LSQ_ENTRY_CREATE) b = false; 
		slot = stq->window_increment (slot);
	}

	return b;
}

dynamic_instr_t*
lsq_t::get_prev_load (dynamic_instr_t *d_instr) {
	ASSERT (d_instr->is_load ());
	ASSERT (d_instr->get_lsq_entry () == LSQ_ENTRY_CREATE);

	if (ldq->head () == d_instr) return 0;
	uint32 lq_slot = d_instr->get_lsq_slot ();
    uint32 last_slot = ldq->window_decrement(ldq->get_last_created());
    lq_slot = ldq->window_decrement (lq_slot);
    
    do
    {
        dynamic_instr_t *prev_dinstr = ldq->peek (lq_slot);
        if (prev_dinstr->get_tid() == d_instr->get_tid())
        {    
            ASSERT (prev_dinstr->priority () < d_instr->priority ());
            return prev_dinstr;
        }
        lq_slot = ldq->window_decrement(lq_slot);
        
    } while (lq_slot != last_slot);
    
    return 0;
}

dynamic_instr_t*
lsq_t::get_prev_store (dynamic_instr_t *d_instr) {
	ASSERT (d_instr->is_store ());
	ASSERT (d_instr->get_lsq_entry () == LSQ_ENTRY_CREATE);

	if (stq->head () == d_instr) return 0;

	uint32 sq_slot = d_instr->get_lsq_slot ();
	sq_slot = stq->window_decrement (sq_slot);

    uint32 last_slot = stq->window_decrement(stq->get_last_created());
    
    do {
    
        dynamic_instr_t *prev_dinstr = stq->peek (sq_slot);
        if (prev_dinstr->get_tid() == d_instr->get_tid())
        {
            ASSERT (prev_dinstr->priority () < d_instr->priority ());
            
            return prev_dinstr;
        }
        
        sq_slot = stq->window_decrement(sq_slot);
    } while (sq_slot != last_slot);
    
    return 0;
}

void
lsq_t::stq_snoop (dynamic_instr_t *d_instr) {
	proc_stats_t *pstats = seq->get_pstats (d_instr->get_tid());
	proc_stats_t *tstats = seq->get_tstats (d_instr->get_tid());
	bool done = false;

	mem_xaction_t *mem = d_instr->get_mem_transaction ();

	ASSERT (mem->get_snoop_status () == SNOOP_STATUS);
	mem->set_snoop_status (SNOOP_NO_CONFLICT);
	
	if (stq->empty ()) return;

	uint32 index = stq->get_last_created ();
	uint32 head = stq->get_last_committed ();

	dynamic_instr_t *st;

	uint64 safe_distance = 0;
	if (g_conf_disambig_prediction) 
		safe_distance = disambig->get_safe_distance (d_instr);
	
	do {	

		st = stq->peek (index);
		ASSERT (st);
		index = stq->window_decrement (index);

		if (st->priority () > d_instr->priority ()) continue;
        
        if (st->get_tid() != d_instr->get_tid()) continue;

		ASSERT (st->priority () < d_instr->priority ());

		if (st->get_lsq_entry () == LSQ_ENTRY_CREATE) {
			if (d_instr->get_stq_ptr() - st->get_stq_ptr() < safe_distance) {
				mem->set_snoop_status (SNOOP_PRED_SAFE_ST);
			} else {
				if (!g_conf_initiate_loads_unresolved)
					mem->set_dependent (st);
				mem->set_snoop_status (SNOOP_UNRESOLVED_ST);
			}
			
			// continue snooping stq
			if (!g_conf_initiate_loads_unresolved && 
				!g_conf_disambig_prediction) break;	
			else 
				continue;
		}	

		if (st->unsafe_store ()) {
			mem->set_dependent (st);
			mem->set_snoop_status (SNOOP_UNSAFE);
			break;
		}
		
		mem_xaction_t *st_mem = st->get_mem_transaction ();
		
		// BLK ld/st
		if (st_mem->get_blk_addr () != mem->get_blk_addr ()) continue;
		
		if (mem->get_size() > sizeof(uint64) ||
			st_mem->get_size() > sizeof(uint64))
		{
			// Need both for bypass
            ASSERT(st->get_cpu_id() == d_instr->get_cpu_id());
			if (mem->get_size() > sizeof(uint64) &&
				st_mem->get_size() > sizeof(uint64))
			{
				mem->set_dependent (st);
				mem->obtain_bypass_val ();
				
				ASSERT (mem->get_snoop_status () != SNOOP_NO_CONFLICT);
				
				done = true;
				
				STAT_INC (pstats->stat_bypass_lsq);
				if (tstats)
					STAT_INC (tstats->stat_bypass_lsq);
			} else {			
				mem->set_dependent (st);
				mem->set_snoop_status (SNOOP_REPLAY);
				
				done = true;
				
				STAT_INC (pstats->stat_bypass_conflict_lsq);
				if (tstats)
					STAT_INC (tstats->stat_bypass_conflict_lsq);
			}
		}
		

		if (st_mem->get_dw_addr () != mem->get_dw_addr ()) continue;
		
		// see if bytemasks match
		if ( (mem->get_dw_bytemask () & st_mem->get_dw_bytemask ())
			== mem->get_dw_bytemask () ) {
			
			mem->set_dependent (st);

			mem->obtain_bypass_val ();
			STAT_INC (pstats->stat_bypass_lsq);
			if (tstats)
				STAT_INC (tstats->stat_bypass_lsq);

			ASSERT (mem->get_snoop_status () != SNOOP_NO_CONFLICT);

			// done with snooping stq
			done = true;

		} else if (mem->get_dw_bytemask () & st_mem->get_dw_bytemask ()) {
			mem->set_dependent (st);
			mem->set_snoop_status (SNOOP_REPLAY);

			// done with snooping stq
			done = true;

			STAT_INC (pstats->stat_bypass_conflict_lsq);
			if (tstats)
				STAT_INC (tstats->stat_bypass_conflict_lsq);
		} 
	} while (index != head && !done);
}

void
lsq_t::ldq_snoop (dynamic_instr_t *d_instr) {
	ASSERT (d_instr->is_store ());

	if (ldq->empty ()) return;

	mem_xaction_t *st_mem = d_instr->get_mem_transaction ();
	uint32 index = ldq->get_last_created ();
	uint32 head  = ldq->get_last_committed ();

	do {	

		dynamic_instr_t *ld = ldq->peek (index);
		ASSERT (ld);
		index = ldq->window_decrement (index);

        // ld from different thread
        if (ld->get_tid() != d_instr->get_tid()) continue;
        
		if (ld->priority () < d_instr->priority ()) break;

		// ld is younger, not issued
		if (ld->get_pipe_stage () < PIPE_MEM_ACCESS_PROGRESS) continue;
        
        
		ASSERT (ld->get_pipe_stage () != PIPE_EXCEPTION);

		if (ld->get_pipe_stage () >= PIPE_MEM_ACCESS_PROGRESS &&
			ld->get_pipe_stage () <= PIPE_COMMIT_CONTINUE) {

            if (d_instr->unsafe_store () && !ld->is_sync_shadow()) {
				ld->set_pipe_stage (PIPE_REPLAY);
                index = ldq->get_last_created ();
                head  = ldq->get_last_committed ();  
				uint64 distance = ld->get_stq_ptr() - d_instr->get_stq_ptr();
				disambig->record_dependence (ld, distance);
				
				STAT_INC (seq->get_pstats(ld->get_tid())->stat_unres_squash);
				STAT_INC (seq->get_tstats(ld->get_tid())->stat_unres_squash);
			} else {
				mem_xaction_t *ld_mem = ld->get_mem_transaction ();

				if (!ld_mem) continue;
				
				ASSERT (ld_mem);
				//ASSERT (st_mem->get_dw_addr () != 0);
				//ASSERT (ld_mem->get_dw_addr () != 0);
				ASSERT (!ld->unsafe_load ());

				generic_transaction_t * gtr = ld->get_mai_instruction ()->get_mem_transaction ();
				if (gtr) {
					ld_mem->get_v9 ()->set_mem_op (gtr);
					ASSERT (ld_mem->get_v9 ()->get_v9_mem_op ()->side_effect == 0);
				}

				
				// XXX one more check needed here.
				if  ((( ld_mem->get_dw_addr () == st_mem->get_dw_addr () ) &&
					 ( (ld_mem->get_dw_bytemask () & st_mem->get_dw_bytemask ()) )) ||
					// BLK ld/st
					(st_mem->get_blk_addr () == ld_mem->get_blk_addr () &&
					 (ld_mem->get_size() > sizeof(uint64) ||
					  st_mem->get_size() > sizeof(uint64))))
                {
					// This is conservative,  It is possible that the load got 
					// a bypass value correctly from a *younger* store, but we 
					// replay it anyway.
					ld->set_pipe_stage (PIPE_REPLAY);
                    index = ldq->get_last_created ();
                    head  = ldq->get_last_committed ();

					uint64 distance = ld->get_stq_ptr() - d_instr->get_stq_ptr();
					disambig->record_dependence (ld, distance);
					
					STAT_INC (seq->get_pstats(ld->get_tid())->stat_unres_squash);
					STAT_INC (seq->get_tstats(ld->get_tid())->stat_unres_squash);
				}
			}
		} 
	} while (index != head);
}

tick_t
lsq_t::handle_store (dynamic_instr_t *d_instr) {
	tick_t latency = 0;
	lsq_entry_t lsq_status = d_instr->get_lsq_entry ();
	mem_xaction_t *mem_xaction = d_instr->get_mem_transaction ();
	v9_mem_xaction_t *v9_mem_xaction = mem_xaction->get_v9 ();

	ASSERT (d_instr->is_store ());

	switch (lsq_status) {
	case LSQ_ENTRY_CREATE:
		hold (d_instr);
		if (g_conf_initiate_loads_unresolved || g_conf_disambig_prediction) 
			ldq_snoop (d_instr);

		latency = g_conf_max_latency;

		ASSERT (v9_mem_xaction->can_stall ());
		break;

	case LSQ_ENTRY_HOLD:
		FAIL;
		break;

	case LSQ_ENTRY_RELEASE:
		ASSERT (!d_instr->speculative ());
		ASSERT (!v9_mem_xaction->is_speculative ());
		d_instr->mark_lsq_entry (LSQ_ENTRY_DONE);
		
		if (!d_instr->immediate_release_store ()) {
			ASSERT (v9_mem_xaction->can_stall ());
			
			mem_xaction->void_xaction ();

			// if held in stb, cannot be unimpl
			if (mem_xaction->unimpl_checks ())
				WARNING;
		}

		latency = 0;
		break;

	case LSQ_ENTRY_DONE:
		ASSERT (d_instr->immediate_release_store () || 
			(g_conf_ldd_std_safe && d_instr->is_ldd_std()));
		latency = 0;
		break;

	default:
		FAIL;
	}

	return latency;
}

tick_t
lsq_t::handle_load (dynamic_instr_t *d_instr) {
	tick_t latency = 0;
	lsq_entry_t lsq_status = d_instr->get_lsq_entry ();
	mem_xaction_t *mem_xaction = d_instr->get_mem_transaction ();
	v9_mem_xaction_t *v9_mem_xaction = mem_xaction->get_v9 ();

	ASSERT (d_instr->is_load ());

	switch (lsq_status) {
	case LSQ_ENTRY_CREATE:
		hold (d_instr);
		latency = g_conf_max_latency;

		if (d_instr->is_prefetch ()) 
			latency = 0;
		else 
			ASSERT (v9_mem_xaction->can_stall ());
		break;

	case LSQ_ENTRY_HOLD:
		FAIL;
		break;

	case LSQ_ENTRY_RELEASE:
		d_instr->mark_lsq_entry (LSQ_ENTRY_DONE);

		latency = 0;
		break;

	case LSQ_ENTRY_DONE:
		ASSERT (d_instr->unsafe_load () || g_conf_ldd_std_safe);

		latency = 0;
		break;

	default:	
		FAIL;
	}

	return latency;
}


// called from memory hierarchy side
tick_t
lsq_t::operate (mem_xaction_t *mem_xaction) 
{
	v9_mem_xaction_t *v9_mem_xaction = mem_xaction->get_v9 ();
	v9_mem_xaction->block_stc ();
	v9_mem_xaction->squashed_noreissue ();

	ASSERT (v9_mem_xaction->is_cpu_xaction ());
	ASSERT (!v9_mem_xaction->is_icache_xaction ()); // data

	dynamic_instr_t *d_instr = v9_mem_xaction->get_dynamic_instr ();
	if (!d_instr)
		d_instr = sequencer_t::icache_d_instr;

	mem_xaction->safety_checks (d_instr);
    
	d_instr->set_mem_transaction (mem_xaction);

	tick_t latency = 0;
	if (d_instr->is_store ()) latency = handle_store (d_instr);
	else if (d_instr->is_load ()) latency = handle_load (d_instr);
	else if (d_instr->get_opcode () != i_flush
		&& d_instr->get_opcode () != i_prefetch
		&& d_instr->get_opcode () != i_prefetcha)
		WARNING;

	return latency;
}

void 
lsq_t::snoop (dynamic_instr_t *d_instr) {
	mem_xaction_t *mem_xaction = d_instr->get_mem_transaction ();

	if (!d_instr->is_load ()) return;
	
	if (g_conf_sync2_runahead && d_instr->is_sync_shadow()) {
		mem_xaction->set_snoop_status(SNOOP_NO_CONFLICT);
		return;
	}		
	
	ASSERT (!d_instr->unsafe_load ());
	stq_snoop (d_instr);

	if (mem_xaction->get_snoop_status () == SNOOP_NO_CONFLICT)
		stbuf_snoop (d_instr);
	else if ((mem_xaction->get_snoop_status () == SNOOP_UNRESOLVED_ST 
		   || mem_xaction->get_snoop_status () == SNOOP_PRED_SAFE_ST)
		&& (g_conf_initiate_loads_unresolved || g_conf_disambig_prediction))
	{
		stbuf_snoop (d_instr);
	}
	
	ASSERT (mem_xaction->get_snoop_status () != SNOOP_STATUS);
}

void
lsq_t::stbuf_snoop (dynamic_instr_t *d_instr) {
	st_buffer_t *st_buffer = seq->get_st_buffer ();

	// set status of mem transaction
//	mem_xaction_t *mem = d_instr->get_mem_transaction ();

//	mem->set_snoop_status (SNOOP_NO_CONFLICT);

	// snoop st buffer
	st_buffer->snoop (d_instr);
}


void
lsq_t::apply_bypass_value (mem_xaction_t *mem_xaction) {

	v9_mem_xaction_t *v9_mem_xaction = mem_xaction->get_v9 ();

	if (mem_xaction->get_snoop_status () == SNOOP_BYPASS_EXTND ||
		mem_xaction->get_snoop_status () == SNOOP_BYPASS_COMPLETE) {

		if (mem_xaction->unimpl_checks ())
			WARNING;

		ASSERT (!g_conf_disable_extended_bypass ||
			!g_conf_disable_complete_bypass);

		ASSERT (v9_mem_xaction->is_cpu_xaction ());
		ASSERT (!v9_mem_xaction->is_icache_xaction ());
		
		dynamic_instr_t *d_instr = v9_mem_xaction->get_dynamic_instr ();
		ASSERT (d_instr);
		ASSERT (d_instr->get_mem_transaction () == mem_xaction);

		mem_xaction->apply_bypass_value ();
	}
}

// XXX check this function

// tail to head sweep of all ldq entries is inefficient but needed to
// support selective re-execution.
void
lsq_t::invalidate_address (invalidate_addr_t *invalidate_addr) {
   	// update # of invalidations from cache
    // UGLY substitution, we don't really know here
	proc_stats_t *pstats = seq->get_pstats (0);
	proc_stats_t *tstats = seq->get_tstats (0);
	STAT_INC (pstats->stat_cache_invalidation);
	if (tstats) 
		STAT_INC (tstats->stat_cache_invalidation);

	// all loads are non-speculative. nothing to invalidate
	if (g_conf_initiate_loads_commit) return;

	// ldq is empty. nothing to invalidate
	if (ldq->empty ()) return;

	addr_t phys_addr = invalidate_addr->get_phys_addr ();
	uint32 size      = invalidate_addr->get_size ();

	// alignment check
	ASSERT ((phys_addr % size) == 0);

	addr_t dw_phys_addr = phys_addr / size;

	uint32 index = ldq->get_last_created ();
	uint32 head  = ldq->get_last_committed ();
    
    //uint32 index = window_increment(ldq->last_committed());
	//uint32 head  = ldq->get_last_committed ();
    //bool full_lsq = ldq->full();
    
	do {	
		dynamic_instr_t *ld = ldq->peek (index);
		ASSERT (ld);
		index = ldq->window_decrement (index);

		if (ld->get_pipe_stage () < PIPE_MEM_ACCESS_PROGRESS) continue;

		mem_xaction_t  *ld_mem = ld->get_mem_transaction ();
		if (!ld_mem) continue;

		// safe load. always executes non-speculatively
		if (ld->unsafe_load ()) continue;

		// load may have issued speculatively
		addr_t         ld_addr = ld_mem->get_phys_addr ();
		addr_t dw_ld_phys_addr = ld_addr / size;

		// different address
		if (dw_ld_phys_addr != dw_phys_addr) continue;

		// ignore if load executed at the head of I-window
        if (ld->get_execute_at_head()) {
            continue;
        }
		// all other loads have to replay
		if (ld->get_pipe_stage () >= PIPE_MEM_ACCESS_PROGRESS &&
			ld->get_pipe_stage () <= PIPE_COMMIT_CONTINUE) {
			ld->set_pipe_stage (PIPE_REPLAY);
			STAT_INC (seq->get_pstats(ld->get_tid())->stat_cache_squash);
            proc_stats_t *tstats = seq->get_tstats (ld->get_tid());
			if (tstats) 
				STAT_INC (tstats->stat_cache_squash);
            index = ldq->get_last_created ();
            head  = ldq->get_last_committed ();
            //break;
		}
			
	} while (index != head);
}

memq_t*
lsq_t::get_ldq (void) {
	return ldq;
}

memq_t*
lsq_t::get_stq (void) {
	return stq;
}

void
lsq_t::write_checkpoint(FILE *file)
{
	disambig->write_checkpoint(file);
}

void
lsq_t::read_checkpoint(FILE *file)
{
	disambig->read_checkpoint(file);
}

