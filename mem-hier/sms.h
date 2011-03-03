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
 
#ifndef _SMS_H
#define _SMS_H
 
class sms_t {
    private:
        map<addr_t, addr_t> pht;
        map<addr_t, addr_t> filter_table;
        map<addr_t, addr_t> accum_table;
        addr_t sms_region_mask;
        addr_t line_bits;
        
        addr_t get_prediction_tag(addr_t PC, addr_t VA);
        addr_t get_mask(const int min, const int max);
        
        
    public:
        sms_t();
        void record_access(mem_trans_t *trans);
        
        
        
        
    
    
};


#endif
