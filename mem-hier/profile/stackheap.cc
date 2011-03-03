/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Stack Heap reference Profile
 * initial author: Koushik Chakraborty
 *
 */
 
 
 
//  #include "simics/first.h"
// RCSID("$Id: stackheap.cc,v 1.1.2.3 2006/01/30 16:43:35 kchak Exp $");

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
#include "stackheap.h"
#include "mem_hier.h"
#include "checkp_util.h"
#include "transaction.h"

stackheap_t::stackheap_t(string name):
    profile_entry_t(name)
{
    stat_read_reference = stats->COUNTER_GROUP("stat_read_reference", "Read Reference", 4);
    stat_read_l1_miss = stats->COUNTER_GROUP("stat_read_l1_miss", "Read L1 Miss", 4);
    stat_read_l2_miss = stats->COUNTER_GROUP("stat_read_l2_miss", "Read L2 Miss", 4);
    
    stat_write_reference = stats->COUNTER_GROUP("stat_write_reference", "Write Reference", 4);
    stat_write_l1_miss = stats->COUNTER_GROUP("stat_write_l1_miss", "Write L1 Miss", 4);
    stat_write_l2_miss = stats->COUNTER_GROUP("stat_write_l2_miss", "Write L2 Miss", 4);
    
    stat_kread_reference = stats->COUNTER_GROUP("stat_kread_reference", "Kernel Read Reference", 4);
    stat_kread_l1_miss = stats->COUNTER_GROUP("stat_kread_l1_miss", "Kernel Read L1 Miss", 4);
    stat_kread_l2_miss = stats->COUNTER_GROUP("stat_kread_l2_miss", "Kernel Read L2 Miss", 4);
    
    stat_kwrite_reference = stats->COUNTER_GROUP("stat_kwrite_reference", "Kernel Write Reference", 4);
    stat_kwrite_l1_miss = stats->COUNTER_GROUP("stat_kwrite_l1_miss", "Kernel Write L1 Miss", 4);
    stat_kwrite_l2_miss = stats->COUNTER_GROUP("stat_kwrite_l2_miss", "Kernel Write L2 Miss", 4);
    
}

void stackheap_t::record_access(mem_trans_t *trans)
{
    if (trans->random_trans) return;
    l1_miss = trans->occurred(MEM_EVENT_L1_DEMAND_MISS | MEM_EVENT_L1_COHER_MISS |
        MEM_EVENT_L1_STALL |  MEM_EVENT_L1_MSHR_PART_HIT);
    l2_miss = trans->occurred(MEM_EVENT_L2_DEMAND_MISS | MEM_EVENT_L2_COHER_MISS);
    
    if (trans->priv) 
        record_kernel_access(trans);
    else 
        record_user_access(trans);
    
    
}

void stackheap_t::record_user_access(mem_trans_t *trans)
{
    
    uint64 stack_ptr = SIM_read_register(trans->ini_ptr, SIM_get_register_number(trans->ini_ptr, "sp"));
    uint64 frame_ptr = SIM_read_register(trans->ini_ptr, SIM_get_register_number(trans->ini_ptr, "fp"));
    uint32 access_type = 0; /* library data segment */
    // Ignore higher bits
    stack_ptr = (stack_ptr & 0xffffffff);
    frame_ptr = (frame_ptr & 0xffffffff);
    if (stack_ptr < trans->virt_addr && trans->virt_addr < frame_ptr) /* proper stack */
        access_type = 1;
    else if (trans->virt_addr < 0xfd000000) /* heap */
        access_type = 3;
    else if ((trans->virt_addr & 0xffb00000) == 0xffb00000) /* remote stack */
        access_type = 2;
     
    // DEBUG_OUT("VA: %llx SP:%llx FP:%llx Type: %s\n", trans->virt_addr,stack_ptr, 
    //     frame_ptr, (access_type == 0) ? "Anon" : ((access_type == 1) ? "proper_stack" : 
    //         (access_type == 2) ? "Remote Stack" : "Heap"));
    // bool l1_miss = trans->occurred(MEM_EVENT_L1_DEMAND_MISS | MEM_EVENT_L1_COHER_MISS |
    //     MEM_EVENT_L1_STALL |  MEM_EVENT_L1_MSHR_PART_HIT);
    // bool l2_miss = trans->occurred(MEM_EVENT_L2_DEMAND_MISS | MEM_EVENT_L2_COHER_MISS);
    if (trans->write) {
        stat_write_reference->inc_total(1, access_type);
        stat_write_l1_miss->inc_total(l1_miss, access_type);
        stat_write_l2_miss->inc_total(l2_miss, access_type);
    } else {
        stat_read_reference->inc_total(1, access_type);
        stat_read_l1_miss->inc_total(l1_miss, access_type);
        stat_read_l2_miss->inc_total(l2_miss, access_type);
    }
        
    
}

void stackheap_t::record_kernel_access(mem_trans_t *trans)
{
    kernel_access_type_t access_type = trans->get_access_type();
    if (trans->write) {
        stat_kwrite_reference->inc_total(1, access_type);
        stat_kwrite_l1_miss->inc_total(l1_miss, access_type);
        stat_kwrite_l2_miss->inc_total(l2_miss, access_type);
    } else {
        stat_kread_reference->inc_total(1, access_type);
        stat_kread_l1_miss->inc_total(l1_miss, access_type);
        stat_kread_l2_miss->inc_total(l2_miss, access_type);
    }
        
}


void stackheap_t::print_stats()
{
    stats->print();
}

bool stackheap_t::stack_access(mem_trans_t *trans)
{
    if (trans->priv) return false;
    uint64 stack_ptr = SIM_read_register(trans->ini_ptr, SIM_get_register_number(trans->ini_ptr, "sp"));
    uint64 frame_ptr = SIM_read_register(trans->ini_ptr, SIM_get_register_number(trans->ini_ptr, "fp"));
    if (stack_ptr < trans->virt_addr && trans->virt_addr < frame_ptr)
        return true;
    return false;
}
