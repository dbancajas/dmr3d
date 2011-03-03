/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    profile spin locks
 * initial author: Philip Wells / Koushik
 *                 based on code by Carl Mauer
 */
 
//  #include "simics/first.h"
// RCSID("$Id: spinlock.cc,v 1.1.2.2 2005/11/17 23:40:06 pwells Exp $");

#include "definitions.h"
#include "config_extern.h"
#include "spinlock.h"
#include "debugio.h"
#include "stats.h"
#include "counter.h"
#include "histogram.h"
#include "transaction.h"
#include "external.h"
#include "verbose_level.h"
#include "mem_hier.h"

#include "isa.h"
#include "dynamic.h"
#include "mem_xaction.h"
#include "mai_instr.h"
#include "mai.h"

spin_lock_t::spin_lock_t(addr_t _pa, addr_t _va, uint32 _num_procs,
	profile_spinlock_t *_p)
	: p(_p), num_procs(_num_procs), phys_addr(_pa), virt_addr(_va), held(false),
	  owner(_num_procs), last_acquire_cycle(0), last_release_cycle(0)
{ 
	init();
}

spin_lock_t::spin_lock_t(uint32 _num_procs, profile_spinlock_t *_p)
	: p(_p), num_procs(_num_procs), phys_addr(0), virt_addr(0), held(0),
	  owner(0), last_acquire_cycle(0), last_release_cycle(0)
{
	init();
}

void
spin_lock_t::init()
{
	first_acquire_attempt = new tick_t[num_procs];
	latest_acquire_attempt = new tick_t[num_procs];
	acquire_wait = new tick_t[num_procs];
	for (uint32 i = 0; i < num_procs; i++ ) {
		first_acquire_attempt[i] = 0;
		latest_acquire_attempt[i] = 0;
		acquire_wait[i] = 0;
	}

	stats = new stats_t();
	
	stat_tot_acquire_wait = stats->COUNTER_GROUP("tot_acquire_wait",
		"total spin wait", num_procs); 

	stat_acquires = stats->COUNTER_BASIC("acquires", "# of acquires");
	stat_acquires_kern = stats->COUNTER_BASIC("acquires_kern", "# of kernel acquires");
	stat_acquires_user = stats->COUNTER_BASIC("acquires_user", "# of user acquires");
	stat_releases = stats->COUNTER_BASIC("releases", "# of releases");
	stat_acquire_attempts = stats->COUNTER_BASIC("aquire_attempts",
		"# of contended acquire attempts");
	stat_cpu_transfers = stats->COUNTER_BASIC("cpu_transfers",
		"# of ownership transfers");
	stat_confused_release = stats->COUNTER_BASIC("confused_rel",
		"# of confused releases");
	
	//histo_length_held = stats->HISTO_EXP2("length_held",
	//	"distribution of lock hold time", 8, 1, 1);
	//histo_acquire_latency = stats->HISTO_EXP2("acquire_wait",
	//	"distribution of acquire contention time", 8, 1, 1);
	//histo_attempts_per_acquire = stats->HISTO_EXP2("attempts_per_acquire",
	//	"distribution of # of attempts before acquire", 8, 1, 1);
	//histo_release_to_acquire = stats->HISTO_EXP2("release_to_acquire",
	//	"distribution of cycles between release and acquire", 8, 1, 1);
}

spin_lock_t::~spin_lock_t()
{
	// TODO:
}

