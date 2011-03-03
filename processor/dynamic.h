/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _DYNAMIC_I_H_
#define _DYNAMIC_I_H_

// pipeline stages. *order important*.

const uint32 EVENT_PIPE_FETCH    =               1 << 0;
const uint32 EVENT_PIPE_FETCH_MEM_ACCESS =       1 << 1;
const uint32 EVENT_PIPE_INSERT =                 1 << 2;
const uint32 EVENT_PIPE_PRE_DECODE =             1 << 3;
const uint32 EVENT_PIPE_DECODE =                 1 << 4;
const uint32 EVENT_PIPE_RENAME =                 1 << 5; 
const uint32 EVENT_PIPE_WAIT =                   1 << 6;
const uint32 EVENT_PIPE_EXECUTE =                1 << 7;
const uint32 EVENT_PIPE_EXECUTE_DONE =           1 << 8;
const uint32 EVENT_PIPE_MEM_ACCESS =             1 << 9;
const uint32 EVENT_PIPE_MEM_ACCESS_SAFE =        1 << 10;
const uint32 EVENT_PIPE_MEM_ACCESS_RETRY =       1 << 11;
const uint32 EVENT_PIPE_MEM_ACCESS_PROGRESS =    1 << 12;
const uint32 EVENT_PIPE_MEM_ACCESS_PROGRESS_S2 = 1 << 13;
const uint32 EVENT_PIPE_MEM_ACCESS_REEXEC =      1 << 14;
const uint32 EVENT_PIPE_COMMIT =                 1 << 15;
const uint32 EVENT_PIPE_COMMIT_CONTINUE =        1 << 16;
const uint32 EVENT_PIPE_EXCEPTION =              1 << 17;
const uint32 EVENT_PIPE_SPEC_EXCEPTION =         1 << 18;
const uint32 EVENT_PIPE_REPLAY =                 1 << 19;
const uint32 EVENT_PIPE_ST_BUF_MEM_ACCESS_PROGRESS = 1 << 20;
const uint32 EVENT_PIPE_END =                    1 << 21;



enum lsq_entry_t {
	LSQ_ENTRY_NONE,
	LSQ_ENTRY_CREATE,
	LSQ_ENTRY_HOLD,
	LSQ_ENTRY_RELEASE,
	LSQ_ENTRY_DONE,
	LSQ_ENTRIES
};

enum fetch_entry_t {
	FETCH_ENTRY_CREATE,
	FETCH_ENTRY_HOLD, 
	FETCH_ENTRY_RELEASE,
	FETCH_ENTRY_DONE,
	FETCH_ENTRIES
};

enum st_buffer_entry_t {
	ST_BUFFER_NONE, 
	ST_BUFFER_INSERT, 
	ST_BUFFER_MEM_HIER, 
	ST_BUFFER_DONE, 
	ST_BUFFER_ENTRIES
};

class dynamic_instr_t {
private:

	// the id for the instruction
	uint64              seq_id;

	// mai instruction pointer
	mai_instruction_t   *mai_instr;

	// sequencer pointer
	sequencer_t         * const seq;
	
	// proc_tm pointer
	chip_t           * const p;

	// ctrl_flow pointer
	ctrl_flow_t         *  ctrl_flow;

	// instruction window pointer
	iwindow_t           * const iwindow;

	// instruction window slot
	int32              slot;

	// lsq slot that the instruction holds. only for lds and sts
	int32              lsq_slot;

	// what stage is it in the lsq entry
	lsq_entry_t         lsq_entry;

	// flat stq ptr if store, ptr to next oldest store if load
	uint64             stq_ptr;

	// st buffer entry
	st_buffer_entry_t  st_buffer_entry;

	// memory transaction associated with the instruction
	mem_xaction_t       *mem_xaction;

	// the pc and npc for this instruction
	const gpc_t         *gpc;

	// the predicted output pc and npc. for all instructions
	gpc_t               *pred_o_gpc;

	// the actual output pc and npc. for all instructions 
	gpc_t               *actual_o_gpc;

	// branch prediction state. pointer for all instructions
	bpred_state_t       *bpred_state;

	// type of instruction (branch, execute, ld, st, ...)
	instruction_type_t  type;

	// additional info for the instruction (atomic, alternate sp)
	uint32              ainfo;

