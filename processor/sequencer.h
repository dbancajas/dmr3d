/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _SEQUENCER_H_
#define _SEQUENCER_H_

// prioritized fetch status
const uint32 FETCH_READY             = 0x00000000;

const uint32 FETCH_GPC_UNKNOWN       = 0x00000001;
const uint32 FETCH_LDQ_FULL          = 0x00000002;
const uint32 FETCH_STQ_FULL          = 0x00000004;
const uint32 FETCH_WINDOW_FULL       = 0x00000010;

const uint32 FETCH_STALL_UNTIL_EMPTY = 0x00000100;
const uint32 FETCH_PENDING_SWITCH    = 0x00000200;

const uint32 FETCH_PENDING_CHECKPOINT = 0x00001000;

enum thread_switch_state_t {
	THREAD_SWITCH_NONE,
	THREAD_SWITCH_CHECK_YIELD,
	THREAD_SWITCH_OUT_START,
	THREAD_SWITCH_OUT_WAIT_CACHE,
	THREAD_SWITCH_OUT_WAIT_TIMER,
	THREAD_SWITCH_IN_START,
	THREAD_SWITCH_IN_WAIT_CACHE,
	THREAD_SWITCH_IN_WAIT_TIMER,
    THREAD_SWITCH_HANDLING_INTERRUPT,
    THREAD_SWITCH_NARROW_CORE,
	THREAD_SWITCH_STATE_NUM
};

class annotation_range {
    public:
        addr_t low;
        addr_t high;
        void reinitialize(addr_t, addr_t);
        
};

class sequencer_t {
private:
	static uint64     seq_id; // static sequence ID of dynamic instructions
	
	uint32            id;  // id of this sequencer
	iwindow_t         *iwindow;
	lsq_t             *lsq;
	st_buffer_t       *st_buffer;

	chip_t            *p;
	fu_t              **fus;

	uint32            *fetch_status;
	ctrl_flow_t       **ctrl_flow;

	eventq_t          *eventq;

	wait_list_t       *dead_list;
	wait_list_t       *recycle_list;

	fastsim_t         *fastsim;
	fetch_buffer_t    *fetch_buffer;
	
	// Pointers to currently mapped mai object, etc.
	mai_t             **mai;
	
	bool               *check_thread_yield;
	ts_yield_reason_t  *thread_yield_reason;
	
	// Waiting to start new thread 
	tick_t             *wait_after_switch;
	tick_t             *last_thread_switch;
	
	// Want to take a checkpoint, but not at a place where we want to stop
	// sequencer (i.e. tl > 0)
    bool               *wait_on_checkpoint;
    
    uint32              num_hw_ctxt;  // Number of hw_ctxts mapped to this core/sequencer
    uint32             *thread_ids;
    
    uint32             *icount;      // Instruction in-flight
	
	proc_stats_t ***pstats_list;
    
    sequencer_t  **mem_hier_seq;
    sequencer_t  **prev_mh_seq;
    
    yags_t        *yags_ptr;
    cascade_t     *cascade_ptr;
	
	tick_t              last_mutex_try_lock;
    uint32              spin_pc_index;
    tick_t              *first_mutex_try_lock;
    
   
    addr_t            last_pc_commit;
    uint64            *last_spinloop_commit;
    
	conf_object_t      *mmu_array;
    spin_heuristic_t   **spin_heuristic;
	
	uint32             *outstanding_state_loads;  // send and waiting
	uint32             *outstanding_state_stores;
	uint32             *remaining_state_loads;    // waiting to send
	uint32             *remaining_state_stores;
	
	thread_switch_state_t *thread_switch_state;
    
    // Inidicates how many VCPUs are waiting to serve an interrupt
    uint32             *early_interrupt_queue;
    tick_t              last_fetch_cycle;
    
    // Function profiling
    map<addr_t, uint32> function_map_index;
    map<addr_t, addr_t> function_start_end;
    map<uint32, string> function_name;
    uint32              total_functions;
    addr_t              binary_high_end;
    FILE *              operand_trace_file;
    
    uint64              except_32_prev;
    uint64              except_16_prev;
    uint64              except_24_prev;
    uint64              except_negative_prev;
    uint64              except16_negative_prev;
    uint64              except24_negative_prev;
    uint64              except32_negative_prev;
    uint64              floating_point_op;
    uint64              operand_interval_mask;
    uint64              most_recent_width_dump;
    
    addr_t              *current_function_start;
    addr_t              *current_function_end;
    uint32              *current_function_index;
    
    // Per sequencer attributues
    uint32              seq_max_issue;
    uint32              seq_max_fetch;
    uint32              seq_max_commit;
    bool                seq_issue_inorder;
    
    profiles_t           *profiles;
    core_power_t         *core_power_profile;
    
    uint32              current_inactive_length;
    
    void operand_trace_dump(uint32);
    
    // dynamic map indicating which registers are not ready to be sourced
    uint32              *register_busy;
    uint32               register_count;
    void                 update_register_busy();
    
    uint32               datapath_width;
    
    addr_t               last_commit_pc;
    addr_t               last_operand_dump_pc;
    
    hash_map <addr_t, uint32, addr_t_hash_fn> profile_pc_type;
    hash_map <addr_t, map<addr_t, bool>::iterator, addr_t_hash_fn> pc_map_iterdb;
    map<addr_t, bool> annotated_pc_map;
    map<addr_t, bool>::iterator curr_pc_map_iter;
    annotation_range curr_annotation_range, prev_annotation_range;
    
public:
	static dynamic_instr_t *icache_d_instr;
    static instruction_id_t icache_instr_id;
	static dynamic_instr_t *mmu_d_instr;
    bool               print_mem_op;
    

public:
	sequencer_t (chip_t *_p, uint32 id, uint32 *mai_ids, uint32 _ctxts);
	~sequencer_t (void);

