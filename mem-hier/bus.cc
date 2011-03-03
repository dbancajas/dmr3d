/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: bus.cc,v 1.1.1.1.10.1 2005/03/10 17:46:31 pwells Exp $
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
bus_t<prot_sm_t, msg_t>::bus_t(string name, uint32 num_up, uint32 num_down)
	: device_t(name, num_up + num_down), num_uplinks(num_up),
	  num_downlinks(num_down)
{ 
	latency = 1; // TODO: bus latency config
	out_trans.clear();
	num_messages = 0;
	pend_print = true;
}

template <class prot_sm_t, class msg_t>
stall_status_t
bus_t<prot_sm_t, msg_t>::message_arrival(message_t *message)
{
	msg_t *msg = static_cast <msg_t *> (message);
	
		// ONLY ONE REQUEST WILL BE PENDING FOR ALL ADDRESSES
	
	msg_class_t mclass = msg->get_msg_class();
	ASSERT(mclass != InvMsgClass);
	if (mclass != Response) {
		if (num_messages > 0) {
			if (pend_print) 
				VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: BUS Pending Message()\n",
			                get_cname(), msg->trans->cpu_id, external::get_current_cycle(), msg->address);
			pend_print = false;				
			return StallPoll;
		}
		
		// No request pending should include this message
		out_trans.insert(msg->address);
		num_messages++;
	} else {
		// Response message the address must match the outstanding request we have
		set<addr_t>::iterator ot_it = out_trans.find(msg->address);
		ASSERT(ot_it != out_trans.end() && num_messages == 1);

	}

	_event_t *e = new _event_t(msg, this, latency, event_handler);
	e->enqueue();
	
	return StallNone;
}

template <class prot_sm_t, class msg_t>
void
bus_t<prot_sm_t, msg_t>::event_handler(_event_t *e)
{
	msg_t *msg = e->get_data();
	bus_t *bus = e->get_context();
	delete e;
	
	stall_status_t ret;
	
	VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: BUS broad cast message()\n",
	            bus->get_cname(), msg->trans->cpu_id, external::get_current_cycle(), msg->address);
	
	for (uint32 i = 0; i < bus->num_downlinks; i++) {
		ret = bus->links[i + bus->num_uplinks]->send(new msg_t(*msg));
	}

	for (uint32 i=0; i < bus->num_uplinks; i++) {
		ret = bus->links[i]->send(new msg_t(*msg));
		ASSERT(ret == StallNone);
	}

	msg_class_t mclass = msg->get_msg_class();
	if (mclass == Response || mclass == RequestNoPend) {
		bus->pend_print = true;
		bus->out_trans.erase(msg->address);
		bus->num_messages--;
		ASSERT(bus->out_trans.size() == 0);
	}
	
	delete msg;
	
}

template <class prot_sm_t, class msg_t>
void
bus_t<prot_sm_t, msg_t>::is_quiet()
{
	return (num_messages == 0);
}

template <class prot_sm_t, class msg_t>
void
bus_t<prot_sm_t, msg_t>::to_file(FILE *file)
{
	// Output class name
	fprintf(file, "%s\n", typeid(this).name());
}

template <class prot_sm_t, class msg_t>
void
bus_t<prot_sm_t, msg_t>::fromfile(FILE *file)
{
	// Input and check class name
	//	string name;
	//	s >> name;
	//	ASSERT(name == typeid(this).name());

}