	// the opcode of the instruction
	opcodes_t           opcode;

	// type of functional unit
	fu_type_t           futype;

	// functional unit pointer on which to execute
	fu_t                *fu;

	// the type of branch
	branch_type_t       branch_type;

	// branch related offset
	int64               offset;

	// branch related: annul bit
	bool                annul;
	
	// branch related: taken or not-taken
	bool                taken;
	
	// any outstanding events to be handled
	instr_event_t       event;
	
	// previous pipe stage: debug related. may be remobed
	pipe_stage_t        prev_pstage;

	// current pipe stage
	pipe_stage_t        pstage;

	// pipe stage that caused exception
	pipe_stage_t        except_pstage;

	// sync instruction type 
	sync_type_t         sync_instr;

	// was the instruction squashed
	bool                squashed;

	// is the instruction done
	bool                done;

	// are there any outstanding events scheduled in the eventq
	uint32              outstanding;

	wait_list_t         *dead_wl;
	wait_list_t         *issue_wl;

	// NOTE: change when scheduler different
	uint32	            enter_wait, ops_ready;

	fetch_entry_t       fetch_entry;

	int64               imm_asi;
    int64               imm_val;
	eventq_t            *eventq;
	
	uint8				membar_type;
    
    // if fetch should be sent to mem-hier
    bool                initiate_fetch; 
    bool                execute_at_head;

    int32               num_srcs, num_dests;
    int32               l_reg  [RI_NUMBER];  // logical registers
    int32               reg_sz [RN_NUMBER];  // # regs for each regbox
    bool                reg_fp [RN_NUMBER];  // fp/int for each regbox
	
	dynamic_instr_t     *prev_d_instr;
	dynamic_instr_t     *next_d_instr;
    core_power_t        *core_power_profile;
    
    uint8               tid;                // thread ID   
	uint8 fetch_miss;
	uint8 load_miss;
	tick_t fetch_latency;
	tick_t load_latency;
    uint32 shift_cnt;
	
	bool                sync_shadow;  // instruction is after a sync, will be sq
	bool                page_fault;
    tick_t              fetch_cycle;
    tick_t              stage_entry_cycle;
    bool                cmp_imm;
    
    int32               *reg_ids;

public:

	dynamic_instr_t     *next_event;
	dynamic_instr_t     *tail_event;


	dynamic_instr_t (sequencer_t * const _seq, uint8 );
	~dynamic_instr_t (void);

	void clear (void);
	void recycle (uint8);

	const uint64 priority ();

	mai_instruction_t* get_mai_instruction (void);

	void insert (void);
	void fetch (void);

	void pre_decode (void);
	void decode (void);
	void rename (void);
	bool decode_instruction_info (void);

//	void schedule (void);

	void execute (void);
	void retire_begin (void);
	void retire (void);
	void mem_access (void);
	void dequeue (void);
	void commit (void);
	void end (void);
	void mem_hier_done (void);

	bool retire_ready (void);
	
	void handle_exception (void);
	void preparefor_exception (void);
	
	void set_pipe_stage (pipe_stage_t _pstage);
	void set_except_pipe_stage (pipe_stage_t _pstage);

	pipe_stage_t get_except_pipe_stage (void);
	pipe_stage_t get_pipe_stage (void);

	void set_window_slot (int32 _slot);
	void set_lsq_slot (int32 _slot);
	uint64 get_stq_ptr();
	void set_stq_ptr(uint64 ptr);

	int32 get_window_slot (void);	
	int32 get_lsq_slot (void);	
	
	instr_event_t get_pending_event (void);
	void mark_event (instr_event_t e);
	
	void wakeup (void);
	sequencer_t* const get_sequencer (void);
	
	void mark_dead (void);
	void mark_dead_without_insert (void);
	void insert_dead_list (void);

	void set_sync (uint32 _type);
	sync_type_t get_sync (void);

	bool safe_commit (void);

	void set_outstanding (uint32 type);
	void reset_outstanding (uint32 type);
	uint32 get_outstanding (void);
	bool get_outstanding (uint32 type);

	bool is_squashed (void);
	void set_squashed (void);
	bool is_sync_shadow (void);
	void set_sync_shadow ();

	bool is_done (void);

	addr_t get_pc (void);
	addr_t get_npc (void);

	void mark_lsq_entry (lsq_entry_t e);
	lsq_entry_t get_lsq_entry ();
	
