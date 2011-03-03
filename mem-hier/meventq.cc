/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    event queue for the memory hierarchy
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: meventq.cc,v 1.2.10.4 2006/12/12 18:36:58 pwells Exp $");

#include "definitions.h"
#include "window.h"
#include "event.h"
#include "meventq.h"

meventq_t::meventq_t (const uint32 size): 
	window_t <generic_event_t>::window_t (size) {
	last_created = 0;	
}

void 
meventq_t::insert (generic_event_t *event, tick_t _c) {

	if (_c == 0) {
		event->do_event();
		return;
	}
	
	// TODO: bigger than window
	if (_c > window_size) {
		long_events.insert(event);
		return;
	}

	_c = (_c + last_created) % window_size;
	uint32 slot = _c;
	
	ASSERT (!event->next);
	ASSERT (!event->tail);
	//ASSERT (!event->is_enqueued());  can happen when moved from head to linear queue
	
	generic_event_t *slot_event = window [slot];
	if (!slot_event) {
		window [slot] = event;
		window [slot]->tail = event;
	} else {
		generic_event_t *tail = window [slot]->tail;
		tail->next = event;
		window [slot]->tail = event;
	}
}

void
meventq_t::advance_cycle () {
	generic_event_t *next_event = 0;
	generic_event_t *event;

	ASSERT (window [last_created] == 0);

	last_created = window_increment (last_created);
	bool done = false;
	
	if (!long_events.empty()) {
		set<generic_event_t *, generic_event_compare>::iterator iter =
			long_events.begin();
			
		// Not always a good idea to change the cost field of a heap, but we 
		// reduce all by one at the same time before heap is modified
		// Oh, and this is slow so hope it's uncommon
		while (iter != long_events.end()) {
			(*iter++)->cycle--;
		}
		iter = long_events.begin();
		while (!long_events.empty() && (*iter)->cycle < window_size) {
			insert(*iter, (*iter)->cycle);           // insert into linear event queue
			long_events.erase(iter); // and delete from leftover heap
			iter = long_events.begin();
		}
	}

	while (!done) {
		event = window [last_created];

		if (!event) return;

		window [last_created] = 0;

		for (; event; event = next_event) {
			next_event = event->next;

			event->next = 0;
			event->tail = 0;
			
			event->do_event ();
		}
	    done = (window[last_created] == 0); 
	}
}

