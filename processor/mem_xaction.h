/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _MEM_XACTION_H_
#define _MEM_XACTION_H_

class mem_xaction_t {

private:
    v9_mem_xaction_t *v9_mem_xaction;
	mem_trans_t       *mem_hier_trans;

	safe_uint64       value;
	uint8             blk_values[64];

	uint32            size;
	addr_t            phys_addr;
	addr_t            virt_addr;

	bool              void_write;
	dynamic_instr_t   *dep_instr;

	safe_uint64       bypass_value;
	uint8             bypass_blk_values[64];
	bool              blk_bypass_valid;

    addr_t            dw_phys_addr;
	uint32            dw_bytemask;

    addr_t            blk_phys_addr;

	snoop_status_t    snoop_status;
	
	bool              second_ldd_memop;

public:
	mem_xaction_t (v9_mem_xaction_t *_v9, mem_trans_t *_trans);
	~mem_xaction_t ();

	void set_value (uint64 _v);
	void set_blk_value (uint8 *_v);
	void set_size (uint32 _s);
	void set_phys_addr (addr_t _a);
	void set_virt_addr (addr_t _v);

	uint64 get_value (void);
	uint8 *get_blk_value (void);
	uint32 get_size (void);
	uint32 get_size_index (void);
	addr_t get_phys_addr (void);
	addr_t get_virt_addr (void);
	
	v9_mem_xaction_t* get_v9 (void);
	void set_v9 (v9_mem_xaction_t *_v9);

	void release_stall (void);
	void apply_store_changes (void);

	void void_xaction (void);
	bool is_void_xaction (void);

	void safety_checks (dynamic_instr_t *d_instr);

	void set_dependent (dynamic_instr_t *d_instr);
	dynamic_instr_t *get_dependent (void);

	mem_trans_t* get_mem_hier_trans (void);

	void obtain_bypass_val (void);
	addr_t get_dw_addr (void);
	addr_t get_blk_addr (void);
	uint32 get_dw_bytemask (void);
	void set_bypass_value (uint64 val);
	void set_bypass_blk_value (uint8 *val);
	uint64 get_bypass_value (void);
	uint8 *get_bypass_blk_value (void);
	void apply_bypass_value (void);
	
	uint64 munge_bypass (uint32 source_mask, uint32 dest_mask,
		uint64 value);

	void set_snoop_status (snoop_status_t snoop_status);
	snoop_status_t get_snoop_status (void);

	void merge_changes (mem_xaction_t *m);

	bool unimpl_checks (void);
	
	bool is_bypass_valid (void);
	bool is_value_valid (void);
	void setup_store_prefetch (void);
	void setup_regular_access (void);
    
    void set_mem_trans (mem_trans_t *_t) { mem_hier_trans = _t;}
};

#endif

