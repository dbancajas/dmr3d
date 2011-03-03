/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _YAGS_H_
#define _YAGS_H_

enum yags_pred_t {
	YAGS_NOT_TAKEN = 0,
	YAGS_WEAKLY_NOT_TAKEN,
	YAGS_WEAKLY_TAKEN,
	YAGS_TAKEN,
	YAGS_PRED
};


class yags_t {

public:
	yags_t (uint32 choice, uint32 exception, uint32 tag);
	~yags_t ();

	bool predict (addr_t pc, direct_state_t h);
	void update (addr_t pc, direct_state_t h, bool taken);
	void update_state (direct_state_t &h, bool taken);
	void fix_state (direct_state_t &h);

	uint32 munge_pc (addr_t pc);
	uint32 generate_choice (uint32 munge_pc);
	uint32 generate_index (uint32 munge_pc, direct_state_t h);
	tag_t  generate_tag (uint32 munge_pc, direct_state_t h);
    void write_checkpoint(FILE *file);
    void read_checkpoint(FILE *file);

private:
	uint32      choice_pht_size;
	uint32      choice_pht_mask;

	uint32      exception_size;
	uint32      exception_mask;

	uint32      tag_bits;
	uint32      tag_mask;
	
	yags_pred_t *choice_pht;
	
	tag_t       *tkn_tag;
	yags_pred_t *tkn_pred;

	tag_t       *ntkn_tag;
	yags_pred_t *ntkn_pred;

	static yags_pred_t taken_update [4];
	static yags_pred_t ntaken_update [4];

};

#endif
