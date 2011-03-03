/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: yags.cc,v 1.1.1.1.14.2 2005/10/28 18:28:04 pwells Exp $");

#include "definitions.h"
#include "yags.h"

yags_pred_t yags_t::taken_update [4] = {YAGS_WEAKLY_NOT_TAKEN, 
	YAGS_WEAKLY_TAKEN, YAGS_TAKEN, YAGS_TAKEN};

yags_pred_t yags_t::ntaken_update [4] = {YAGS_NOT_TAKEN, 
	YAGS_NOT_TAKEN, YAGS_WEAKLY_NOT_TAKEN, YAGS_WEAKLY_TAKEN};

    
yags_t::yags_t (uint32 choice, uint32 exception, uint32 tag) {
	
	choice_pht_size = 1 << choice;
	choice_pht_mask = choice_pht_size - 1;

	choice_pht = new yags_pred_t[choice_pht_size];	

	for (uint32 i = 0; i < choice_pht_size; i++)
		choice_pht [i] = YAGS_WEAKLY_TAKEN;

	exception_size = 1 << exception;
	exception_mask = exception_size - 1;

	tag_bits = tag;
	tag_mask = (1 << tag_bits) - 1;

	tkn_tag   = new tag_t[exception_size];
	ntkn_tag  = new tag_t[exception_size];
	tkn_pred  = new yags_pred_t[exception_size];
	ntkn_pred = new yags_pred_t[exception_size];

	for (uint32 i = 0; i < exception_size; i++) {
		tkn_tag   [i] = 0;
		ntkn_tag  [i] = 0;
		tkn_pred  [i] = YAGS_TAKEN;
		ntkn_pred [i] = YAGS_NOT_TAKEN;
	}
}

yags_t::~yags_t () {
	delete [] choice_pht;
	delete [] tkn_tag;
	delete [] ntkn_tag;
	delete [] tkn_pred;
	delete [] ntkn_pred;
}

void
yags_t::write_checkpoint(FILE *file) {
    for (uint32 i = 0; i < choice_pht_size; i++) 
        fprintf(file, "%u\n", (uint32)choice_pht[i]);
    for (uint32 i = 0; i < exception_size; i++)
        fprintf(file, "%llu %llu %u %u\n", tkn_tag[i], ntkn_tag[i],
                (uint32)tkn_pred[i], (uint32)ntkn_pred[i]);
}

void
yags_t::read_checkpoint(FILE *file) 
{
    for (uint32 i = 0; i < choice_pht_size; i++)
        fscanf(file, "%u\n", (uint32 *) &choice_pht[i]);
    for (uint32 i = 0; i < exception_size; i++)
        fscanf(file, "%llu %llu %u %u\n", &tkn_tag[i], &ntkn_tag[i],
                (uint32 *) &tkn_pred[i], (uint32 *) &ntkn_pred[i]);

}


bool 
yags_t::predict (addr_t pc, direct_state_t h) {
	uint32 shifted_pc = munge_pc (pc);
	uint32 choice_index = generate_choice (shifted_pc);
	
	ASSERT (choice_index < choice_pht_size);
	
	bool taken_bias = (choice_pht [choice_index] >= YAGS_WEAKLY_TAKEN);
	bool predict = taken_bias;

	uint32 except_index = generate_index (shifted_pc, h);
	ASSERT (except_index < exception_size);

	tag_t tag = generate_tag (shifted_pc, h);

	if (taken_bias) {
		if (tkn_tag [except_index] == tag)
			predict = (tkn_pred [except_index] >= YAGS_WEAKLY_TAKEN);
	} else {
		if (ntkn_tag [except_index] == tag) 
			predict = (ntkn_pred [except_index] >= YAGS_WEAKLY_TAKEN);
	}

	return predict;
}

void 
yags_t::update_state (direct_state_t &h, bool taken) {
	h = ((h << 1) | (taken & 0x1));
}

void
yags_t::fix_state (direct_state_t &h) {
	h = h ^ 0x1;
}

void
yags_t::update (addr_t pc, direct_state_t h, bool taken) {
	const yags_pred_t *update_fn;

	uint32 shifted_pc = munge_pc (pc);
	uint32 choice_index = generate_choice (shifted_pc);
	ASSERT (choice_index < choice_pht_size);

	uint32 except_index = generate_index (shifted_pc, h);
	ASSERT (except_index < exception_size);

	tag_t tag = generate_tag (shifted_pc, h);
	yags_pred_t old_pred = choice_pht [choice_index];


	if (taken) 
		update_fn = yags_t::taken_update;
	else 
		update_fn = yags_t::ntaken_update;

	choice_pht [choice_index] = update_fn [old_pred];	

	if (old_pred >= YAGS_WEAKLY_TAKEN) {
		if (tkn_tag [except_index] == tag) {
			yags_pred_t except_pred = tkn_pred [except_index];
			tkn_pred [except_index] = update_fn [except_pred];
		} else if (!taken) {
			tkn_tag [except_index] = tag;
			tkn_pred [except_index] = YAGS_WEAKLY_NOT_TAKEN;
		}
	} else {
		if (ntkn_tag [except_index] == tag) {
			yags_pred_t except_pred = ntkn_pred [except_index];
			ntkn_pred [except_index] = update_fn [except_pred];
		} else if (taken) {
			ntkn_tag [except_index] = tag;
			ntkn_pred [except_index] = YAGS_WEAKLY_TAKEN;
		}
	}
}

uint32 
yags_t::munge_pc (addr_t pc) {
	return (uint32) ((pc >> 2) & 0xffffffff);
}

uint32 
yags_t::generate_choice (uint32 munge_pc) {
	return (munge_pc & choice_pht_mask);
}

uint32
yags_t::generate_index (uint32 munge_pc, direct_state_t h) {
	return ((munge_pc ^ h) & exception_mask);
}

tag_t
yags_t::generate_tag (uint32 munge_pc, direct_state_t h) {
	return (tag_t) (munge_pc & tag_mask);
}

