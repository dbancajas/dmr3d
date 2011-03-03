/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: message.h,v 1.2.10.2 2005/11/29 19:11:48 pwells Exp $
 *
 * description:    network message class
 * initial author: Philip Wells 
 *
 */
 
#ifndef _MESSAGE_H_
#define _MESSAGE_H_


enum msg_class_t {
	RequestPendResp,
	Response,
	RequestNoPend,
	Invalidate,      
	InvMsgClass,
	AddressMsg,
	DataMsg,
};

enum io_request_t {
	IONone,
	IOAccessBegin,   // Sent by IO device to start an access
	IOAccessOK,      // Sent by other devices (caches) to signal flush complete
	IOAccessRead,    // Sent between io dev and main mem only
	IOAccessWrite,   // Sent between io dev and main mem only
	IOAccessDone,    // Sent by IO device after access is over
	IOMax
};

class message_t {
 public:
	message_t(mem_trans_t *trans, addr_t _addr, uint32 _size,
		const uint8 *_data = NULL);
	message_t(message_t &msg);
	
	// Virtual destructor to delete child class as well
	virtual ~message_t();
	virtual msg_class_t get_msg_class();
	mem_trans_t *get_trans();
	
	void bank_insert();
	tick_t get_bank_latency();
	
	uint32 size;     // Size of the data we're talking about
	addr_t address;  // Physical address

	uint8 *data;  // Pointer to data

	io_request_t io_type;
	
 protected:
	mem_trans_t *trans;
	
	tick_t bank_insert_time;
};

#endif // _MESSAGE_H_
