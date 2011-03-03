/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: iwindow.cc,v 1.4.4.4 2006/08/18 14:30:19 kchak Exp $");

#include "definitions.h"
#include "mai.h"
#include "window.h"
#include "iwindow.h"
#include "sequencer.h"
#include "fu.h"
#include "isa.h"
#include "dynamic.h"
#include "mai_instr.h"
#include "lsq.h"
#include "config_extern.h"

iwindow_t::iwindow_t (const uint32 size) : 
	window_t <dynamic_instr_t>::window_t (size) {}

instr_event_t
iwindow_t::insert (dynamic_instr_t *d_instr) {

	dynamic_instr_t *tail_d_instr = tail (d_instr->get_tid());
	bool ret = window_t <dynamic_instr_t>::insert (d_instr);

	if (ret) {
		d_instr->set_window_slot (last_created);

		d_instr->set_prev_d_instr (tail_d_instr);
		if (tail_d_instr) tail_d_instr->set_next_d_instr (d_instr);

		return EVENT_OK; 
	} else {
		return EVENT_FAILED;
	}
}


dynamic_instr_t * iwindow_t::tail(uint8 id)
{
    dynamic_instr_t *tail_d_instr = window[last_created];
    if (tail_d_instr == NULL || 
        (tail_d_instr && tail_d_instr->get_tid() == id))
        return tail_d_instr;
        
    // Need to traverse
    uint32 slot = window_decrement(last_created);
    while (slot != last_committed)
    {
        tail_d_instr = window [slot];
        ASSERT(tail_d_instr);
        if (tail_d_instr->get_tid() == id)
            return tail_d_instr;
        
        slot = window_decrement(slot);
    }
    
    return NULL;
}
        

void 
iwindow_t::squash (dynamic_instr_t *d_instr, lsq_t *lsq) {
	dynamic_instr_t *ld_squash_instr; 
	dynamic_instr_t *st_squash_instr;

	if (!d_instr) return;

	ld_squash_instr = 0;
	st_squash_instr = 0;
	
//	uint32 slot = window_increment (d_instr->get_window_slot ());
	uint32 slot = d_instr->get_window_slot ();
//    bool  need_collapse = (slot != window_increment(last_committed)); 
    bool  need_collapse = false;
    uint32 squash_count = 0;
    uint32 squash_tid = d_instr->get_tid();

//	while (slot != window_increment (last_created)) {
	do {
		dynamic_instr_t *squash_instr = window [slot];
		ASSERT (squash_instr);

        if (squash_instr->get_tid () == squash_tid)
        {
            if (!ld_squash_instr && 
                squash_instr->is_load () &&
			squash_instr->get_lsq_slot () != -1) 
			ld_squash_instr = squash_instr;
            
            if (!st_squash_instr && 
                squash_instr->is_store () && 
			squash_instr->get_lsq_slot () != -1)
			st_squash_instr = squash_instr;
            dynamic_instr_t *prev_d_instr = squash_instr->get_prev_d_instr ();
            if (prev_d_instr) prev_d_instr->set_next_d_instr (0);
            dynamic_instr_t *next_d_instr = squash_instr->get_next_d_instr ();
            if (next_d_instr) next_d_instr->set_prev_d_instr (0);
            window [slot] = 0;
            squash_instr->mark_dead ();
            squash_instr->set_squashed ();
            squash_count++;

        } else need_collapse = true;
        
		slot = window_increment (slot);
	} while (slot != window_increment (last_created));

    
	if (ld_squash_instr) lsq->squash_loads (ld_squash_instr);
	if (st_squash_instr) lsq->squash_stores (st_squash_instr);
    
    if (need_collapse) {
        collapse();
        reassign_window_slot();
    } else {
        last_created = window_decrement (d_instr->get_window_slot ());
    }
    
    d_instr->get_sequencer()->update_icount(squash_tid, squash_count);
	
}


void iwindow_t::reassign_window_slot()
{

    for (uint32 i = 0; i < window_size; i++)
    {
        if (window[i]) window[i]->set_window_slot(i);
        //else break;
    }
}

void 
iwindow_t::debug (void) {
	for (uint32 i = 0; i < get_window_size (); i++) {
		dynamic_instr_t *d_instr = window [i];
		
		DEBUG_OUT ("\n%d: ", i);
		if (!d_instr) continue;
		
		mai_instruction_t *mai_instr = d_instr->get_mai_instruction ();
		
		DEBUG_OUT ("%c  ", mai_instr->debug_stage ());
		if (static_cast <uint32> (mai_instr->get_stage ()) < 
			static_cast <uint32> (INSTR_DECODED)) continue;
					
		mai_instr->debug (false); 
	}
	
	DEBUG_OUT ("\n");
}

void
iwindow_t::print (dynamic_instr_t *d) {
    uint32 total = 0;
    uint32 sz = get_window_size ();

    bool all = (d == 0);
    uint8 tid = d ? d->get_tid() : 0;
    DEBUG_OUT ("\n");

    for (uint32 i = 0; i < sz; i++) {
        dynamic_instr_t *d_instr = window [(i+last_committed) % sz];
        if (!d_instr || (!all && d_instr && d_instr->get_tid () != tid))
            continue;
        else total ++;

        DEBUG_OUT ("\n%02u %-3d: %llx sid %-8lld stage %-20s @ %llu ",
                d_instr->get_tid(), d_instr->get_window_slot (), d_instr->get_pc(), 
                d_instr->priority (), d_instr->pstage_str ().c_str (), d_instr->get_stage_entry_cycle ());

        mai_instruction_t *mai_instr = d_instr->get_mai_instruction ();
        DEBUG_OUT ("%c  ", mai_instr->debug_stage ());
        if (static_cast <uint32> (mai_instr->get_stage ()) <
        static_cast <uint32> (INSTR_DECODED)) {
            continue;
        }
        mai_instr->debug (false);
    }

    if (head ())
        DEBUG_OUT ("\ncpu_id %d #instr %d fetch_status %d lsq_full %d\n",
            head ()->get_cpu_id (), total, head ()->get_sequencer ()->get_fu_status (head()->get_tid()),
            head ()->get_sequencer ()->get_lsq ()->get_ldq ()->full ());
}

dynamic_instr_t*
iwindow_t::get_prev_d_instr (dynamic_instr_t *d_instr) {

	if (head () == d_instr) return 0;

	uint32 index = d_instr->get_window_slot ();
	index = window_decrement (index);
    
    while (index != last_committed)
    {
        
        dynamic_instr_t *prev_dinstr = peek (index);
        
        if (prev_dinstr->get_tid () == d_instr->get_tid())
        {
            ASSERT (prev_dinstr->priority () < d_instr->priority ());
            ASSERT (d_instr->get_prev_d_instr () == prev_dinstr);
            
            return prev_dinstr;
        }
        
        index = window_decrement(index);
        
    }
    
    return 0;
}


dynamic_instr_t *
iwindow_t:: peek_instr_for_commit (set<uint8> tlist)
{
    
    uint32 slot = window_increment (last_committed);
    uint32 head = window_increment(last_created);
    dynamic_instr_t *d_instr;
    do
    {
        d_instr = window [slot];
        if (tlist.find(d_instr->get_tid()) == tlist.end())
            return d_instr;
        slot = window_increment(slot);
    } while (slot != head);
    
    return NULL;
    
}


dynamic_instr_t *
iwindow_t::head_per_thread(uint8 tid)
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
    
    FAIL;
    
}


bool iwindow_t::empty_ctxt(uint8 tid)
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
