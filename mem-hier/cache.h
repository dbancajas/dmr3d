/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cache.h,v 1.1.1.1.10.6 2006/07/28 01:29:54 pwells Exp $
 *
 * description:    base abstract cache class
 * initial author: Philip Wells 
 *
 */
 
#ifndef _CACHE_H_
#define _CACHE_H_

// Cache configuration structure
struct cache_config_t {
	cache_config_t() : size(0), lsize(0), assoc(0), banks(0), bank_bw(0),
		bank_stripe_bits(0), req_q_size(0), wait_q_size(0), num_mshrs(0),
		requests_per_mshr(0), lookup_latency(0), writeback_buffersize(0), bank_leak_energy(5),
        bits_out(64)
	{ }
	
	char * name;
	uint32 size;
	uint32 lsize;
	uint32 assoc;
	uint32 banks;
	uint32 bank_bw;
	uint32 bank_stripe_bits;
	uint32 req_q_size;
	uint32 wait_q_size;
	uint32 num_mshrs;
	uint32 requests_per_mshr;
	uint32 lookup_latency;
	uint32 writeback_buffersize;
    uint32 bank_leak_energy; // in Mw
    uint32 bits_out;
};


// Base cache class with basic functionality
class cache_t : public device_t {

public:	
	cache_t(string name, uint32 _num_links, cache_config_t *conf);
	virtual ~cache_t();

	// Utility functions
	addr_t get_size();
	addr_t get_tag(const addr_t addr);
	addr_t get_index(const addr_t addr);
	addr_t get_offset(const addr_t addr);
	addr_t get_address(const addr_t tag, const addr_t index, const addr_t offset);
	addr_t get_line_address(const addr_t addr);
	uint32 get_bank(const addr_t addr);

	addr_t get_mask(const int min, const int max);

	uint32 get_lsize();
	
	// Helper for non-line bank striping 
	addr_t insert_bits(addr_t address, addr_t middle, uint32 mid_start,
		uint32 mid_bits);
	addr_t remove_bits(addr_t address, uint32 mid_start, uint32 mid_bits);
	
	
	
	virtual stall_status_t process_message(message_t *message) = 0;
	
protected:

	// Cache parameters
	uint32 numlines;
	uint32 lsize;
	uint32 assoc;
	uint32 num_banks;
	uint32 bank_bw;
	uint32 request_q_size;
	uint32 wait_q_size;
	
	uint32 numlines_bits;
	uint32 lsize_bits;
	uint32 assoc_bits;
	uint32 bank_bits;
	uint32 bank_low_bit;

	addr_t tag_mask;
	uint32 tag_shift;
	addr_t index_mask;
	uint32 index_shift;

	addr_t offset_mask;
	addr_t bank_mask;
	uint32 numwb_buffers; // Number of WriteBack Buffer
	
    
    bool non_power_sets;
    bool non_power_banks;
    uint32 num_sets;
    
    // Power profile
    cache_power_t *cache_power;

    
    addr_t get_non_power_index(const addr_t);
    
public:
	// Stats
	st_entry_t *stat_num_read_hit;
    st_entry_t *stat_num_read_coher_miss;
    st_entry_t *stat_num_read_coher_miss_kernel;
	st_entry_t *stat_num_read_miss;
    st_entry_t *stat_num_read_miss_kernel;
	st_entry_t *stat_num_read_part_hit_mshr;
	st_entry_t *stat_num_fetch_hit;
	st_entry_t *stat_num_fetch_miss;
    st_entry_t *stat_num_fetch_miss_kernel;
	st_entry_t *stat_num_fetch_part_hit_mshr;
	st_entry_t *stat_num_write_hit;
    st_entry_t *stat_num_write_coher_miss;
    st_entry_t *stat_num_write_coher_miss_kernel;
	st_entry_t *stat_num_write_miss;
    st_entry_t *stat_num_write_miss_kernel;
	st_entry_t *stat_num_write_part_hit_mshr;
	st_entry_t *stat_num_read_stall;
	st_entry_t *stat_num_fetch_stall;
	st_entry_t *stat_num_write_stall;
	
	st_entry_t *stat_num_read;
	st_entry_t *stat_num_fetch;
	st_entry_t *stat_read_hit_ratio;
	st_entry_t *stat_fetch_hit_ratio;
	st_entry_t *stat_num_write;
	st_entry_t *stat_write_hit_ratio;
	st_entry_t *stat_num_requests;
	st_entry_t *stat_hit_ratio;
	st_entry_t *stat_num_stall;
	st_entry_t *stat_num_ld_speculative;         
	st_entry_t *stat_num_invalidates;         
	
	st_entry_t *stat_num_prefetch;
	st_entry_t *stat_num_useful_pref;
	st_entry_t *stat_num_partial_pref;
	st_entry_t *stat_num_useless_pref;
    
	st_entry_t *stat_num_vcpu_state_hit;
	st_entry_t *stat_num_vcpu_state_miss;
	st_entry_t *stat_num_vcpu_state_other;
    st_entry_t *stat_num_vcpu_c2c;

	st_entry_t *stat_bank_q_lat_histo;
	st_entry_t *stat_bank_distro;
    
    group_counter_t **stat_coherence_transition;
    group_counter_t **stat_coherence_action;
    
    cache_power_t *get_cache_power();
    
};

#endif // _CACHE_H_