void
spin_lock_t::lock_read(mem_trans_t *trans, dynamic_instr_t *dinst)
{
	mai_t *mai = dinst->get_mai_instruction()->get_mai_object();
	uint32 cpu = mai->get_id();
	tick_t cycle = external::get_current_cycle();
	tick_t latency = cycle - trans->request_time;

	ASSERT(cpu < mem_hier_t::ptr()->get_num_processors());
	
	// If the lock is not being held then no point in doing this
	if (!held)
		return;
	
	// If I am already holding the lock then this read is stupid!
	if (owner == cpu)
		return;
	
	acquire_attempt(cycle, cpu, latency);

	if (g_conf_debug_level & verb_t::spinlock) {
		uint64 value = SIM_read_phys_memory(mai->get_cpu_obj(),
			trans->phys_addr, trans->size);
		
		char temp[256];
	
		snprintf(temp, 255, "%s PA:0x%08llx VA:0x%016llx %02d      load 0x%012llx",
			trans->supervisor ? "priv" : "user",
			trans->phys_addr,
			trans->virt_addr,
			p->get_locks_held(cpu),
			value);
		
		dinst->print(temp);
	}
	
}

void
spin_lock_t::lock_write(mem_trans_t *trans, dynamic_instr_t *dinst)
{
	mai_t *mai = dinst->get_mai_instruction()->get_mai_object();
	uint32 cpu = mai->get_id();
	tick_t cycle = external::get_current_cycle();
	tick_t latency = cycle - trans->request_time;

	ASSERT(cpu < mem_hier_t::ptr()->get_num_processors());
	
	release(cycle, cpu, latency);

	if (g_conf_debug_level & verb_t::spinlock) {
		uint64 value = SIM_read_phys_memory(mai->get_cpu_obj(),
			trans->phys_addr, trans->size);
		
		char temp[256];
	
		snprintf(temp, 255, "%s PA:0x%08llx VA:0x%016llx %02d     store 0x%012llx",
			trans->supervisor ? "priv" : "user",
			trans->phys_addr,
			trans->virt_addr,
			p->get_locks_held(cpu),
			value);
		
		dinst->print(temp);
	}
}

void
spin_lock_t::lock_atomic(mem_trans_t *trans, dynamic_instr_t *dinst)
{
	tick_t cycle = external::get_current_cycle();
	tick_t latency = cycle - trans->request_time;
	mai_t *mai = dinst->get_mai_instruction()->get_mai_object();
	uint32 cpu = mai->get_id();
	uint64 store_data = SIM_read_phys_memory(mai->get_cpu_obj(),
			trans->phys_addr, trans->size);

	ASSERT(cpu < mem_hier_t::ptr()->get_num_processors());
   
	string status = "unknown";

	if (store_data != 0x0) {
		// Presumed Aquire Attempt
		if (held) {
			// Someone holding the lock it should not be us!!
			if (owner == cpu) return;

			acquire_attempt(cycle, cpu, latency);
			status = "contend";
			
		} else {
			// Some one acquired the lock
			acquire_success(cycle, cpu, latency, trans->supervisor);
			status = "acquire";
		}
		
	} else {
		// Store value of zero
		// Assumed release
		release(cycle, cpu, latency);
		status = "release";
	}

	if (g_conf_debug_level & verb_t::spinlock) {
		char temp[256];
	
		snprintf(temp, 255, "%s PA:0x%08llx VA:0x%016llx %02d a %7s 0x%012llx",
			trans->supervisor ? "priv" : "user",
			trans->phys_addr,
			trans->virt_addr,
			p->get_locks_held(cpu),
			status.c_str(),
			store_data);
		
		dinst->print(temp);
	}
}

void
spin_lock_t::acquire_attempt(tick_t cycle, uint32 cpu, tick_t latency)
{
	if (first_acquire_attempt[cpu] == 0) {
		ASSERT(latest_acquire_attempt[cpu] == 0);
		// requested, not completed time
		first_acquire_attempt[cpu] = cycle - latency;
	}
	
	if (latest_acquire_attempt[cpu] != 0) {
		if (cycle - latest_acquire_attempt[cpu] > timeout) {
			STAT_INC(p->stat_timeouts);
			// Assume there was a context switch or something in between so we
			// count only the latency of this operation
			acquire_wait[cpu] += latency;
			stat_tot_acquire_wait->inc_total(latency, cpu);
		} else {
			acquire_wait[cpu] += cycle - latest_acquire_attempt[cpu];
			stat_tot_acquire_wait->inc_total(cycle - latest_acquire_attempt[cpu],
				cpu);
			p->histo_attempt_latency->inc_total(1,
				cycle - latest_acquire_attempt[cpu]);
		}
	} else {
		// Again, if this is first attempt, only count op latency
		ASSERT(acquire_wait[cpu] == 0);
		acquire_wait[cpu] = latency;
		stat_tot_acquire_wait->inc_total(latency, cpu);
	}
	latest_acquire_attempt[cpu] = cycle;
	STAT_INC(stat_acquire_attempts);
	STAT_INC(p->stat_acquire_attempts);
}

