/* Copyright (c) 2004 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mem_tracer.h,v 1.1.2.8 2005/11/23 23:23:31 pwells Exp $
 *
 * description:    traced mem requests
 * initial author: Philip Wells 
 *
 */
 
#ifndef _MEM_TRACER_H_
#define _MEM_TRACER_H_

#include "profiles.h"

static const uint32 NUM_SYSCALLS = 512;

// User == 0, syscall 1..256
static const uint32 NUM_USAGE_BINS = NUM_SYSCALLS;

class address_t {
public:
	address_t();
	address_t(addr_t _addr, bool _ifetch);
	address_t(addr_t _addr, uint32 _kernel, uint32 _user, uint32 _l3_miss, 
		uint32 _l2_miss, uint32 _l1_miss);
	
	uint64 total();
	float kernel_ratio();
	bool mostly_kernel();
	bool mostly_user();
	float l1_miss_rate();
	float l2_miss_rate();
	bool ifetch();
	
	bool operator< (address_t &rhs);
	
	uint32 addr;  // ifetch hidden at bit 31
	uint32 kernel;
	uint32 user;
	uint32 l3_miss;  // Missed L1 and L2 and L3
	uint32 l2_miss;  // Missed L1 and L2
	uint32 l1_miss;  // Missed L1, hit L2
	
	uint32 last_usage_bin;
	uint32 last_cpu;
};	
	
struct address_sort_fn : public binary_function <address_t *, address_t *, bool>
{
	bool operator() (address_t *addr1, address_t *addr2) const
	{ 
		return (*addr1 < *addr2);
	}
}; 

class mem_tracer_t : public profile_entry_t {
	
public:
	
	mem_tracer_t();
	~mem_tracer_t();
	
	void mem_request(mem_trans_t *trans);
	int is_user(addr_t addr);
	
	void print_stats();

    void to_file(FILE *f);
    void from_file(FILE *f);
	
private:

	base_counter_t *stat_address_data;
	base_counter_t *stat_kernel_address_data;
	base_counter_t *stat_user_address_data;
	base_counter_t *stat_shared_address_data;
	
	base_counter_t *stat_kernel_access_data;
	base_counter_t *stat_user_access_data;

	base_counter_t *stat_kernel_access_to_kernel_data;
	base_counter_t *stat_kernel_access_to_shared_data;
	base_counter_t *stat_kernel_access_to_user_data;
	base_counter_t *stat_user_access_to_user_data;
	base_counter_t *stat_user_access_to_shared_data;
	base_counter_t *stat_user_access_to_kernel_data;

	base_counter_t *stat_l1_miss_to_user_data;
	base_counter_t *stat_l1_miss_to_shared_data;
	base_counter_t *stat_l1_miss_to_kernel_data;
	base_counter_t *stat_l2_miss_to_user_data;
	base_counter_t *stat_l2_miss_to_shared_data;
	base_counter_t *stat_l2_miss_to_kernel_data;
	base_counter_t *stat_l3_miss_to_user_data;
	base_counter_t *stat_l3_miss_to_shared_data;
	base_counter_t *stat_l3_miss_to_kernel_data;

	base_counter_t *stat_address_instr;
	base_counter_t *stat_kernel_address_instr;
	base_counter_t *stat_user_address_instr;
	base_counter_t *stat_shared_address_instr;
	
	base_counter_t *stat_kernel_access_instr;
	base_counter_t *stat_user_access_instr;

	base_counter_t *stat_kernel_access_to_kernel_instr;
	base_counter_t *stat_kernel_access_to_shared_instr;
	base_counter_t *stat_kernel_access_to_user_instr;
	base_counter_t *stat_user_access_to_user_instr;
	base_counter_t *stat_user_access_to_shared_instr;
	base_counter_t *stat_user_access_to_kernel_instr;

	base_counter_t *stat_l1_miss_to_user_instr;
	base_counter_t *stat_l1_miss_to_shared_instr;
	base_counter_t *stat_l1_miss_to_kernel_instr;
	base_counter_t *stat_l2_miss_to_user_instr;
	base_counter_t *stat_l2_miss_to_shared_instr;
	base_counter_t *stat_l2_miss_to_kernel_instr;
	base_counter_t *stat_l3_miss_to_user_instr;
	base_counter_t *stat_l3_miss_to_shared_instr;
	base_counter_t *stat_l3_miss_to_kernel_instr;

	ratio_print_t *stat_l1_miss_to_user_ratio_data;
	ratio_print_t *stat_l1_miss_to_shared_ratio_data;
	ratio_print_t *stat_l1_miss_to_kernel_ratio_data;
	ratio_print_t *stat_l2_miss_to_user_ratio_data;
	ratio_print_t *stat_l2_miss_to_shared_ratio_data;
	ratio_print_t *stat_l2_miss_to_kernel_ratio_data;
	ratio_print_t *stat_l3_miss_to_user_ratio_data;
	ratio_print_t *stat_l3_miss_to_shared_ratio_data;
	ratio_print_t *stat_l3_miss_to_kernel_ratio_data;

	ratio_print_t *stat_l1_miss_to_user_ratio_instr;
	ratio_print_t *stat_l1_miss_to_shared_ratio_instr;
	ratio_print_t *stat_l1_miss_to_kernel_ratio_instr;
	ratio_print_t *stat_l2_miss_to_user_ratio_instr;
	ratio_print_t *stat_l2_miss_to_shared_ratio_instr;
	ratio_print_t *stat_l2_miss_to_kernel_ratio_instr;
	ratio_print_t *stat_l3_miss_to_user_ratio_instr;
	ratio_print_t *stat_l3_miss_to_shared_ratio_instr;
	ratio_print_t *stat_l3_miss_to_kernel_ratio_instr;

	histo_1d_t *stat_kernel_ratio_histo_data;
	histo_1d_t *stat_kernel_ratio_histo_instr;

	base_counter_t *stat_confused_access_data;
	base_counter_t *stat_confused_access_instr;
	
	group_counter_t *stat_syscall_instr;
	group_counter_t *stat_syscall_data;
	group_counter_t *stat_syscall_unique_instr;
	group_counter_t *stat_syscall_unique_data;
	
	group_counter_t *stat_uniqueness_instr;
	group_counter_t *stat_uniqueness_data;

	group_counter_t *stat_same_usage;
	group_counter_t **stat_switch_usage;
	group_counter_t **stat_l1miss_usage;

	histo_1d_t *stat_kernel_reuse_histo_instr;
	histo_1d_t *stat_kernel_reuse_histo_data;
	histo_1d_t *stat_user_reuse_histo_instr;
	histo_1d_t *stat_user_reuse_histo_data;
	
	
	hash_map < addr_t, address_t*, addr_t_hash_fn > addresses;

	hash_map < addr_t, address_t*, addr_t_hash_fn > syscall_addr[NUM_SYSCALLS];
};

#endif /* _MEM_TRACER_H */
