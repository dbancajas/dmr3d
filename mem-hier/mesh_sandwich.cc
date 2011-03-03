/* Copyright (c) 2006 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mesh_sandwich.cc,v 1.1.2.1 2006/07/28 13:55:23 pwells Exp $
 *
 * description:    declaration of a interconnect for cmp_incl_l2bank
 * initial author: Philip Wells 
 *
 *
 *  Sets up this mesh interconnect that sandwiches banks between cores
 *    Assumes # banks == # cores
 *
 *       core0  core1  core2  core3 
 *         |      |      |      |
 *       --+------+------+------+--
 *         |      |      |      |
 *       --+bank0-+b1----+b2----+b3
 *         |      |      |      |
 *       --+b4----+b5----+b6----+b7
 *         |      |      |      |
 *       --+------+------+------+--
 *         |      |      |      |
 *       core4  core5  core6  core7 
*/

//  #include "simics/first.h"
// RCSID("$Id: mesh_sandwich.cc,v 1.1.2.1 2006/07/28 13:55:23 pwells Exp $");

#include "definitions.h"
#include "external.h"
#include "debugio.h"
#include "verbose_level.h"
#include "device.h"
#include "link.h"
#include "interconnect_msg.h"
#include "interconnect.h"
#include "mesh_sandwich.h"
#include "config_extern.h"
#include "stats.h"
#include "counter.h"
#include "mem_hier.h"

mesh_sandwich_t::mesh_sandwich_t(uint32 _devices, uint32 _x_size) :
	mesh_simple_t(_devices)
{
	x_size = _x_size;
}

uint32
mesh_sandwich_t::get_dest_x(uint32 dest_id) {
	// Top or bottom
	if (dest_id <= private_devices)
		return ((dest_id / num_devices_per_node) % get_size_x());
	// Middle rows
	else
		return ((dest_id - (private_devices + 1)) % get_size_x());
}

uint32
mesh_sandwich_t::get_dest_y(uint32 dest_id) {
	// Top or bottom
	if (dest_id <= private_devices)
		return (dest_id < num_devices_per_side ? 0 : get_size_y() - 1);
	// Middle rows
	else
		return ((dest_id - (private_devices + 1)) / get_size_x()) + 1;
}

uint32
mesh_sandwich_t::get_node_device_id(uint32 x, uint32 y)
{
	uint32 ret;
	// Bottom row (private devices)
	if (y == 0)
		ret = x * num_devices_per_node;
	// Top row (private devices)
	else if (y == get_size_y() - 1)
		ret = (num_devices_per_side + x * num_devices_per_node);
	// Middle rows (shared devices)
	else
		ret = ((private_devices+1) + get_size_x()*(y-1) + x);
	
	ASSERT(ret >= 0 && ret < interconnect_devices);
	return ret;
}

void mesh_sandwich_t::setup_interconnect()
{
    // As many as interconnect there is
    ASSERT(interconnect_devices == interconnect_id);
	
	ASSERT((private_devices + 1) % 2 == 0);
	// For private devices
	num_devices_per_side = (private_devices + 1) / 2;
	num_devices_per_node = num_devices_per_side / x_size;

	y_size = 2 + (interconnect_devices - (private_devices+1)) / x_size;
	
	links_per_node = 6;
	
	ASSERT(x_size > 0 && x_size <= num_devices_per_side);
	ASSERT(interconnect_devices % x_size == 0);
	
	// Setup routers
	uint32 num_routers = x_size * y_size;
	routers = new mesh_router_t * [num_routers];

	// Need to create before setting up links
	for (uint32 y = 0; y < y_size; y++) {
		for (uint32 x = 0; x < x_size; x++) {
			uint32 router_id = x + x_size * y;
			routers[router_id] = new mesh_router_t(this, x, y, false, false);
		}
	}
	
	// Bidirectional
	uint32 num_internal_links = links_per_node * x_size * y_size;
	
    internal_links = new link_t * [num_internal_links];

	device_t **device_per_node_ptr = new device_t * [num_devices_per_node];

	for (uint32 y = 0; y < y_size; y++) {
		for (uint32 x = 0; x < x_size; x++) {
			uint32 router_id = x + x_size * y;
			uint32 linkstart_id = router_id * 6;
			uint32 node_device_id = get_node_device_id(x, y);
			
			// create entry links to/from node (really from intercon device)
			internal_links[linkstart_id] = new bandwidth_fifo_link_t(
				this, routers[router_id],
				0,   // latency 
				0);
			// internal_links[linkstart_id+1] = new bandwidth_fifo_link_t(
				// routers[router_id], devices[node_device_id],
				// g_conf_cmp_interconnect_latency, 0);
			// XXXX
			// Passing Fifo is a hack to prevent deadlock when a cache blocks a down message
			// waiting for another down message.  
			// Should probably just fix cache

			// Multiple dests per node is a hack to allow separate caches and/or
			// multiple cores to be close to a single network node
			uint32 local_num_devices_per_node;
			if (y == 0 || y == y_size-1) {
				for (uint32 d = 0; d < num_devices_per_node; d++) {
					device_per_node_ptr[d] = devices[node_device_id+d];
				}
				local_num_devices_per_node = num_devices_per_node;
			} else { 
				// For internal nodes
				local_num_devices_per_node = 1;
			}
				
			internal_links[linkstart_id+1] = new passing_fifo_link_t(
				routers[router_id], devices[node_device_id], 
				0,  // latency
				0,
				interconnect_devices, local_num_devices_per_node, device_per_node_ptr);
			routers[router_id]->set_link(4, internal_links[linkstart_id+1]);
			
			// create right/left links from router to another router
			// with wraparound
			// XXX But this wraparound isn't necessarily used...
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
				// Hack to make local bank closer than other links
				uint32 y_lat = g_conf_cmp_interconnect_latency;
				if (y == 0 || y == y_size-2)
					y_lat = 2;
				uint32 up_id = x + x_size * (y + 1);
				ASSERT(up_id != router_id);
				internal_links[linkstart_id+4] = new bandwidth_fifo_link_t(
					routers[router_id], routers[up_id],
					y_lat, 0);
				routers[router_id]->set_link(0, internal_links[linkstart_id+4]);
			}

			if (y > 0) { // not bottom
				// Hack to make local bank closer than other links
				uint32 y_lat = g_conf_cmp_interconnect_latency;
				if (y == y_size-1 || y == 1)
					y_lat = 2;
				uint32 down_id = x + x_size * (y - 1);
				ASSERT(down_id != router_id);
				internal_links[linkstart_id+5] = new bandwidth_fifo_link_t(
					routers[router_id], routers[down_id],
					y_lat, 0);
				routers[router_id]->set_link(1, internal_links[linkstart_id+5]);
			}

		}
	}
}

