/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _WAIT_LIST_H_
#define _WAIT_LIST_H_

class wait_list_t : public window_t <dynamic_instr_t> {
	
private:
	wait_list_t *next;
	wait_list_t *end;

public: 
	wait_list_t (const uint32 size);
	~wait_list_t ();

	wait_list_t* create_wait_list (void);
	wait_list_t* get_end_wait_list (void);
	wait_list_t* get_next_wl (void);
	void set_end_wait_list (wait_list_t *wl);
	void set_next_wait_list (wait_list_t *wl);

	uint32 get_size (void);
    void debug (void);
};

#endif
