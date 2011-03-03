
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
 
 
#ifndef _POWER_PROFILE_H_
#define _POWER_PROFILE_H_



class power_profile_t : public profile_entry_t {
    public:
        power_profile_t(string name);
        virtual double get_current_power() = 0;
        virtual double get_current_switching_power() = 0;
        virtual double get_current_leakage_power() = 0;
        
};

#endif
