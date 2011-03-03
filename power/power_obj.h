
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Power object header class
 *        It has all the support classes that are required by various power
 *        accounting classes associated with the major structures
 * initial author: Koushik Chakraborty
 *
 */ 

#ifndef _POWER_OBJECT_H_
#define _POWER_OBJECT_H_


class power_object_t {
    
    private:
    
        basic_circuit_t *bct;
        cacti_time_t * cacti_time;
        leakage_t *leakage_obj;
        
        
        
    public:
    
        power_object_t();
        cacti_time_t *get_cacti_time();
        leakage_t *get_leakage_obj();
        basic_circuit_t *get_basic_circuit();
        
        
};

#endif
