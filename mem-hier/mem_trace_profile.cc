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

 
 
//  #include "simics/first.h"
// RCSID("$Id: mem_trace_profile.cc,v 1.39 2004/09/13 14:19:40 kchak Exp $");

#include "definitions.h"
#include "config_extern.h"
#include "verbose_level.h"
#include "device.h"
#include "cache.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "profiles.h"
#include "mem_trace_profile.h" 
 
static double stat_add2_fn(st_entry_t ** arr) {
	return (arr[0]->get_total() + arr[1]->get_total());
}

static double stat_ratio2_fn(st_entry_t ** arr) {
	return ((double)arr[0]->get_total() / (double)(arr[1]->get_total()));
}


void trace_rec_t::to_file(FILE *f)
{
    
    fprintf(f, "%llx %llu %llx %u %u\n",mem_addr, request_time, replace_addr, 
        (uint32)hit, (uint32) type); 
}


void trace_rec_t::from_file(FILE *f)
{
    uint32 i, j;
    fscanf(f, "%llx %llu %llx %u %u\n", &mem_addr, &request_time, &replace_addr,
        &i, &j);
    hit = (bool) i;
    type = (uint8) j;    
}

trace_rec_t::trace_rec_t(FILE *f)
{
    from_file(f);
}

mem_trace_profile_t::mem_trace_profile_t(string name, uint32 _ways, uint32 _sets, cache_t *_cache) 
    : profile_entry_t(name), ways(_ways), sets(_sets), cache(_cache)
{
    content_set = new set<addr_t>[sets];
    mem_trace = new list<trace_rec_t *>[sets];
    optimal_hit = stats->COUNTER_BASIC("optimal_hit", "Optimal Hit");
    optimal_miss = stats->COUNTER_BASIC("optimal_miss", "optimal_miss");
    stat_cache_hit = stats->COUNTER_GROUP("cache_hit_profile", "Cache Hit Info", 2);
    
    st_entry_t *stat_num_request_arr[] = {
		optimal_hit, 
		optimal_miss,
	};
    
    stat_num_requests = stats->STAT_PRINT("requests", "# of reads", 3, 
		stat_num_request_arr, &stat_add2_fn, true);
    st_entry_t *stat_hit_ratio_arr[] = {
		optimal_hit,
        stat_num_requests
	};
	stat_hit_ratio = stats->STAT_PRINT("hit_ratio",
		"ratio of requests that hit", 2, stat_hit_ratio_arr,
		&stat_ratio2_fn, false);

    stat_index_count = stats->COUNTER_BASIC("stat_index_count", "# of Sets considered");
    stat_self_replace = stats->COUNTER_GROUP("stat_self_replace", "# of self replacements", 4);
    stat_replace_match = stats->COUNTER_GROUP("stat_replace_match", "Replace Match", 4);
    stat_replace_damage = stats->COUNTER_GROUP("stat_replace_damage", "Damage done due to replacement", 5);
    atl_it = access_time_list.end();
    current_trace_count = 0;
}
 
void mem_trace_profile_t::calculate_miss()
{
    for (uint32 i = 0; i < sets; i++)
    {
        // Populate Content Set
        while (mem_trace[i].size()) {
            trace_rec_t * mem_tr = mem_trace[i].front();
            mem_trace[i].pop_front();
            determine_missinfo(mem_tr, i);
            trace_rec_list.push_back(mem_tr);
        }
    }
    access_time_list.clear();
    atl_it = access_time_list.end();
    current_trace_count = 0;
}

trace_rec_t *mem_trace_profile_t::get_trace_rec(addr_t mem_addr, uint64 request_time,
    addr_t replace, bool _hit, uint8 _type)
{
    trace_rec_t *ret;
    if (trace_rec_list.size())
    {
        ret = trace_rec_list.front();
        trace_rec_list.pop_front();
        ret->mem_addr = mem_addr;
        ret->request_time = request_time;
        ret->replace_addr = replace;
        ret->hit = _hit;
        ret->type = _type;
        return ret;
    }
    
    ret = new trace_rec_t(mem_addr, request_time, replace, _hit, _type);
    return ret;
}


uint64 mem_trace_profile_t::record_mem_trans(addr_t mem_addr, uint64 request_time, 
    addr_t replace, bool _hit, uint8 _type)
{
    current_trace_count++;
    addr_t line_addr = cache->get_line_address(mem_addr);
    trace_rec_t *trace_rec = get_trace_rec(line_addr, request_time, replace, _hit, _type);
    uint32 index = cache->get_index(mem_addr);
    mem_trace[index].push_back(trace_rec);
    if (mem_trace[index].size() > 1000 * ways)
    {
        trace_rec = mem_trace[index].front();
        mem_trace[index].pop_front();
        determine_missinfo(trace_rec, index);
        trace_rec_list.push_back(trace_rec);
    }
    
   
    return current_trace_count;
       
}

void mem_trace_profile_t::update_content_set(addr_t *tag, uint64 index)
{
    /*
    content_set[index].clear();
    for (uint32 i = 0; i < ways; i++)
        content_set[index].insert(tag[i]);
    */
}

