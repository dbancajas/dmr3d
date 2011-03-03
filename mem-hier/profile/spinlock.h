/* Copyright (c) 2004 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: spinlock.h,v 1.1.2.1 2005/11/16 23:38:05 pwells Exp $
 *
 * description:    profile spin locks
 * initial author: Philip Wells 
 *
 */
 
/* NOTES:

Loads are considered acquire attempts, though they can't actually acquire
without next doing an atomic


*/
 
#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

#include "profiles.h"

class spin_lock_t {

public:
	spin_lock_t(addr_t pa, addr_t va, uint32 num_procs, profile_spinlock_t *p);
	spin_lock_t(uint32 num_procs, profile_spinlock_t *p);
	void init();
	~spin_lock_t();

	// register a load operation on this lock
	void lock_read(mem_trans_t *trans, dynamic_instr_t *dinst);
	
	// register a store operation on this lock
	void lock_write(mem_trans_t *trans, dynamic_instr_t *dinst);
	
	// register an atomic operation on this lock
	void lock_atomic(mem_trans_t *trans, dynamic_instr_t *dinst);

	bool mostly_kernel();
	bool mostly_user();

	// print the lock statistics
	void print_stats();
	
    void to_file(FILE *f);
    void from_file(FILE *f);

	// TEMP: fix
	static const tick_t timeout = 10000; 
	
private:
	profile_spinlock_t *p;
	uint32 num_procs;

	// physical address of this lock
	addr_t phys_addr;
	// one virtual address of this lock
	addr_t virt_addr;

	// true if lock is held, and owner cpu id (or previous owner if not held)
	bool                  held;
	uint32                owner;
	
	// Cycle of most recent acuire/release
	tick_t                last_acquire_cycle;
	tick_t                last_release_cycle;

	// Per cpu, cycle of first acquire attempt (since last release) 
	tick_t                *first_acquire_attempt;
	// Per cpu, cycle of most recent acquire attempt (if since last release) 
	tick_t                *latest_acquire_attempt;
	// Per cpu, cycles attempting to acquire this lock (not just latest-first)
	tick_t                *acquire_wait;

	// Stats
	stats_t *stats;

	group_counter_t *stat_tot_acquire_wait;

	base_counter_t *stat_acquires;
	base_counter_t *stat_acquires_kern;
	base_counter_t *stat_acquires_user;
	base_counter_t *stat_releases;
	base_counter_t *stat_acquire_attempts;
	base_counter_t *stat_cpu_transfers;
	base_counter_t *stat_confused_release;
	
	
	//histo_1d_t      *histo_length_held;
	//histo_1d_t      *histo_acquire_latency;
	//histo_1d_t      *histo_attempts_per_acquire;
	//histo_1d_t      *histo_release_to_acquire;
	
	// Collect stats for an attempted (and failed) acquire
	void acquire_attempt(tick_t cycle, uint32 cpu, tick_t latency);
	void acquire_success(tick_t cycle, uint32 cpu, tick_t latency, bool priv);
	void release(tick_t cycle, uint32 cpu, tick_t latency);
};

class profile_spinlock_t : public profile_entry_t {

public:
	profile_spinlock_t ();
	~profile_spinlock_t ();
	
	void init();
	
	void profile_instr_commits(dynamic_instr_t *dinst);

	void print_stats();
	
    void to_file(FILE *f);
    void from_file(FILE *f);
	
	void inc_locks_held(uint32 thread_id);
	void dec_locks_held(uint32 thread_id);
	uint32 get_locks_held(uint32 thread_id);
	
private:
	uint32 num_procs;

	uint32 *locks_held; // array of # of locks currently held
	
	hash_map <addr_t, spin_lock_t*, addr_t_hash_fn> locks;

public:
	// Global Lock Stats
	base_counter_t *stat_num_locks;
	base_counter_t *stat_timeouts;
	group_counter_t *stat_tot_acquire_wait;
	group_counter_t *stat_tot_acquire_wait_kern;
	group_counter_t *stat_tot_acquire_wait_user;
	base_counter_t *stat_acquires;
	base_counter_t *stat_acquires_kern;
	base_counter_t *stat_acquires_user;
	base_counter_t *stat_releases;
	base_counter_t *stat_acquire_attempts;
	base_counter_t *stat_cpu_transfers;
	base_counter_t *stat_confused_release;
		
	histo_1d_t      *histo_length_held;
	histo_1d_t      *histo_acquire_latency;
	histo_1d_t      *histo_acquire_latency_kern;
	histo_1d_t      *histo_acquire_latency_user;
	histo_1d_t      *histo_attempt_latency;
	histo_1d_t      *histo_attempts_per_acquire;
	histo_1d_t      *histo_release_to_acquire;
	
};

#endif // _SPIN_LOCK_H_


