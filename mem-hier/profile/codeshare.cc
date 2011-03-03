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
// RCSID("$Id: codeshare.cc,v 1.1.2.3 2006/01/30 16:43:35 kchak Exp $");

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
#include "codeshare.h"
#include "mem_hier.h"
#include "checkp_util.h"


addr_t
codeshare_t::get_mask(const int min, const int max) 
{
	if (max == -1)
		return (~0 << min);
	else 
		return ((~0 << min) & ~(~0 << max));
}

codeshare_t::codeshare_t(string _name, uint32 _nump) :
	profile_entry_t(_name), num_procs(_nump)
{

    code_block_trace = new map<addr_t, uint64>[num_procs];
    page_access_trace = new map<addr_t, uint64>[num_procs];
    fetch_ptr = new map<addr_t, uint64>::iterator [num_procs];
    page_ptr = new map<addr_t, uint64>::iterator [num_procs];
    icache_mask = get_mask (0, 6); // 64-byte cache line size
    page_mask = get_mask (0, 13); // 8K page size
	
	// Initialize the counter
    stat_page_share = stats->HISTO_UNIFORM("Page-share-histo", "Histogram of page sharing info", num_procs, 1, 1);
	stat_code_block_share = stats->HISTO_UNIFORM("code_block_share", "Code Block sharing", num_procs, 1, 1);
    stat_user_block_share = stats->HISTO_UNIFORM("user_block_share", "User Code Blocks Share", num_procs, 1, 1);
    stat_os_block_share = stats->HISTO_UNIFORM("os_block_share", "OS block share", num_procs, 1, 1);
    stat_dynamic_code_share = stats->HISTO_UNIFORM("Weighted_code_block_share", "Weighted Code Block sharing", num_procs, 1, 1);
    
    for (uint32 i = 0; i < num_procs; i++)
    {
        fetch_ptr[i] = code_block_trace[i].end();
        page_ptr[i] = page_access_trace[i].end();
    }
}


void
codeshare_t::record_fetch(mem_trans_t *trans)
{
    uint8 cpu = trans->cpu_id;
    addr_t cache_line_addr = (trans->phys_addr & (~icache_mask));
    addr_t page_addr = (trans->phys_addr & (~page_mask));
    
    // Block level marking
    if (fetch_ptr[cpu] != code_block_trace[cpu].end())
    {
        if (fetch_ptr[cpu]->first == cache_line_addr)
        { 
            // do nothing;
            fetch_ptr[cpu]->second++;
        } else {
            map<addr_t, uint64>::iterator it;
            bool add_new_entry = true;
            if (fetch_ptr[cpu]->first < cache_line_addr)
            {
                for (it = fetch_ptr[cpu];it != code_block_trace[cpu].end(); it++)
                {
                    if (it->first == cache_line_addr)
                    {
                        it->second++;
                        add_new_entry = false;
                        fetch_ptr[cpu] = it;
                        break;
                    }
                }
            } else {
                for (it = code_block_trace[cpu].begin(); it != fetch_ptr[cpu]; it++)
                {
                    if (it->first == cache_line_addr) {
                        it->second++;
                        add_new_entry = false;
                        fetch_ptr[cpu] = it;
                        break;
                    }
                }
            }
            if (add_new_entry) {
                code_block_trace[cpu][cache_line_addr] = 1;
                fetch_ptr[cpu] = code_block_trace[cpu].find(cache_line_addr);
                if (trans->priv) os_instr_blocks.insert(cache_line_addr);
            }
        }
    } else {
        code_block_trace[cpu][cache_line_addr] = 1;
        fetch_ptr[cpu] = code_block_trace[cpu].find(cache_line_addr);
        if (trans->priv) os_instr_blocks.insert(cache_line_addr);
    }
    
    // Page marking should be light weight and mark pages only for user code
    if (trans->priv == 0)
    {
        if (page_ptr[cpu] != page_access_trace[cpu].end() &&
            page_ptr[cpu]->first == page_addr)
        {
            page_ptr[cpu]->second++;
        } else {
            page_access_trace[cpu][page_addr]++;
            page_ptr[cpu] = page_access_trace[cpu].find(page_addr);
        }
    }
    
}
    
   
void
codeshare_t:: print_stats()
{
    //ASSERT(report_count == num_processors);
    //report_count = 0;
	gather_stats();
	stats->print();
    stats->clear();
    
}

void
codeshare_t::gather_stats()
{
	map<addr_t, uint32> unified_block_trace;
    for (uint32 i = 0; i < num_procs; i++)
    {
        map<addr_t, uint64>::iterator it;
        for (it = code_block_trace[i].begin(); it != code_block_trace[i].end(); it++)
        {
            unified_block_trace[it->first]++;   
        }
    }
    map<addr_t, uint32> unified_page_trace;
    for (uint32 i = 0; i < num_procs; i++)
    {
        map<addr_t, uint64>::iterator it;
        for (it = page_access_trace[i].begin(); it != page_access_trace[i].end(); it++)
        {
            unified_page_trace[it->first]++;   
        }
    }
    
    map<addr_t, uint32>::iterator u_it;
    for (u_it = unified_block_trace.begin(); u_it != unified_block_trace.end(); u_it++)
    {
        stat_code_block_share->inc_total(1, u_it->second);  
        if (os_instr_blocks.find(u_it->first) != os_instr_blocks.end())
            stat_os_block_share->inc_total(1, u_it->second);
        else
            stat_user_block_share->inc_total(1, u_it->second);
    }
    for (u_it = unified_page_trace.begin(); u_it != unified_page_trace.end(); u_it++)
    {
        stat_page_share->inc_total(1, u_it->second);   
    }
}

void
codeshare_t::to_file(FILE *file)
{
    checkpoint_util_t *cp = new checkpoint_util_t();
    for (uint32 i = 0; i < num_procs; i++)
    {
        cp->map_addr_t_uint64_to_file(page_access_trace[i], file);
        cp->map_addr_t_uint64_to_file(code_block_trace[i], file);
    }
    cp->set_addr_t_to_file( os_instr_blocks, file);
    delete cp;
}

void
codeshare_t::from_file(FILE *file)
{
    checkpoint_util_t *cp = new checkpoint_util_t();
    for (uint32 i = 0; i < num_procs; i++)
    {
        cp->map_addr_t_uint64_from_file(page_access_trace[i], file);
        cp->map_addr_t_uint64_from_file(code_block_trace[i], file);
    }
    cp->set_addr_t_from_file(os_instr_blocks, file);
    delete cp;
}

