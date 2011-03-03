/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: device.h,v 1.1.1.1.10.6 2006/03/02 23:58:42 pwells Exp $
 *
 * description:    a generic 'device' of the memory hierarchy connected by links
 * initial author: Philip Wells 
 *
 */
 
#ifndef _DEVICE_H_
#define _DEVICE_H_

class device_t {

 public:
	device_t (string _name, uint32 _numlinks);
	virtual ~device_t();
	
	// Inform device of an arrived message
	//   Returns 0 on failure (need block request and retry it)
	virtual stall_status_t message_arrival(message_t *msg) = 0;

	//For dram
	//virtual void advance_cycle();//not probably a good idea, talk to professor about this
	
	// For checkpointing
	virtual void from_file(FILE *) = 0;
	virtual void to_file(FILE *) = 0;
    void stats_read_checkpoint(FILE *);
    void stats_write_checkpoint(FILE *);
    void profiles_read_checkpoint(FILE *);
    void profiles_write_checkpoint(FILE *);
	// Return true if any outstanding action that this devices knows about
	virtual bool is_quiet() = 0;
	
	void set_link(uint32 idx, link_t *link);
	link_t *get_link(uint32 idx);
    
	
	string get_name();
	const char *get_cname();

	void print_stats();
    void clear_stats();
    void create_direct_link(uint32 link_id, device_t *dest);
    void create_fifo_link(uint32 link_id, device_t *dest, uint64 latency,
		uint64 size);
    void set_interconnect_id(uint32);
    
    virtual void cycle_end();
 protected:

	string name;
	uint32 numlinks;
	link_t **links;

    uint32 interconnect_id;
	stats_t *stats;
    profiles_t *profiles;
};


#endif // _DEVICE_T_
