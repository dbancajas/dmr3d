/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_incl_l1.h,v 1.1.2.6 2005/07/29 20:38:10 pwells Exp $
 *
 * description:    declaration of a CMP L1 stuff
 * initial author: Koushik Chakraborty 
 *
 */
 
#ifndef _CMP_INCL_WT_L1_H_
#define _CMP_INCL_WT_L1_H_

#include "cmp_incl.h"

////////////////////////////////////////////////////////////////////////////////
// CMP inclusive L1 statemachine declarations
//    This is inherited by tcache and protocol specific line
class cmp_incl_wt_l1_protocol_t : public cmp_incl_wt_protocol_t {
public:
	/// States
	static const uint32 StateUnknown     = 0;
	static const uint32 StateNotPresent  = 1;
	static const uint32 StateInvalid     = 2;
	static const uint32 StateInvToSh 	 = 3;
	static const uint32 StateShared    = 4;
	static const uint32 StateShToInv    = 5;
    static const uint32 StatePendingWrite = 6;
	static const uint32 StateMax         = 7;
	
	
	/// Actions
	static const uint32 ActionReplace       = 0;
	static const uint32 ActionProcLD        = 1;
	static const uint32 ActionProcST        = 2;
	static const uint32 ActionDataResp      = 3;
	static const uint32 ActionInvalidate    = 4;
    static const uint32 ActionWriteComplete = 5;
    static const uint32 ActionFlush         = 6;
    static const uint32 ActionTSTLD         = 7;
    static const uint32 ActionTSTST         = 8;
    static const uint32 ActionTSTResp       = 9;
    static const uint32 ActionMax           = 10;
	
	
	/// Links
	static const uint8 interconnect_link = 0;
	static const uint8 proc_link = 1;
	static const uint8 io_link = 255;  // Not implemented

	struct mshr_type_t {
		mem_trans_t *trans;
		uint32 request_type;
	};
	
	static const bool l1_stats = true;
	static const bool l2_stats = false;
	static const bool l3_stats = false;
	
	/// Typedefs
	typedef cmp_incl_wt_l1_line_t                            prot_line_t;
	typedef transfn_return_t<uint32, prot_line_t>            tfn_ret_t;
	typedef transfn_info_t<uint32, uint32, prot_line_t>      tfn_info_t;
    typedef cmp_incl_wt_l1_cache_t                           cache_type_t;
	typedef cmp_incl_wt_msg_t                                downmsg_t;
	typedef simple_proc_msg_t                                upmsg_t;
	
	static const string state_names[][2];
	static const string action_names[][2];
	static const string prot_name[];
};

#endif /* _CMP_INCL_L2_H_ */
