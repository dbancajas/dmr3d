/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: cascade.cc,v 1.2.10.1 2005/03/22 13:11:33 kchak Exp $");

#include "definitions.h"
#include "cascade.h"

cascade_t::cascade_t (uint32 filter, uint32 except, uint32 shift) {
	except_shift = shift;
	
	filter_table_bits = filter;
	filter_table_size = 1 << filter;
	filter_table_mask = filter_table_size - 1;

	filter_table = new addr_t[filter_table_size];
    for (uint32 i = 0; i < filter_table_size; i++)
        filter_table[i] = 0;
	for (uint32 i = 0; i < filter_table_size; i++)
		filter_table [i] = 0;

	except_table_bits = except;
	except_table_size = 1 << except;
	except_table_mask = except_table_size - 1;
	except_table = new addr_t[except_table_size];
	except_tags  = new tag_t[except_table_size];
    for (uint32 i = 0; i < except_table_size; i++)
    {
        except_table[i] = 0;
        except_tags[i] = 0;
    }
	for (uint32 i = 0; i < except_table_size; i++) {
		except_table [i] = 0;
		except_tags [i] = 0;
	}
}

cascade_t::~cascade_t () {
	delete [] filter_table;
	delete [] except_table;
	delete [] except_tags;
}

addr_t
cascade_t::predict (addr_t pc, indirect_state_t h) {

	addr_t ret_addr;

	uint32 shifted_pc = munge_pc (pc);	
	uint32 filter_index = generate_index (shifted_pc);
	ASSERT (filter_index < filter_table_size);

	uint32 except_index = generate_except_index (pc, h);
	ASSERT (except_index < except_table_size);
	uint32 except_tag = generate_except_tag (pc);

	if (except_tags [except_index] == except_tag) {
		ret_addr = except_table [except_index];
	} else {
		ret_addr = filter_table [filter_index];
	}

	return ret_addr;
}

void
cascade_t::update_state (indirect_state_t &h, addr_t addr) {
	addr  = addr >> 2;
	h = (h << 8) | ((addr ^ (addr >> 8)) & 0xff);
}

void 
cascade_t::fix_state (indirect_state_t &h, addr_t addr) {
	h = h >> 8;
	update_state (h, addr);
}

void
cascade_t::update (addr_t pc, indirect_state_t h, addr_t target_pc) {
	uint32 shifted_pc = munge_pc (pc);
	uint32 filter_index = generate_index (shifted_pc);
	ASSERT (filter_index < filter_table_size);

	uint32 except_index = generate_except_index (pc, h);
	ASSERT (except_index < except_table_size);
	uint32 except_tag = generate_except_tag (pc);

	if (except_tags [except_index] == except_tag) {
		except_table [except_index] = target_pc;
	} else if (filter_table [filter_index] != target_pc) { 
		// misprediction or miss
		filter_table [filter_index] = target_pc;
		except_tags  [except_index] = except_tag;
		except_table [except_index] = target_pc;
	}
}

uint32
cascade_t::munge_pc (addr_t pc) {
	return (uint32) ((pc >> 2) & 0xffffffff);
}

uint32
cascade_t::generate_index (uint32 munged_pc) {
	uint32 index;
	index = ((munged_pc ^ (munged_pc >> filter_table_bits) ^
	         (munged_pc >> (2 * except_table_bits))) & filter_table_mask);
	return index;
}

uint32
cascade_t::generate_except_index (uint32 shifted_pc, indirect_state_t h) {
	uint32 munged_pc = shifted_pc ^ (shifted_pc >> except_table_bits) ^ 
		(shifted_pc >> (2 * except_table_bits));
	uint32 history = (h & 0xff) ^ (((h >> 8) & 0xff) << except_shift) ^ 
		(((h >> 16) & 0xff) << (2 * except_shift));

	return ((history ^ munged_pc) & except_table_mask);
}

uint32
cascade_t::generate_except_tag (addr_t pc) {
	return (munge_pc (pc));
}


void
cascade_t::read_checkpoint(FILE *file)
{
    for (uint32 i = 0; i < filter_table_size; i++)
       fscanf(file, "%llu\n", &filter_table[i]);
    for (uint32 i = 0; i < except_table_size; i++)
        fscanf(file, "%llu %llu\n", &except_table[i], &except_tags[i]);
    
}

void
cascade_t::write_checkpoint(FILE *file)
{
    for (uint32 i = 0; i < filter_table_size; i++)
        fprintf(file, "%llu\n", filter_table[i]);
    for (uint32 i = 0; i < except_table_size; i++)
        fprintf(file, "%llu %llu\n", except_table[i], except_tags[i]);
        
}
