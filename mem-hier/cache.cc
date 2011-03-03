/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    base abstract cache class
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: cache.cc,v 1.1.1.1.10.3 2006/07/28 01:29:54 pwells Exp $");

#include "definitions.h"
#include "config.h"
#include "device.h"
#include "line.h"
#include "link.h"
#include "cache.h"
#include "transaction.h"
#include "mshr.h"
#include "external.h"
#include "debugio.h"
#include "verbose_level.h"

///////////////////////////
// Cache Utility Functions
//

cache_t::cache_t(string name, uint32 _num_links, cache_config_t *conf)
	: device_t(name, _num_links), non_power_sets(false), non_power_banks(false)
{
	// Calculate cache parameters from config
	lsize = conf->lsize;
	assoc = conf->assoc;
	numlines = conf->size * 1024 / lsize; // size is in Kbytes
    num_sets = numlines / assoc;
	numwb_buffers = conf->writeback_buffersize;
	num_banks = conf->banks;
	bank_bw = conf->bank_bw;
	request_q_size = conf->req_q_size;
	wait_q_size = conf->wait_q_size;
	bank_low_bit = conf->bank_stripe_bits;
	
	if (assoc == 0) assoc = numlines;

	numlines_bits = (uint32) log2(numlines);
	lsize_bits = (uint32) log2(lsize);
	assoc_bits = (uint32) log2(assoc);
	bank_bits = (uint32) log2(num_banks);

    // Handle non power of two caches
	if (numlines != (uint32) (1 << numlines_bits))
        non_power_sets = true;
    if (num_banks != (uint32) (1 << bank_bits))
        non_power_banks = true;
    
    ASSERT(numlines == num_sets  * assoc);
	ASSERT(lsize == (uint32) (1 << lsize_bits));
	ASSERT(assoc == (uint32) (1 << assoc_bits));
	//ASSERT(num_banks == (uint32) (1 << bank_bits));
	
	// Below allows banking selection on arbitrary bits (> offset), but lines 
	// stored in one array so banks are used as low n bits of index
	offset_mask = get_mask(0, lsize_bits);
	uint32 index_bits;
	
	if (bank_low_bit == 0 || bank_low_bit == lsize_bits) {
		// default case
		bank_low_bit = lsize_bits;

		index_bits = numlines_bits - assoc_bits;
	}
	else {
		// Set index/tag mask/shift to be those of the munged address with
		// the bank-selection bits munged_addr
		ASSERT(bank_low_bit >= lsize_bits);
		index_bits = numlines_bits - assoc_bits - bank_bits;
	}
	// For non power of two maintain the entire index bits in the tag
	tag_shift = non_power_sets ? lsize_bits : index_bits + lsize_bits;
	tag_mask = get_mask(tag_shift, -1);
	index_shift = lsize_bits;
	index_mask = get_mask(lsize_bits, tag_shift);
	bank_mask = get_mask(bank_low_bit, bank_low_bit+bank_bits);


	VERBOSE_OUT(verb_t::config,
		"%10s config: %u %u %u %u, %u %u %u %u, 0x%llx 0x%llx 0x%llx 0x%llx %d %d\n",
		get_cname(), numlines, lsize, assoc, num_banks,
		numlines_bits, lsize_bits, assoc_bits, bank_bits,
		index_mask, tag_mask, offset_mask, bank_mask, request_q_size, 
		wait_q_size);
}

cache_t::~cache_t()
{ 
}

addr_t
cache_t::get_size() {
    return numlines * lsize;
}

// Return mask from bit min (inclusinve) to bit max (exclusive)
addr_t
cache_t::get_mask(const int min, const int max) 
{
	if (max == -1)
		return (~0ULL << min);
	else 
		return ((~0ULL << min) & ~(~0ULL << max));
}

addr_t
cache_t::get_tag(const addr_t addr)
{
	if (bank_low_bit == lsize_bits)
		return (addr & tag_mask) >> tag_shift;
		
	addr_t munged_addr = remove_bits(addr, bank_low_bit, bank_bits);
	return ((munged_addr & tag_mask) >> tag_shift);
}

addr_t
cache_t::get_index(const addr_t addr) 
{
    if (non_power_sets) 
        return get_non_power_index(addr);
	if (bank_low_bit == lsize_bits)
		return (addr & index_mask) >> index_shift;

	addr_t munged_addr = remove_bits(addr, bank_low_bit, bank_bits);
	return (((munged_addr & index_mask) >> index_shift) << bank_bits) | get_bank(addr);
}

addr_t
cache_t::get_non_power_index(const addr_t addr)
{
    return (addr >> index_shift) % num_sets;
}


addr_t
cache_t::get_offset(const addr_t addr)
{
    return (addr & offset_mask);
}

uint32
cache_t::get_bank(const addr_t addr)
{
    if (non_power_banks)
        return (addr >> index_shift) % num_banks;
    
    return (addr & bank_mask) >> bank_low_bit;
}

addr_t
cache_t::get_address(const addr_t tag, const addr_t index, const addr_t offset)
{
    if (non_power_sets)
        return (tag << tag_shift) | offset;
    
	// Normal calculation
	if (bank_low_bit == lsize_bits)
		return (tag << tag_shift) | (index << index_shift) | offset;
	
	// Allow banking selection on arbitrary bits (> offset)
	// Bank select is in low bits of index argument (to match normal case)
	addr_t bank = index & get_mask(0, bank_bits);
	addr_t real_index = (index >> bank_bits);
	
	addr_t address = (tag << tag_shift) | (real_index << index_shift) | offset;
	address = insert_bits(address, bank, bank_low_bit, bank_bits);
	return address; 
}

addr_t
cache_t::get_line_address(const addr_t addr)
{
	return (addr & (~offset_mask));
}

// Accessor functions
uint32
cache_t::get_lsize()
{
	return (lsize);
}

addr_t cache_t::insert_bits(addr_t address, addr_t middle, 
	uint32 mid_start, uint32 mid_bits)
{
	addr_t top = address >> (mid_start);  // NOT + mid_bits!
	addr_t bottom = address & get_mask(0, mid_start);
	
	return (bottom | (middle << mid_start) | (top << mid_start + mid_bits)); 	
}

addr_t cache_t::remove_bits(addr_t address, uint32 mid_start, uint32 mid_bits)
{
	addr_t top = address >> (mid_start + mid_bits);
	addr_t bottom = address & get_mask(0, mid_start);
	
	return (bottom | (top << mid_start));
}

cache_power_t *cache_t::get_cache_power()
{
    return cache_power;
}