	eventq_t          *get_eventq (void);

	static uint64 generate_seq_id (void);

	chip_t* get_chip (void);
	iwindow_t* get_iwindow (void);
	lsq_t* get_lsq (void);
	st_buffer_t *get_st_buffer (void);
	mai_t *get_mai_object (uint8);
	ctrl_flow_t* get_ctrl_flow (uint8);

	void advance_cycle (void);

	void start (void);
	void finish (void);

	void insert_dead (dynamic_instr_t *d_instr);
	void cleanup_dead (void);
	
	fu_t* get_fu_resource (fu_type_t unit);

	void safety_checks (void);

	bool fu_ready (uint8 );

	bool get_fu_status (uint8 tid, uint32 s);
	uint32 get_fu_status (uint8 tid);
	void set_fu_status (uint8 tid, uint32 s);
	void reset_fu_status (uint8 tid, uint32 s);

	addr_t get_pc (uint8 tid);
	addr_t get_npc (uint8 tid);

	void set_interrupt (uint8 tid, int64 _v);
	int64 get_interrupt (uint8 tid);
	int64 get_shadow_interrupt (uint8 tid);
	void set_shadow_interrupt (uint8 , int64 _v);

	void handle_interrupt (void);

	mem_hier_handle_t *get_mem_hier (void);
	void front_end_status (void);

	void handle_simulation (void);

	uint8 mai_to_tid(mai_t *_ma);
	void prepare_for_interrupt (mai_t *_mai);
	void squash_inflight_instructions (uint8, bool force_squash = false);
	void forward_progress_check (void);

	dynamic_instr_t *recycle (uint8);
	void insert_recycle (dynamic_instr_t *d_instr);
	void schedule (void);
	wait_list_t* adjust_wait_list (wait_list_t * &wl);

	void structure_stats (void);
    

	void prepare_fastsim (void);
	void finish_fastsim (void);

	fastsim_t *get_fastsim (void);

    void write_checkpoint(FILE *file);
    void read_checkpoint(FILE *file);
    bool st_buffer_empty();
	
	// Switch sequencer to a new Simics/OS CPU
	void switch_to_thread(mai_t *mai, uint8 t_ctxt, bool checkp);
	
	proc_stats_t *get_pstats(uint8 seq_ctxt);
	proc_stats_t *get_tstats(uint8 seq_ctxt);
	proc_stats_t **get_pstats_list(uint8 seq_ctxt);
	proc_stats_t **get_tstats_list(uint8 seq_ctxt);
	uint32 get_id();
	
	fetch_buffer_t *get_fetch_buffer();
	void prepare_for_checkpoint(uint32);
	bool ready_for_checkpoint();
    
    void smt_commit ();
    bool process_instr_for_commit (dynamic_instr_t *);
	
	void potential_thread_switch(uint8 tid, ts_yield_reason_t why);
	void check_for_thread_switch();
    
    uint8 select_thread_for_fetch(list<uint8>);
    
    
    uint32 get_thread(uint32);
    void    update_icount (uint32 tid, uint32 num);
    void    decrement_icount (uint32 tid);
	
	void generate_pstats_list();
	void print_stats();
    
    void set_mem_hier_seq(sequencer_t *_s, uint32 ctxt);
    sequencer_t *get_mem_hier_seq(uint32 tid);
    sequencer_t *get_prev_mh_seq(uint32 tid);
    
    void invalidate_address(sequencer_t *_s, invalidate_addr_t *addr);
    
    yags_t *get_yags_ptr();
    cascade_t *get_cascade_ptr();

	tick_t get_last_mutex_lock();
	void set_last_mutex_lock();
    void set_first_mutex_try(uint32 spin_index);
    uint32 get_last_spin_index();
    spin_heuristic_t *get_spin_heuristic(uint32 tid = 0);
	void thread_switch_stats(uint32 ctxt, bool on_off);
	void fetch_vcpu_state(uint32 vcpu_id);
	void store_vcpu_state(uint32 vcpu_id);
	void send_state_trans(uint32 vcpu_id, addr_t pa, bool load);
	void send_vcpu_state_one(uint32 vcpu_id, bool load);
	void complete_switch_request(mem_trans_t *trans);
    bool is_spill_trap(uint32 tid);
    bool is_fill_trap(uint32 tid);
    void interrupt_on_running_thread(uint32 tid);
    tick_t get_last_thread_switch(uint32 tid);
    addr_t get_return_top(uint32 tid);
    void push_return_top(addr_t r_addr, uint32 tid);
    
    void ctrl_flow_redirect(uint32 tid);
    
    void switch_out_epilogue(uint32);
    void switch_in_epilogue(uint32);
    
    
    // Random debugging functions
    void debug_cache_miss(uint32 );
    void set_mai_thread (uint32 ctxt, mai_t *_mai);
	
	uint32 get_num_hw_ctxt();
    bool active_core();
    
    void initialize_function_records ();
    void update_current_function (uint32 tid, addr_t PC);
    uint32 get_function_index (uint32 tid);
    inline uint32 get_total_functions () { return total_functions;} 
	
    void setup_sequencer_attributes ();
    core_power_t *get_core_power_profile ();
    
    // Register busy unit
    void set_register_busy(uint32, uint32);
    uint32 get_register_busy(uint32);
    
    void narrow_core_switch(sequencer_t *seq);
    bool determine_narrow_core_switch(uint8 tid);
    uint32 query_annotation(addr_t PC);
    void initialize_annotated_pc();
    
    
private:
	proc_stats_t *stats;

};

#endif
