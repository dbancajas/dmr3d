/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: unip_two_msg.h,v 1.1.2.1 2005/08/24 19:20:08 pwells Exp $
 *
 * description:    definitions for simple mainmem message
 * initial author: Philip Wells 
 *
 */
 
#ifndef _UNIP_TWO_MSG_H_
#define _UNIP_TWO_MSG_H_

// TODO:  Shouldn't be included here, but needs to be for now to inherit
#include "message.h"
#include "unip_two.h"

class unip_two_msg_t : public message_t, public unip_two_protocol_t {
	
 public:
	// Constructor
 	unip_two_msg_t(mem_trans_t *trans, addr_t addr, uint32 size,
	                   const uint8 *data, uint32 _type)
		: message_t(trans, addr, size, data), type(_type)
	{ }
	
	// Use default copy constructor
	// Use default destructor
	
	// Message Type field

	msg_class_t get_msg_class() 
	{ 
		return InvMsgClass;
	}
	
	uint32 type;
};

#endif // _UNIP_TWO_MSG_H_
