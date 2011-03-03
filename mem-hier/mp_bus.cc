/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mp_bus.cc,v 1.1.4.4 2005/05/27 19:12:23 pwells Exp $
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
#include <math.h>

#include "config.h"

resource_desc_t::resource_desc_t()
{
    addr_bus = -1;
    data_bus = -1;
}

delete_trans_t::delete_trans_t(addr_t _addr, tick_t _cycle) :
    msg_addr(_addr), delete_cycle(_cycle)
{
    // DO nothing
}
bus_wire_t::bus_wire_t()
{
    addr_set.clear();
    free_cycle = external::get_current_cycle();
}

bool
bus_wire_t::pending_address(addr_t _addr)
{
    return (addr_set.find(_addr) != addr_set.end());
}

template<class msg_t>
mp_bus_payload_t<msg_t>::mp_bus_payload_t(msg_t *_msg, resource_desc_t *desc) :
    protocol_msg(_msg), resource_id(desc)
{
        
}

template<class msg_t>
msg_t *
mp_bus_payload_t<msg_t>::get_msg()
{
    return protocol_msg;
}

template<class msg_t>
resource_desc_t *
mp_bus_payload_t<msg_t>::get_resource()
{
    return resource_id;
}
    
template <class prot_sm_t, class msg_t>
mp_bus_t<prot_sm_t, msg_t>::mp_bus_t(string name, uint32 num_up, uint32 num_down)
	: device_t(name, num_up + num_down), num_uplinks(num_up),
	  num_downlinks(num_down)
{ 
	incremental_latency = 1; // TODO: bus latency config
	addr_bus = new bus_wire_t[g_conf_num_addr_bus];
    data_bus = new bus_wire_t[g_conf_num_data_bus];
    last_delete_cycle = external::get_current_cycle();
    delete_shadow = 0;
}

template<class prot_sm_t, class msg_t>
void
mp_bus_t<prot_sm_t, msg_t>::process_delete_list(tick_t cycle)
{
    list<delete_trans_t *>::iterator dit = delete_list.begin();
    while (dit != delete_list.end())
    {
        delete_trans_t *dtrans = *dit;
        if (dtrans->delete_cycle < cycle) {
            for (uint32 bus_id = 0; bus_id < g_conf_num_addr_bus; bus_id++)
            {
                if (addr_bus[bus_id].pending_address(dtrans->msg_addr)) {
                    VERBOSE_OUT(verb_t::bus, "%10s: %2u @ %12llu 0x%016llx: Delete pending()\n",
                    get_cname(), bus_id, external::get_current_cycle(), dtrans->msg_addr);
                    addr_bus[bus_id].addr_set.erase(dtrans->msg_addr);
                    break;
                }
            }
            delete dtrans;
            delete_list.erase(dit);
            dit = delete_list.begin();
            continue;
        } 
        break;
    }
}
    

template<class prot_sm_t, class msg_t>
stall_status_t
mp_bus_t<prot_sm_t, msg_t>::exist_pending_address(addr_t msg_addr)
{
    bool check_addr_pending = false;
    for (uint32 i = 0; i < g_conf_num_addr_bus; i++)
    {
        if (addr_bus[i].pending_address(msg_addr)) {
            VERBOSE_OUT(verb_t::bus, "%10s: %2u @ %12llu 0x%016llx: pending transaction()\n",
                    get_cname(), i, external::get_current_cycle(), msg_addr);
            check_addr_pending = true;
            break;
        }
    }
    if (!check_addr_pending) return StallNone;
    // Address is present ... need to check if it is also present in delete trans
    list<delete_trans_t *>::iterator dit = delete_list.begin();
    while (dit != delete_list.end())
    {
        delete_trans_t *dtrans = *dit;
        if (dtrans->msg_addr == msg_addr)
            return StallPoll;
        dit++;
    }
    return StallSetEvent;
}

