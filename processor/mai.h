/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _MAI_H_
#define _MAI_H_

#define ASI_IMMU_DEMAP                      (0x57)
#define ASI_DMMU_DEMAP                      (0x5f)

class mai_t {
private: 
	chip_t    *p;
	conf_object_t *cpu;
	conf_object_t *shadow_cpu;
	
	sparc_v9_interface_t *v9_interface;
	int64 tl_reg;
    int64 pstate_reg;
	int64 tick_reg;
	int64 stick_reg;
    int64 pc_reg;
    int64 npc_reg;
	bool u2;

	uint64 saved_tick;
	uint64 saved_stick;
	
	safe_uint64        ivec;
    uint64     syscall;
    uint64     syscall_b4_interrupt;
	bool idle_loop;
    bool spin_loop;
    uint64 kstack_region;
	conf_object_t *mmu_obj;
	cmp_mmu_iface_t *mmu_iface;
    uint64 supervisor_entry_step;
    bpred_state_t *bpred_state;
    
    uint32 cpu_id;
    addr_t last_user_pc;
    tick_t last_eviction;
    uint64 last_8bit_commit;
    uint64 last_16bit_commit;
    uint64 last_24bit_commit;
    
    uint32 register_count;
    bool *register_read_first;
    bool *register_write_first;

public:
	mai_t (conf_object_t *_cpu, chip_t *_p);

	~mai_t (void);

	conf_object_t *get_cpu_obj ();
	void piq (void);
	void break_sim (tick_t when);

	addr_t get_pc (void);
	addr_t get_npc (void);

	uint64 get_tl (void);
	uint64 get_tt (uint64 tl);
	uint64 get_tstate (uint64 tl);
	uint64 get_user_g1 ();
	uint64 get_register (string reg);

	bool is_user_trap();
	bool is_fill_spill (uint64 tt);
	bool is_tlb_trap (uint64 tt);
	bool is_interrupt_trap(uint64 tt);

	void handle_interrupt (sequencer_t *seq, proc_stats_t *pstats);
	void set_interrupt(uint64 vector);
	void reset_interrupt();
	bool pending_interrupt();
	uint64 get_interrupt();

	void set_reorder_buffer_size (uint64 entries);
	bool is_supervisor (void);

	void setup_trap_context (bool wrpr = false);
	void setup_syscall_interrupt (uint64 tt);

	void ino_setup_trap_context (uint64 tt, bool wrpr = false,
		int64 wrpr_val = 0);
	void ino_finish_trap_context ();
	void ino_pause_trap_context ();
	void ino_resume_trap_context ();
	
	void set_shadow_cpu_obj (conf_object_t *_cpu);
	conf_object_t *get_shadow_cpu_obj (void);

	bool check_shadow (void);
	void copy_master_simics_attributes (void);

	void simics_lsq_enable (conf_object_t *_cpu, bool b);
	void auto_probe_sim_parameters (void); 
	uint64 get_memory_image_size (void);
	
	int32 get_id();
    void write_checkpoint(FILE *file);
    void read_checkpoint(FILE *file);
	
	proc_stats_t *get_tstats();
    uint64      get_syscall_num();
    uint64      get_syscall_b4_interrupt();
	bool is_idle_loop();
	void set_idle_loop(bool idle);
    bool is_spin_loop();
    void set_spin_loop(bool _spin);
    
    void set_syscall_num(bool provided = false, uint64 val = 0); 
    void set_kstack_region();
    uint64 get_kstack_region();
    bool is_syscall_trap(uint64 tt);    
	void save_tick_reg();
	void restore_tick_reg();
    void reset_syscall_num();
    
    bool is_32bit();
	
	void switch_mmu_array(conf_object_t *mmu_array);
	
	exception_type_t demap_asi(v9_memory_transaction_t *mem_op);
		
	// MMU callback
	static exception_type_t static_demap_asi(conf_object_t *obj,
		generic_transaction_t *mem_op);
      
    bool is_pending_interrupt();  
    bool can_preempt();
    void mode_change();
    void set_last_user_pc(addr_t _pc);
    addr_t get_last_user_pc();
    bpred_state_t *get_bpred_state();
    tick_t get_last_eviction();
    void set_last_eviction(tick_t cycle);
    void set_register(string reg, uint64 val);
    
    void update_register_read(int32 idx);
    void update_register_write(int32 idx);
    
    uint32 get_register_read_first();
    uint32 get_register_write_first();
    void stat_register_read_write(proc_stats_t *pstats, proc_stats_t *tstats);
    void reset_register_read_write();
    
    uint32 get_register_count () { return register_count;}
    
    inline void set_8bit_commit (uint64 val) { last_8bit_commit = val;}
    inline void set_16bit_commit (uint64 val) { last_16bit_commit = val;}
    inline void set_24bit_commit (uint64 val) { last_24bit_commit = val;}
    
    inline uint64 get_8bit_commit () { return last_8bit_commit;}
    inline uint64 get_16bit_commit () { return last_16bit_commit;}
    inline uint64 get_24bit_commit () { return last_24bit_commit;}
};


#endif