	opcodes_t          get_opcode (void);
	instruction_type_t get_type (void);
	branch_type_t      get_branch_type (void);
	bool               get_annul (void);
	addr_t             get_offset (void);
	

	bool               is_taken (void);
	void               set_taken (bool _t);
  
	void set_bpred_state (bpred_state_t *bpred);
	void set_pred_o_gpc (gpc_t *_gpc);
	void set_actual_o_gpc (addr_t n_pc, addr_t n_gpc);

	gpc_t* get_actual_o_gpc (void);
	gpc_t* get_pred_o_gpc (void);

	bool match_o_gpc (void);

	bpred_state_t *get_bpred_state (void);
	void kill_all (); 

	bool is_load (void);
	bool is_load_alt (void);
	bool is_ldd_std();

	bool is_store (void);
	bool is_store_alt (void);
	bool is_atomic (void);
	bool is_prefetch (void);
	bool speculative (void);

	bool immediate_release_store (void);
	bool unsafe_store (void);
	bool unsafe_load (void);
	bool is_unsafe_alt (void);
    bool sequencing_membar(void);
	uint8 get_membar_type();

	mem_xaction_t* get_mem_transaction (void);
	void set_mem_transaction (mem_xaction_t *_t);

	void set_st_buffer_status (st_buffer_entry_t status);
	st_buffer_entry_t get_st_buffer_status (void);

	void release_lsq_hold (void);
	void release_st_hold (void);
	void release_ld_hold (void);
	void ld_mem_hier_done (void);
	void st_mem_hier_done (void);
	void ld_mem_access (void);
	void safe_ld_mem_access (void);
	void st_mem_access (void);
	void release_st_buffer_hold (void);

	void set_mem_hier_pending (void);
	void reset_mem_hier_pending (void);
	bool get_mem_hier_pending (void);

	bool not_issued (void);

	void pop_dead_wait_list (void);
	void insert_dead_wait_list (dynamic_instr_t *d_instr);

	void pop_issue_wait_list (void);
	void insert_issue_wait_list (dynamic_instr_t *d_instr);

	void pop_wait_list (wait_list_t *wl);
	void insert_wait_list (wait_list_t *start_wl, dynamic_instr_t *d_instr);

	void retry_mem_access (void);
	void initiate_mem_hier (bool quiet = false);

	void commit_stats (void);
	fetch_entry_t get_fetch_entry (void);
	void set_fetch_entry (fetch_entry_t e);

	void release_fetch_hold (void);
	void execute_after_release (void);
	void st_prefetch (void);

	void exception_stats (void);
    void set_initiate_fetch(bool);
    bool get_initiate_fetch();
    bool get_execute_at_head();

	int64 get_imm_asi (void);
	fu_t *get_fu (void);

	string pstage_str (void);
   	uint32 get_cpu_id (void);
	
	dynamic_instr_t *get_prev_d_instr (void);
	dynamic_instr_t *get_next_d_instr (void);

	void set_prev_d_instr (dynamic_instr_t *prev);
	void set_next_d_instr (dynamic_instr_t *next);
	
	void print (string stage);

	void temp (void);
    
    uint8   get_tid ();
    void    set_tid  (uint8);
    void    observe_kstack_op();
    tick_t  get_fetch_cycle();
    void update_stage_stats ();
    bool verify_npc_val ();
    void analyze_operand_width();
    void analyze_negative_ops();
    
    bool check_register_busy();
    
    inline int32 get_num_srcs () { return num_srcs;}
    inline int32 get_num_dests () { return num_dests;}
    tick_t get_stage_entry_cycle () { return stage_entry_cycle;}
    

private:
	static const string cc0_s; 
	static const string cc2_s;
	static const string cond_s;
	static const string fcn_s;
	static const string i_s;
	static const string imm22_s;
	static const string op_s;
	static const string op2_s;
	static const string op3_s;
	static const string opf_s;
	static const string rcond_s;
	static const string rd_s;
	static const string rs1_s;
	static const string rs2_s;
	static const string simm13_s;
	static const string x_s;
	static const string a_s;
	static const string d16lo_s;
	static const string d16hi_s;
	static const string disp22_s;
	static const string disp19_s;
	static const string disp30_s;
	static const string imm_asi_s;
};

#endif
