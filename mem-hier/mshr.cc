/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mshr.cc,v 1.1.1.1.10.1 2005/03/10 17:46:32 pwells Exp $ 
 *
 * description:    cache MSHRs
 * initial author: Philip Wells 
 *
 */
 
#include "definitions.h"
#include "device.h"
#include "cache.h"
#include "mshr.h"
#include "external.h"
#include "debugio.h"
#include "verbose_level.h"

template <class type_t>
mshr_t<type_t>::mshr_t(const uint32 lsize, cache_t *_cache, const addr_t _addr,
               const uint32 _max_requests)
	: cache(_cache), addr(_addr), max_requests(_max_requests)
{
	// Initialize bitvector
	valid = new bool[cache->get_lsize()];
	clear_valid_bits();
}

template <class type_t>
mshr_t<type_t>::~mshr_t()
{
	delete [] valid;
}
	
template <class type_t>
void
mshr_t<type_t>::set_valid_bits(const addr_t _addr, const uint32 size) {

	VERBOSE_OUT(verb_t::mshrs,
			  "@%12llu 0x%016llx: mshr_t<type_t>::set_valid_bits() mshr addr: 0x%016llx,"
			  "size: %u\n", 
			  external::get_current_cycle(), _addr, addr, size);
	
	addr_t offset = cache->get_offset(_addr);
	ASSERT(offset + size <= cache->get_lsize());

	for (uint32 i = offset; i < offset + size; i++) {
		valid[i] = true;
	}
}

template <class type_t>
bool
mshr_t<type_t>::check_valid_bits(const addr_t _addr, const uint32 size) {

	VERBOSE_OUT(verb_t::mshrs,
			  "@%12llu 0x%016llx: mshr_t::check_valid_bits() mshr addr: "
			  "0x%016llx, size: %u\n", 
			  external::get_current_cycle(), addr, _addr, size);

	addr_t offset = cache->get_offset(_addr);
	ASSERT(offset + size <= cache->get_lsize());

	for (uint32 i = offset; i < offset + size; i++) {
		if (valid[i] == false) return false;
	}
	// All were valid
	VERBOSE_OUT(verb_t::mshrs, "valid\n");
	return true;
}

template <class type_t>
void
mshr_t<type_t>::clear_valid_bits()
{
	for (uint32 i = 0; i < cache->get_lsize(); i++) {
		valid[i] = false;
	}
}

template <class type_t>
bool
mshr_t<type_t>::enqueue_request(type_t *trans) 
{
	VERBOSE_OUT(verb_t::mshrs,
			  "@%12llu 0x%016llx: mshr_t::enqueue_request()\n", 
			  external::get_current_cycle(), addr);
		   
	if (requests.size() < max_requests) {
		requests.push_back(trans);
		return true;
	} else return false;
}

template <class type_t>
bool
mshr_t<type_t>::can_enqueue_request() 
{
	return (requests.size() < max_requests);
}

template <class type_t>
type_t *
mshr_t<type_t>::pop_request()
{
	if (requests.size() == 0) {
		VERBOSE_OUT(verb_t::mshrs,
				  "@%12llu 0x%016x: mshr_t::pop_request(): none to pop!\n", 
				  external::get_current_cycle(), addr);
		return NULL;
	}

	type_t *ret = requests.front();
	requests.pop_front();

	VERBOSE_OUT(verb_t::mshrs,
			  "@%12llu 0x%016llx: mshr_t::pop_request()\n", 
			  external::get_current_cycle(), addr);

	return ret;
}

template <class type_t>
type_t *
mshr_t<type_t>::peek_request()
{
	if (requests.size() > 0) return NULL;
	else return (requests.front());
}


//////////////////////////////////////////////////////////////////////////////
// MSHRs functions

template <class type_t>
mshrs_t<type_t>::mshrs_t(cache_t *_cache, const uint32 _max_mshrs,
                 const uint32 _max_request)
	: cache(_cache), max_mshrs(_max_mshrs), max_requests_per_mshr(_max_request),
	  num_mshrs(0)
{ }

template <class type_t>
mshr_t<type_t> *
mshrs_t<type_t>::lookup(const addr_t addr)
{
	return (mshrs[addr]);
}

template <class type_t>
mshr_t<type_t> *
mshrs_t<type_t>::allocate_mshr(const addr_t addr)
{
	// Shouldn't already have one for this address
	ASSERT(!lookup(addr));
	
	if (num_mshrs == max_mshrs) return NULL;
	
	mshr_t<type_t> *mshr = new mshr_t<type_t>(cache->get_lsize(), cache, addr,
	                          max_requests_per_mshr);
	mshrs[addr] = mshr; 

	num_mshrs++;
	ASSERT(num_mshrs <= max_mshrs);
	
	return mshr;
}

template <class type_t>
void
mshrs_t<type_t>::deallocate_mshr(const addr_t addr)
{
	// Should already have one for this address
	mshr_t<type_t> *mshr = lookup(addr);
	ASSERT(mshr);

	mshrs.erase(addr);
	delete mshr;
	
	num_mshrs--;
	ASSERT(num_mshrs >= 0);
}

template <class type_t>
uint32
mshrs_t<type_t>::get_num_allocated()
{
	return num_mshrs;
}

template <class type_t>
bool
mshrs_t<type_t>::can_allocate_mshr()
{
	return (num_mshrs < max_mshrs);
}
