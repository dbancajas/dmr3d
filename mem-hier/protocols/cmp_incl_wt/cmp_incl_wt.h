/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_incl.h,v 1.1.2.8 2005/06/20 19:18:06 kchak Exp $
 *
 * description:    common declaration of a CMP inclusive protocol
 * initial author: Philip Wells 
 *
 */
 
#ifndef _CMP_INCL_WT_H_
#define _CMP_INCL_WT_H_

////////////////////////////////////////////////////////////////////////////////
// CMP inclusive declarations
//    Common to all caches and protocols
class cmp_incl_wt_protocol_t {
public:

	/// CMP Incl Message Types 
	// L1 -> L2
	static const uint32 TypeRead              = 0;
	static const uint32 TypeReadEx            = 1;
	static const uint32 TypeReplace           = 2;
    static const uint32 TypeInvAck            = 3;
	// L2 -> L1 or L1 -> L1 
    static const uint32 TypeDataResp          = 4;
	// L2 -> L1
    static const uint32 TypeInvalidate        = 5;
	static const uint32 TypeUpdate            = 6;
    static const uint32 TypeWriteComplete     = 7;
	// Required
	static const uint32 TypeStorePref         = 9;
    static const uint32 TypeReadPref          = 10;
	

	static const string names[][2];
};

#endif /* _CMP_INCL_WT_H_ */