void mem_trace_profile_t::check_damage(trace_rec_t *mem_tr, uint32 index)
{
    set<addr_t> unique_reference_set;
    list<trace_rec_t *>::iterator it = mem_trace[index].begin();
    while (it != mem_trace[index].end())
    {
        trace_rec_t *rec = *it;
        if (!g_conf_only_read_lookahead || rec->type)
            unique_reference_set.insert(rec->mem_addr);
        if (unique_reference_set.size() == ways)
            break;
        it++;
    }
    
    //ASSERT(unique_reference_set.size() == ways);
    if (unique_reference_set.find(mem_tr->replace_addr) !=
        unique_reference_set.end())
    {
        // Damage done by this replacement   
        
        // Check if no-allocate was the correct choice
        if (unique_reference_set.find(mem_tr->mem_addr) ==
            unique_reference_set.end())
            stat_replace_damage->inc_total(1, mem_tr->type);
        else
            stat_replace_damage->inc_total(1, 3);
    } else {
        stat_replace_damage->inc_total(1, 4);
    }
    
}

void mem_trace_profile_t::determine_missinfo(trace_rec_t *mem_tr, uint32 index)
{
    addr_t mem_addr = mem_tr->mem_addr;
    index_set.insert(index);
    STAT_SET(stat_index_count, index_set.size());
    //uint64 curr_access_time = mem_tr->request_time;
    
    
    if (mem_tr->hit)
        stat_cache_hit->inc_total(1, 0);
    else
        stat_cache_hit->inc_total(1, 1);
    
    check_damage(mem_tr, index);
    if (content_set[index].find(mem_addr) != content_set[index].end())
    {
        STAT_INC(optimal_hit);
    } else {
        STAT_INC(optimal_miss);
        // Bootstrap problem ....
        if (content_set[index].size() < ways) {
            content_set[index].insert(mem_addr);
            return;
        }
        
        // Finding a Replacement Dude
         // Intermediate Address from the present content set
         
         
        // New Algorithm .. without requiring the access_time_list
        
        set<addr_t> replacement_set = content_set[index];
        
        list<trace_rec_t *>::iterator it = mem_trace[index].begin();
        while (it != mem_trace[index].end() && replacement_set.size())
        {
            trace_rec_t *rec = *it;
            if (rec->mem_addr == mem_addr)
                break;
            if (content_set[index].find(rec->mem_addr) != content_set[index].end())
                replacement_set.erase(rec->mem_addr);
            it++;   
        }
        
        addr_t replace_addr = mem_addr;
        if (replacement_set.size() && it != mem_trace[index].end())
        {
            
            // Replacement necessary
            
            while (it != mem_trace[index].end())
            {
                trace_rec_t *rec = *it;
                if (replacement_set.find(rec->mem_addr) != replacement_set.end())
                {
                    replace_addr = rec->mem_addr;
                    replacement_set.erase(rec->mem_addr);
                }
                it++;
            }
            // Replacement set consists of addresses for which
            // no future transaction found as far as we can lookahead
            if (replacement_set.size())
                replace_addr = *(replacement_set.begin());
            
            // Replacement
            content_set[index].erase(replace_addr);
            content_set[index].insert(mem_addr);
            
        }
        
        if (replace_addr == mem_addr) 
            stat_self_replace->inc_total(1, 1 + mem_tr->type);
        else
            stat_self_replace->inc_total(1, 0);
        
        
        if (mem_tr->replace_addr) {
            if (mem_tr->replace_addr == replace_addr)
                stat_replace_match->inc_total(1, 0);
            else {
                if (content_set[index].find(mem_tr->replace_addr) !=
                content_set[index].end()) {
                    if (replace_addr == mem_addr)
                        stat_replace_match->inc_total(1, 1);
                    else
                        stat_replace_match->inc_total(1, 2);
                } else {
                    stat_replace_match->inc_total(1, 3);
                }
                    
            }
                    
        }
    
    
    }
   
}


void mem_trace_profile_t::to_file(FILE *f)
{
    fprintf(f, "%s\n", typeid(this).name());
		
    for (uint32 i = 0; i < sets; i++)
    {
        list<trace_rec_t *>::iterator it;
        fprintf(f, "%u\n", mem_trace[i].size());
        for (it = mem_trace[i].begin(); it != mem_trace[i].end(); it++)
        {
            trace_rec_t *tr_rec = *it;
            tr_rec->to_file(f);
            delete tr_rec;
        }
    }
}

void mem_trace_profile_t::from_file(FILE *f)
{
    char classname[g_conf_max_classname_len];
	fscanf(f, "%s\n", classname);

    if (strcmp(classname, typeid(this).name()) != 0)
        cout << "Read " << classname << " Require " << typeid(this).name() << endl;
	ASSERT(strcmp(classname, typeid(this).name()) == 0);
	
    uint32 count;
    trace_rec_t *rec;
    for (uint32 i = 0; i < sets; i++)
    {
        fscanf(f, "%u\n", &count);
        for (uint32 j = 0; j < count; j++) {
            rec = new trace_rec_t(f);
            mem_trace[i].push_back(rec);
        }
    }
}


