/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: st_buffer.cc,v 1.1.1.1.14.6 2006/08/18 14:30:21 kchak Exp $");

#include "definitions.h"
#include "config_extern.h"
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
#include "proc_stats.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"

st_buffer_t::st_buffer_t (const uint32 size, sequencer_t *_seq) :
	window_t <dynamic_instr_t>::window_t (size), seq (_seq) {
		num_hw_ctxt = seq->get_num_hw_ctxt();
}


void
st_buffer_t::insert (dynamic_instr_t *d_instr) {
	bool ret = window_t <dynamic_instr_t>::insert (d_instr);

	ASSERT (ret == true);

	d_instr->set_st_buffer_status (ST_BUFFER_INSERT);
}

// XXX handle many instructions
void
st_buffer_t::advance_cycle () {
	proc_stats_t *pstats = seq->get_pstats (0);

	if (pstats) pstats->st_buffer_stats (this);

	
    for(uint32 i = 0; i < num_hw_ctxt; i++)
    {
        if (empty ()) return;
        dynamic_instr_t *d_instr = head_ctxt (i);
        if (!d_instr) continue;
        
        mem_xaction_t *mem_xaction = d_instr->get_mem_transaction ();
        
        switch (d_instr->get_st_buffer_status ()) {
            
        case ST_BUFFER_INSERT:
            // st buffer status should be changed before initiate_mem_hier
            d_instr->set_st_buffer_status (ST_BUFFER_MEM_HIER);
            d_instr->set_pipe_stage (PIPE_ST_BUF_MEM_ACCESS_PROGRESS);
            
            d_instr->initiate_mem_hier ();
            break;
            
        case ST_BUFFER_MEM_HIER:
            break;
            
        case ST_BUFFER_DONE:
            mem_xaction->apply_store_changes ();
            
            pop_head (d_instr);
            d_instr->set_st_buffer_status (ST_BUFFER_ENTRIES);
            d_instr->insert_dead_list ();
            
            break;
            
        default:	
            FAIL;
        }
        
    }
}

// internal. do not use.
void
st_buffer_t::panic_clear () {
	while (head ()) {
		dynamic_instr_t *d_instr = head ();
		pop_head (d_instr);

		mem_xaction_t *mem_xaction = d_instr->get_mem_transaction ();
		mem_xaction->apply_store_changes ();

		d_instr->insert_dead_list ();
	}
}

void
st_buffer_t::snoop (dynamic_instr_t *d_instr) {
	proc_stats_t *pstats = seq->get_pstats (d_instr->get_tid());
	bool done = false;

	mem_xaction_t *mem = d_instr->get_mem_transaction ();

	ASSERT (mem->get_snoop_status () == SNOOP_NO_CONFLICT ||
		mem->get_snoop_status () == SNOOP_UNRESOLVED_ST ||
		mem->get_snoop_status () == SNOOP_PRED_SAFE_ST);

	if (empty ()) return; 

	uint32 index = get_last_created (); // tail
	uint32 head = get_last_committed ();

	dynamic_instr_t *st;

	do {

		// order important
		st = peek (index);
		ASSERT (st);
		index = window_decrement (index);

		mem_xaction_t *st_mem = st->get_mem_transaction ();

        // SMT check
        if (st->get_tid () != d_instr->get_tid ()) continue;
        
        
		if (st_mem->get_blk_addr () != mem->get_blk_addr ()) continue;
		
		// BLK ld/st
		if (mem->get_size() > sizeof(uint64) ||
			st_mem->get_size() > sizeof(uint64))
		{
			// Need both for bypass
			if (mem->get_size() > sizeof(uint64) &&
				st_mem->get_size() > sizeof(uint64))
			{
                ASSERT(st->get_cpu_id() == d_instr->get_cpu_id());
				mem->set_dependent (st);
				mem->obtain_bypass_val ();
				
				ASSERT (mem->get_snoop_status () != SNOOP_NO_CONFLICT &&
					mem->get_snoop_status () != SNOOP_UNRESOLVED_ST &&
					mem->get_snoop_status () != SNOOP_PRED_SAFE_ST);
				
				done = true;
				
				STAT_INC (pstats->stat_bypass_stb);
				
			} else {			
				mem->set_dependent (st);
				mem->set_snoop_status (SNOOP_REPLAY);
				
				done = true;
				
				STAT_INC (pstats->stat_replays);
				STAT_INC (pstats->stat_bypass_conflict_stb);
			}
		}
		
		if (st_mem->get_dw_addr () != mem->get_dw_addr ()) continue;
		
		// same quad word
		if (  (mem->get_dw_bytemask () & st_mem->get_dw_bytemask ()) 
				== mem->get_dw_bytemask ()  ) {
			mem->set_dependent (st);
			mem->obtain_bypass_val ();

			ASSERT (mem->get_snoop_status () != SNOOP_NO_CONFLICT);
            if (g_conf_trace_pipe) d_instr->print ("st_buff bypass:");
			ASSERT (mem->get_snoop_status () != SNOOP_NO_CONFLICT &&
				mem->get_snoop_status () != SNOOP_UNRESOLVED_ST &&
				mem->get_snoop_status () != SNOOP_PRED_SAFE_ST);

			done = true;

			STAT_INC (pstats->stat_bypass_stb);

		} else if (mem->get_dw_bytemask () & st_mem->get_dw_bytemask ()) {
			mem->set_dependent (st);
			mem->set_snoop_status (SNOOP_REPLAY);

			done = true;

			STAT_INC (pstats->stat_replays);
			STAT_INC (pstats->stat_bypass_conflict_stb);
		}
	} while (index != head && !done);
}


bool st_buffer_t::empty_ctxt(uint32 tid)
{
    if (empty()) return true;
    uint32 slot = last_created;
    
    do {
        if (window[slot]->get_tid() == tid)
            return false;
        slot = window_decrement(slot);
    } while (slot != last_committed);
    
    return true;
    
}

dynamic_instr_t *st_buffer_t::head_ctxt(uint32 tid)
{
    
    if (head()->get_tid() == tid) return head();
    uint32 slot = window_increment(last_committed);
    bool full_override = full();
    
    dynamic_instr_t *d_instr;
    while (slot != window_increment(last_created) || full_override)
    {
        d_instr = window [slot];
        if (d_instr->get_tid() == tid)
            return d_instr;
        slot = window_increment(slot);
        full_override = false;
    }
    
    return 0;
}
