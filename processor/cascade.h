/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _CASCADE_H_
#define _CASCADE_H_


class cascade_t {

public:
	cascade_t (uint32 filter, uint32 except, uint32 shift);
	~cascade_t ();

	addr_t predict (addr_t pc, indirect_state_t h);
	void update (addr_t pc, indirect_state_t h, addr_t target_pc);

	void update_state (indirect_state_t &h, addr_t addr);
	void fix_state (indirect_state_t &h, addr_t addr);

	uint32 munge_pc (addr_t pc);
	uint32 generate_index (uint32 munged_pc);
	uint32 generate_except_index (uint32 shifted_pc, indirect_state_t h);
	uint32 generate_except_tag (addr_t pc);
    void read_checkpoint(FILE *file);
    void write_checkpoint(FILE *file);

private:
	uint32 filter_table_bits;
	uint32 filter_table_size;
	uint32 filter_table_mask;

	addr_t *filter_table;

	uint32 except_table_bits;
	uint32 except_table_size;
	uint32 except_table_mask;

	uint32 except_shift;

	addr_t *except_table;
	tag_t  *except_tags;

};
#endif
