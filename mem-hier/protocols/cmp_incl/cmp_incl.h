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
 
#ifndef _CMP_INCL_H_
#define _CMP_INCL_H_

////////////////////////////////////////////////////////////////////////////////
// CMP inclusive declarations
//    Common to all caches and protocols
class cmp_incl_protocol_t {
public:

	/// CMP Incl Message Types 
	// L1 -> L2
	static const uint32 TypeRead              = 0;
	static const uint32 TypeReadEx            = 1;
	static const uint32 TypeUpgrade           = 2;
	static const uint32 TypeReplace           = 3;
	static const uint32 TypeWriteBackClean    = 4;
	static const uint32 TypeWriteBack         = 5;
    static const uint32 TypeInvAck            = 6;
	static const uint32 TypeFwdAck            = 7;
	static const uint32 TypeFwdNack           = 8;
	static const uint32 TypeFwdAckData        = 9;
	// L2 -> L1 or L1 -> L1 
    static const uint32 TypeDataRespShared    = 10;
    static const uint32 TypeDataRespExclusive = 11;
	// L2 -> L1
    static const uint32 TypeInvalidate        = 12;
	static const uint32 TypeReadFwd           = 13;
    static const uint32 TypeReadExFwd         = 14;
	static const uint32 TypeWriteBackAck      = 15;
    static const uint32 TypeUpgradeAck        = 16;
    static const uint32 TypeReplaceAck        = 17;
	// Required
	static const uint32 TypeStorePref         = 18;
	static const uint32 TypeReadPref          = 19;
	

	static const string names[][2];
};

#endif /* _CMP_INCL_H_ */
