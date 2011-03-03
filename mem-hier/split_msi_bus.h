/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: split_msi_bus.h,v 1.1.1.1.10.1 2005/03/10 17:46:32 pwells Exp $
 *
 * description:    implements the bus
 * initial author: Koushik Chakraborty
 *
 */
 
#ifndef _SPLIT_MSI_BUS_H_
#define _SPLIT_MSI_BUS_H_



// main memory
template <class prot_t, class msg_t>
class split_msi_bus_t : public device_t {
	
 public:
	typedef event_t<msg_t, split_msi_bus_t> _event_t;

	split_msi_bus_t(string name, uint32 num_uplinks, uint32 num_downlinks);	
	stall_status_t message_arrival(message_t *message);

	void from_file(FILE *file);
	void to_file(FILE *file);
	
	static void event_handler(_event_t *e);
	
	// TEMP
	bool is_quiet() { return true; }
	bool pend_print; // debug helper bool ... will be removed later
	
 protected:
	tick_t latency;
	set<addr_t> out_trans;
	set<tick_t> busy_addr_cycle;
	uint32 num_uplinks;
	uint32 num_downlinks;
	uint32 num_messages;
	tick_t last_addr_cycle;
	tick_t last_data_cycle;
	addr_t addr_on_data_bus;
	addr_t addr_on_addr_bus;
};

#include "split_msi_bus.cc"

#endif // _SPLIT_MSI_BUS_H_
