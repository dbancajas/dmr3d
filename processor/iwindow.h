/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _IWINDOW_H_
#define _IWINDOW_H_

class iwindow_t : public window_t <dynamic_instr_t> {

public:
	iwindow_t (const uint32 size);

	instr_event_t insert (dynamic_instr_t *d_instr);
	void squash (dynamic_instr_t *d_instr, lsq_t *lsq);
	void debug (void);
	void print (dynamic_instr_t *d = 0); 
    dynamic_instr_t * tail (uint8);
	dynamic_instr_t* get_prev_d_instr (dynamic_instr_t *d_instr);
    dynamic_instr_t * peek_instr_for_commit (set<uint8> tlist);
    
    void reassign_window_slot (void);
    dynamic_instr_t * head_per_thread(uint8 tid);
    bool empty_ctxt (uint8 tid);
};

#endif
