
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _FASTSIM_H_
#define _FASTSIM_H_

class fastsim_t {
private:
	conf_object_t *cpu;
	uint64 icount;
	int64 ivec;
	bool in_fastsim;

public:	
	fastsim_t (conf_object_t *_cpu);
	~fastsim_t ();

	void set_interrupt (int64 v);
	int64 get_interrupt (void);
	void sim_icount (uint64 _icount);
	bool sim (void);
	void debug (instruction_id_t instr_id);
	bool fastsim_mode (void);
};

#endif
