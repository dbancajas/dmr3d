/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: verbose_level.h,v 1.2.2.3 2006/09/28 15:18:31 pwells Exp $
 *
 * description:    implementation of a one-level uniprocessor cache
 * initial author: Koushik Chakraborty
 *
 */
 
#ifndef _VERBOSE_LEVEL_H
#define _VERBOSE_LEVEL_H

class verb_t {

public:
// Debug level constants
	static const uint32 config = (1<<0);
	static const uint32 stats = (1<<1);
	static const uint32 power = (1<<2);
	static const uint32 requests = (1<<3);
	static const uint32 mainmem = (1<<4);
	static const uint32 transfn = (1<<5);
	static const uint32 links = (1<<6);
	static const uint32 mshrs = (1<<7);
	static const uint32 events = (1<<8);
	static const uint32 bus = (1<<9);
	static const uint32 transitions = (1<<10);
	static const uint32 io = (1<<11);
	static const uint32 data = (1<<12);
	static const uint32 mmu = (1<<14);
	static const uint32 banks = (1<<15);
	static const uint32 spinlock = (1<<16);
    static const uint32 checkpoints = (1 << 17);
	static const uint32 l1miss_trace = (1<<17);
	static const uint32 access_trace = (1<<18);
};


// Maybe shouldn't go here...
#define VERBOSE_DATA(_name, _cycles, _address, _size, _data) \
if (g_conf_cache_data) { \
	VERBOSE_OUT(verb_t::data, "%10s @ %12llu 0x%016llx: ", \
				_name, _cycles, _address); \
	for (size_t _i = 0; _i < _size; _i++) { \
		if (_i % 16 == 0) { \
			if (_i != 0) VERBOSE_OUT(verb_t::data, "\n%46s", ""); \
			VERBOSE_OUT(verb_t::data, "%02x ", _i); \
		} else if (_i % 8 == 0) \
			VERBOSE_OUT(verb_t::data, " "); \
		VERBOSE_OUT(verb_t::data, "%02x", _data[_i]); \
	} \
	VERBOSE_OUT(verb_t::data, "\n"); \
}
	
#endif	
