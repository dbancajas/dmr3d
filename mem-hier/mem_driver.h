/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mem_driver.h,v 1.3.4.14 2006/12/12 18:36:57 pwells Exp $
 *
 * description:    translates between the simics 'timing-model' and mem-hier
 *                 interfaces
 * initial author: Philip Wells 
 *
 */
 
#ifndef _MEM_DRIVER_H_
#define _MEM_DRIVER_H_


class mem_driver_t : public mem_hier_handle_iface_t {
	
 public:
	
	mem_driver_t(proc_object_t *_proc);
	conf_object_t *get_cpu();
	
	// Simics interface
	cycles_t timing_operate(conf_object_t *proc_obj, conf_object_t *space, 
		map_list_t *map, generic_transaction_t *mem_op);
	cycles_t snoop_operate(conf_object_t *proc_obj, conf_object_t *space, 
		map_list_t *map, generic_transaction_t *mem_op);
		
	// Memhier interface
	void complete_request(conf_object_t *obj, conf_object_t *cpu, 
		mem_trans_t *trans);
	void invalidate_address(conf_object_t *obj, conf_object_t *cpu, 
		invalidate_addr_t *invalid_addr);
	
	void print_stats();
	
	// TODO: get
	void set_cpus(conf_object_t **cpus, uint32 num_cpus, uint32 num_threads);
	void set_runtime_config(string config);
	string get_runtime_config();
	
	// TODO:
	void set_checkpoint_file(string filename) { }
	string get_checkpoint_file() { return ""; }
	
	void stall_mem_hier();
	bool is_mem_hier_quiet();
    void clear_stats();
    mem_hier_t *get_mem_hier();
    void release_proc_stall(uint32 id);
    void stall_thread(uint32 id);
    void leak_transactions();
    void inspect_interrupt();
    cycles_t bootstrap_os_pause(conf_object_t *, generic_transaction_t *);
    void inspect_bootstrap_interrupt();
    bool is_paused(uint32 id);
    void handle_synchronous_interrupt(uint32 src, uint32 target);
    bool is_bootstrap();
    set<addr_t> cross_call_handler;
    
	void check_silent_stores(mem_trans_t *, generic_transaction_t *);

private:

	// Number of cycles to stall when not polling
	static const int max_stall = 1000000000;

	mem_hier_t *mem_hier;
	uint32 num_threads;
	// Map of outstanding transcactions
	// +1 for I/O device
	hash_map < int, mem_trans_t *> *transactions;
	
	string runtime_config_file;
    cycles_t initiation_cycle;
    bool   break_sim_called;
    proc_object_t *proc;
    
	// fake cache-trace-mode
	addr_t *last_fetch_cache_addr;
	
    // Checkpoint Functions
    void auto_advance_memhier();
	bool pending_checkpoint;
    bool advance_memhier_cycles;
    bool *paused;
    int *cached_trans_id;
    tick_t *paused_time;
    bool *raise_early_interrupt;
    bool bootstrap;
    uint32 bootstrap_count;
    
    // Warmup checkpoint
	warmup_trace_t *warmup_trace;

};

#endif /* _MEM_DRIVER_H */
