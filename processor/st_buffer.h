/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _ST_BUFFER_H_
#define _ST_BUFFER_H_

class st_buffer_t : public window_t <dynamic_instr_t> {

public:
	st_buffer_t (const uint32 size, sequencer_t *_seq);
	void advance_cycle (void);
	void snoop (dynamic_instr_t *d_instr);	
	void insert (dynamic_instr_t *d_instr);

	void panic_clear ();
    bool empty_ctxt (uint32 tid);
    dynamic_instr_t *head_ctxt (uint32 tid);

private:
	sequencer_t *seq;
    uint32 num_hw_ctxt;
};

#endif
