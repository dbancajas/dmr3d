
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

 /* description:    Power profile header class: base case inherited by all
 * specific power implementations
 * initial author: Koushik Chakraborty
 *
 */ 
 
 

//  #include "simics/first.h"
// RCSID("$Id: power_profile.cc,v 1.4.2.15 2006/11/14 20:02:07 kchak Exp $");

#include "definitions.h" 
#include "profiles.h"
#include "power_profile.h"


power_profile_t::power_profile_t(string name)
: profile_entry_t(name)
{
    
    
}