void
spin_lock_t::acquire_success(tick_t cycle, uint32 cpu, tick_t latency, bool priv)
{
	acquire_attempt(cycle, cpu, latency);
	
	p->inc_locks_held(cpu);

	STAT_INC(stat_acquires);
	STAT_INC(p->stat_acquires);
	if (priv) {
		STAT_INC(stat_acquires_kern);
		STAT_INC(p->stat_acquires_kern);
	} else {
		STAT_INC(stat_acquires_user);
		STAT_INC(p->stat_acquires_user);
	}
	
	if (owner != cpu) {
		STAT_INC(stat_cpu_transfers);
		STAT_INC(p->stat_cpu_transfers);
	}

	// This is put here to not tally stuff for assumed locks that are never
	// actually acquired, rather than increment after each attempt
	ASSERT(acquire_wait[cpu] <= cycle - first_acquire_attempt[cpu]);
	p->stat_tot_acquire_wait->inc_total(acquire_wait[cpu], cpu);
	
	ASSERT(cycle >= first_acquire_attempt[cpu]);
	//histo_acquire_latency->inc_total(1, cycle - first_acquire_attempt[cpu]);
	p->histo_acquire_latency->inc_total(1, cycle - first_acquire_attempt[cpu]);
	if (priv) {
		p->histo_acquire_latency_kern->inc_total(1, cycle -
			first_acquire_attempt[cpu]);
		p->stat_tot_acquire_wait_kern->inc_total(acquire_wait[cpu], cpu);
	} else {
		p->histo_acquire_latency_user->inc_total(1, cycle -
			first_acquire_attempt[cpu]);
		p->stat_tot_acquire_wait_user->inc_total(acquire_wait[cpu], cpu);
	}
	ASSERT(cycle >= last_release_cycle);
	//histo_release_to_acquire->inc_total(1, cycle - last_release_cycle);
	p->histo_release_to_acquire->inc_total(1, cycle - last_release_cycle);

	
	held = true;
	owner = cpu;
	last_acquire_cycle = cycle;

	first_acquire_attempt[cpu] = 0;
	latest_acquire_attempt[cpu] = 0;
	acquire_wait[cpu] = 0;
	
//	if (cpu == 1) g_conf_trace_commits = true;
}


void
spin_lock_t::release(tick_t cycle, uint32 cpu, tick_t latency)
{
	if (!held || cpu != owner) {
		STAT_INC(stat_confused_release);
		STAT_INC(p->stat_confused_release);
	}
	else {
		STAT_INC(stat_releases);
		STAT_INC(p->stat_releases);
		//histo_length_held->inc_total(1, cycle - last_acquire_cycle);
		p->histo_length_held->inc_total(1, cycle - last_acquire_cycle);

		held = false;
		last_release_cycle = cycle;
		p->dec_locks_held(cpu);
	}
//	if (cpu == 1) g_conf_trace_commits = false;
}
	
void spin_lock_t::print_stats()
{
	stats->print();
	DEBUG_LOG("lock PA 0x%08x\n", phys_addr);
}

void spin_lock_t::to_file(FILE *f)
{
	fprintf(f, "0x%llx 0x%llx %d %u %llu %llu\n",
		phys_addr, virt_addr, held ? 1 : 0, owner, last_acquire_cycle, 
		last_release_cycle);

	for (uint32 i = 0; i < num_procs; i++) {
		fprintf(f, "%llu %llu %llu\n", first_acquire_attempt[i], 
			latest_acquire_attempt[i], acquire_wait[i]);
	}

	stats->to_file(f);
}

