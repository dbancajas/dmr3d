/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mem_hier.h,v 1.7.4.26 2006/07/28 01:29:55 pwells Exp $
 *
 * description:    main class for memory hierarchy
 * initial author: Philip Wells 
 *
 */
#ifndef _MEM_HIER_H_
#define _MEM_HIER_H_

struct link_descriptor {
	uint32 link_id;
	bool buffered;
	uint32 link_latency;
	uint32 msgs_per_link;
};

typedef struct link_descriptor link_desc_t;
//class unip_two_l1d_prot_sm_t;
typedef unip_two_l1d_prot_sm_t prot_t;
typedef simple_mainmem_msg_t msg_t;
//class device_t;

class mem_hier_t {
	
public:
	mem_hier_t(proc_object_t *_processor);
	void init(); // Must be called after all attributes are set
	
	// Called by processor
	mem_return_t make_request(conf_object_t *cpu, mem_trans_t *trans);
	
	// Called by cache when request completes -- calls processor
	void complete_request(conf_object_t *cpu, mem_trans_t *trans);
	// Called by cache when addresses are invalidated -- calls processor
	void invalidate_address(conf_object_t *cpu, invalidate_addr_t *ia);

	static mem_hier_handle_iface_t *get_handler();
	static void set_handler(mem_hier_handle_iface_t *_mhh);

	void checkpoint_and_quit();
	bool is_quiet();
        bool quiet_and_ready();
	void write_checkpoint(string filename);
	void read_checkpoint(string filename);
	
	void advance_cycle();
	static void run_cycle(conf_object_t *cpu, mem_hier_t *mh);
	
	meventq_t *get_eventq();
	tick_t get_g_cycles();
	
	void set_cpus(conf_object_t **_cpus, uint32 _num);
	conf_object_t **get_cpus();
	conf_object_t *get_cpu_object(uint32 id);
	generic_proc_t *get_proc_object(conf_object_t *cpu);
	
	uint32 get_num_processors();
	const char *get_name();
	
	config_t *get_config_db();
	void read_runtime_config_file(string);

	void print_stats();
	void clear_stats();
	
	proc_object_t *get_module_obj();
	// Pointer to the memory hierarchy object
	// Set by init.cc
	static mem_hier_t *ptr();
	static void set_ptr(mem_hier_t *);
    
    	mem_trans_t *get_mem_trans();
    
    	bool is_condor_checkpoint();
    	uint32 round_up_pow2(uint32 val);
	
	void profile_commits(dynamic_instr_t *dinst);
	
	bool static is_thread_state(addr_t address);
    	void reset_spin_commit(uint32 thread);
    	void update_leak_stat(uint32 thread); 
    	void cleanup_previous_stats(FILE *file, const char *pattern);
	
	static uint32 get_machine_id(uint32 cpu_id);
    	void update_mem_link_latency(tick_t);
    
    	base_counter_t *get_cycle_counter();

	uint64 *get_l1_misses_to_bank(uint32 cpu_id);
	void clear_l1_misses_to_bank(uint32 cpu_id);

private:
	//dram_t *dram_ptr;
	typedef dram_t<prot_t, msg_t> drampt;
	drampt *dramptr;
	typedef event_t<mem_trans_t, mem_hier_t> _stall_event_t;
	typedef event_t<string, mem_hier_t> _quiesce_event_t;

	mem_return_t handle_proc_request(conf_object_t *cpu, mem_trans_t *trans);
	mem_return_t handle_io_dev_request(conf_object_t *cpu, mem_trans_t *trans);
	void defer_trans(mem_trans_t *trans);

	void request_stats(conf_object_t *cpu, mem_trans_t *trans);
	void complete_stats(mem_trans_t *trans);

	// Create memory hierarchy functions
	void create_network();
	// These are implemented in networks.cc
	void init_network();
	int add_device(device_t *);
	void join_devices(uint32 devA, uint32 devB, uint32 numlinkAB, link_desc_t *ldA,
	                  uint32 numlinkBA, link_desc_t *ldB);
	void reallocate_links();
	void reallocate_devices();
	void create_minimal_default();
	void create_mp_minimal_default();
	void create_unip_one_default();
	void create_unip_one_dram(); //dramsim network configuration
	void create_unip_one_bus();
	void create_mp_bus();
	void create_two_level_mp_bus();
	void create_unip_one_mp_msi();
	void create_unip_two_default();
	void create_unip_two_default_dram(); //DRAMSim2
	void create_two_level_split_mp();
	void create_unip_one_cachedata();
    void create_cmp_incl(bool);
    void create_cmp_incl_l2banks();
    void create_cmp_excl();
    void create_cmp_excl_3l();
    void create_cmp_incl_3l();
    void register_icache_dcache_power(uint32 *, uint32 *);
    void handle_simulation();
    
    
	// Helper variables for network construction
	uint32 devices_allocated;
	uint32 links_allocated;
	uint32 deviceindex;
	uint32 linkindex;
	