template<class prot_sm_t, class msg_t>
void
mp_bus_t<prot_sm_t, msg_t>::calculate_bus_latency(tick_t &msg_lat, resource_desc_t *res)
{
    tick_t curr_cycle = external::get_current_cycle();
    if (res->addr_bus != -1 && res->data_bus != -1)
    {
        if (curr_cycle > addr_bus[res->addr_bus].free_cycle)
        {
            addr_bus[res->addr_bus].free_cycle = curr_cycle + msg_lat;
            data_bus[res->data_bus].free_cycle = curr_cycle + msg_lat;
        } else {
            tick_t tmp_lat = addr_bus[res->addr_bus].free_cycle - curr_cycle;
            if (tmp_lat < msg_lat) {
                addr_bus[res->addr_bus].free_cycle += msg_lat + 1;
                data_bus[res->data_bus].free_cycle += msg_lat + 1;
            } else {
                addr_bus[res->addr_bus].free_cycle++;
                data_bus[res->data_bus].free_cycle++;
                msg_lat = tmp_lat;
            }
        }
    } else if (res->addr_bus != -1) {
        if (curr_cycle > addr_bus[res->addr_bus].free_cycle)
        {
            addr_bus[res->addr_bus].free_cycle = curr_cycle + msg_lat;
        } else {
            tick_t tmp_lat = addr_bus[res->addr_bus].free_cycle - curr_cycle;
            if (tmp_lat < msg_lat) {
                addr_bus[res->addr_bus].free_cycle += msg_lat + 1;
            } else {
                addr_bus[res->addr_bus].free_cycle++;
                msg_lat = tmp_lat;
            }
        }
    } else {
        if (curr_cycle > data_bus[res->data_bus].free_cycle)
        {
            data_bus[res->data_bus].free_cycle = curr_cycle + msg_lat;
        } else {
            tick_t tmp_lat = data_bus[res->data_bus].free_cycle - curr_cycle;
            if (tmp_lat < msg_lat) {
                data_bus[res->data_bus].free_cycle += msg_lat + 1;
            } else {
                data_bus[res->data_bus].free_cycle++;
                msg_lat = tmp_lat;
            }
        }
    }
    
}

template<class prot_sm_t, class msg_t>
resource_desc_t *
mp_bus_t<prot_sm_t, msg_t>::reserve_resource(addr_t msg_addr,bool addr_req, bool data_req, tick_t &msg_lat)
{
   
    resource_desc_t *res = new resource_desc_t();
    if (addr_req && data_req) {
        // Need to find one address bus and data bus with same free_cycle availibility
        // Writeback message needs to find earliest cycle when both
        // data and address can be transmitted with least amount
        // of cycle waste
        
        // Algorithm : (a) find earliest free data and addr cycles
        //             
        
        tick_t earliest_data = data_bus[0].free_cycle;
        uint32 dbus_id = 0;
        for (uint32 i = 1; i < g_conf_num_data_bus; i++)
        {
            if (earliest_data > data_bus[i].free_cycle) {
                dbus_id = i;
                earliest_data = data_bus[i].free_cycle;
            }
        }
        
        tick_t earliest_addr = addr_bus[0].free_cycle;
        uint32 abus_id = 0;
        for (uint32 i = 1; i < g_conf_num_addr_bus; i++) {
            if (earliest_addr > addr_bus[i].free_cycle) {
                abus_id = i;
                earliest_addr = addr_bus[i].free_cycle;
            }
        }
        
        // Choose the greater of the two and find the closest match
        // in the other set of buses
        if (earliest_data > earliest_addr) {
            tick_t diff = earliest_data - earliest_addr;
            for (uint32 i = 0; i < g_conf_num_addr_bus; i++)
            {
                if (abs(int(addr_bus[i].free_cycle - data_bus[dbus_id].free_cycle)) < diff)
                {
                    abus_id = i;
                }
            }
            
        } else {
            int diff = earliest_addr - earliest_data;
            for (uint32 i = 0; i < g_conf_num_data_bus; i++)
            {
                if (abs(int(data_bus[i].free_cycle - addr_bus[abus_id].free_cycle)) < diff)
                {
                    dbus_id = i;
                }
            }
        }
        if (addr_bus[abus_id].free_cycle > data_bus[dbus_id].free_cycle)
            data_bus[dbus_id].free_cycle = addr_bus[abus_id].free_cycle;
        else
            addr_bus[abus_id].free_cycle = data_bus[dbus_id].free_cycle;
        res->addr_bus = abus_id;
        res->data_bus = dbus_id;
        // Found the right match!
        calculate_bus_latency(msg_lat, res);
        addr_bus[res->addr_bus].addr_set.insert(msg_addr);
        
    } else if (addr_req) {
        res->addr_bus = 0;
        for (uint32 i = 1; i < g_conf_num_addr_bus;i++)
        {
            if (addr_bus[i].free_cycle < addr_bus[res->addr_bus].free_cycle)
                res->addr_bus = i;
        }
        calculate_bus_latency(msg_lat, res);
        addr_bus[res->addr_bus].addr_set.insert(msg_addr);
        
    } else {
        ASSERT(data_req);
        res->data_bus = 0;
        for (uint32 i = 1; i < g_conf_num_data_bus;i++)
        {
            if (data_bus[i].free_cycle < data_bus[res->data_bus].free_cycle)
                res->data_bus = i;
        }
        calculate_bus_latency(msg_lat, res);
    }
    return res;
}

