/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: codeshare.h,v 1.1.2.3 2006/01/30 16:43:35 kchak Exp $
 *
 * description:    Class for code page profiling
 * initial author: Koushik Chakraborty 
 *
 */
 
#ifndef _FUNCTION_PROFILE_H_
#define _FUNCTION_PROFILE_H_



class function_profile_t : public profile_entry_t {

private:

	
	
public:
	function_profile_t(string name);
	void parse_branch_recorder();
	
	
	
};

#endif // _FUNCTION_PROFILE_H_
