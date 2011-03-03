/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: unip_two_l1i.h,v 1.1.2.1 2005/08/24 19:20:07 pwells Exp $
 *
 * description:    declaration of a unip two l1d stuff
 * initial author: Koushik Chakraborty 
 *
 */
 
#ifndef _UNIP_TWO_L1I_H_
#define _UNIP_TWO_L1I_H_

#include "unip_two.h"

////////////////////////////////////////////////////////////////////////////////
// CMP inclusive L1 statemachine declarations
//    This is inherited by tcache and protocol specific line
class unip_two_l1i_protocol_t : public unip_two_protocol_t {
public:
	/// States
	static const uint32 StateUnknown     = 0;
	static const uint32 StateNotPresent  = 1;
	static const uint32 StateInvalid     = 2;
	static const uint32 StateInv_Rd      = 3;
	static const uint32 StateInv_Pref 	 = 4;
	static const uint32 StatePresent     = 5;
	static const uint32 StateMax         = 6;
	
	/// Actions
	static const uint32 ActionReplace       = 0;
	static const uint32 ActionProcIFetch    = 1;
	static const uint32 ActionRecvData      = 2;
	static const uint32 ActionRecvInv       = 3;
    static const uint32 ActionPrefetch      = 4;
	static const uint32 ActionFlush         = 5;
	static const uint32 ActionMax           = 6;
	
	/// Links
	static const uint8 proc_link = 0;
	static const uint8 l2_link = 1;
	static const uint8 io_link = 255;  // Not implemented

	typedef mem_trans_t                       mshr_type_t;
	
	static const bool l1_stats = true;
	static const bool l2_stats = false;
	static const bool l3_stats = false;
	
	/// Typedefs
	typedef unip_two_l1i_line_t                           prot_line_t;
	typedef transfn_return_t<uint32, prot_line_t>         tfn_ret_t;
	typedef transfn_info_t<uint32, uint32, prot_line_t>   tfn_info_t;
    typedef unip_two_l1i_cache_t                          cache_type_t;
	typedef unip_two_msg_t                                downmsg_t;
	typedef simple_proc_msg_t                             upmsg_t;
	
	static const string state_names[][2];
	static const string action_names[][2];
	static const string prot_name[];
};

#endif /* _UNIP_TWO_L1I_H_ */