template <class prot_sm_t, class msg_t>
stall_status_t
mp_bus_t<prot_sm_t, msg_t>::message_arrival(message_t *message)
{
	tick_t curr_cycle = external::get_current_cycle();
	tick_t msg_lat = 1;
	
    // Process the delete list first
    while (last_delete_cycle < curr_cycle) {
        process_delete_list(++last_delete_cycle);
    }
	msg_t *msg = static_cast <msg_t *> (message);
	stall_status_t ret = StallPoll; // Default return is stalling!
		// ONLY ONE REQUEST WILL BE PENDING FOR ALL ADDRESSES
	
	VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: message arrival  %s \n",
	            get_cname(), msg->sender_id, external::get_current_cycle(), 
				msg->address, msg_t::names[msg->type][0].c_str());
	
	msg_class_t mclass = msg->get_msg_class();
    resource_desc_t *resource_id;
			   
	switch(mclass) {
		case RequestPendResp:
		   // Disallow the request if there is a pending to be transmitted
		   // OR if there is an address on data bus in current cycle matches
		   // this address
		   // OR if there is an address on the address bus in previous cycle
		   // that matches this address
           
		   ret = exist_pending_address(msg->address);
           if (ret == StallNone) {
               resource_id = reserve_resource(msg->address, true, false, msg_lat);
               VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: Address Grant with delay %u ()\n",
               get_cname(), msg->sender_id, external::get_current_cycle(), msg->address, msg_lat);
		   }
		   break;
	   case Response:
		   // Can't Stall the response ... need to find the next free data_cycle
           resource_id = reserve_resource(msg->address, false, true, msg_lat);
		   VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: Response with delay %u ()\n",
		               get_cname(), msg->sender_id, external::get_current_cycle(), msg->address, msg_lat);
		   ret = StallNone;
		   break;
	   case RequestNoPend:
		   ret = exist_pending_address(msg->address);
           if (ret != StallNone) {
			   // Pending outstanding transaction on the same line
			   VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: Pending WB/UPG Message()\n",
			   get_cname(), msg->sender_id, external::get_current_cycle(), msg->address);
		   } else {
			   // need to handle writeback and upgrade differently
			   if (msg->type == msg_t::WriteBack) {   
				   resource_id = reserve_resource(msg->address, true, true, msg_lat);
			   } else  {
				   resource_id = reserve_resource(msg->address, true, false, msg_lat);
			   }
               if (resource_id) {
                   VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: UPGD/WB grant with delay %u ()\n",
                   get_cname(), msg->sender_id, external::get_current_cycle(), msg->address, msg_lat);
               } else {
                   ret = StallPoll;
               }
		   }
		   
		   break;
		default:
		     ASSERT(0);
	}
		   
	
	if (ret == StallNone) {
        mp_bus_payload_t<msg_t> *payload = new mp_bus_payload_t<msg_t>(msg, resource_id);
		_event_t *e = new _event_t(payload, this, msg_lat, event_handler);
		e->enqueue();
	}
	
	return ret;
}

template <class prot_sm_t, class msg_t>
void
mp_bus_t<prot_sm_t, msg_t>::event_handler(_event_t *e)
{
    mp_bus_payload_t<msg_t> *payload = e->get_data();
	msg_t *msg = payload->get_msg();
    resource_desc_t *resource = payload->get_resource();
	mp_bus_t *bus = e->get_context();
	delete e;
	
	stall_status_t ret;
	
	VERBOSE_OUT(verb_t::bus, "%10s: %02u @ %12llu 0x%016llx: BUS broad cast message %s()\n",
	            bus->get_cname(), msg->sender_id, external::get_current_cycle(), 
				msg->address, msg_t::names[msg->type][0].c_str());
	
	for (uint32 i = 0; i < bus->num_downlinks; i++) {
		ret = bus->links[i + bus->num_uplinks]->send(new msg_t(*msg));
		ASSERT(ret == StallNone);
	}

	for (uint32 i=0; i < bus->num_uplinks; i++) {
		ret = bus->links[i]->send(new msg_t(*msg));
		ASSERT(ret == StallNone);
	}

	msg_class_t mclass = msg->get_msg_class();
	if (mclass == Response || mclass == RequestNoPend) {
        delete_trans_t *dtrans = new delete_trans_t(msg->address, 
                        external::get_current_cycle() + bus->delete_shadow);
        bus->delete_list.push_back(dtrans);      
	}
	delete msg;
    delete resource;
	delete payload;
    
	
}

template <class prot_sm_t, class msg_t>
void
mp_bus_t<prot_sm_t, msg_t>::to_file(FILE *file)
{
	// Output class name
	fprintf(file, "%s\n", typeid(this).name());
}

template <class prot_sm_t, class msg_t>
void
mp_bus_t<prot_sm_t, msg_t>::from_file(FILE *file)
{
    char classname[g_conf_max_classname_len];
	fscanf(file, "%s\n", classname);

    if (strcmp(classname, typeid(this).name()) != 0)
        cout << "Read " << classname << " expecting " << typeid(this).name() << endl;
	ASSERT(strcmp(classname, typeid(this).name()) == 0);
	
}
