/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: simple_proc_msg.h,v 1.4.2.1 2005/06/03 14:04:03 kchak Exp $
 *
 * description:    definitions for simple proc message
 * initial author: Philip Wells 
 *
 */
 
#ifndef _SIMPLE_PROC_MSG_H_
#define _SIMPLE_PROC_MSG_H_

#include "message.h"


class simple_proc_msg_t : public message_t {
public:

	/// CMP Incl Message Types 
	// L1 -> L2
	static const uint32 ProcRead              = 0;
	static const uint32 ProcWrite             = 1;
	static const uint32 StorePref             = 2;
	static const uint32 ReadPref              = 3;
	static const uint32 DataResp              = 4;
    static const uint32 ReqComplete           = 5;
	static const uint32 ProcInvalidate        = 6;

	static const string names[][2];
    
    
    simple_proc_msg_t(mem_trans_t *trans, addr_t addr, uint32 size,
	                   const uint8 *data, uint32 _type) :
		message_t(trans, addr, size, data), type(_type) 
	{ }
    
    uint32 get_type() { return type;}
    
  private:
    
    uint32 type;
};

/*
enum simple_proc_msg_type_t {
	spTypeProcRead,
	spTypeProcWrite,
	spTypeStorePref,
    spTypeReadPref,
	spTypeDataResp,
	spTypeReqComplete,
	spTypeInvalidate,     
	spTypeMax
};

// TODO:  Shouldn't be included here, but needs to be for now to inherit
#include "message.h"

class simple_proc_msg_t : public message_t {
	
 public:
	typedef enum simple_proc_msg_type_t      type_t;
	
	static const type_t ProcRead;
	static const type_t ProcWrite;
	static const type_t StorePref;
    static const type_t ReadPref;
	static const type_t DataResp;
	static const type_t ReqComplete;
	static const type_t ProcInvalidate;   

	static const string names[][2];

	// Constructor
 	simple_proc_msg_t(mem_trans_t *trans, addr_t addr, uint32 size,
	                   const uint8 *data, type_t _type) :
		message_t(trans, addr, size, data), type(_type) 
	{ }
 
	// Message Type Field
	
	msg_class_t get_msg_class() 
	{ 
		if (type == DataResp)
			return Response;
		if (type == ProcInvalidate)
			return Invalidate;
		if (type == ProcRead || type == ProcWrite || type == StorePref)
			return RequestPendResp;
		if(type == ReqComplete)
			return RequestNoPend;
		return InvMsgClass;
	}
	
	type_t type;
};
*/


#endif // _SIMPLE_PROC_MSG_
