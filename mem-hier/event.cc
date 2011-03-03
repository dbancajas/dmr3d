/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    event functionality for mem-hier
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: event.cc,v 1.2.10.1 2005/03/11 17:07:47 pwells Exp $");

#include "definitions.h"
#include "event.h"
#include "external.h"
#include "debugio.h"
#include "verbose_level.h"
#include "mem_hier.h"
#include "window.h"
#include "meventq.h"

generic_event_t::generic_event_t()
	: tail(NULL), next(NULL), enqueued(false)
{ }

void generic_event_t::enqueue()
{
	VERBOSE_OUT(verb_t::events, "generic_event_t::enqueue()\n");
	mem_hier_t::ptr()->get_eventq()->insert(this, cycle);
	enqueued = true;
}

void
generic_event_t::requeue(tick_t delta)
{
	cycle = delta;
	enqueue();
}

tick_t
generic_event_t::get_cycle()
{
	return cycle;
}

bool
generic_event_t::is_enqueued()
{
	return enqueued;
}

void
generic_event_t::mark_dequeued()
{
	enqueued = false;
}

