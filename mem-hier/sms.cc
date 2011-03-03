/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cache.h,v 1.1.1.1.10.5 2006/03/03 23:33:01 kchak Exp $
 *
 * description:    Spatial Memory Streaming Class
 * initial author: Koushik Chakraborty
 *
 */


//  #include "simics/first.h"
// RCSID("$Id: mem_hier.cc,v 1.13.2.60 2006/03/29 03:01:28 kchak Exp $");
#include "definitions.h"
#include "config.h"
#include "config_extern.h"
#include "mem_hier.h"
#include "transaction.h"
#include "isa.h"
#include "profiles.h"
#include "dynamic.h"
#include "external.h"
#include "debugio.h"
#include "verbose_level.h"
#include "sms.h" 
 
sms_t::sms_t()
{
     sms_region_mask = get_mask(0, (const int)g_conf_sms_region_bits);
     line_bits = (addr_t) log2(g_conf_l1d_lsize);
     
}


void sms_t::record_access(mem_trans_t *trans)
{
    // Ignore instruction fetch
    if (trans->ifetch || trans->io_device) return;
    
    addr_t PC = 0;
    if (g_conf_use_processor) {
        PC = trans->dinst->get_pc();
    } else {
        PC = SIM_get_program_counter(trans->ini_ptr);
    }
    ASSERT(PC);
    addr_t prediction_tag = get_prediction_tag(PC, trans->virt_addr);
    
    
}


addr_t sms_t::get_prediction_tag(addr_t PC, addr_t VA)
{
    addr_t region_offset = (VA & sms_region_mask) >> line_bits;
    addr_t pred_tags = (PC << g_conf_sms_region_bits) | region_offset;
    DEBUG_OUT("PC:%llx SHIFT_PC:%llx VA:%llx region_offset:%llx pred_tag:%llx\n", 
        PC, (PC << g_conf_sms_region_bits), VA,region_offset, pred_tags);
    return pred_tags;
}

addr_t sms_t::get_mask(const int min, const int max) 
{
	if (max == -1)
		return (~0 << min);
	else 
		return ((~0 << min) & ~(~0 << max));
}
