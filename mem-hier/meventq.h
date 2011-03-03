/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: meventq.h,v 1.1.10.1 2005/08/30 14:38:31 pwells Exp $
 *
 * description:    event queue for the memory hierarchy
 * initial author: Philip Wells 
 *
 */
 

#ifndef _MEVENT_H_
#define _MEVENT_H_

struct generic_event_compare {
	bool operator() (generic_event_t *e1, generic_event_t *e2) const
	{
		return (e1->get_cycle() < e2->get_cycle());
	}
	
};

class meventq_t : public window_t <generic_event_t> {
private:
	set<generic_event_t *, generic_event_compare>  long_events;

public:
	meventq_t (const uint32 size);
	void insert (generic_event_t *_e, tick_t _c);
	void advance_cycle ();
};

#endif // _MEVENT_H_
