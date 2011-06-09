
#ifndef _MEM_HIER_HANDLE_H_
#define _MEM_HIER_HANDLE_H_

class mem_hier_handle_iface_t {

public:
	virtual ~mem_hier_handle_iface_t() { }
	
	virtual void complete_request (conf_object_t *obj, conf_object_t *cpu, 
		mem_trans_t *trans) = 0;
		
	virtual void invalidate_address (conf_object_t *obj, conf_object_t *cpu, 
		invalidate_addr_t *invalid_addr) = 0;

};

class mem_hier_handle_t : public mem_hier_handle_iface_t {

private:
	conf_object_t *obj;
	mem_hier_interface_t *iface;
	
	mem_hier_t *mem_hier;
	string checkpoint_file;
	string mh_runtime_config_file;
    	chip_t    *chip;

public:
	//
	mem_hier_handle_t (conf_object_t *obj, mem_hier_interface_t *iface,
		proc_object_t *proc);
	~mem_hier_handle_t ();

	//
	cycles_t lsq_operate (conf_object_t *proc_obj, conf_object_t *space, 
		map_list_t *map, generic_transaction_t *mem_op);

	static cycles_t static_lsq_operate (conf_object_t *proc_obj, conf_object_t *space, 
		map_list_t *map, generic_transaction_t *mem_op);

	//
	cycles_t snoop_operate (conf_object_t *proc_obj, conf_object_t *space,
		map_list_t *map, generic_transaction_t *mem_op);

	static cycles_t static_snoop_operate (conf_object_t *proc_obj, conf_object_t *space,
		map_list_t *map, generic_transaction_t *mem_op);

	//
	void make_request (mem_trans_t *trans);

	//
	void complete_request (conf_object_t *obj, conf_object_t *cpu, 
		mem_trans_t *trans);
	
	static void static_complete_request (conf_object_t *obj, conf_object_t *cpu, 
		mem_trans_t *trans);

	//
	void invalidate_address (conf_object_t *obj, conf_object_t *cpu, 
		invalidate_addr_t *invalid_addr);

	static void static_invalidate_address (conf_object_t *obj, conf_object_t *cpu, 
		invalidate_addr_t *invalid_addr);

	// 
	cycles_t handle_icache (mem_xaction_t *mem_xaction);
	
	void print_stats (void);
	
	// TODO: get
	void set_cpus (conf_object_t **cpus, uint32 num_cpus);
	void set_runtime_config (string config);
	string get_runtime_config ();
	
	// TODO:
	void set_checkpoint_file (string filename) { }
	string get_checkpoint_file () { return ""; }

	// DRAMSim2 utilities
	void printStats();

    	// Checkpoint utilities
	void stall_mem_hier();
    	bool is_mem_hier_quiet();
    
    	void clear_stats();
	
	tick_t get_mem_g_cycles();
	void profile_commits(dynamic_instr_t *dinst);
    
};

#endif

