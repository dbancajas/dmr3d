/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    functionality for generic mem-hier device
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: device.cc,v 1.1.1.1.10.5 2006/03/02 23:58:42 pwells Exp $");

#include "definitions.h"
#include "stats.h"
#include "profiles.h"
#include "device.h"
#include "link.h"

device_t::device_t (string _name, uint32 _numlinks)
	: name(_name), numlinks(_numlinks)
{
	if (_numlinks) links = new link_t *[numlinks];
	for (uint32 i = 0; i < numlinks; i++) links[i] = NULL;
	
	stats = new stats_t(name);
    profiles = new profiles_t(name);
}

device_t::~device_t()
{
	delete [] links;
}

void
device_t::set_link(uint32 idx, link_t *link)
{
	ASSERT(idx >= 0 && idx < numlinks);
	links[idx] = link;
}

link_t *
device_t::get_link(uint32 idx)
{
	ASSERT(idx >= 0 && idx < numlinks);
	return links[idx];
}

string
device_t::get_name()
{
	return name;
}

const char *
device_t::get_cname()
{
	return name.c_str();
}

void
device_t::print_stats()
{ 
	stats->print();
    profiles->print();
}

void
device_t::clear_stats()
{
	stats->clear();
}

void
device_t::create_direct_link(uint32 linkid, device_t *dest)
{
    ASSERT(linkid < numlinks);
    links[linkid] = new link_t(this, dest);
}
    
void
device_t::create_fifo_link(uint32 linkid, device_t *dest, uint64 latency,
	uint64 size)
{
    ASSERT(linkid < numlinks);
    links[linkid] = new fifo_link_t(this, dest, latency, size);
}
    
void device_t::set_interconnect_id(uint32 id)
{
    interconnect_id = id;
}

void 
device_t::stats_write_checkpoint(FILE *fl)
{
    stats->to_file(fl);
}

void
device_t::stats_read_checkpoint(FILE *fl)
{
    stats->from_file(fl);
}



void 
device_t::profiles_write_checkpoint(FILE *fl)
{
    profiles->to_file(fl);
}

void
device_t::profiles_read_checkpoint(FILE *fl)
{
    profiles->from_file(fl);
}

void 
device_t::cycle_end()
{
    profiles->cycle_end();
}
/*void
device_t::advance_cycle()
{
   
}*/
