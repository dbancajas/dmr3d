/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_excl.h,v 1.1.2.3 2006/02/10 21:08:55 pwells Exp $
 *
 * description:    common declaration of a CMP exclusive protocol
 * initial author: Philip Wells 
 *
 */
 
#ifndef _CMP_EXCL_H_
#define _CMP_EXCL_H_

////////////////////////////////////////////////////////////////////////////////
// CMP exclusive declarations
//    Common to all caches and protocols
class cmp_excl_protocol_t {
public:

	/// CMP Excl Message Types 
	// L1 -> L2
	static const uint32 TypeRead              = 0;
	static const uint32 TypeReadEx            = 1;
	static const uint32 TypeUpgrade           = 2;
	static const uint32 TypeReplace           = 3;
	static const uint32 TypeWriteBackClean    = 4;
	static const uint32 TypeWriteBack         = 5;
    static const uint32 TypeInvAck            = 6;
	// L2 -> L1 or L1 -> L1 
    static const uint32 TypeDataRespShared    = 7;
    static const uint32 TypeDataRespExclusive = 8;
	// L2 -> L1
    static const uint32 TypeInvalidate        = 9;
	static const uint32 TypeReadFwd           = 10;
    static const uint32 TypeReadExFwd         = 11;
	static const uint32 TypeWriteBackAck      = 12;
    static const uint32 TypeUpgradeAck        = 13;
    static const uint32 TypeReplaceAck        = 14;
	// Require
	static const uint32 TypeStorePref         = 15;
	static const uint32 TypeReadPref          = 16;

	// L1 -> L2
    static const uint32 TypeReadNoCache       = 17;
    static const uint32 TypeWriteNoCache      = 18;
	

	static const string names[][2];
};

#endif /* _CMP_EXCL_H_ */
