/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: interconnect_msg.h,v 1.1.2.3 2005/06/13 16:22:58 kchak Exp $
 *
 * description:    
 * initial author: 
 *
 */
 
#ifndef _INTERCONNECT_MSG_H_
#define _INTERCONNECT_MSG_H_

#include "message.h"

class interconnect_msg_t : public message_t {
	
public:
	// Constructor
 	interconnect_msg_t(mem_trans_t *trans, addr_t addr, uint32 size,
		const uint8 *data, uint32 _source, uint32 _dest, bool _addr_msg) :
		
		message_t(trans, addr, size, data), 
		source(_source), dest(_dest), addr_msg(_addr_msg)
	{ }
	
	// Use default copy constructor
	// Use default destructor
	
	uint32 get_source() { return source; }
	uint32 get_dest() { return dest; }
    bool   get_addr_msg() { return addr_msg;}
	
protected:
	uint32 source;    // Source node ID
	uint32 dest;      // Destination node ID
    bool   addr_msg;
    
};

#endif // _INTERCONNECT_MSG_H_
