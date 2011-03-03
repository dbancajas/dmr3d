/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: simple_mainmem_msg.h,v 1.2.10.2 2005/06/10 17:03:23 kchak Exp $
 *
 * description:    definitions for simple mainmem message
 * initial author: Philip Wells 
 *
 */
 
#ifndef _SIMPLE_MAINMEM_MSG_H_
#define _SIMPLE_MAINMEM_MSG_H_

#include "message.h"

class simple_mainmem_msg_t : public message_t {
	
public: 
	static const uint32 Read        = 0;
	static const uint32 ReadEx      = 1;
	static const uint32 WriteBack   = 2;
	static const uint32 DataResp    = 3;
	static const uint32 WBAck       = 4;
	static const uint32 DRAMUpdate  = 5;
 
	static const string names[][2];

	// Constructor
 	simple_mainmem_msg_t(mem_trans_t *trans, addr_t addr, uint32 size,
		const uint8 *data, uint32 _type, uint32 id=0)
		: message_t(trans, addr, size, data), type(_type), sender_id(id), 
		need_wb_ack(false)
	{ }
	
	// Message Type field
	msg_class_t get_msg_class() 
	{
		if (type == Read || type == ReadEx)
			return RequestPendResp;
		if (type == DataResp)
			return Response;
		if (type == WriteBack)
			return RequestNoPend;
		return InvMsgClass;
	
	}
	
	uint32 get_type() { return type; }
	uint32 get_sender_id() { return sender_id; }
	bool need_writeback_ack() { return need_wb_ack; }
        void set_type(uint32 _type) { type = _type;}

protected:
	uint32 type;
	uint32 sender_id;
	bool need_wb_ack;
};

#endif // _SIMPLE_MAINMEM_MSG_H_
