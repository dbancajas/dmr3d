/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    generic cache line
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: line.cc,v 1.3.4.2 2005/11/17 23:40:04 pwells Exp $");

#include "definitions.h"
#include "config_extern.h"
#include "line.h"
#include "device.h"
#include "cache.h"
#include "external.h"
#include <iomanip>

void
line_t::init(cache_t *_cache, addr_t _idx, uint32 _lsize)
{
	cache = _cache;
	index = _idx;
	lsize = _lsize;

	data = (g_conf_cache_data) ? new uint8[lsize] : NULL;
	
	last_use = 0;
	tag = 0;  // Init tag to zero
	prefetched = 0;
}

line_t::~line_t()
{
	if (g_conf_cache_data) delete [] data;
}

void
line_t::reinit(addr_t _tag)
{
	tag = _tag;
	prefetched = 0;
}

void 
line_t::wb_init(addr_t _tag, addr_t _index, line_t *cache_line)
{
	tag = _tag;
	index = _index;
	prefetched = 0;
}

addr_t
line_t::get_index()
{
	return index;
}

addr_t
line_t::get_tag()
{
	return tag;
}

addr_t
line_t::get_address()
{
	return cache->get_address(tag, index, 0);
}

cache_t *
line_t::get_cache()
{
	return cache;
}

tick_t
line_t::get_last_use()
{
	return last_use;
}

void
line_t::mark_use()
{
	last_use = external::get_current_cycle();
}

const uint8 *
line_t::get_data()
{
	return data;
}

void
line_t::store_data(const uint8 *_data, const addr_t offset, const uint32 size)
{
	ASSERT(_data);
	ASSERT(offset + size <= lsize);
	ASSERT(data);

	memcpy(&data[offset], _data, size);
}

void
line_t::to_file(FILE *file)
{
	fprintf(file, "%llx %llx %llu ", tag, index, last_use);
	if (g_conf_cache_data) {
		for (uint32 i = 0; i < lsize; i++) {
			fprintf(file, "%02x", (int) data[i]);
		}
	}
	fprintf(file, "\n");
}

void
line_t::from_file(FILE *file)
{
	fscanf(file, "%llx %llx %llu ", &tag, &index, &last_use);
	if (g_conf_cache_data) {
		for (uint32 i = 0; i < lsize; i++) {
			uint32 temp;
			int ret = fscanf(file, "%02x", &temp);
			ASSERT(ret != EOF);
			data[i] = temp;
		}
	}
	fscanf(file, "\n");

}


void line_t::set_index(addr_t _idx)
{
    index = _idx;   
}

void line_t::set_tag(addr_t _tag)
{
    tag = _tag;
}

void line_t::set_last_use(tick_t _lu)
{
    last_use = _lu;
}
