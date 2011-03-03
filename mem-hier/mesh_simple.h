/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mesh_simple.h,v 1.1.2.2 2006/07/28 01:29:55 pwells Exp $
 *
 * description:    declaration of a interconnect for cmp_incl
 * initial author: Koushik Chakraborty 
 *
*/
 
#ifndef _MESH_SIMPLE_H_
#define _MESH_SIMPLE_H_

class mesh_router_t : public device_t {
public:
	mesh_router_t(mesh_simple_t *_mesh, uint32 _x, uint32 _y, bool _wrap_x, 
		bool _wrap_y);
	
	virtual stall_status_t message_arrival(message_t *);
	
	void from_file(FILE *file) {}
	void to_file(FILE *file) {}
	bool is_quiet() { return true;}

private:
	mesh_simple_t *mesh;
	uint32 x,y;
	bool wrap_x, wrap_y;
};

 
class mesh_simple_t : public interconnect_t {
	
public:
	mesh_simple_t(uint32 _num_devices);
	virtual ~mesh_simple_t() {}
	virtual uint32 get_latency(uint32 source, uint32 dest);
	virtual void setup_interconnect();
	virtual stall_status_t message_arrival(message_t *);
	
	virtual uint32 get_size_x();
	virtual uint32 get_size_y();
	virtual uint32 get_dest_x(uint32 dest_id);
	virtual uint32 get_dest_y(uint32 dest_id);

	void from_file(FILE *file) {}
	void to_file(FILE *file) {}
	
protected:
	mesh_router_t **routers;
	
	uint32 x_size;
	uint32 y_size;
	
	uint32 num_rounters;
	uint32 links_per_node;
};

#endif // _MESH_SIMPLE_H_

