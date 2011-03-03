/* Copyright (c) 2006 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mesh_simple.cc,v 1.1.2.3 2006/07/28 01:29:55 pwells Exp $
 *
 * description:    declaration of a interconnect for cmp_incl
 * initial author: Philip Wells 
 *
 *
 *  Sets up this simple mesh-style interconnect:
 *    Assumes # banks == # cores
 *
 *       bank0  bank1  bank2  bank3
 *         |      |      |      |
 *       --+------+------+------+--        <- carries messages from banks
 *         |      |      |      |
 *       --+------+------+------+--         <- carries messages from cores
 *         |      |      |      |
 *       core0  core1  core2  core3 
*/

//  #include "simics/first.h"
// RCSID("$Id: mesh_simple.cc,v 1.1.2.3 2006/07/28 01:29:55 pwells Exp $");

#include "definitions.h"
#include "external.h"
#include "debugio.h"
#include "verbose_level.h"
#include "device.h"
#include "link.h"
#include "interconnect_msg.h"
#include "interconnect.h"
#include "mesh_simple.h"
#include "config_extern.h"
#include "stats.h"
#include "counter.h"
#include "mem_hier.h"

mesh_router_t::mesh_router_t(mesh_simple_t *_mesh, uint32 _x, uint32 _y,
	bool _wrap_x, bool _wrap_y) :
	device_t("", 5), mesh(_mesh), x(_x), y(_y), wrap_x(_wrap_x), wrap_y(_wrap_y)
{
	// 5 unidirectional links out -- not all are always set  
	// 0 - up (y+1)
	// 1 - down (y-1)
	// 2 - right (x+1);
	// 3 - left (x-1)
	// 4 - out (to node)
	
	ASSERT(!wrap_y);  // UNIMPL.
}

stall_status_t
mesh_router_t::message_arrival(message_t *message)
{
    interconnect_msg_t *inmsg = static_cast<interconnect_msg_t *> (message);
	uint32 to_x = mesh->get_dest_x(inmsg->get_dest());
	uint32 to_y = mesh->get_dest_y(inmsg->get_dest());
	uint32 size_x = mesh->get_size_x();
	
	uint32 link;
	bool cutthrough; 
	tick_t bw_delay = 0;
	if (inmsg->get_msg_class() == AddressMsg)
		bw_delay = g_conf_cmp_interconnect_bw_addr;
	else
		bw_delay = g_conf_cmp_interconnect_bw_data;
		

	if (to_x != x) {
		if (wrap_x) {
			// pick shortest left or right
			uint32 x_right_dist = (to_x > x) ? (to_x - x) : (to_x + size_x - x);
			uint32 x_left_dist = (x > to_x) ? (x - to_x) : (x + size_x - to_x);
			link = (x_right_dist <= x_left_dist) ? 2 : 3;
		}
		else if (to_x > x)
			link = 2;
		else if (to_x < x)
			link = 3;
		else FAIL;
		
		cutthrough = true;
	} else if (to_y > y) {  // up
		link = 0;
		cutthrough = true;
	} else if (to_y < y) {  // down
		link = 1;
		cutthrough = true;
	} else {  // node
		ASSERT(to_y == y && to_x == x);
		link = 4;
		cutthrough = false;
	}
		
    VERBOSE_OUT(verb_t::links, "Mesh router@ %12llu (%u, %u) Msg: src %u dest %u, link %u, bw %u cut? %s\n",
        mem_hier_t::ptr()->get_g_cycles(), 
		x, y, inmsg->get_source(), inmsg->get_dest(), link, bw_delay, 
		cutthrough ? "yes" : "no");

	ASSERT(links[link]);
		
	return links[link]->send(message, 0,
		bw_delay, cutthrough);  // cuthrough
}


mesh_simple_t::mesh_simple_t(uint32 _devices) :
    interconnect_t (_devices)
{

}

uint32
mesh_simple_t::get_size_x() {
	return x_size;
}

uint32
mesh_simple_t::get_size_y() {
	return y_size;
}

uint32
mesh_simple_t::get_dest_x(uint32 dest_id) {
	return (dest_id <= private_devices) ? dest_id : dest_id - private_devices - 1;
}

uint32
mesh_simple_t::get_dest_y(uint32 dest_id) {
	return (dest_id <= private_devices) ? 0 : 1;
}