void spin_lock_t::from_file(FILE *f)
{
	int temp_held;
	fscanf(f, "0x%llx 0x%llx %d %u %llu %llu\n",
		&phys_addr, &virt_addr, &temp_held, &owner, &last_acquire_cycle, 
		&last_release_cycle);
	held = (temp_held == 1);
		
	for (uint32 i = 0; i < num_procs; i++) {
		fscanf(f, "%llu %llu %llu\n", &first_acquire_attempt[i], 
			&latest_acquire_attempt[i], &acquire_wait[i]);
	}

	stats->from_file(f);
	
}



//**************************************************************************
profile_spinlock_t::profile_spinlock_t()
	: profile_entry_t("Spin Lock Profiler")
{
	init();
}

profile_spinlock_t::~profile_spinlock_t() { }


void
profile_spinlock_t::init()
{
	num_procs = mem_hier_t::ptr()->get_num_processors();
	
	locks_held = new uint32[num_procs];
	for (uint32 i = 0; i < num_procs; i++ )
		locks_held[i] = 0;
	
	stat_num_locks = stats->COUNTER_BASIC("num_locks", "# of identified locks");
	stat_timeouts = stats->COUNTER_BASIC("num_timeouts", "# of lock acquires that timeout");
	stat_tot_acquire_wait = stats->COUNTER_GROUP("glob_tot_acquire_wait",
		"total spin wait", num_procs); 
	stat_tot_acquire_wait_kern = stats->COUNTER_GROUP("glob_tot_acquire_wait_kern",
		"total kernel spin wait", num_procs);
	stat_tot_acquire_wait_user = stats->COUNTER_GROUP("glob_tot_acquire_wait_user",
		"total user spin wait", num_procs); 

	stat_acquires = stats->COUNTER_BASIC("glob_acquires", "global # of acquires");
	stat_acquires_kern = stats->COUNTER_BASIC("glob_acquires_kern", "global # of kernel acquires");
	stat_acquires_user = stats->COUNTER_BASIC("glob_acquires_user", "global # of user acquires");
	stat_releases = stats->COUNTER_BASIC("glob_releases", "global # of releases");
	stat_acquire_attempts = stats->COUNTER_BASIC("glob_aquire_attempts",
		"global # of contended acquire attempts");
	stat_cpu_transfers = stats->COUNTER_BASIC("glob_cpu_transfers",
		"global # of ownership transfers");
	stat_confused_release = stats->COUNTER_BASIC("glob_confused_rel",
		"global # of confused releases");
	
	histo_length_held = stats->HISTO_EXP2("glob_length_held",
		"global distribution of lock hold time", 8, 2, 1);
	histo_acquire_latency = stats->HISTO_EXP2("glob_acquire_wait",
		"global distribution of acquire contention time", 8, 2, 1);
	histo_acquire_latency_kern = stats->HISTO_EXP2("glob_acquire_wait_kern",
		"global distribution of kernel acquire contention time", 8, 2, 1);
	histo_acquire_latency_user = stats->HISTO_EXP2("glob_acquire_wait_user",
		"global distribution of user acquire contention time", 8, 2, 1);
	histo_attempts_per_acquire = stats->HISTO_EXP2("glob_attempts_per_acquire",
		"global distribution of # of attempts before acquire", 8, 1, 1);
	histo_release_to_acquire = stats->HISTO_EXP2("glob_release_to_acquire",
		"global distribution of cycles between release and acquire", 8, 4, 1);
	histo_attempt_latency = stats->HISTO_EXP2("glob_attempt_latency",
		"global distribution of cycles between acquire attempts", 8, 3, 1);
}

uint32
profile_spinlock_t::get_locks_held(uint32 thread_id)
{
	ASSERT(thread_id < num_procs);
	return locks_held[thread_id];
}

