/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Code share implementation
 * initial author: Koushik Chakraborty
 *
 */
 
 
 
//  #include "simics/first.h"
// RCSID("$Id: function_profile.cc,v 1.1.2.3 2006/01/30 16:43:35 kchak Exp $");

#include "definitions.h"
#include "config_extern.h"
#include "profiles.h"
#include "device.h"
#include "external.h"
#include "transaction.h"
#include "config.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "profiles.h"
#include "function_profile.h"
#include "mem_hier.h"
#include "checkp_util.h"
#include "isa.h"
#include "instruction_info.h"


function_profile_t::function_profile_t(string _name) :
	profile_entry_t(_name)
{
    parse_branch_recorder();
   
}

void function_profile_t::parse_branch_recorder()
{
    addr_t source, target;
    uint32 count;
    char type[16];
    conf_object_t *cpu = SIM_proc_no_2_ptr(0);
    inst_info_t *i_info = new inst_info_t();
    DEBUG_OUT("cpu privilege level : %u\n", SIM_cpu_privilege_level(cpu));
    
    FILE *file = fopen("my_file", "r");
    uint32 success = 0;
    while (fscanf(file, "%llx %llx %u %s\n", &source, &target, &count, type) == 4)
    {
        success++;
        source = source & 0xffffffff;
        if (source > 0x1000000) continue;
        try {
                
                addr_t phys_addr = SIM_logical_to_physical(cpu, Sim_DI_Instruction, source);
                
                //addr_t phys_addr = source;
                if (phys_addr) {
                    i_info->decode_instruction_info(phys_addr, cpu);
                    if (i_info->get_branch_type() == BRANCH_CALL)
                    DEBUG_OUT("callee %llx target %llx frequency %u\n",
                        source, target, count);
                } else {
                    DEBUG_OUT("Could not translate source %llx\n", source);
                }
            
        } catch (int e) {
            
            
        }
        
        
    }
    
}



