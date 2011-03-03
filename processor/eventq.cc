/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: eventq.cc,v 1.2.6.1 2005/10/28 18:28:01 pwells Exp $");

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

#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "config_extern.h"

eventq_t::eventq_t (const uint32 size): 
	window_t <dynamic_instr_t>::window_t (size) {
	last_created = 0;	
}

void 
eventq_t::insert (dynamic_instr_t *d_instr, tick_t _c) {

	ASSERT (_c < window_size);

	_c = (_c + last_created) % window_size;
	uint32 slot = _c;
	
	ASSERT (!d_instr->next_event);
	ASSERT (!d_instr->tail_event);
	
	d_instr->set_outstanding (PENDING_EVENTQ);
		
	dynamic_instr_t *slot_instr = window [slot];
	if (!slot_instr) {
		window [slot] = d_instr;
		window [slot]->tail_event = d_instr;
	} else {
		dynamic_instr_t *tail_event = window [slot]->tail_event;
		tail_event->next_event = d_instr;
		window [slot]->tail_event = d_instr;
	}
}

void
eventq_t::advance_cycle () {
	dynamic_instr_t *next_d_instr = 0;
	dynamic_instr_t *d_instr;

	ASSERT (window [last_created] == 0);

	last_created = window_increment (last_created);
	d_instr = window [last_created];

	if (!d_instr) return;

	window [last_created] = 0;

	for (; d_instr; d_instr = next_d_instr) {
		next_d_instr = d_instr->next_event;

		d_instr->reset_outstanding (PENDING_EVENTQ);
		d_instr->next_event = 0;
		d_instr->tail_event = 0;

		d_instr->wakeup ();
	}
}

