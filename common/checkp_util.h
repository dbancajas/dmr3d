/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: checkp_util.h,v 1.1.2.4 2006/07/28 01:29:52 pwells Exp $
 *
 * description:    Utility Class for checkpointing
 * initial author: Koushik Chakraborty
 *
 */
 
#ifndef _CHECKP_UTIL_H_
#define _CHECKP_UTIL_H_

class checkpoint_util_t {
    public:
        checkpoint_util_t();
        
        // OUTPUT FUNCTIONS
        
        // set
        void set_addr_t_to_file(set<addr_t> &, FILE *f);
        void set_uint64_to_file(set<uint64> &, FILE *f);
        void set_uint32_to_file(set<uint32> &, FILE *f);
        void set_uint8_to_file(set<uint8> &, FILE *f);
        // list
        void list_addr_t_to_file(list<addr_t> &, FILE *f);
        void list_tick_t_to_file(list<tick_t> &, FILE *f);
        void list_uint32_to_file(list<uint32> &, FILE *f);
        
        
        void map_addr_t_uint64_to_file(map<addr_t, uint64> &, FILE *f);
        void map_addr_t_addr_t_to_file(map<addr_t, addr_t> &, FILE *f);
        void map_addr_t_uint8_to_file(map<addr_t, uint8> &, FILE *f);
        void map_uint32_uint32_to_file(map<uint32, uint32> &, FILE *f);
        void map_uint64_uint64_to_file(map<uint64, uint64> &, FILE *f);
        
        
        
        
        // INPUT FUNCTIONS
        void set_addr_t_from_file(set<addr_t> &, FILE *f);
        void set_uint64_from_file(set<uint64> &, FILE *f);
        void set_uint32_from_file(set<uint32> &, FILE *f);
        void set_uint8_from_file(set<uint8> &, FILE *f);
        
        
        // list
        void list_addr_t_from_file(list<addr_t> &, FILE *f);
        void list_tick_t_from_file(list<tick_t> &, FILE *f);
        void list_uint32_from_file(list<uint32> &, FILE *f);
        
        // Map
        void map_addr_t_uint64_from_file(map<addr_t, uint64> &, FILE *f);
        void map_addr_t_addr_t_from_file(map<addr_t, addr_t> &, FILE *f);
        void map_addr_t_uint8_from_file(map<addr_t, uint8> &, FILE *f);
        void map_uint32_uint32_from_file(map<uint32, uint32> &, FILE *f);
        void map_uint64_uint64_from_file(map<uint64, uint64> &, FILE *f);
        
};

#endif // _MEM_HIER_H_
