/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    handles simics specific calls
 * initial author: Philip Wells 
 *
 */
 
 
//  // #include "simics/first.h"
// // RCSID("$Id: external.cc,v 1.3.10.5 2006/12/12 18:36:57 pwells Exp $");

#include "definitions.h"
#include "external.h"
#include "event.h"
#include "mem_hier.h"
#include "debugio.h"
#include "verbose_level.h"
#include "proc_tm.h"
#include "startup.h"

uint32 external::num_events = 0;

void
external::event_handler(conf_object_t *cpu, mem_hier_t *mh)
{
	VERBOSE_OUT(verb_t::events, "static event_handler()\n");
	mh->advance_cycle();

	post_event(mh);
}

// Post an event to simics event queue to run the next cycle
void
external::post_event(mem_hier_t *mh)
{
	VERBOSE_OUT(verb_t::events, "external::post_event()\n");
	SIM_time_post_cycle(SIM_proc_no_2_ptr(0),
						1, Sim_Sync_Processor,
						(event_handler_t) event_handler,
						mh);
}

uint32
external::get_num_events()
{
	return num_events;
}

tick_t
external::get_current_cycle()
{
	return mem_hier_t::ptr()->get_g_cycles();
}

void
external::read_from_memory(const addr_t start, const size_t size,
	                       uint8 *buffer)
{
	conf_object_t *mem_img = mem_hier_t::ptr()->get_module_obj()->mem_image;
	image_interface *img_iface = mem_hier_t::ptr()->get_module_obj()->mem_image_iface;
	
	ASSERT_MSG(mem_img, "No simics memory image set to read from!");
	ASSERT_MSG(img_iface, "No simice memory image interface set to read from!");
	
	img_iface->read(mem_img, buffer, start, size);
}

void
external::write_to_memory(const addr_t start, const size_t size,
	                      uint8 *buffer)
{
	conf_object_t *mem_img = mem_hier_t::ptr()->get_module_obj()->mem_image;
	image_interface *img_iface = mem_hier_t::ptr()->get_module_obj()->mem_image_iface;
	
	ASSERT_MSG(mem_img, "No simics memory image set to write to!");
	ASSERT_MSG(img_iface, "No simice memory image interface set to write to!");
	
	img_iface->write(mem_img, buffer, start, size);
}

void
external::quit(int ret)
{
	SIM_quit(ret);
}

void
external::write_configuration(string filename)
{
	SIM_write_configuration_to_file(filename.c_str());
}

