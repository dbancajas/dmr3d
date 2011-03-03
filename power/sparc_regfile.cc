
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Implementation file: SPARC regfile for power estimation in ms2sim
 * initial author: Koushik Chakraborty
 *
 */ 
 

 

//  #include "simics/first.h"
// RCSID("$Id: sparc_regfile.cc,v 1.10.2.17 2006/03/02 23:58:42 kchak Exp $");

#include "definitions.h"
#include "config_extern.h" 
 

#include <math.h>
#include "isa.h"
#include "dynamic.h"
#include "counter.h"
#include "stats.h"
#include "profiles.h"
#include "basic_circuit.h"
#include "cacti_interface.h"

#include "sparc_regfile.h"
 

sparc_regfile_t::sparc_regfile_t(uint32 _num, uint32 _w, string s_name)
    : shortname(s_name), data_width(_w), num_regs(_num)
{
	regfile_total_power = 0;
	regfile_total_power_cc1 = 0;
	regfile_total_power_cc2 = 0;
	regfile_total_power_cc3 = 0;
	total_regfile_access = 0;
	max_regfile_access = 0;
    
    result = new result_type();
    area_result = new arearesult_type() ;
    ar_subbank = new area_type();
    
    regfile_access = 0;
    total_pop_count_cycle = 0;
    num_pop_count_cycle = 0;
    af = 0;
    
}

uint32 sparc_regfile_t::get_num_regs()
{
    return num_regs;
}

void sparc_regfile_t::increment_access()
{
    regfile_access++;
}



