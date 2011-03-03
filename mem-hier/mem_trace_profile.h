/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: tcache.h,v 1.4.2.14 2005/07/25 17:51:41 kchak Exp $
 *
 * description:    Memory Trace Profile
 * initial author: Koushik Chakraborty 
 *
 */

 
 
class trace_rec_t {
    
    public:
        addr_t mem_addr;
        uint64 request_time;
        addr_t replace_addr;
        bool hit;
        uint8 type;
        trace_rec_t(addr_t _addr, uint64 _time, addr_t _replace, bool _hit, uint8 _type) :
        mem_addr(_addr), request_time(_time), replace_addr(_replace), hit(_hit),
        type(_type) {}
        trace_rec_t(FILE *f);
        
        void to_file(FILE *f);
        void from_file(FILE *f);
        
};


class mem_trace_profile_t : public profile_entry_t {
    
    public:
        void calculate_miss();
        uint64 record_mem_trans(addr_t mem_addr, uint64 request_time, addr_t replace, 
            bool _hit, uint8 type);
        
        mem_trace_profile_t(string name, uint32 _ways, uint32 _sets, cache_t *_cache);
        void update_content_set(addr_t *tag, uint64 index);
        
    
    
    
    private:
    
        uint32 ways;
        uint32 sets;
        cache_t *cache;
        set<addr_t> *content_set;
        map<addr_t, list<uint64> > access_time_list;
        list<trace_rec_t *> *mem_trace;
        list<trace_rec_t *> trace_rec_list;
        set<uint32>   index_set;
     
        st_entry_t *optimal_hit;
        st_entry_t *optimal_miss;
        st_entry_t *stat_hit_ratio;
        st_entry_t *stat_num_requests;
        st_entry_t *stat_index_count;
        st_entry_t *stat_replace_match;
        st_entry_t *stat_cache_hit;
        st_entry_t *stat_self_replace;
        st_entry_t *stat_replace_damage;
        
        
        trace_rec_t *get_trace_rec(addr_t mem_addr, uint64 request_time, 
            addr_t replace, bool _hit, uint8 type);
        void determine_missinfo(trace_rec_t *mem_tr, uint32 set);
        void check_damage(trace_rec_t *mem_tr, uint32 set);
    
    
        // Helper stuff
        list<trace_rec_t *>::iterator mtr_it;
        map<addr_t, list<uint64> >::iterator atl_it;
        uint64 current_trace_count;
        
        
        void to_file(FILE  *f);
        void from_file(FILE *f);
};
