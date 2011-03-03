/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _CHIP_H_
#define _CHIP_H_

class chip_t {
private:

	// Per core structures
	sequencer_t **seq;
	mmu_t **mmu;
    
	// Per OS/Simics CPU structures
	mai_t **mai;
    conf_object_t **cpus;
	fastsim_t **fastsim;
    thread_context_t **t_ctxt;
	proc_stats_t ***tstats_list;

	// Single instance structures
	thread_scheduler_t *scheduler;
	mem_hier_handle_t *mem_hier;
    mem_driver_t *mem_driver;
	string   config;
	
	config_t *config_db;  // todo, make per sequencer

	uint32 num_cpus;
	uint32 num_seq;
    map<uint64, uint64> lwp_syscall;
	bool initialized;
    
    uint32 disable_core;
    uint32 enable_core;
    
    disable_timer_t *disable_timer;
    
    uint64 heatrun_migration_epoch;

#ifdef POWER_COMPILE
    power_object_t *power_obj;
    power_profile_t **power_entity_list;
    uint32 num_power_entities;
    double *last_power_vals;
    double *last_switching_power_vals;
    double *last_leakage_power_vals;
    uint64  temperature_interval;
    uint64  di_dt_interval;
    
    vector<double> power_vals;
    double_counter_t *stat_didt_sd;
    double_counter_t *stat_didt_mean;
    double_counter_t *stat_didt_min;
    double_counter_t *stat_didt_max;
    double_counter_t *stat_avg_delta;
    double_counter_t *stat_max_delta;
    double previous_didt_power_val;
    void update_power_sd();
    void update_di_dt_power();
    
    
#endif    
    
public:	
	tick_t g_cycles; 

	chip_t (conf_object_t *_cpus[], uint32 _num_cpus, string &_c);
    
    void init ();
    void init_stats ();

	~chip_t (void);

	void advance_cycle ();

	sequencer_t *get_sequencer (uint32 sequencer_id);
	mai_t *get_mai_object (uint32 sequencer_id);
	fastsim_t *get_fastsim (uint32 sequencer_id);

	tick_t get_g_cycles (void);

	proc_stats_t* get_tstats (uint32 thread_id);

	void set_interrupt (uint32 cpu_id, int64 vector);
//	void set_shadow_interrupt (int64 vector);

	void set_config (string &c);
	string get_config (void);
	config_t *get_config_db (void);

	void set_mem_hier (mem_hier_handle_t *_mem_hier);
	mem_hier_handle_t *get_mem_hier (void);

	void print_stats (void);
	void print_stats (proc_stats_t *pstats);

	proc_stats_t **get_tstats_list (uint32 thread_id);
	void generate_tstats_list (void);

//	void change_to_user_cfg (void);
//	void change_to_kernel_cfg (void);
	
    // Checkpoint stuff
    void stall_front_end (void);
    bool ready_for_checkpoint ();

    void write_checkpoint(FILE *file);
    void read_checkpoint(FILE *file);
	
	sequencer_t *get_sequencer_from_thread(uint32 thread_id);
    
	thread_scheduler_t *get_scheduler();
	
	void switch_to_thread(sequencer_t *seq, uint32 tid, mai_t *thread);
	
	mmu_t *get_mmu (uint32 id);
	uint32 get_num_sequencers();
	uint32 get_num_cpus();
    
    mai_t *get_mai_from_thread(uint32 tid);
    uint8 get_seq_ctxt(uint32 tid);
    
    void  idle_thread_context (uint32);
    conf_object_t ** get_cpus ();
    
    void clear_stats ();
    void invalidate_address(sequencer_t *_s, invalidate_addr_t *invalid_addr);
    void handle_simulation();
    uint64 get_lwp_syscall(uint64);
    void set_lwp_syscall(uint64, uint64);
    void check_for_thread_yield();
    void set_mem_driver(mem_driver_t *_mem_driver);
    mem_driver_t *get_mem_driver();
    void handle_mode_change(uint32 thread_id);
    void handle_thread_stat();
    thread_context_t *get_thread_ctxt(uint32 thread);
    uint64 closest_lwp_match(uint64 sp);
    void simulate_enable_disable_core();
	
	uint32 get_machine_id(uint32 vcpu_id);
	void magic_instruction(conf_object_t *cpu, int64 magic_value);
    void handle_synchronous_interrupt(uint32 src, uint32 target);
    void record_synchronous_interrupt(uint32 src, uint32 target);
    void update_cross_call_handler(addr_t);
    bool identify_cross_call_handler(addr_t);
    
    void inspect_register_vals(uint32 cpu_id);
    uint32 current_occupancy(sequencer_t *, uint32 );
    
    void set_hidden_stats();
    void narrow_core_switch(sequencer_t *);
    void squash_remote_inflight(sequencer_t *);
    sequencer_t *get_remote_sequencer(sequencer_t *);
    
    
#ifdef POWER_COMPILE
    power_object_t *get_power_obj();
    void register_power_entity(power_profile_t *pf);
    void update_core_temperature();
#endif    
	
private:

	stats_t *stats;
	group_counter_t *stat_magic_trans;
    histo_1d_t   *histo_syncint_interval;
    group_counter_t *stat_syncint_src;
    group_counter_t *stat_syncint_tgt;
    breakdown_counter_t *stat_cross_call_handler;
    map<addr_t, uint32> cc_handler_list;
    tick_t   last_synch_interrupt;
    histo_1d_t *histo_wait_distr;
    histo_1d_t *stat_load_balance_histo;
    histo_1d_t *stat_max_imbalance_histo;
    histo_1d_t *stat_active_core_histo;
    histo_1d_t *stat_interval_power;
    histo_1d_t *stat_interval_switching_power;
    histo_1d_t *stat_interval_leakage_power;
    

};

#endif
