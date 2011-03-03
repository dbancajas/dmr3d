/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: disambig.h,v 1.1.2.1 2005/11/22 00:07:03 pwells Exp $
 *
 * description:    memory disambiguation - distance predictor
 * initial author: Philip Wells 
 *
 */
 
#ifndef _DISAMBIG_H_
#define _DISAMBIG_H_

class disambig_entry_t {
public:
	disambig_entry_t () : load_pc (0), safe_distance (0) { };
	addr_t load_pc;
	uint64 safe_distance;
};


class disambig_t {
public:
	disambig_t (uint64 size);
	~disambig_t ();
	
	void record_dependence (dynamic_instr_t *load, uint64 distance);
	uint64 get_safe_distance (dynamic_instr_t *load);

	void write_checkpoint(FILE *file);
    void read_checkpoint(FILE *file);

private:
	disambig_entry_t *distances;
	uint64 num_entries;
	addr_t entry_mask;
};

#endif // _DISAMBIG_H_