	static void stall_event_handler(_stall_event_t *e);
	static void quiesce_event_handler(_quiesce_event_t *e);

	static mem_hier_handle_iface_t *mhh;
	static mem_hier_t *g_ptr;

	meventq_t *eventq;
	tick_t g_cycles;

	bool is_initialized; //  Has mem-hier been initialized yet?
	string runtime_config_file;

	proc_object_t *module_obj;
	conf_object_t **cpus;
	uint32 num_processors;
    uint32 num_threads;
	link_t **links;
	device_t **devices;
	uint32 num_links;
	uint32 num_devices;
	
	uint32 num_deferred;  // Number of transactions deferred by a stall 
	tick_t stall_delta;   // Cycles to delay request if mem-hier stalls
	uint32 num_io_writes; // Number of outstanding transactions from I/O devices
	
	config_t *config;
	
	bool *currently_idle;
	tick_t *idle_start;
	list<mem_trans_t *> delete_list;
    
    // Checkpoint related
    bool checkpoint_signal_pending;
    bool checkpoint_taken;
    
    // Step Count
    uint64 *last_step_count;
	
    // Mem-hier response error detection
    trans_rec_t *tr_rec;

    // Random Tester
    random_tester_t *random_tester;
	// Statistics
	stats_t *stats;
	bool stats_print_enqueued;
	
    // mem_hier_trans list
    uint32 mem_trans_array_index;
    mem_trans_t **mem_trans_array;

	base_counter_t *stat_cycles;
	base_counter_t *stat_num_requests;
	base_counter_t *stat_num_reads;
	base_counter_t *stat_num_writes;
	base_counter_t *stat_num_atomics;
	base_counter_t *stat_num_io_space;
	base_counter_t *stat_num_io_device;
	base_counter_t *stat_num_io_device_phys_space;
	base_counter_t *stat_num_uncache;
	base_counter_t *stat_num_uncache_io_space;
	base_counter_t *stat_num_uncache_io_device;
	base_counter_t *stat_num_may_not_stall;
	base_counter_t *stat_num_squash;
    base_counter_t *stat_num_commits;
    base_counter_t *stat_user_commits;
    group_counter_t *stat_thread_user_commits;
    group_counter_t *stat_thread_user_cycles;
	histo_1d_t *stat_request_size;
	histo_1d_t *stat_requesttime_histo;
    histo_1d_t *stat_instr_reqtime_histo;
    histo_1d_t *stat_data_reqtime_histo;
	histo_1d_t *stat_vcpu_state_requesttime;
	histo_1d_t *stat_cpu_histo;
	histo_1d_t *stat_asi_histo;
	group_counter_t *stat_idle_cycles_histo;
	
	base_counter_t *stat_l1_hit;
	base_counter_t *stat_l1_miss;
	base_counter_t *stat_l1_coher_miss;
	base_counter_t *stat_l1_mshr_part_hit;
	base_counter_t *stat_l1_stall;
	
	base_counter_t *stat_l2_hit;
	base_counter_t *stat_l2_miss;
	base_counter_t *stat_l2_coher_miss;
	base_counter_t *stat_l2_mshr_part_hit;
	base_counter_t *stat_l2_req_fwd;
	base_counter_t *stat_l2_stall;
    
    group_counter_t *stat_leaked_trans;
    histo_1d_t      *stat_memlinklatency_histo;
    
    // Profile
    profiles_t *profiles;

	mem_tracer_t *address_profiler;
	profile_spinlock_t *spin_lock_profiler;
    stackheap_t *stackheap_profile;
    codeshare_t *codeshare;
    uint64      *spin_step_count;
    conf_object_t **vcpus;
	
	uint64 **l1_misses_to_bank;
	uint64 **l2_misses_to_bank;
	
    
};

#endif // _MEM_HIER_H_
