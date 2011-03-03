/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mainmem.h,v 1.2.10.2 2006/02/01 19:42:09 pwells Exp $
 *
 * description:    implements the main memory
 * initial author: Philip Wells 
 *
 */
 
#ifndef _MAINMEM_H_
#define _MAINMEM_H_

// main memory
template <class prot_t, class msg_t>
class main_mem_t : public device_t {
	
 public:
	typedef event_t<msg_t, main_mem_t> _event_t;

	main_mem_t(string name, uint32 num_links);	
	virtual ~main_mem_t() { }
	
	virtual stall_status_t message_arrival(message_t *message);

	virtual void from_file(FILE *file);
	virtual void to_file(FILE *file);
	virtual bool is_quiet();
	
	static const uint8 cache_link = 0;
	static const uint8 io_link = 1;

 protected:
	tick_t latency;
	uint32 num_requests; // current number of outstanding requests

	void write_memory(addr_t addr, size_t size, uint8 *data);
	void read_memory(addr_t addr, size_t size, uint8 *data);

	static void event_handler(_event_t *e);
	
	tick_t get_latency(message_t *message);

	//stats_t *stats; //profiling stuff
	bool stats_print_enqueued;
	st_entry_t *stat_requests;
};

#include "mainmem.cc"

#endif // _MAINMEM_H_
