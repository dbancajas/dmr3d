/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_excl_3l.h,v 1.1.2.2 2005/08/02 20:28:46 pwells Exp $
 *
 * description:    common declaration of a CMP exclusive protocol
 * initial author: Philip Wells 
 *
 */
 
#ifndef _CMP_EXCL_3L_H_
#define _CMP_EXCL_3L_H_

////////////////////////////////////////////////////////////////////////////////
// CMP exclusive declarations
//    Common to all caches and protocols
class cmp_excl_3l_protocol_t {
public:

	/// CMP Excl Message Types 
	// L2 -> L3, L1 -> L2
	static const uint32 TypeRead              = 0;
	static const uint32 TypeReadEx            = 1;
	static const uint32 TypeUpgrade           = 2;
	static const uint32 TypeReplace           = 3;
	static const uint32 TypeWriteBackClean    = 4;
	static const uint32 TypeWriteBack         = 5;
    static const uint32 TypeInvAck            = 6;
	// L3 -> L2 or L2 -> L2, or L2 -> L1
    static const uint32 TypeDataRespShared    = 7;
    static const uint32 TypeDataRespExclusive = 8;
	// L3 -> L2, L2 -> L1
    static const uint32 TypeInvalidate        = 9;
	static const uint32 TypeReadFwd           = 10;
    static const uint32 TypeReadExFwd         = 11;
	static const uint32 TypeWriteBackAck      = 12;
    static const uint32 TypeUpgradeAck        = 13;
    static const uint32 TypeReplaceAck        = 14;
    // L2 -> L2 Dir
    static const uint32 TypeReplaceNote       = 15;
	// Required
	static const uint32 TypeStorePref         = 16;
	static const uint32 TypeReadPref          = 17;

	static const uint32 TypeL1DataRespShared  = 18;
	static const uint32 TypeL1InvAck          = 19;

	static const string names[][2];
};

#endif /* _CMP_EXCL_3L_H_ */
