/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 */

/* description:    Checkpoint Util implementation
 * initial author: Koushik Chakraborty 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: checkp_util.cc,v 1.1.2.4 2006/07/28 01:29:51 pwells Exp $");
 
#include "definitions.h"
#include "external.h"
#include "config.h"
#include "config_extern.h"
#include "checkp_util.h"


checkpoint_util_t::checkpoint_util_t()
{
    // Blank
}


//// OUTPUT FUNCTION

void checkpoint_util_t::set_addr_t_to_file(set<addr_t> &my_set, FILE *f)
{
    set<addr_t>::iterator it;
    fprintf(f, "%u \n", my_set.size());
    for (it = my_set.begin(); it != my_set.end(); it++)
    {
        fprintf(f, "%llx ", *it);
    }
    fprintf(f, "\n");
    
}


void checkpoint_util_t::set_addr_t_from_file(set<addr_t> &my_set, FILE *f)
{
    uint32 count;
    fscanf(f, "%u \n", &count);
    addr_t item;
    for (uint32 i = 0; i < count; i++)
    {
        fscanf(f, "%llx ", &item);
        my_set.insert(item);
    }
    fscanf(f, "\n");
}


void checkpoint_util_t::set_uint64_to_file(set<uint64> &my_set, FILE *f)
{
    set<uint64>::iterator it;
    fprintf(f, "%u \n", my_set.size());
    for (it = my_set.begin(); it != my_set.end(); it++)
    {
        fprintf(f, "%llu ", *it);
    }
    fprintf(f, "\n");
    
}


void checkpoint_util_t::set_uint64_from_file(set<uint64> &my_set, FILE *f)
{
    uint32 count;
    fscanf(f, "%u \n", &count);
    uint64 item;
    for (uint32 i = 0; i < count; i++)
    {
        fscanf(f, "%llu ", &item);
        my_set.insert(item);
    }
    fscanf(f, "\n");
}


void checkpoint_util_t::set_uint32_to_file(set<uint32> &my_set, FILE *f)
{
    set<uint32>::iterator it;
    fprintf(f, "%u \n", my_set.size());
    for (it = my_set.begin(); it != my_set.end(); it++)
    {
        fprintf(f, "%u ", *it);
    }
    fprintf(f, "\n");
    
}

void checkpoint_util_t::set_uint32_from_file(set<uint32> &my_set, FILE *f)
{
    uint32 count;
    fscanf(f, "%u \n", &count);
    uint32 item;
    for (uint32 i = 0; i < count; i++)
    {
        fscanf(f, "%u ", &item);
        my_set.insert(item);
    }
    fscanf(f, "\n");
}


void checkpoint_util_t::set_uint8_to_file(set<uint8> &my_set, FILE *f)
{
    set<uint8>::iterator it;
    fprintf(f, "%u \n", my_set.size());
    for (it = my_set.begin(); it != my_set.end(); it++)
    {
        fprintf(f, "%u ", *it);
    }
    fprintf(f, "\n");
    
}

void checkpoint_util_t::set_uint8_from_file(set<uint8> &my_set, FILE *f)
{
    uint32 count;
    fscanf(f, "%u \n", &count);
    uint32 item;
    for (uint32 i = 0; i < count; i++)
    {
        fscanf(f, "%u ", &item);
        my_set.insert((uint8)item);
    }
    fscanf(f, "\n");
}


void checkpoint_util_t::list_addr_t_to_file(list<addr_t> &my_list, FILE *f)
{
    fprintf(f, "%u\n", my_list.size());
    list<addr_t>::iterator it;
    for (it = my_list.begin(); it != my_list.end(); it++)
        fprintf(f, "%llx ", *it);
    fprintf(f, "\n");
}

void checkpoint_util_t::list_tick_t_to_file(list<tick_t> &my_list, FILE *f)
{
    fprintf(f, "%u\n", my_list.size());
    list<tick_t>::iterator it;
    for (it = my_list.begin(); it != my_list.end(); it++)
        fprintf(f, "%llx ", *it);
    fprintf(f, "\n");
}

void checkpoint_util_t::list_uint32_to_file(list<uint32> &my_list, FILE *f)
{
    fprintf(f, "%u\n", my_list.size());
    list<uint32>::iterator it;
    for (it = my_list.begin(); it != my_list.end(); it++)
        fprintf(f, "%u ", *it);
    fprintf(f, "\n");
}


void checkpoint_util_t::list_tick_t_from_file(list<tick_t> &my_list, FILE *f)
{
    uint32 count;
    fscanf(f, "%u\n", &count);
    tick_t item;
    for (uint32 i = 0; i < count; i++)
    {
        fscanf(f, "%llx ", &item);
        my_list.push_back(item);
    }
    fscanf(f, "\n");
}

void checkpoint_util_t::list_addr_t_from_file(list<addr_t> &my_list, FILE *f)
{
    uint32 count;
    fscanf(f, "%u\n", &count);
    addr_t item;
    for (uint32 i = 0; i < count; i++)
    {
        fscanf(f, "%llx ", &item);
        my_list.push_back(item);
    }
    fscanf(f, "\n");
}
    
void checkpoint_util_t::list_uint32_from_file(list<uint32> &my_list, FILE *f)
{
    uint32 count;
    fscanf(f, "%u\n", &count);
    uint32 item;
    for (uint32 i = 0; i < count; i++)
    {
        fscanf(f, "%u ", &item);
        my_list.push_back(item);
    }
    fscanf(f, "\n");
}
    
    

void checkpoint_util_t::map_addr_t_uint64_to_file(map<addr_t, uint64> &my_map, FILE *f)
{
    fprintf(f, "%u\n", my_map.size());
    map<addr_t, uint64>::iterator it;
    for (it = my_map.begin(); it != my_map.end(); it++)
        fprintf(f, "%llx %llu ", it->first, it->second);
    fprintf(f, "\n");
}



void checkpoint_util_t::map_addr_t_uint64_from_file(map<addr_t, uint64> &my_map, FILE *f)
{
    uint32 count;
    addr_t first;
    uint64 second;
    fscanf(f, "%u\n", &count);
    for (uint32 i = 0; i < count; i++)
    {
        fscanf(f, "%llx %llu ", &first, &second);
        my_map[first] = second;
    }
    fscanf(f, "\n");
    
}



void checkpoint_util_t::map_addr_t_uint8_to_file(map<addr_t, uint8> &my_map, FILE *f)
{
    fprintf(f, "%u\n", my_map.size());
    map<addr_t, uint8>::iterator it;
    for (it = my_map.begin(); it != my_map.end(); it++)
        fprintf(f, "%llx %u ", it->first, it->second);
    fprintf(f, "\n");
}



void checkpoint_util_t::map_addr_t_uint8_from_file(map<addr_t, uint8> &my_map, FILE *f)
{
    uint32 count;
    addr_t first;
    uint32 second;
    fscanf(f, "%u\n", &count);
    for (uint32 i = 0; i < count; i++)
    {
        fscanf(f, "%llx %u ", &first, &second);
        my_map[first] = (uint8) second;
    }
    fscanf(f, "\n");
    
}

void checkpoint_util_t::map_addr_t_addr_t_to_file(map<addr_t, addr_t> &my_map, FILE *f)
{
    fprintf(f, "%u\n", my_map.size());
    map<addr_t, addr_t>::iterator it;
    for (it = my_map.begin(); it != my_map.end(); it++)
        fprintf(f, "%llx %llx ", it->first, it->second);
    fprintf(f, "\n");
}


void checkpoint_util_t::map_addr_t_addr_t_from_file(map<addr_t, addr_t> &my_map, FILE *f)
{
    uint32 count;
    addr_t first;
    addr_t second;
    fscanf(f, "%u\n", &count);
    for (uint32 i = 0; i < count; i++)
    {
        fscanf(f, "%llx %llx ", &first, &second);
        my_map[first] = second;
    }
    fscanf(f, "\n");
    
}

void checkpoint_util_t::map_uint32_uint32_to_file(map<uint32, uint32> &my_map, FILE *f)
{
   fprintf(f, "%u\n", my_map.size());
    map<uint32, uint32>::iterator it;
    for (it = my_map.begin(); it != my_map.end(); it++)
        fprintf(f, "%u %u ", it->first, it->second);
    fprintf(f, "\n");
}

void checkpoint_util_t::map_uint64_uint64_to_file(map<uint64, uint64> &my_map, FILE *f)
{
   fprintf(f, "%u\n", my_map.size());
    map<uint64, uint64>::iterator it;
    for (it = my_map.begin(); it != my_map.end(); it++)
        fprintf(f, "%llu %llu ", it->first, it->second);
    fprintf(f, "\n");
}

void checkpoint_util_t::map_uint32_uint32_from_file(map<uint32, uint32> &my_map, FILE *f)
{
    uint32 count;
    uint32 first;
    uint32 second;
    fscanf(f, "%u\n", &count);
    for (uint32 i = 0; i < count; i++)
    {
        fscanf(f, "%u %u ", &first, &second);
        my_map[first] = second;
    }
    fscanf(f, "\n");
}

void checkpoint_util_t::map_uint64_uint64_from_file(map<uint64, uint64> &my_map, FILE *f)
{
    uint32 count;
    uint64 first;
    uint64 second;
    fscanf(f, "%u\n", &count);
    for (uint32 i = 0; i < count; i++)
    {
        fscanf(f, "%llu %llu ", &first, &second);
        my_map[first] = second;
    }
    fscanf(f, "\n");
}
