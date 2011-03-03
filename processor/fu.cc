/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: fu.cc,v 1.1.1.1 2004/09/30 18:32:27 pwells Exp $");

#include "definitions.h"
#include "fu.h"

// overloaded prefix ++ for fu_type_t enum
fu_type_t 
operator++ (fu_type_t &unit) {
	int temp = unit;
	++temp;
	return unit = (fu_type_t) temp;
}

fu_t::fu_t (fu_type_t _id, uint32 _latency, uint32 _count) {
	id = _id;
	latency = _latency;
	count = _count;
	pipe_latency = 1;
	
	avail_count = count;
}

fu_t::~fu_t () {
}

fu_type_t
fu_t::get_id () {
	return id;
}

uint32
fu_t::get_latency () {
	return latency;
}

uint32 
fu_t::get_count () {
	return count;
}

uint32 
fu_t::get_pipe_latency () {
	return pipe_latency;
}

uint32 
fu_t::get_avail_count () {
	return avail_count;
}

bool 
fu_t::get_fu () {
	ASSERT (avail_count >= 0);
	if (avail_count > 0) {
		avail_count--;
		return true;
	} else 
		return false;
}

void 
fu_t::return_fu () {
	avail_count++;
	ASSERT (avail_count <= count);
}

