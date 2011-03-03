/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mesh_sandwich.h,v 1.1.2.1 2006/07/28 13:55:23 pwells Exp $
 *
 * description:    declaration of a interconnect for cmp_incl_l2bank
 * initial author: Philip Wells 
 *
*/
 

#ifndef _MESH_SANDWICH_H_
#define _MESH_SANDWICH_H_

#include "mesh_simple.h"


class mesh_sandwich_t : public mesh_simple_t {
	
public:
	mesh_sandwich_t(uint32 _num_devices, uint32 _x_size);
	virtual ~mesh_sandwich_t() {}

	virtual void setup_interconnect();
	
	virtual uint32 get_dest_x(uint32 dest_id);
	virtual uint32 get_dest_y(uint32 dest_id);
	virtual uint32 get_node_device_id(uint32 x, uint32 y);

protected:

	uint32 num_devices_per_side;
	uint32 num_devices_per_node;
};

#endif // _MESH_SANDWICH_H_
