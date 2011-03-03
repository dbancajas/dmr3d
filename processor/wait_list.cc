/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: wait_list.cc,v 1.1.1.1.14.2 2006/08/18 14:30:21 kchak Exp $");

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


wait_list_t::wait_list_t (const uint32 size) :
	window_t <dynamic_instr_t>::window_t (size) {
	end = this;
	next = 0;
}

wait_list_t::~wait_list_t () {
	delete next;
}

wait_list_t*
wait_list_t::get_end_wait_list () {
	return end;
}

wait_list_t*
wait_list_t::create_wait_list () {
	wait_list_t *wl;

	ASSERT (end->next == 0);

	wl = new wait_list_t (g_conf_wait_list_size);
	end->next = wl;
	end = wl;

	return end;
}

wait_list_t*
wait_list_t::get_next_wl () {
	return next;
}

void
wait_list_t::set_end_wait_list (wait_list_t *wl) {
	end = wl;
}

void
wait_list_t::set_next_wait_list (wait_list_t *wl) {
	next = wl;
}

uint32
wait_list_t::get_size () {
	wait_list_t *wl = this;
	uint32 size = 0;

	for (; wl; wl = wl->next, ++size);

	return size;
}


void 
wait_list_t::debug () {
    for (uint32 i = 0; i < get_window_size (); i++)
    {
        dynamic_instr_t *d = window[i];
        if (!d) continue;
        DEBUG_OUT ("%llx \n", d);
    }
    
}

