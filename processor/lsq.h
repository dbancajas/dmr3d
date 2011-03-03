/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */


#ifndef _LSQ_H_
#define _LSQ_H_


class memq_t : public window_t <dynamic_instr_t> {

public:
	memq_t (const uint32 size);
	instr_event_t insert (dynamic_instr_t *d_instr);

	void squash (dynamic_instr_t *d_instr);
    void reassign_lsq_slot (void);
};

class lsq_t {

private:

	sequencer_t *seq;

	memq_t *ldq;
	memq_t *stq;

	disambig_t *disambig;
	uint64 store_count;

public:
	lsq_t (sequencer_t *_seq);
	~lsq_t ();

	instr_event_t insert_load (dynamic_instr_t *d_instr);
	instr_event_t insert_store (dynamic_instr_t *d_instr);

	void pop_load (dynamic_instr_t *d_instr);
	void pop_store (dynamic_instr_t *d_instr);

	void squash_loads (dynamic_instr_t *d_instr);
	void squash_stores (dynamic_instr_t *d_instr);
	bool prev_stores_executed (dynamic_instr_t *d_instr);

	void hold (dynamic_instr_t *d_instr);
	void release_hold (dynamic_instr_t *d_instr);

	tick_t handle_load (dynamic_instr_t *d_instr);
	tick_t handle_store (dynamic_instr_t *d_instr);

	tick_t operate (mem_xaction_t *mem_xaction);

	void snoop (dynamic_instr_t *d_instr);
	void stbuf_snoop (dynamic_instr_t *d_instr);
	void apply_bypass_value (mem_xaction_t *mem_xaction);
	void stq_snoop (dynamic_instr_t *d_instr);
	void ldq_snoop (dynamic_instr_t *d_instr);
	dynamic_instr_t* get_prev_store (dynamic_instr_t *d_instr);
	dynamic_instr_t* get_prev_load (dynamic_instr_t *d_instr);

	void invalidate_address (invalidate_addr_t *inv_addr);

	memq_t *get_ldq (void);
	memq_t *get_stq (void);
	
    void write_checkpoint(FILE *file);
    void read_checkpoint(FILE *file);
};

#endif
