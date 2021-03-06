/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */
 
/* $Id: prefetch_engine.cc,v 1.2.2.4 2005/11/03 15:02:55 pwells Exp $
 *
 * description:    Common prefetch engine attached to every cache
 * initial author: Koushik Chakraborty
 *
 */
 
#include "definitions.h"
#include "config_extern.h"
#include "protocols.h"
#include "device.h"
#include "mem_hier.h"
#include "transaction.h"



template <class cache_t>
prefetch_engine_t<cache_t>::
prefetch_engine_t(cache_t *_cache, uint64 _prefetchalg)
	: cache(_cache), prefetch_algorithm(_prefetchalg)
{
}


template<class cache_t>
void
prefetch_engine_t<cache_t>::initiate_prefetch(mem_trans_t *trans, bool up_msg)
{

    // All prefetchers are on up_msg for now. This can be changed later if needed
    if (!up_msg) return;
    if ((prefetch_algorithm & ICACHE_STREAM_PREFETCH) == ICACHE_STREAM_PREFETCH)
        icache_stream_prefetch(trans);
    if ((prefetch_algorithm & DCACHE_STRIDE_PREFETCH) == DCACHE_STRIDE_PREFETCH)
        dcache_stride_prefetch(trans);
}

template<class cache_t>
void
prefetch_engine_t<cache_t>::icache_stream_prefetch(mem_trans_t *trans)
{
    if (g_conf_l1i_next_line_prefetch && trans && trans->ifetch && !trans->is_prefetch()
        && trans->occurred(MEM_EVENT_L1_DEMAND_MISS))
    {
        addr_t addr = cache->get_line_address(trans->phys_addr);
        for (uint32 n = 0; n < g_conf_l1i_next_line_pref_num; n++) {

			addr += cache->get_lsize();
			mem_trans_t *prefetch_trans = mem_hier_t::ptr()->get_mem_trans();
            prefetch_trans->copy(*trans);
            prefetch_trans->call_complete_request = 0;
            prefetch_trans->cache_prefetch = 1;
            prefetch_trans->phys_addr = addr;
            prefetch_trans->size = cache->get_lsize();
            prefetch_trans->read = 1;
            prefetch_trans->write = 0;
            prefetch_trans->dinst = NULL;
            prefetch_trans->ifetch = 1;
            prefetch_trans->completed = false;
            prefetch_trans->clear_all_events();
            prefetch_trans->pending_messages = 1;
            cache->prefetch_helper(addr, prefetch_trans, true);
		}
        
    }
    
}

template<class cache_t>
void
prefetch_engine_t<cache_t>::dcache_stride_prefetch(mem_trans_t *trans)
{
    // DEBUG_OUT("%s Got here %s %s!\n", cache->get_name().c_str(), trans->is_prefetch() ? "Pref" : "NoPref",
    //     trans->occurred(MEM_EVENT_L1DIR_DEMAND_MISS) ? "Miss" : "Hit");
    
    if (trans->is_prefetch()) return;
    if (!trans->occurred(MEM_EVENT_L1DIR_DEMAND_MISS)) return;
    // DEBUG_OUT("NOW initiating\n");
    addr_t addr = cache->get_line_address(trans->phys_addr) + cache->get_lsize();
    mem_trans_t *prefetch_trans = mem_hier_t::ptr()->get_mem_trans();
    prefetch_trans->copy(*trans);
    prefetch_trans->call_complete_request = 0;
    prefetch_trans->cache_prefetch = 1;
    prefetch_trans->phys_addr = addr;
    prefetch_trans->size = cache->get_lsize();
    prefetch_trans->read = 1;
    prefetch_trans->write = 0;
    prefetch_trans->dinst = NULL;
    prefetch_trans->ifetch = 0;
    prefetch_trans->completed = true;
    prefetch_trans->clear_all_events();
    prefetch_trans->pending_messages = 1;
    
    cache->prefetch_helper(addr, prefetch_trans, true);
    
}
