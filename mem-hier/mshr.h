/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mshr.h,v 1.1.1.1.10.1 2005/03/10 17:46:32 pwells Exp $
 *
 * description:    cache MSHRs
 * initial author: Philip Wells 
 *
 */
 
#ifndef _MSHR_H_
#define _MSHR_H_

#include <vector>
#include <list>
#include <map>

template <class type_t>
class mshr_t {

 public:

	mshr_t(const uint32 lsize, cache_t *_cache, const addr_t _addr,
	       const uint32 _max_request);
	~mshr_t();
	
	void set_addr(const addr_t _addr) { addr = _addr; }
	void set_valid_bits(const addr_t _addr, const uint32 size);
	bool check_valid_bits(const addr_t _addr, const uint32 size);
	void clear_valid_bits();

	addr_t get_addr() { return addr; }

	bool can_enqueue_request();  // Check if space avail to enqueue
	bool enqueue_request(type_t *trans);
	type_t *pop_request();
	type_t *peek_request();
	
 private:

	cache_t *cache;
	addr_t addr;
	bool *valid;
	list<type_t *> requests;
	uint32 max_requests;
};

template <class type_t>
class mshrs_t {

 public:
	mshrs_t(cache_t *_cache, const uint32 _max_mshrs,
	        const uint32 _max_request);
	
	mshr_t<type_t> *lookup(const addr_t addr);
	mshr_t<type_t> *allocate_mshr(const addr_t addr);
	void deallocate_mshr(const addr_t addr);
	
	bool can_allocate_mshr();  // Check if an MSHR available
	uint32 get_num_allocated();

 private:
	
	cache_t *cache;
	map<addr_t, mshr_t<type_t> *> mshrs;
	uint32 max_mshrs;
	uint32 max_requests_per_mshr;
	uint32 num_mshrs;
};

#include "mshr.cc"

#endif // _MSHR_H_
