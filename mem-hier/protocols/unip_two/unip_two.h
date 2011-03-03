/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: unip_two.h,v 1.1.2.1 2005/08/24 19:20:06 pwells Exp $
 *
 * description:    common declaration of a unip two protocol
 * initial author: Philip Wells 
 *
 */
 
#ifndef _UNIP_TWO_H_
#define _UNIP_TWO_H_

////////////////////////////////////////////////////////////////////////////////
// unip two declarations
//    Common to all caches and protocols
class unip_two_protocol_t {
public:

	/// unip two Message Types 
	static const uint32 TypeIFetch			= 0;
	static const uint32 TypeRead			= 1;
	static const uint32 TypeWrite			= 2;
	static const uint32 TypeDataResp		= 3;
    static const uint32 TypeReadPref		= 4;
    static const uint32 TypeStorePref		= 5;
	static const uint32 TypeInvalidate		= 6;

	static const string names[][2];
};

#endif /* _UNIP_TWO_H_ */
