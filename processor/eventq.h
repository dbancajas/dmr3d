/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */


#ifndef _EVENT_H_
#define _EVENT_H_

class eventq_t : public window_t <dynamic_instr_t> {

public:
	eventq_t (const uint32 size);
	void insert (dynamic_instr_t *d_instr, tick_t _c);
	void advance_cycle ();
};

#endif
