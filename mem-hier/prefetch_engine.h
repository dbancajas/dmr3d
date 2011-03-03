/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: prefetch_engine.h,v 1.2 2005/03/10 17:04:52 kchak Exp $
 *
 * description:    Common prefetch engine attached to every cache
 * initial author: Koushik Chakraborty
 *
 */
 
#ifndef _PREFETCH_ENGINE_H_
#define _PREFETCH_ENGINE_H_

const uint64 ICACHE_STREAM_PREFETCH = 1<<0;
const uint64 DCACHE_STRIDE_PREFETCH = 1<<1;



template <class cache_t>
class prefetch_engine_t {
    
    private:
    cache_t *cache;
    uint64  prefetch_algorithm;
    void icache_stream_prefetch(mem_trans_t *trans);
    void dcache_stride_prefetch(mem_trans_t *trans);
    
    
    public:
    prefetch_engine_t(cache_t *cache, uint64 prefetch_alg);
    void initiate_prefetch(mem_trans_t *trans, bool up_msg);
    
};

    
    
// Include template CC
#include "prefetch_engine.cc"

#endif /* _PREFETCH_ENGINE_H_ */
