
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Power object class implementation
 *        It has all the support classes that are required by various power
 *        accounting classes associated with the major structures
 * initial author: Koushik Chakraborty
 *
 */ 

 


//  // #include "simics/first.h"
// // RCSID("$Id: power_object.cc,v 1.10.2.17 2006/03/02 23:58:42 kchak Exp $");

#include "definitions.h"
#include "config_extern.h" 
#include "verbose_level.h"

#include "profiles.h"
#include "circuit_param.h"
#include "cacti_def.h"
#include "cacti_interface.h"
#include "basic_circuit.h"
#include "power_defs.h"
#include "cacti_time.h"
#include "leakage.h"
#include "power_obj.h"

power_object_t::power_object_t()
{
    bct = new basic_circuit_t();
    leakage_obj = new leakage_t((double)g_conf_technology_node/1000);
    cacti_time = new cacti_time_t((double)g_conf_technology_node/1000, 
        leakage_obj, bct);
    
    
    
}

cacti_time_t *power_object_t::get_cacti_time()
{
    return cacti_time;
}

leakage_t *power_object_t::get_leakage_obj()
{
    return leakage_obj;
}

basic_circuit_t *power_object_t::get_basic_circuit()
{
    return bct;
    
}

