/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: external.h,v 1.2 2004/10/01 18:06:00 pwells Exp $
 *
 * description:    handles simics specific calls
 * initial author: Philip Wells 
 *
 */
 
#ifndef _EXTERNAL_H_
#define _EXTERNAL_H_


// For making calls to simics

class external
{
 public:

	// Post an event to simics event queue
	static void post_event(mem_hier_t *mh);
	static void event_handler(conf_object_t *cpu, mem_hier_t *mh);

	
	static tick_t get_current_cycle();
	
	static void read_from_memory(const addr_t start, const size_t size,
	                             uint8 *buffer);
	static void write_to_memory(const addr_t start, const size_t size,
	                            uint8 *buffer);

	static uint32 get_num_events();
	static void quit(int ret);
	static void write_configuration(string filename);

 private:
	static uint32 num_events;
};


#endif // _EXTERNAL_H_
