/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: split_unip_bus.cc,v 1.1.1.1.10.1 2005/03/10 17:46:32 pwells Exp $
 *
 * description:    implements the bus
 * initial author: Koushik Chakraborty
 *
 */
 
#include "definitions.h"
#include "protocols.h"
#include "device.h"
#include "link.h"
#include "message.h"
#include "event.h"
#include "debugio.h"
#include "verbose_level.h"

#include "config.h"

template <class prot_sm_t, class msg_t>
split_unip_bus_t<prot_sm_t, msg_t>::split_unip_bus_t(string name, uint32 num_up, uint32 num_down)
	: device_t(name, num_up + num_down), num_uplinks(num_up),
	  num_downlinks(num_down)
{ 
	latency = 1; // TODO: bus latency config
	out_trans.clear();
	num_messages = 0;
	pend_print = true;
	last_data_cycle = 0;
	last_addr_cycle = 0;
}

template <class prot_sm_t, class msg_t>
stall_status_t
split_unip_bus_t<prot_sm_t, msg_t>::message_arrival(message_t *message)
{
	tick_t curr_cycle = external::get_current_cycle();
	tick_t msg_lat = latency;
	
	msg_t *msg = static_cast <msg_t *> (message);
	stall_status_t ret;
	VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: message arrival %s()\n",
	            get_cname(), msg->sender_id, external::get_current_cycle(), 
				msg->address, msg_t::names[msg->type][0].c_str());
	
		// ONLY ONE REQUEST WILL BE PENDING FOR ALL ADDRESSES
	
	msg_class_t mclass = msg->get_msg_class();
	set<addr_t>::iterator ot_it= out_trans.find(msg->address);
	set<tick_t>::iterator acycle_it = busy_addr_cycle.find(curr_cycle + msg_lat);		   
			   
	switch(mclass) {
		case RequestPendResp:
		   // Disallow the request if there is a pending to be transmitted
		   if (last_addr_cycle == curr_cycle + msg_lat) 
			   ret=StallPoll;
		   else {
			   // check if an outstanding request is pending
			   if (ot_it != out_trans.end() || acycle_it != busy_addr_cycle.end()) {
				   // Pending outstanding transaction on the same line
				   VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: BUS Pending Message()\n",
				   get_cname(), msg->sender_id, external::get_current_cycle(), msg->address);
				   ret  = StallPoll;
			   } else {
				    VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: Address Grant ()\n",
					get_cname(), msg->sender_id, external::get_current_cycle(), msg->address);
				  
				   // Can send this is in the next address cycle
				   last_addr_cycle = curr_cycle+msg_lat;
				   ret=StallNone;
				   out_trans.insert(msg->address);
			   }
		   }
		   break;
	   case Response:
	       VERBOSE_OUT(verb_t::bus, "Response message request\n");
		   // Can't Stall the response ... need to find the next free data_cycle
		   while (last_data_cycle >= curr_cycle + msg_lat) 
			   msg_lat += latency;
		   last_data_cycle = curr_cycle+ msg_lat;
		   VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: Response with delay %u ()\n",
		          get_cname(), msg->sender_id, external::get_current_cycle(), msg->address, msg_lat);
		   ret = StallNone;
		   break;
	   case RequestNoPend:
		   if (ot_it != out_trans.end()) {
			   // Pending outstanding transaction on the same line
			   VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: Pending WB Message()\n",
			   get_cname(), msg->sender_id, external::get_current_cycle(), msg->address);
			   ret  = StallPoll;
		   } else {
			   
			   // need to handle writeback and upgrade differently
			   if (msg->type == msg_t::WriteBack) {   
				   out_trans.insert(msg->address);
				   while (last_data_cycle >= curr_cycle + msg_lat) 
					   msg_lat += latency;
				   last_data_cycle = curr_cycle+ msg_lat;
				   if (last_addr_cycle == last_data_cycle) {
					   // This is an unlikely case but still possible
					   msg_lat += latency;
					   last_data_cycle += latency;
				   }
				   // Mark the Address cycle as busy
				   busy_addr_cycle.insert(last_data_cycle);
				   VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: WriteBack with delay %u last data cycle = %u()\n",
				   get_cname(), msg->sender_id, external::get_current_cycle(), msg->address, msg_lat, last_data_cycle);
				   ret = StallNone;
			  
			   } else {
				   ASSERT(0);
			   }
		   }
		   
		   break;
		default:
		     ASSERT(0);
	}
		   
	
	if (ret == StallNone) {
		_event_t *e = new _event_t(msg, this, msg_lat, event_handler);
		e->enqueue();
	}
	
	return ret;
}

template <class prot_sm_t, class msg_t>
void
split_unip_bus_t<prot_sm_t, msg_t>::event_handler(_event_t *e)
{
	msg_t *msg = e->get_data();
	split_unip_bus_t *bus = e->get_context();
	delete e;
	tick_t curr_cycle = external::get_current_cycle();
	
	
	stall_status_t ret;
	
	VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: BUS broad cast message %s()\n",
	            bus->get_cname(), msg->sender_id, external::get_current_cycle(), 
				msg->address, msg_t::names[msg->type][0].c_str());
	
	msg_class_t mclass = msg->get_msg_class();
	
	if (mclass != Response) {
		VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: Sending Down %s()\n",
	            bus->get_cname(), msg->sender_id, external::get_current_cycle(), 
				msg->address, msg_t::names[msg->type][0].c_str());
	
		for (uint32 i = 0; i < bus->num_downlinks; i++) {
			ret = bus->links[i + bus->num_uplinks]->send(new msg_t(*msg));
			ASSERT(ret == StallNone);
			
		}
	} else {
		VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: Sending Up %s()\n",
	            bus->get_cname(), msg->sender_id, external::get_current_cycle(), 
				msg->address, msg_t::names[msg->type][0].c_str());
	
		for (uint32 i=0; i < bus->num_uplinks; i++) {
			
			ret = bus->links[i]->send(new msg_t(*msg));
			ASSERT(ret == StallNone);
		}
	}

	if (mclass == Response || mclass == RequestNoPend) {
		bus->pend_print = true;
		bus->out_trans.erase(msg->address);
		bus->num_messages--;
	}
	
	// If WriteBack clear out the address cycle block
	if (msg->type == msg_t::WriteBack) {
		ASSERT(bus->busy_addr_cycle.find(curr_cycle) != bus->busy_addr_cycle.end());
		bus->busy_addr_cycle.erase(curr_cycle);
	}
	
	delete msg;
	
}

template <class prot_sm_t, class msg_t>
void
split_unip_bus_t<prot_sm_t, msg_t>::to_file(FILE *file)
{
	// Output class name
	fprintf(file, "%s\n", typeid(this).name());
}

template <class prot_sm_t, class msg_t>
void
split_unip_bus_t<prot_sm_t, msg_t>::from_file(FILE *file)
{
	// Input and check class name
	//	string name;
	//	s >> name;
	//	ASSERT(name == typeid(this).name());

}
