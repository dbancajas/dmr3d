/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _MAI_INSTR_H_
#define _MAI_INSTR_H_

class mai_instruction_t {
private:
	instruction_id_t instr_id;
	instruction_id_t shadow_instr_id;

	conf_object_t *cpu;
	conf_object_t *shadow_cpu;

	mai_t *mai;

	mai_instr_stage_t stage;
    bool mode32bit;
	
public:
	mai_instruction_t (mai_t *mai, dynamic_instr_t *d_instr);
	~mai_instruction_t (void);

	dynamic_instr_t* get_dynamic_instr (void);
	mai_t* get_mai_object (void);
	instruction_id_t get_instr_id (void);
	instruction_id_t get_shadow_instr_id (void);
	conf_object_t* get_cpu (void);
	instr_event_t insert (void);
	instr_event_t fetch (void);
	instr_event_t decode (void);
	instr_event_t execute (void);
	instr_event_t retire (void);
	instr_event_t commit (void);
	instr_event_t end (void);
	
	instr_event_t handle_exception (void);
	instr_event_t squash_further (void);
	instr_event_t squash_from (void);

	int64 get_instruction_field (const string &field);
	void debug (bool verbose);
	void shadow_debug (bool verbose);
	void debug (bool verbose, instruction_id_t id);
	mai_instr_stage_t get_stage (void);
	void set_stage (mai_instr_stage_t _stage);
	const char debug_stage (void);

	bool done_create (void);
	bool done_fetch (void);
	bool done_decode (void);
	bool done_execute (void);
	bool done_commit (void);
	
	bool readyto_execute (void); 

	tick_t stall_cycles (void);

	bool speculative (void);
	uint32 sync_type (void);

	bool branch_taken (void);

	generic_transaction_t *get_mem_transaction (void);
	v9_memory_transaction_t *get_v9_mem_transaction (void);

	bool finished_stalling (void);
	bool is_stalling (void);
    bool is_32bit();

	int32 read_input_asi ();
   	void get_reg (int32 *reg, int32 &num_src, int32 &num_dest);
   	void test_reg (int32 *reg, int32 num_src, int32 num_dest);
	void write_input_reg (int32 rid, uint64 value);
	void write_output_reg (int32 rid, uint64 value);
	int64 read_output_reg (int32 rid);
    int64 read_input_reg (int32 rid);

	void shadow_replay (mai_instr_stage_t reach);
    bool correct_instruction ();

	void shadow_catchup ();
	void reg_check (bool auto_correct);
    void check_pc_npc (const gpc_t *gpc);
    void analyze_register_dependency ();

	mmu_info_t *fetch_mmu_info;
	mmu_info_t *ldst_mmu_info;
};

#endif
