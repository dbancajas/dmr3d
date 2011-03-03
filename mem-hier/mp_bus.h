/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mp_bus.h,v 1.1.4.2 2005/03/11 16:31:11 kchak Exp $
 *
 * description:    implements the bus
 * initial author: Koushik Chakraborty
 *
 */
 
#ifndef _MP_BUS_H_
#define _MP_BUS_H_

class delete_trans_t {
    public:
    addr_t msg_addr;
    tick_t delete_cycle;
    delete_trans_t(addr_t, tick_t);
};
    
class resource_desc_t {
    public:
    int addr_bus;
    int data_bus;
    resource_desc_t();
};

class bus_wire_t {
    public:
    set<addr_t> addr_set;
    tick_t free_cycle;
    bus_wire_t();
    bool pending_address(addr_t);
};

template <class msg_t>
class mp_bus_payload_t {
    msg_t *protocol_msg;
    resource_desc_t *resource_id;
    
    public:
    mp_bus_payload_t(msg_t *msg,resource_desc_t *res);
    msg_t *get_msg();
    resource_desc_t *get_resource();
};

// main memory
template <class prot_t, class msg_t>
class mp_bus_t : public device_t {
	
 public:
	typedef event_t<mp_bus_payload_t<msg_t>, mp_bus_t> _event_t;

	mp_bus_t(string name, uint32 num_uplinks, uint32 num_downlinks);	
	stall_status_t message_arrival(message_t *message);

	void from_file(FILE *file);
	void to_file(FILE *file);
	
	static void event_handler(_event_t *e);
	
	// TEMP
	bool is_quiet() { return true; }
	bool pend_print; // debug helper bool ... will be removed later
	
 protected:
	tick_t incremental_latency;
	uint32 num_uplinks;
	uint32 num_downlinks;
    bus_wire_t *addr_bus;
    bus_wire_t *data_bus;
    list<delete_trans_t *> delete_list;
    tick_t last_delete_cycle;
    tick_t delete_shadow;
    
    stall_status_t exist_pending_address(addr_t addr);
    resource_desc_t *reserve_resource(addr_t addr, bool addr, bool data, tick_t &latency);
    void calculate_bus_latency(tick_t &latency, resource_desc_t *res);
    void process_delete_list(tick_t);
};

#include "mp_bus.cc"

#endif // _MP_BUS_H_
