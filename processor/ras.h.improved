/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _RAS_H_
#define _RAS_H_


class ras_state_t {
public:	
	uint32 top;
	uint32 next_free;

	ras_state_t ();
	void copy_state (ras_state_t *rs);
	void clear ();
};

class ras_t {
private:
	uint32 entries;
	uint32 mask;
	addr_t *table;
	uint32 *next_top; 
	
	uint32 *filter_tag;
	addr_t *filter_target;
	
	uint32 filter_table_size;
	uint32 filter_mask;
	uint32 filter_max;
	uint32 filter_thresh;

public:
	ras_t (uint32 bits, uint32 filter_bits);
	~ras_t ();
	
	void debug();

	void push (addr_t return_pc, ras_state_t *rs);
	addr_t pop (ras_state_t *rs);

	uint32 increment (uint32 a);
	void fix_state(ras_state_t *rs, addr_t return_pc, addr_t pred_pc);
	
	bool filter_pc(addr_t return_pc);
	void update_commit (addr_t return_pc);
	uint32 index_pc(addr_t pc);
    addr_t peek_top(ras_state_t *rs);


    void write_checkpoint(FILE *file);
    void read_checkpoint(FILE *file);

};
#endif
