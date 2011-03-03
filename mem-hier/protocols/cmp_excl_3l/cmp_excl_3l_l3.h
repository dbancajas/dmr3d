/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_excl_3l_l3.h,v 1.1.2.1 2005/07/29 20:45:02 pwells Exp $
 *
 * description:    declaration of a CMP L3 stuff
 * initial author: Philip Wells 
 *
 */
 
#ifndef _CMP_EXCL_3L_L3_H_
#define _CMP_EXCL_3L_L3_H_

#include "cmp_excl.h"

////////////////////////////////////////////////////////////////////////////////
// CMP exclusive L3 statemachine declarations
//    This is inherited by tcache and protocol specific line
class cmp_excl_3l_l3_protocol_t : public cmp_excl_3l_protocol_t {
public:
	/// States
	static const uint32 StateUnknown         = 0;
	static const uint32 StateNotPresent      = 1;
	static const uint32 StateInvalid         = 2;
	static const uint32 StateClean           = 3;
	static const uint32 StateDirty           = 4;
    static const uint32 StatePrefetchPend    = 5;
    static const uint32 StateMax             = 6;
	
	/// Actions
	static const uint32 ActionReplace       = 0;
	static const uint32 ActionFlush         = 1;
	static const uint32 ActionL2Get         = 2;
	static const uint32 ActionL2Replace     = 3;
	static const uint32 ActionL2WriteBack   = 4;
	static const uint32 ActionDataResp      = 5;
    static const uint32 ActionPrefetch      = 6;
    static const uint32 ActionMax           = 7;
	
	/// Links
	static const uint8 l2dir_link = 0;
	static const uint8 mainmem_addr_link = 1;
	static const uint8 mainmem_data_link = 2;
	static const uint8 io_link = 255;  // Not implemented

	/// MSHR type
	typedef void mshr_type_t;
	
	static const bool l1_stats = false;
	static const bool l2_stats = false;
	static const bool l3_stats = true;
	
	/// Typedefs
	typedef cmp_excl_3l_l3_line_t                         prot_line_t;
	typedef transfn_return_t<uint32, prot_line_t>         tfn_ret_t;
	typedef transfn_info_t<uint32, uint32, prot_line_t>   tfn_info_t;
    typedef cmp_excl_3l_l3_cache_t                        cache_type_t;
	typedef cmp_excl_3l_msg_t                             upmsg_t;
	typedef simple_mainmem_msg_t                          downmsg_t;
	
	static const string state_names[][2];
	static const string action_names[][2];
	static const string prot_name[];
};

#endif /* _CMP_EXCL_3L_L3_H_ */
