/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _PROC_TM_H_
#define _PROC_TM_H_

class proc_tm_t {
private:
	uint32 id;
	sequencer_t *seq;
	mai_t *mai;
	proc_stats_t *pstats;
	proc_stats_t *k_pstats;

    conf_object_t *cpu;
    
	config_t *config_db;
	string   config;

	mem_hier_handle_t *mem_hier;
	proc_stats_t **pstats_list;

	mmu_t *mmu;
	
public:	
	tick_t g_cycles; 

public:
	proc_tm_t (uint32 _id, conf_object_t *_cpu, string &_c);
    
    void init ();

	~proc_tm_t (void);

	void set_id (uint32 _id);
	uint32 get_id ();

	void advance_cycle ();
	sequencer_t *get_sequencer ();
	mai_t *get_mai_object ();

	tick_t get_g_cycles (void);

	proc_stats_t* get_pstats (void);
	void set_interrupt (int64 vector);
	void set_shadow_interrupt (int64 vector);

	void set_config (string &c);
	string get_config (void);
	config_t *get_config_db (void);

	void set_mem_hier (mem_hier_handle_t *_mem_hier);
	mem_hier_handle_t *get_mem_hier (void);

	void print_stats (void);
	void print_stats (proc_stats_t *pstats);

	proc_stats_t **get_pstats_list (void);
	proc_stats_t **generate_pstats_list (void);

	void change_to_user_cfg (void);
	void change_to_kernel_cfg (void);
	
    // Checkpoint stuff
    void stall_front_end (void);
    bool ready_for_checkpoint ();
    bool pending_checkpoint;
    bool wait_on_checkpoint;
    void write_checkpoint(FILE *file);
    void read_checkpoint(FILE *file);
    
	mmu_t *get_mmu (void);
};

#endif
