/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: bus.h,v 1.1.1.1.10.1 2005/03/10 17:46:31 pwells Exp $
 *
 * description:    implements the bus
 * initial author: Koushik Chakraborty
 *
 */
 
#ifndef _BUS_H_
#define _BUS_H_



// main memory
template <class prot_t, class msg_t>
class bus_t : public device_t {
	
 public:
	typedef event_t<msg_t, bus_t> _event_t;

	bus_t(string name, uint32 num_uplinks, uint32 num_downlinks);	
	stall_status_t message_arrival(message_t *message);

	void from_file(FILE *file);
	void to_file(FILE *file);
	
	static void event_handler(_event_t *e);
	
	bool is_quiet();
	bool pend_print; // debug helper bool ... will be removed later
	
 protected:
	tick_t latency;
	set<addr_t> out_trans;
	uint32 num_uplinks;
	uint32 num_downlinks;
	uint32 num_messages;
};

#include "bus.cc"

#endif // _MAINMEM_H_
