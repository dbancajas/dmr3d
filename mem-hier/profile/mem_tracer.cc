/* Copyright (c) 2004 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 */

/* description:    trace mem trans
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: mem_tracer.cc,v 1.1.2.9 2005/12/16 17:14:26 pwells Exp $");
 
#include "definitions.h"
#include "mem_tracer.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "config_extern.h"
#include "transaction.h"

#include "proc_stats.h"
#include "isa.h"
#include "dynamic.h"
#include "sequencer.h"
#include "mem_hier.h"

address_t::address_t(addr_t _addr, bool _ifetch)
	: kernel(0), user(0), l3_miss(0), l2_miss(0), l1_miss(0),
	last_usage_bin(99999), last_cpu(0)
{ 
	addr = (uint32) (_addr & 0x7fffffff) | (_ifetch ? 0x80000000 : 0);
	ASSERT(ifetch() == _ifetch);
}

address_t::address_t()
	: addr(0), kernel(0), user(0), l3_miss(0), l2_miss(0), l1_miss(0),
	last_usage_bin(99999), last_cpu(0)
{ 
}

address_t::address_t(addr_t _addr, uint32 _kernel, uint32 _user,
	uint32 _l3_miss, uint32 _l2_miss, uint32 _l1_miss)
	: addr(_addr), kernel(_kernel), user(_user), 
	l3_miss(_l3_miss), l2_miss(_l2_miss), l1_miss(_l1_miss)
{
}

uint64
address_t::total()
{
	return kernel + user;
}

float
address_t::kernel_ratio()
{
	return ((float) kernel) / (float) total();
}

bool
address_t::mostly_kernel()
{
	return kernel_ratio()*100 >= g_conf_prof_mostly_kernel_ratio;
}

bool
address_t::mostly_user() {
	return (1 - kernel_ratio())*100 >= g_conf_prof_mostly_user_ratio;
}
	
bool
address_t::operator< (address_t &rhs)
{
	return (total() < rhs.total());
}

bool
address_t::ifetch()
{
	return (addr & 0x80000000);
}

float
address_t::l1_miss_rate()
{
	return ((float) (l1_miss + l2_miss)) / (float) total();
}

float
address_t::l2_miss_rate()
{
	return ((float) l2_miss) / (float) total();
}

mem_tracer_t::mem_tracer_t() :
	profile_entry_t("Memory Address Tracer")
{
	stat_address_data = stats->COUNTER_BASIC ("address_data", 
	"# of distinct addresses_data");
	stat_kernel_address_data = stats->COUNTER_BASIC ("kernel_address_data", 
	"# of mostly kernel addresses_data");
	stat_user_address_data = stats->COUNTER_BASIC ("user_address_data", 
	"# of mostly user addresses_data");
	stat_shared_address_data = stats->COUNTER_BASIC ("shared_address_data", 
	"# of shared addresses_data");
	stat_kernel_access_data = stats->COUNTER_BASIC ("kernel_access_data", 
	"# of kernel accesses_data");
	stat_user_access_data = stats->COUNTER_BASIC ("user_access_data", 
	"# of user accesses_data");
	stat_kernel_access_to_kernel_data = stats->COUNTER_BASIC ("kernel_access_to_kernel_data", 
	"# of kernel accesses to kernel addresses_data");
	stat_user_access_to_kernel_data = stats->COUNTER_BASIC ("user_access_to_kernel_data", 
	"# of user accesses to kernel addresses_data");
	stat_kernel_access_to_shared_data = stats->COUNTER_BASIC ("kernel_access_to_shared_data", 
	"# of kernel accesses to shared addresses_data");
	stat_user_access_to_shared_data = stats->COUNTER_BASIC ("user_access_to_shared_data", 
	"# of user accesses to shared addresses_data");
	stat_kernel_access_to_user_data = stats->COUNTER_BASIC ("kernel_access_to_user_data", 
	"# of kernel accesses to user addresses_data");
	stat_user_access_to_user_data = stats->COUNTER_BASIC ("user_access_to_user_data", 
	"# of user accesse to user addressess_data");
	
	stat_kernel_ratio_histo_data = stats->HISTO_UNIFORM ("kernel_ratio_data", 
	"ratio of kernel to user accesses for each address_data", 101, 1, 0);
	stat_kernel_ratio_histo_data->set_void_zero_entries();
	stat_kernel_ratio_histo_data->set_hidden();

	stat_l1_miss_to_kernel_data = stats->COUNTER_BASIC ("l1_miss_to_kern_data", 
	"# of l1 misses to kernel addressess_data");
	stat_l1_miss_to_shared_data = stats->COUNTER_BASIC ("l1_miss_to_shared_data", 
	"# of l1 misses to shared addressess_data");
	stat_l1_miss_to_user_data = stats->COUNTER_BASIC ("l1_miss_to_user_data", 
	"# of l1 misses to user addressess_data");
	stat_l2_miss_to_kernel_data = stats->COUNTER_BASIC ("l2_miss_to_kern_data", 
	"# of l2 misses to kernel addressess_data");
	stat_l2_miss_to_shared_data = stats->COUNTER_BASIC ("l2_miss_to_shared_data", 
	"# of l2 misses to shared addressess_data");
	stat_l2_miss_to_user_data = stats->COUNTER_BASIC ("l2_miss_to_user_data", 
	"# of l2 misses to user addressess_data");
	stat_l3_miss_to_kernel_data = stats->COUNTER_BASIC ("l3_miss_to_kern_data", 
	"# of l3 misses to kernel addressess_data");
	stat_l3_miss_to_shared_data = stats->COUNTER_BASIC ("l3_miss_to_shared_data", 
	"# of l3 misses to shared addressess_data");
	stat_l3_miss_to_user_data = stats->COUNTER_BASIC ("l3_miss_to_user_data", 
	"# of l3 misses to user addressess_data");

	stat_address_instr = stats->COUNTER_BASIC ("address_instr", 
	"# of distinct addresses_instr");
	stat_kernel_address_instr = stats->COUNTER_BASIC ("kernel_address_instr", 
	"# of mostly kernel addresses_instr");
	stat_user_address_instr = stats->COUNTER_BASIC ("user_address_instr", 
	"# of mostly user addresses_instr");
	stat_shared_address_instr = stats->COUNTER_BASIC ("shared_address_instr", 
	"# of shared addresses_instr");
	stat_kernel_access_instr = stats->COUNTER_BASIC ("kernel_access_instr", 
	"# of kernel accesses_instr");
	stat_user_access_instr = stats->COUNTER_BASIC ("user_access_instr", 
	"# of user accesses_instr");
	stat_kernel_access_to_kernel_instr = stats->COUNTER_BASIC ("kernel_access_to_kernel_instr", 
	"# of kernel accesses to kernel addresses_instr");
	stat_user_access_to_kernel_instr = stats->COUNTER_BASIC ("user_access_to_kernel_instr", 
	"# of user accesses to kernel addresses_instr");
	stat_kernel_access_to_shared_instr = stats->COUNTER_BASIC ("kernel_access_to_shared_instr", 
	"# of kernel accesses to shared addresses_instr");
	stat_user_access_to_shared_instr = stats->COUNTER_BASIC ("user_access_to_shared_instr", 
	"# of user accesses to shared addresses_instr");
	stat_kernel_access_to_user_instr = stats->COUNTER_BASIC ("kernel_access_to_user_instr", 
	"# of kernel accesses to user addresses_instr");
	stat_user_access_to_user_instr = stats->COUNTER_BASIC ("user_access_to_user_instr", 
	"# of user accesse to user addressess_instr");
	
	stat_kernel_ratio_histo_instr = stats->HISTO_UNIFORM ("kernel_ratio_instr", 
	"ratio of kernel to user accesses for each address_instr", 101, 1, 0);
	stat_kernel_ratio_histo_instr->set_void_zero_entries();
	stat_kernel_ratio_histo_instr->set_hidden();

	stat_l1_miss_to_kernel_instr = stats->COUNTER_BASIC ("l1_miss_to_kern_instr", 
	"# of l1 misses to kernel addressess_instr");
	stat_l1_miss_to_shared_instr = stats->COUNTER_BASIC ("l1_miss_to_shared_instr", 
	"# of l1 misses to shared addressess_instr");
	stat_l1_miss_to_user_instr = stats->COUNTER_BASIC ("l1_miss_to_user_instr", 
	"# of l1 misses to user addressess_instr");
	stat_l2_miss_to_kernel_instr = stats->COUNTER_BASIC ("l2_miss_to_kern_instr", 
	"# of l2 misses to kernel addressess_instr");
	stat_l2_miss_to_shared_instr = stats->COUNTER_BASIC ("l2_miss_to_shared_instr", 
	"# of l2 misses to shared addressess_instr");
	stat_l2_miss_to_user_instr = stats->COUNTER_BASIC ("l2_miss_to_user_instr", 
	"# of l2 misses to user addressess_instr");
	stat_l3_miss_to_kernel_instr = stats->COUNTER_BASIC ("l3_miss_to_kern_instr", 
	"# of l3 misses to kernel addressess_instr");
	stat_l3_miss_to_shared_instr = stats->COUNTER_BASIC ("l3_miss_to_shared_instr", 
	"# of l3 misses to shared addressess_instr");
	stat_l3_miss_to_user_instr = stats->COUNTER_BASIC ("l3_miss_to_user_instr", 
	"# of l3 misses to user addressess_instr");

	stat_confused_access_instr = stats->COUNTER_BASIC ("confused_access_instr", 
	"ifetch access to a presumed data line");
	stat_confused_access_data = stats->COUNTER_BASIC ("confused_access_data", 
	"data access to a presumed instr line");
	
	stat_syscall_instr = stats->COUNTER_GROUP ("syscall_instr",
	"breakdown of instructions from each syscall", NUM_SYSCALLS);
	stat_syscall_instr->set_void_zero_entries();

	stat_syscall_data = stats->COUNTER_GROUP ("syscall_data",
	"breakdown of data from each syscall", NUM_SYSCALLS);
	stat_syscall_data->set_void_zero_entries();

	stat_syscall_unique_instr = stats->COUNTER_GROUP ("syscall_unique_instr",
	"breakdown of unique instructions from each syscall", NUM_SYSCALLS);
	stat_syscall_unique_instr->set_void_zero_entries();

	stat_syscall_unique_data = stats->COUNTER_GROUP ("syscall_unique_data",
	"breakdown of unique data from each syscall", NUM_SYSCALLS);
	stat_syscall_unique_data->set_void_zero_entries();

	stat_uniqueness_instr = stats->COUNTER_GROUP ("uniqueness_instr",
	"breakdown of number of users of syscall isntr", mem_hier_t::ptr()->get_num_processors());
	stat_uniqueness_data = stats->COUNTER_GROUP ("uniqueness_data",
	"breakdown of number of users of syscall data", mem_hier_t::ptr()->get_num_processors());

	stat_kernel_reuse_histo_instr = stats->HISTO_EXP2 ("kernel_reuse_histo_instr",
	"reuse of instruction lines in OS", 12, 2, 1);

	stat_kernel_reuse_histo_data = stats->HISTO_EXP2 ("kernel_reuse_histo_data",
	"reuse of data lines in OS", 12, 2, 1);

	stat_user_reuse_histo_instr = stats->HISTO_EXP2 ("user_reuse_histo_instr",
	"reuse of instruction lines in user", 12, 2, 1);

	stat_user_reuse_histo_data = stats->HISTO_EXP2 ("user_reuse_histo_data",
	"reuse of data lines in OS", 12, 2, 1);

	stat_same_usage = stats->COUNTER_GROUP ("same_usage",
	"repeat accesses to the same usage bin", NUM_SYSCALLS);
	stat_same_usage->set_void_zero_entries();
	stat_same_usage->set_hidden();
	
	stat_switch_usage = new group_counter_t * [NUM_SYSCALLS];
	stat_l1miss_usage = new group_counter_t * [NUM_SYSCALLS];
	char stat_name[128];
	char miss_name[128];
	for (uint32 i = 0; i < NUM_SYSCALLS; i++) {
		snprintf(stat_name, 128, "switch_usage_%u", i);
		snprintf(miss_name, 128, "l1miss_usage_%u", i);
		stat_switch_usage[i] = stats->COUNTER_GROUP (stat_name,
		"accesses switch to different usage bin", NUM_SYSCALLS + 1000);
		stat_switch_usage[i]->set_void_zero_entries();
		stat_l1miss_usage[i] = stats->COUNTER_GROUP (miss_name,
		"L1 misses switch to different usage bin", NUM_SYSCALLS + 1000);
		stat_l1miss_usage[i]->set_void_zero_entries();
	}

}

mem_tracer_t::~mem_tracer_t() {
}

int
mem_tracer_t::is_user(addr_t addr)
{
	addr >>= g_conf_prof_ma_granularity_bits;
	address_t *address = addresses[addr];
	if (!address) return -1;
	else if (address->mostly_user()) return 1;
	else return 0;
	
}

void 
mem_tracer_t::mem_request(mem_trans_t *mem_trans)
{
	// Ignore instruction fetch, e.g
	if (g_conf_prof_ma_ignore_ifetch && mem_trans->ifetch) return;
	// Ignore i/o devices
	if (mem_trans->io_device) return;
	// Ignore instruction fetch, e.g
	if (g_conf_prof_ma_ignore_stores && mem_trans->write) return;
	// Ignore if not a read or write
	if (!mem_trans->read && !mem_trans->write) return;
	
	addr_t addr = mem_trans->phys_addr >> g_conf_prof_ma_granularity_bits;
	uint32 size = (1 << g_conf_prof_ma_granularity_bits);
	uint32 locs = mem_trans->size <= size ? 1 : mem_trans->size / size;
	
	for (uint32 i = 0; i < locs; i++) {
		address_t *address = addresses[addr];
		
		if (!address) {
			address = new address_t(addr, mem_trans->ifetch);
			addresses[addr] = address;
		}
		
		bool priv = mem_trans->supervisor;
		if (mem_trans->ifetch && !address->ifetch()) {
			// first used as data, probably by kernel
			address->addr &= 0x7fffffff;
		}
		if (mem_trans->ifetch != address->ifetch()) { 
			if (mem_trans->ifetch) STAT_INC(stat_confused_access_instr);
			else STAT_INC(stat_confused_access_data);
		}
		
		if (priv) {
			if (mem_trans->ifetch) STAT_INC(stat_kernel_access_instr);
			else STAT_INC(stat_kernel_access_data);
			address->kernel++;
		} else {
			if (mem_trans->ifetch) STAT_INC(stat_user_access_instr);
			else STAT_INC(stat_user_access_data);
			address->user++;
		}
		
		
		// Syscall stata
		uint64 syscall = 0;
		proc_stats_t **tstatslist = mem_trans->mem_hier_seq->get_tstats_list(0);
		if (tstatslist) {
			proc_stats_t *tstats0 = tstatslist[0];
			syscall = STAT_GET(tstats0->stat_syscall_num);
			if (syscall) {
				(syscall_addr[syscall])[addr] = address;			
			}
		}
		
		// Usage bin stats
		// 0 == user or non-syscall kernel
		uint32 usage_bin = syscall;
//		if (usage_bin == address->last_usage_bin &&
//			mai_cpu_id == address->last_cpu)
//		{
//			STAT_INC(stat_same_usage, usage_bin);
//		}

		uint32 new_usage_bin = syscall;
		uint32 mai_cpu_id = mem_trans->sequencer->get_id();
		if (mai_cpu_id != address->last_cpu)
			new_usage_bin += 1000;

		if (address->last_usage_bin != 99999)
			STAT_INC(stat_switch_usage[address->last_usage_bin], new_usage_bin);
		
		if (mem_trans->occurred(MEM_EVENT_L1_DEMAND_MISS |
			MEM_EVENT_L1_COHER_MISS | MEM_EVENT_L1_STALL |
			MEM_EVENT_L1_MSHR_PART_HIT))
		{
			if (address->last_usage_bin != 99999) {
				STAT_INC(stat_l1miss_usage[address->last_usage_bin],
					new_usage_bin);
			}
		}
		
		address->last_usage_bin = usage_bin;
		address->last_cpu = mai_cpu_id;
		
		
		if (!mem_trans->occurred(MEM_EVENT_L1_HIT)) {
			address->l1_miss++;
			if (!mem_trans->occurred(MEM_EVENT_L2_HIT)) {
				address->l2_miss++;
				if (mem_trans->occurred(MEM_EVENT_L3_DEMAND_MISS)) {
					address->l3_miss++;
				}
			}
		}
		
		addr++; // next address if large access
	}

	return;
}

void
mem_tracer_t::print_stats()
{
	hash_map < addr_t, address_t *, addr_t_hash_fn >::iterator itr =
		addresses.begin();
		
	vector <address_t *> temp_vector;
	
	while (itr != addresses.end()) {
		address_t *address = itr->second;

		if (g_conf_prof_ma_print_addresses)
			temp_vector.insert(temp_vector.end(), address);

		itr++;
		
		if (address->ifetch()) {
			STAT_INC(stat_address_instr);
			stat_kernel_ratio_histo_instr->inc_total(1,
				(int)(100*address->kernel_ratio()));
	
			if (address->mostly_kernel()) {
				ASSERT(!address->mostly_user());
				STAT_INC(stat_kernel_address_instr);
	
				STAT_ADD(stat_kernel_access_to_kernel_instr, address->kernel);
				STAT_ADD(stat_user_access_to_kernel_instr, address->user);
				
				stat_kernel_reuse_histo_instr->inc_total(1,
					address->kernel+address->user);
	
				STAT_ADD(stat_l1_miss_to_kernel_instr, address->l1_miss);
				STAT_ADD(stat_l2_miss_to_kernel_instr, address->l2_miss);
				STAT_ADD(stat_l3_miss_to_kernel_instr, address->l3_miss);
			}			
			else if (address->mostly_user()) {
				STAT_INC(stat_user_address_instr);
				
				STAT_ADD(stat_kernel_access_to_user_instr, address->kernel);
				STAT_ADD(stat_user_access_to_user_instr, address->user);
	
				stat_user_reuse_histo_instr->inc_total(1,
					address->kernel+address->user);
	
				STAT_ADD(stat_l1_miss_to_user_instr, address->l1_miss);
				STAT_ADD(stat_l2_miss_to_user_instr, address->l2_miss);
				STAT_ADD(stat_l3_miss_to_user_instr, address->l3_miss);
			}
			else {
				STAT_INC(stat_shared_address_instr);
				
				STAT_ADD(stat_kernel_access_to_shared_instr, address->kernel);
				STAT_ADD(stat_user_access_to_shared_instr, address->user);
	
				STAT_ADD(stat_l1_miss_to_shared_instr, address->l1_miss);
				STAT_ADD(stat_l2_miss_to_shared_instr, address->l2_miss);
				STAT_ADD(stat_l3_miss_to_shared_instr, address->l3_miss);
			}
		} else {
			STAT_INC(stat_address_data);
			stat_kernel_ratio_histo_data->inc_total(1,
				(int)(100*address->kernel_ratio()));
	
			if (address->mostly_kernel()) {
				ASSERT(!address->mostly_user());
				STAT_INC(stat_kernel_address_data);
	
				STAT_ADD(stat_kernel_access_to_kernel_data, address->kernel);
				STAT_ADD(stat_user_access_to_kernel_data, address->user);
	
				stat_kernel_reuse_histo_data->inc_total(1,
					address->kernel+address->user);
	
				STAT_ADD(stat_l1_miss_to_kernel_data, address->l1_miss);
				STAT_ADD(stat_l2_miss_to_kernel_data, address->l2_miss);
				STAT_ADD(stat_l3_miss_to_kernel_data, address->l3_miss);
			}			
			else if (address->mostly_user()) {
				STAT_INC(stat_user_address_data);
				
				STAT_ADD(stat_kernel_access_to_user_data, address->kernel);
				STAT_ADD(stat_user_access_to_user_data, address->user);
	
				stat_user_reuse_histo_data->inc_total(1,
					address->kernel+address->user);
	
				STAT_ADD(stat_l1_miss_to_user_data, address->l1_miss);
				STAT_ADD(stat_l2_miss_to_user_data, address->l2_miss);
				STAT_ADD(stat_l3_miss_to_user_data, address->l3_miss);
			}
			else {
				STAT_INC(stat_shared_address_data);
				
				STAT_ADD(stat_kernel_access_to_shared_data, address->kernel);
				STAT_ADD(stat_user_access_to_shared_data, address->user);
	
				STAT_ADD(stat_l1_miss_to_shared_data, address->l1_miss);
				STAT_ADD(stat_l2_miss_to_shared_data, address->l2_miss);
				STAT_ADD(stat_l3_miss_to_shared_data, address->l3_miss);
			}
		}
	}

	vector <address_t *>::iterator start = temp_vector.begin();
	vector <address_t *>::iterator end = temp_vector.end();
	
	stat_syscall_instr->clear();
	stat_syscall_data->clear();
	stat_syscall_unique_instr->clear();
	stat_syscall_unique_data->clear();
	stat_uniqueness_instr->clear();
	stat_uniqueness_data->clear();
	
	/// Big loop.
	for (uint32 sys = 0; sys < NUM_SYSCALLS; sys++) {
		itr = syscall_addr[sys].begin();
		
		while (itr != syscall_addr[sys].end()) {
			addr_t addr = itr->first;
			address_t *address = itr->second;
			itr++;
		
			if (address->ifetch()) {
				STAT_INC(stat_syscall_instr, sys);
				
				uint32 users = 0;
				for (uint32 other_sys = 0; other_sys < NUM_SYSCALLS; other_sys++) {
					if (syscall_addr[other_sys].find(addr) !=
						syscall_addr[other_sys].end())
					{
						users++;
					}
				}
				ASSERT(users >= 1);
				if (users == 1)
					STAT_INC(stat_syscall_unique_instr, sys);
				else if (users > 8)
					users = 8;
				stat_uniqueness_instr->inc_total(1, users-1);
			}
			else {	
				STAT_INC(stat_syscall_data, sys);
				
				uint32 users = 0;
				for (uint32 other_sys = 0; other_sys < NUM_SYSCALLS; other_sys++) {
					if (syscall_addr[other_sys].find(addr) !=
						syscall_addr[other_sys].end())
					{
						users++;
					}
				}
				ASSERT(users >= 1);
				if (users == 1)
					STAT_INC(stat_syscall_unique_data, sys);
				else if (users > 8)
					users = 8;
				stat_uniqueness_data->inc_total(1, users-1);
			}
		}
	}
	
	for (uint32 i = 0; i < NUM_SYSCALLS; i++) {
		if (stat_switch_usage[i]->get_total() == 0)
			stat_switch_usage[i]->set_hidden();
		else 
			stat_switch_usage[i]->set_hidden(false);
		
		if (stat_l1miss_usage[i]->get_total() == 0)
			stat_l1miss_usage[i]->set_hidden();
		else 
			stat_l1miss_usage[i]->set_hidden(false);
	}
	
	stats->print();

	STAT_SET(stat_address_instr, 0);
	STAT_SET(stat_kernel_address_instr, 0);
	STAT_SET(stat_user_address_instr, 0);
	STAT_SET(stat_shared_address_instr, 0);
	STAT_SET(stat_kernel_access_to_kernel_instr, 0);
	STAT_SET(stat_user_access_to_kernel_instr, 0);
	STAT_SET(stat_kernel_access_to_user_instr, 0);
	STAT_SET(stat_user_access_to_user_instr, 0);
	STAT_SET(stat_kernel_access_to_shared_instr, 0);
	STAT_SET(stat_user_access_to_shared_instr, 0);
	STAT_SET(stat_address_data, 0);
	STAT_SET(stat_kernel_address_data, 0);
	STAT_SET(stat_user_address_data, 0);
	STAT_SET(stat_shared_address_data, 0);
	STAT_SET(stat_kernel_access_to_kernel_data, 0);
	STAT_SET(stat_user_access_to_kernel_data, 0);
	STAT_SET(stat_kernel_access_to_user_data, 0);
	STAT_SET(stat_user_access_to_user_data, 0);
	STAT_SET(stat_kernel_access_to_shared_data, 0);
	STAT_SET(stat_user_access_to_shared_data, 0);

	if (g_conf_prof_ma_print_addresses) {
		address_sort_fn mysort;
		sort(start, end, mysort);
		
		end--;
		while (end != start) {
			address_t *address = *end;
			
			DEBUG_LOG("0x%08x %0.3f %llu %llu %llu %f %f\n", address->addr,
			address->kernel_ratio(), address->total(), address->kernel,
			address->user, address->l1_miss_rate(), address->l2_miss_rate());
			
			end--;
		}
	}
}

void mem_tracer_t::to_file(FILE *file) {
	// Output class name
	fprintf(file, "%s\n", typeid(this).name());
		
	fprintf(file, "%d\n", addresses.size());
	
	hash_map < addr_t, address_t*, addr_t_hash_fn >::iterator itr;
		
	uint32 count = 0;
	itr = addresses.begin();
	while (itr != addresses.end()) {
		fprintf(file, "%llx %x %x %x %x %x %x\n", itr->first,
			itr->second->addr, itr->second->kernel, itr->second->user,
			itr->second->l3_miss, itr->second->l2_miss, itr->second->l1_miss);
		itr++;
		count++;
	}
	ASSERT(count == addresses.size());
	
	for (uint32 sys = 0; sys < NUM_SYSCALLS; sys++) {
		itr = syscall_addr[sys].begin();
		
		fprintf(file, "%d\n", syscall_addr[sys].size());

		while (itr != syscall_addr[sys].end()) {
			addr_t addr = itr->first;
			ASSERT(addr);
			fprintf(file, "0x%llx\n", addr);
			itr++;
		}
	}
	
	stats->to_file(file);
}


void mem_tracer_t::from_file(FILE *file) {
	// Input and check class name
	char classname[g_conf_max_classname_len];
	fscanf(file, "%s\n", classname);

    if (strcmp(classname, typeid(this).name()) != 0)
        cout << "Read " << classname << " Require " << typeid(this).name() << endl;
	ASSERT(strcmp(classname, typeid(this).name()) == 0);


	uint32 size;
	fscanf(file, "%d\n", &size);
	
	address_t *address;
	addr_t temp_address;
	
	for (uint32 i = 0; i < size; i++) {
		address = new address_t();
		int ret = fscanf(file, "%llx %x %x %x %x %x %x\n", &temp_address,
			&address->addr, &address->kernel, &address->user,
			&address->l3_miss, &address->l2_miss, &address->l1_miss);
		ASSERT(ret == 7);
		addresses[temp_address] = address;
	}
	
	for (uint32 sys = 0; sys < NUM_SYSCALLS; sys++) {
		fscanf(file, "%d\n", &size);

		addr_t addr;
		for (uint32 i = 0; i < size; i++) {
			int ret = fscanf(file, "0x%llx\n", &addr);
			ASSERT(ret == 1);
			address_t *address = addresses[addr];
			ASSERT(address);
			(syscall_addr[sys])[addr] = address;
		}
	}
	
	stats->from_file(file);
}