void
profile_spinlock_t::inc_locks_held(uint32 thread_id)
{
	locks_held[thread_id]++;
}

void
profile_spinlock_t::dec_locks_held(uint32 thread_id)
{
	locks_held[thread_id]--;
}

void
profile_spinlock_t::profile_instr_commits(dynamic_instr_t *dinst)
{
	mem_trans_t *trans = dinst->get_mem_transaction()->get_mem_hier_trans();
	ASSERT(trans);
	
	if (trans->ifetch) return;
	if (trans->size > 8) return;

	// We don't want these...
	bool ld_st_dd = (dinst->get_opcode() == i_ldd ||
		dinst->get_opcode() == i_ldda ||
		dinst->get_opcode() == i_lddf ||
		dinst->get_opcode() == i_lddfa ||
		dinst->get_opcode() == i_std ||
		dinst->get_opcode() == i_stda ||
		dinst->get_opcode() == i_stdf ||
		dinst->get_opcode() == i_stdfa);
	if (ld_st_dd) return;

	spin_lock_t *lock;
	if (locks.find(trans->phys_addr) == locks.end()) {
		// Make a new lock when we see an atomic 
		if (trans->atomic) {
			STAT_INC(stat_num_locks);
			lock = new spin_lock_t(trans->phys_addr, trans->virt_addr, 
				num_procs, this);
			ASSERT(lock);
			locks[trans->phys_addr] = lock;
		}
		else return;
	}

	lock = locks[trans->phys_addr];

	if (trans->atomic) {
		lock->lock_atomic(trans, dinst);
	}
	else if (trans->read) {
		lock->lock_read(trans, dinst);
	}
	else if (trans->write) {
		lock->lock_write(trans, dinst);
	}	
}

void
profile_spinlock_t::print_stats()
{
	if (g_conf_prof_sl_print_each_lock) {
		hash_map <addr_t, spin_lock_t*, addr_t_hash_fn>::iterator itr = 
			locks.begin();
		
		while (itr != locks.end()) {
			spin_lock_t *lock = itr->second;
			if (lock) lock->print_stats();
			else WARNING;
			itr++;
		}
	}

	profile_entry_t::print_stats();
}


void profile_spinlock_t::to_file(FILE *file) {
	// Output class name
	fprintf(file, "%s\n", typeid(this).name());
		
	fprintf(file, "%d\n", locks.size());
	
	for (uint32 i = 0; i < num_procs; i++)
		fprintf(file, "%u\n", locks_held[i]);

	hash_map <addr_t, spin_lock_t*, addr_t_hash_fn>::iterator itr;
		
	uint32 count = 0;
	itr = locks.begin();
	while (itr != locks.end()) {
		ASSERT(locks[itr->first]);
		ASSERT (itr->second);
		fprintf(file, "%llx\n", itr->first);
		itr->second->to_file(file);
		count++;
		itr++;
	}
	ASSERT(count == locks.size());
	
	stats->to_file(file);
}


void profile_spinlock_t::from_file(FILE *file) {
	
	// Input and check class name
	char classname[g_conf_max_classname_len];
	fscanf(file, "%s\n", classname);

    if (strcmp(classname, typeid(this).name()) != 0)
        cout << "Read " << classname << " Require " << typeid(this).name() << endl;
	ASSERT(strcmp(classname, typeid(this).name()) == 0);


	uint32 size;
	fscanf(file, "%d\n", &size);
	
	for (uint32 i = 0; i < num_procs; i++)
		fscanf(file, "%u\n", &locks_held[i]);

	spin_lock_t *lock;
	addr_t temp_address;
	
	for (uint32 i = 0; i < size; i++) {
		fscanf(file, "%llx\n", &temp_address);
		lock = new spin_lock_t(num_procs, this);
		lock->from_file(file);
		locks[temp_address] = lock;
	}
	
	stats->from_file(file);
}


