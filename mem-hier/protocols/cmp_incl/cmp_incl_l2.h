/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_incl_l2.h,v 1.1.2.9 2005/07/29 20:38:10 pwells Exp $
 *
 * description:    declaration of a CMP L2 stuff
 * initial author: Philip Wells 
 *
 */
 
#ifndef _CMP_INCL_L2_H_
#define _CMP_INCL_L2_H_

#include "cmp_incl.h"

////////////////////////////////////////////////////////////////////////////////
// CMP inclusive L2 statemachine declarations
//    This is inherited by tcache and protocol specific line
class cmp_incl_l2_protocol_t : public cmp_incl_protocol_t {
public:
	/// States
	static const uint32 StateUnknown     = 0;
	static const uint32 StateNotPresent  = 1;
	static const uint32 StateInvalid     = 2;
	static const uint32 StateInvToMod    = 3;
	static const uint32 StateShared 	 = 4;
	static const uint32 StateShToInv     = 5;
	static const uint32 StateShFwd       = 6;
	static const uint32 StateShToMod     = 7;
	static const uint32 StateModified    = 8;
	static const uint32 StateModToSh     = 9;
	static const uint32 StateModToInv    = 10;
	static const uint32 StateMax         = 11;
	
	
	/// Actions
	static const uint32 ActionReplace       = 0;
	static const uint32 ActionFlush         = 1;
	static const uint32 ActionL1Read        = 2;
	static const uint32 ActionL1ReadEx      = 3;
	static const uint32 ActionL1Upgrade     = 4;
	static const uint32 ActionL1Replace     = 5;
	static const uint32 ActionL1WriteBack   = 6;
	static const uint32 ActionL1WriteBackClean = 7;
	static const uint32 ActionL1InvAck      = 8;
	static const uint32 ActionL1FwdAck      = 9;
	static const uint32 ActionL1FwdNack     = 10;
	static const uint32 ActionL1FwdAckData  = 11;
	static const uint32 ActionDataResponse  = 12;
    static const uint32 ActionL1TST         = 13;
	static const uint32 ActionMax           = 14;
	
	/// Links
	static const uint8 interconnect_link = 0;
	static const uint8 mainmem_addr_link = 1;
	static const uint8 mainmem_data_link = 2;
	static const uint8 io_link = 255;  // Not implemented

	
	/// MSHR type
	struct mshr_type_t {
		mem_trans_t *trans;
		uint32 requester_id;
		uint32 request_type;
	};
	
	static const bool l1_stats = false;
	static const bool l2_stats = true;
	static const bool l3_stats = false;
	
	/// Typedefs
	typedef cmp_incl_l2_line_t                            prot_line_t;
	typedef transfn_return_t<uint32, prot_line_t>         tfn_ret_t;
	typedef transfn_info_t<uint32, uint32, prot_line_t>   tfn_info_t;
    typedef cmp_incl_l2_cache_t                           cache_type_t;
	typedef cmp_incl_msg_t                                upmsg_t;
	typedef simple_mainmem_msg_t                          downmsg_t;
	
	static const string state_names[][2];
	static const string action_names[][2];
	static const string prot_name[];
};

#endif /* _CMP_INCL_L2_H_ */
