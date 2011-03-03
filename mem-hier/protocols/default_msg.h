/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: default_msg.h,v 1.3 2005/01/18 17:51:54 pwells Exp $
 *
 * description:    definitions for a simple, default message
 * initial author: Philip Wells 
 *
 */
 
#ifndef _DEFAULT_MSG_H_
#define _DEFAULT_MSG_H_


enum default_msg_type_t {
	
	dTypeRead,
	dTypeWrite,
	dTypeStorePref,
	dTypeDataResp,
	dTypeReqComplete,
	dTypeMax
};

// TODO:  Shouldn't be included here, but needs to be for now to inherit
#include "message.h"

class default_msg_t : public message_t {
	
 public:
	typedef enum default_msg_type_t type_t;

	static const type_t ProcRead;  // == Read
	static const type_t ProcWrite; // == Write
	static const type_t StorePref;
	static const type_t Read;
	static const type_t ReadEx;    // == Read
	static const type_t WriteBack; // == Write
	static const type_t DataResp;
	static const type_t ReqComplete;
	static const type_t WBAck; // == ReqComplete

	static const string names[][2];

	// Constructor
    default_msg_t(mem_trans_t *trans, addr_t addr, uint32 size, 
		const uint8 *data, type_t _type)
		: message_t(trans, addr, size, data), type(_type),
		need_wb_ack(true) // default
	{ }
	
	// Message type decider function
	bool proc_request() { return type == Read || type == ReadEx;}
	bool response_to_proc() { return type == DataResp;}
	
	// Message Type field
	
	type_t type;
	bool need_wb_ack;
};

#endif // _DEFAULT_MSG_H_
