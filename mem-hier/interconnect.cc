/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: interconnect.cc,v 1.1.2.11 2006/03/02 23:58:42 pwells Exp $
 *
 * description:    declaration of a interconnect for cmp_incl
 * initial author: Koushik Chakraborty 
 *
*/

//  #include "simics/first.h"
// RCSID("$Id: interconnect.cc,v 1.1.2.11 2006/03/02 23:58:42 pwells Exp $");

#include "definitions.h"
#include "external.h"
#include "debugio.h"
#include "verbose_level.h"
#include "device.h"
#include "link.h"
#include "interconnect_msg.h"
#include "interconnect.h"
#include "config_extern.h"
#include "stats.h"
#include "counter.h"
#include "mem_hier.h"


interconnect_t::interconnect_t(uint32 _devices) :
    device_t ("interconnect", 0), interconnect_devices(_devices)
{
    interconnect_id = 0;
    devices = new device_t *[interconnect_devices];
    private_devices = 0;
	
	stat_num_data_messages = stats->COUNTER_BASIC("num_data_messages", 
	"Number of data messages");
	stat_num_addr_messages = stats->COUNTER_BASIC("num_addr_messages", 
	"Number of address messages");
    stat_migration_addr_msg = stats->COUNTER_BASIC("migration_addr_msg",
        "Number of address messages due to migration");
    stat_migration_data_msg = stats->COUNTER_BASIC("migration_data_msg",
        "Number of data messages due to migration");
    
    stat_inactive_link_cycles = stats->COUNTER_DOUBLE("inactive_link_cycles",
        "Number of cycles with Inactive links");
    
}

uint32 interconnect_t::get_interconnect_id(device_t *device)
{
    devices[interconnect_id] = device;
    return interconnect_id++;
}

uint32 interconnect_t::get_clustered_id(device_t *device , uint32 num)
{
    uint32 ret = interconnect_id;
    private_devices = interconnect_id - 1;
    for (uint32 i = 0; i < num; i++) {
        devices[interconnect_id++] = device;
    }
    return ret;
}

void interconnect_t::setup_interconnect()
{
    // As many as interconnect there is
    ASSERT(interconnect_devices == interconnect_id);
    uint32 link_count = interconnect_id * interconnect_id;
    internal_links = new link_t  *[link_count];
    link_active = new bool[link_count];
    last_active = new uint32[link_count];
    bzero(link_active, sizeof(bool) * link_count);
    bzero(last_active, sizeof(uint32) * link_count);
    
    
    for (uint32 i = 0; i < interconnect_id * interconnect_id; i++)
    {
        uint32 j = i % interconnect_id;
        uint32 k = i / interconnect_id;
		if (g_conf_cmp_interconnect_bw_addr) {
			internal_links[i] = new bandwidth_fifo_link_t(devices[k], devices[j],
				10, 0);
		} else {
			internal_links[i] = new fifo_link_t(devices[k], devices[j], 10, 0);
		}
    }
}

uint32 interconnect_t::get_latency(uint32 source, uint32 dest)
{
    // Hacked for 3 level : add additional latency for cache to cache transfer
    if (dest > private_devices || source > private_devices)
        return g_conf_cmp_interconnect_latency;
    // CACHE to CACHE transfers
    if (g_conf_no_c2c_latency) return 0;
    uint32 add_lat = (g_conf_memory_topology == "cmp_incl") ? g_conf_l1d_latency : g_conf_l2_latency;
    return g_conf_cmp_additional_c2c_latency + g_conf_cmp_interconnect_latency + add_lat;

}


stall_status_t interconnect_t::message_arrival(message_t *message)
{
    interconnect_msg_t *inmsg = static_cast<interconnect_msg_t *> (message);
    VERBOSE_OUT(verb_t::links, "InterConnect Mssage src Id %u Name %s dest Id %u Dest Name %s\n",
        inmsg->get_source(), devices[inmsg->get_source()]->get_cname(),
        inmsg->get_dest(), devices[inmsg->get_dest()]->get_cname());
		
	tick_t bw_delay = 0;
	if (inmsg->get_msg_class() == AddressMsg)
		bw_delay = g_conf_cmp_interconnect_bw_addr;
	else
		bw_delay = g_conf_cmp_interconnect_bw_data;
		
		
    uint32 latency = get_latency(inmsg->get_source(), inmsg->get_dest());
    uint32 internal_link_id = inmsg->get_source() * interconnect_id + inmsg->get_dest();
    stall_status_t ret = internal_links[internal_link_id]->send(message, latency, bw_delay);
    link_active[internal_link_id] = true;
    
	// Count msgs
	if (ret == StallNone) {
        if (mem_hier_t::ptr()->is_thread_state(inmsg->address))
        {
            if (inmsg->get_addr_msg())
                STAT_INC(stat_migration_addr_msg);
            else
                STAT_INC(stat_migration_data_msg);
        } else {
            if (inmsg->get_addr_msg())
                STAT_INC(stat_num_addr_messages);
            else
                STAT_INC(stat_num_data_messages);
        }
            
	}
	
    return ret;
}

void interconnect_t::from_file(FILE *file)
{
		stats->from_file(file);	
}

void interconnect_t::to_file(FILE *file)
{
		stats->to_file(file);	
}


void interconnect_t::cycle_end()
{
    uint32 num_links = interconnect_id * interconnect_id;
    uint32 inactive_threshold = g_conf_cmp_interconnect_latency + 15;
    for (uint32 i = 0; i < num_links; i++)
    {
        if (link_active[i]) last_active[i] = 0;
        else if (last_active[i] < inactive_threshold) last_active[i]++;
        link_active[i] = false;
    }
    uint32 inactives = 0;
    for (uint32 i = 0; i < num_links; i++) {
        if (last_active[i] == inactive_threshold)
            inactives++;
    }
    stat_inactive_link_cycles->inc_total((double)inactives/(double)num_links);    
        
}
