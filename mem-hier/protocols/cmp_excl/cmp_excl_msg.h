/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_excl_msg.h,v 1.1.2.1 2005/07/22 15:09:27 pwells Exp $
 *
 * description:    
 * initial author: 
 *
 */
 
#ifndef _CMP_EXCL_MSG_H_
#define _CMP_EXCL_MSG_H_

#include "interconnect_msg.h"
#include "cmp_excl.h"

class cmp_excl_msg_t : public interconnect_msg_t, public cmp_excl_protocol_t {
	
public:
	// Constructor
 	cmp_excl_msg_t(mem_trans_t *trans, addr_t addr, uint32 size,
		const uint8 *data, uint32 source, uint32 dest,
		uint32 _type, uint32 _requester_id, bool _addr_msg) :
		
		interconnect_msg_t(trans, addr, size, data, source, dest, _addr_msg), 
		type(_type), requester_id(_requester_id)
	{ }
	
	// Use default copy constructor
	// Use default destructor
	
	uint32 get_type() { return type; }
	uint32 get_requester() { return requester_id; }
	
protected:
	uint32 type;            // Message Type
	uint32 requester_id;    // Original Requester
};

#endif // _CMP_EXCL_MSG_H_
