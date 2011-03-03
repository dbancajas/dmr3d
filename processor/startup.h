
#ifndef _STARTUP_H_
#define _STARTUP_H_

class proc_object_t {
public:
	conf_object_t              obj;    

	chip_t                     *chip;
	uint32                     num_cpus;

	mem_hier_handle_t          *mem_hier;
	mem_driver_t               *mem_driver;
	
	timing_model_interface_t   *timing_iface;
	timing_model_interface_t   *snoop_iface;

	proc_interface_t           *proc_iface;

	mmu_interface_t            *mmu_interface;
	
	// mh integreation
	conf_object_t *mem_image;
	image_interface *mem_image_iface;

	int64 tl_reg;
	
	string config_value_to_get;
	
//	proc_tm_t* search_proc_tm (conf_object_t *cpu, bool &shadow);
	mem_hier_handle_t *get_mem_hier (void);
	mem_driver_t *get_mem_driver (void);
	void set_mem_hier (mem_hier_handle_t *_mem_hier);
	void set_mem_driver (mem_driver_t *_mem_driver);

	void create_dummy_mem_hier (void);
	void get_stats (void);

    void stall_processor (void);
    void raise_checkpoint_hap ();
    
    void do_finalize ();
    void clear_stats ();
    
    tick_t  get_chip_cycles();
    
    bool pending_checkpoint;
    bool *cross_call_ignore;
    
    void read_mem_hier_config (string c);
    bool mem_hier_config_read;
   
};


#endif