void mesh_simple_t::setup_interconnect()
{
    // As many as interconnect there is
    ASSERT(interconnect_devices == interconnect_id);
	ASSERT(interconnect_devices / 2 == (private_devices+1));
	
	x_size = private_devices + 1;
	y_size = 2;
	links_per_node = 6;
	
	// Setup routers
	uint32 num_routers = x_size * y_size;
	routers = new mesh_router_t * [num_routers];

	for (uint32 i = 0; i < num_routers; i++) {
		routers[i] = new mesh_router_t(this, get_dest_x(i),get_dest_y(i),
			true, false);
	}

	// Bidirectional
	uint32 num_internal_links = links_per_node * x_size * y_size;
	
    internal_links = new link_t * [num_internal_links];

	for (uint32 y = 0; y < y_size; y++) {
		for (uint32 x = 0; x < x_size; x++) {
			uint32 router_id = x + x_size * y;
			uint32 linkstart_id = router_id * 6;
			uint32 node_device_id = router_id;
			
			// create entry links to/from node (really from intercon device)
			internal_links[linkstart_id] = new bandwidth_fifo_link_t(
				this, routers[router_id],
				g_conf_cmp_interconnect_latency, 0);
			// internal_links[linkstart_id+1] = new bandwidth_fifo_link_t(
				// routers[router_id], devices[node_device_id],
				// g_conf_cmp_interconnect_latency, 0);
			// XXXX
			// This is a hack to prevent deadlock when a cache blocks a down message
			// waiting for another down message.  
			// Should probably just fix cache
			internal_links[linkstart_id+1] = new passing_fifo_link_t(
				routers[router_id], devices[node_device_id], 0, 0,
				interconnect_devices);
			routers[router_id]->set_link(4, internal_links[linkstart_id+1]);
			
			// create right/left links from router to another router
			// with wraparound
			uint32 right_id = (x == x_size-1) ? 0 : (x + 1);
			uint32 left_id = (x == 0) ? (x_size - 1) : (x - 1);
			right_id += x_size * y;
			left_id += x_size * y;
			ASSERT(right_id != router_id);
			ASSERT(left_id != router_id);
			internal_links[linkstart_id+2] = new bandwidth_fifo_link_t(
				routers[router_id], routers[right_id],
				g_conf_cmp_interconnect_latency, 0);
			routers[router_id]->set_link(2, internal_links[linkstart_id+2]);
			internal_links[linkstart_id+3] = new bandwidth_fifo_link_t(
				routers[router_id], routers[left_id],
				g_conf_cmp_interconnect_latency, 0);
			routers[router_id]->set_link(3, internal_links[linkstart_id+3]);
			
			// create up/down links from router to another router
			// no wraparound
			if (y < y_size - 1) { // not top
				uint32 up_id = x + x_size * (y + 1);
				ASSERT(up_id != router_id);
				internal_links[linkstart_id+4] = new bandwidth_fifo_link_t(
					routers[router_id], routers[up_id],
					g_conf_cmp_interconnect_latency, 0);
				routers[router_id]->set_link(0, internal_links[linkstart_id+4]);
			}

			if (y > 0) { // not bottom
				uint32 down_id = x + x_size * (y - 1);
				ASSERT(down_id != router_id);
				internal_links[linkstart_id+5] = new bandwidth_fifo_link_t(
					routers[router_id], routers[down_id],
					g_conf_cmp_interconnect_latency, 0);
				routers[router_id]->set_link(1, internal_links[linkstart_id+5]);
			}

		}
	}
}

uint32 mesh_simple_t::get_latency(uint32 source, uint32 dest)
{
	return 0;
	
    if (g_conf_no_c2c_latency) return 0;
	return g_conf_cmp_interconnect_latency;
}


stall_status_t mesh_simple_t::message_arrival(message_t *message)
{
    interconnect_msg_t *inmsg = static_cast<interconnect_msg_t *> (message);
    VERBOSE_OUT(verb_t::links, "Mesh Intercon Mssage src Id %u Name %s dest Id %u Dest Name %s\n",
        inmsg->get_source(), devices[inmsg->get_source()]->get_cname(),
        inmsg->get_dest(), devices[inmsg->get_dest()]->get_cname());
		
	tick_t bw_delay = 0;
	if (inmsg->get_msg_class() == AddressMsg)
		bw_delay = g_conf_cmp_interconnect_bw_addr;
	else
		bw_delay = g_conf_cmp_interconnect_bw_data;
		
    uint32 latency = get_latency(inmsg->get_source(), inmsg->get_dest());
	
	// Use appropriate 'from-node' link
	uint32 link_id = links_per_node * (get_dest_x(inmsg->get_source()) +
		x_size * get_dest_y(inmsg->get_source()));
	
	ASSERT(internal_links[link_id]);
	// msg can be freed in send...
	uint32 msg_class = inmsg->get_msg_class();
	
	stall_status_t ret = internal_links[link_id]->
		send(message, latency, bw_delay, true);

	// Count msgs
	if (ret == StallNone) {
		if (msg_class == AddressMsg)
			STAT_INC(stat_num_addr_messages);
		else
			STAT_INC(stat_num_data_messages);
	}
	
    return ret;
}

