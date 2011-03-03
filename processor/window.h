/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _WINDOW_T_
#define _WINDOW_T_

template <class T>
class window_t {

public:
	window_t (const uint32 size);
	~window_t ();

	bool slot_avail (void);
	uint32 get_window_size (void);
	uint32 window_increment (uint32 index);
	uint32 window_decrement (uint32 index);
	uint32 window_wrap (uint32 index);

	bool insert (T *d);

	T *head (void);
	T *tail (void);

	bool pop_head (T *d); // Indicates if we need to reassign slots
	void pop_tail (T *d);

	void erase (uint32 index);

	uint32 get_last_created (void);
	uint32 get_last_committed (void);

	T *peek (uint32 index);

	bool empty (void);
	bool full (void);
    void collapse (void);

protected:
	uint32 window_size;
	uint32 last_created;
	uint32 last_committed;

	T      **window;
    T      **tmps;

};

#include "window.cc"

#endif

