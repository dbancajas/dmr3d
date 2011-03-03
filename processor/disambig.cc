/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    memory disambiguation - distance predictor
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: disambig.cc,v 1.1.2.1 2005/11/22 00:07:02 pwells Exp $");

#include "definitions.h"
#include "isa.h"
#include "dynamic.h"
#include "disambig.h"
#include "config_extern.h"

disambig_t::disambig_t (uint64 size) {
	num_entries = size;
	entry_mask = num_entries-1;
	
	distances = new disambig_entry_t [num_entries];
}

disambig_t::~disambig_t () {
	delete [] distances;
}

void
disambig_t::record_dependence (dynamic_instr_t *load, uint64 distance)
{
	uint64 entry = entry_mask & (load->get_pc () >> 2);
	ASSERT (entry < num_entries);
	
	// ignore conflicts
	// TODO: associative
	distances[entry].load_pc = load->get_pc ();
	distances[entry].safe_distance = distance; 
}

uint64
disambig_t::get_safe_distance (dynamic_instr_t *load)
{
	uint64 entry = entry_mask & (load->get_pc () >> 2);
	ASSERT (entry < num_entries);

	if (distances[entry].load_pc == load->get_pc ()) 
		return distances[entry].safe_distance;
	else 
		return ~0;
}

void
disambig_t::write_checkpoint(FILE *file) {
    for (uint32 i = 0; i < num_entries; i++) { 
        fprintf(file, "%llx %llu\n", distances[i].load_pc, 
			distances[i].safe_distance);
	}
}

void
disambig_t::read_checkpoint(FILE *file) 
{
    for (uint32 i = 0; i < num_entries; i++) { 
        fscanf(file, "%llx %llu\n", &distances[i].load_pc, 
			&distances[i].safe_distance);
	}
}


