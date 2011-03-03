/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_incl_3l_l2.h,v 1.1.2.3 2005/08/02 20:28:46 pwells Exp $
 *
 * description:    declaration of a CMP L2 stuff
 * initial author: Koushik Chakraborty 
 *
 */
 
#ifndef _CMP_INCL_3L_L2_H_
#define _CMP_INCL_3L_L2_H_


////////////////////////////////////////////////////////////////////////////////
// CMP exclusive L2 statemachine declarations
//    This is inherited by tcache and protocol specific line
class cmp_incl_3l_l2_protocol_t : public cmp_incl_3l_protocol_t {
public:
	/// States
	static const uint32 StateUnknown       = 0;
	static const uint32 StateNotPresent    = 1;
	static const uint32 StateInvalid       = 2;
	static const uint32 StateInvToShEx     = 3;
	static const uint32 StateInvToMod      = 4;
	static const uint32 StateShared        = 5;
	static const uint32 StateModified      = 6;
	static const uint32 StateModifiedShared = 7;
	static const uint32 StateShToMod       = 8;
	static const uint32 StateShInvToMod    = 9;
	static const uint32 StateModToSh       = 10;
	static const uint32 StateModToInv      = 11;
	static const uint32 StateModToInvOnFwd = 12;
	static const uint32 StateModToInvOnInv = 13;
	static const uint32 StateModToModToInv = 14;
    static const uint32 StateShToShToInv   = 15;
    static const uint32 StateShToInvOnInv  = 16;
    static const uint32 StateShToInv       = 17;
    static const uint32 StateShToInvToMod  = 18;
	static const uint32 StateMax           = 19;
	
	
	/// Actions
	static const uint32 ActionReplace       = 0;
	static const uint32 ActionL1Read        = 1;
	static const uint32 ActionL1ReadEx      = 2;
	static const uint32 ActionL1Upgrade     = 3;
	static const uint32 ActionL1Replace     = 4;
	static const uint32 ActionL1WriteBack   = 5;
	static const uint32 ActionL1WriteBackClean = 6;
	static const uint32 ActionL1InvAck      = 7;
	static const uint32 ActionL1DataRespSh  = 8;
	static const uint32 ActionDataShared    = 9;
	static const uint32 ActionDataExclusive = 10;
	static const uint32 ActionUpgradeAck    = 11;
	static const uint32 ActionInvalidate    = 12;
	static const uint32 ActionWriteBackAck  = 13;
    static const uint32 ActionReplaceAck    = 14;
	static const uint32 ActionReadFwd       = 15;
	static const uint32 ActionReadExFwd     = 16;
    static const uint32 ActionFlush         = 17;
    static const uint32 ActionMax           = 18;
	
	
	/// Links
	static const uint8 interconnect_link = 0;
	static const uint8 icache_link = 1;
	static const uint8 dcache_link = 2;
	static const uint8 io_link = 255;  // Not implemented
	
	/// Sender of up messages will be either:
	static const uint8 icache_id = 0;
	static const uint8 dcache_id = 1;

	struct mshr_type_t {
		mem_trans_t *trans;
		uint32 request_type;
		bool icache;
	};
	
	static const bool l1_stats = false;
	static const bool l2_stats = true;
	static const bool l3_stats = false;
	
	/// Typedefs
	typedef cmp_incl_3l_l2_line_t                         prot_line_t;
	typedef transfn_return_t<uint32, prot_line_t>         tfn_ret_t;
	typedef transfn_info_t<uint32, uint32, prot_line_t>   tfn_info_t;
    typedef cmp_incl_3l_l2_cache_t                        cache_type_t;
	typedef cmp_incl_3l_msg_t                             downmsg_t;
	typedef cmp_incl_3l_msg_t                             upmsg_t;
	
	static const string state_names[][2];
	static const string action_names[][2];
	static const string prot_name[];
};

#endif /* _CMP_INCL_3L_L3_H_ */
