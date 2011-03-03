/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: iodev.h,v 1.1.1.1.10.1 2005/03/10 17:46:32 pwells Exp $
 *
 * description:    I/O device of memory hierarchy
 * initial author: Philip Wells 
 *
 */
 
#ifndef _IODEV_H_
#define _IODEV_H_

enum io_dev_state_t {
	IOStateNone,
	IOStateFlushWait,
	IOStateReadWait,
	IOStateWriteWait,
	IOStateMax
};


// for polymorphism
class generic_io_dev_t : public device_t {
 public:
	generic_io_dev_t(string name, uint32 num_links) : device_t(name, num_links)
	{ }

	// Called to initiate a transaction
	virtual mem_return_t make_request(mem_trans_t *trans) = 0;
};


// Virtual I/O device
template <class prot_sm_t, class msg_t>
class io_dev_t : public generic_io_dev_t {

 public:
	typedef typename prot_sm_t::action_t action_t;
	typedef typename msg_t::type_t type_t;

	io_dev_t(string name, uint32 _num_sharers);
	
	// Called to initiate a transaction
	virtual mem_return_t make_request(mem_trans_t *trans);

	// Called by network
	virtual stall_status_t message_arrival(message_t *message);

	void from_file(FILE *file);
	void to_file(FILE *file);
	bool is_quiet();
	
	static const uint8 mainmem_link = 0;  
	static const uint8 cache_link = 1;

private:

	uint32 num_sharers;  // Number of caches that I need to wait for
	mem_trans_t *outstanding;  // currently only one outstanding at a time
	io_dev_state_t state;
	uint32 pending_flush_acks; // number of cache flush acks pending
	
};

#include "iodev.cc"

#endif // _IODEV_H_
