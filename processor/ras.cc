/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID ("$Id: ras.cc,v 1.1.1.1.14.1 2005/03/22 13:11:34 kchak Exp $");

#include "definitions.h"
#include "ras.h"

ras_state_t::ras_state_t () {
	clear ();
}

void 
ras_state_t::clear () {
	top = 0; 
	next_free = 0;
}

void
ras_state_t::copy_state (ras_state_t *rs) {
	top = rs->top;
	next_free = rs->next_free;
}

ras_t::ras_t (uint32 bits, uint32 filter_bits) {
	entries  = 1 << bits;
	mask     = entries - 1;
	
	table    = new addr_t[entries];
	next_top = new uint32[entries];

	for (uint32 i = 0; i < entries; i++) {
		table [i] = 0;
		next_top [i] = 0;
	}
	
	filter_table_size = 1 << filter_bits;
	filter_mask  = filter_table_size - 1;
	filter_max = 3;
	filter_thresh = 2;
	
    filter_tag = new uint32[filter_table_size];
    filter_target = new addr_t[filter_table_size];
	
	for (uint32 i = 0; i < filter_table_size; i++) {
		filter_tag [i] = 0;
		filter_target [i] = 0;
	}

}

void
ras_t::write_checkpoint(FILE *file)
{
    for (uint32 i = 0; i < entries; i++)
        fprintf(file, "%llu %u\n", table[i], next_top[i]);
    for (uint32 i = 0; i < filter_table_size; i++)
        fprintf(file, "%llu %u\n", filter_target[i], filter_tag[i]);
}

void
ras_t::read_checkpoint(FILE *file)
{
    for (uint32 i = 0; i < entries; i++) 
        fscanf(file, "%llu %u\n", &table[i], &next_top[i]);
    for (uint32 i = 0; i < filter_table_size; i++)
        fscanf(file, "%llu %u\n", &filter_target[i], &filter_tag[i]);
}


void
ras_t::push (addr_t return_pc, ras_state_t *rs) {
    ASSERT (rs->next_free < entries);


	// DEBUG_OUT("%20s push 0x%08llx %02d %02d sat:%d 0x%08llx %d\n", " ", return_pc,
	// 	rs->top, rs->next_free, filter_tag[index_pc(return_pc)], 
	// 	filter_target[index_pc(return_pc)],
	// 	index_pc(return_pc));
	
	
	
	if (filter_pc(return_pc)) {
		// DEBUG_OUT("ignoring: %02d\n", filter_tag[index_pc(return_pc)]);
		return;
	}
	
	table [rs->next_free] = return_pc;
	next_top [rs->next_free] = rs->top;

	rs->top = rs->next_free;
	rs->next_free = increment (rs->next_free);
}

addr_t
ras_t::pop (ras_state_t *rs) {
     ASSERT (rs->top < entries);

	 
	 // DEBUG_OUT("%20s pop  0x%08llx %02d %02d\n", " ", table[rs->top],
		//  rs->top, next_top[rs->top]);

	addr_t addr = table [rs->top];
	rs->top     = next_top [rs->top];

	return addr;
}


void
ras_t::fix_state (ras_state_t *rs, addr_t correct_pc, addr_t pred_pc) {
	
	if (filter_table_size <= 1) return;
	
	// DEBUG_OUT("%s fix  0x%08llx %02d %02d sat:%d\n", " ", correct_pc,
	// 	rs->top, next_top[rs->top], filter_tag[index_pc(pred_pc)]);	

	//Search top of RAS for the correct PC
    uint32 new_top = rs->top;
	bool found_it = false;
    for (uint32 i = 0; i < 4; i++)
    {
        if (table[new_top] == correct_pc) {
			found_it = true;
            break;
        }
        new_top = next_top[new_top];
    }

	if (!found_it) return;
	
	// DEBUG_OUT("%s   found  0x%08llx\n", " ", correct_pc);	

	// If we found the right one...
	// first update filter for already popped pred_pc
	uint32 idx = index_pc(pred_pc);
	// DEBUG_OUT("%s   fixing  0x%08llx %02d %02d sat:%d\n", " ", pred_pc,
	// 	rs->top, next_top[rs->top], filter_tag[idx]);	
	
	if (filter_target[idx] == pred_pc) {
		if (filter_tag[idx] < filter_max) 
			filter_tag[idx]++;
	} else {
		filter_target[idx] = pred_pc;
		filter_tag[idx] = filter_thresh;
	}
	
	// then.., go back through stack and update filter table with rest if any
	uint32 index = rs->top;
    while (index != new_top) { 

		addr_t skip_pc = table[index];
		idx = index_pc(skip_pc);

		// DEBUG_OUT("%s   fixing  0x%08llx %02d %02d sat:%d\n", " ", skip_pc,
		// 	rs->top, next_top[rs->top], filter_tag[idx]);	
		
		if (filter_target[idx] == skip_pc) {
			if (filter_tag[idx] < filter_max) 
				filter_tag[idx]++;
		} else {
			filter_target[idx] = skip_pc;
			filter_tag[idx] = filter_thresh;
		}
		
        index = next_top[index];
    }

	// Finally update top of ras
	new_top = next_top[new_top];
	rs->top = new_top;
	return;	
}

bool
ras_t::filter_pc(addr_t pc) {
	if (filter_table_size <= 1) return false;

	uint32 idx = index_pc(pc);
	if (filter_target[idx] == pc) {
		return (filter_tag[idx] >= filter_thresh); 
	}
	
	return false;

}

void
ras_t::update_commit(addr_t return_pc)
{
	if (filter_table_size <= 1) return;
	
	uint32 idx = index_pc(return_pc);
	if (filter_target[idx] == return_pc) {
		// DEBUG_OUT("%24s update  0x%08llx sat:%d\n", " ", return_pc,
			// filter_tag[idx]);
	
		if (filter_tag[idx] > 0) 
			filter_tag[idx]--;
	}	
}

uint32
ras_t::index_pc(addr_t pc)
{
	addr_t temp_pc = pc >> 3;
	return (temp_pc & filter_mask);
}

uint32
ras_t::increment (uint32 a) {
	return ((a + 1) & mask);
}

ras_t::~ras_t () {
	delete [] table;
	delete [] next_top;
}

void
ras_t::debug()
{
	for (uint32 i = 0; i < entries; i++) {
		if (table[i])
			DEBUG_OUT("%02d 0x%08x %02d\n", i, table[i], next_top[i]);
	}
}

addr_t ras_t::peek_top(ras_state_t *rs)
{
    return table[rs->top];
}
