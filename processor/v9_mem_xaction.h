/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _V9_MEM_XACTION_H_
#define _V9_MEM_XACTION_H_

class v9_mem_xaction_t {

private:   
    mai_t *mai;

    generic_transaction_t *mem_op;   
    conf_object_t *space;
    map_list_t *map;

    conf_object_t *cpu;

public:
    v9_mem_xaction_t (mai_t *_mai, generic_transaction_t *_mem_op, 
        conf_object_t *_space, map_list_t *_map);
    ~v9_mem_xaction_t ();

	void release_stall ();
	void write_int64_phys_memory (addr_t paddr, 
		uint64 value, uint32 size, bool ie);
	void write_blk_phys_memory (addr_t paddr, 
		uint8 *value, uint32 size, bool ie);

	uint64 get_int64_value ();
	void get_blk_value (uint8 *value);
	uint32 get_size ();
	addr_t get_phys_addr ();
	addr_t get_virt_addr ();

	void block_stc ();
	void squashed_noreissue ();
	void void_xaction ();

	bool is_cpu_xaction ();
	bool is_icache_xaction ();
	bool is_store ();
	bool is_atomic ();

	dynamic_instr_t *get_dynamic_instr ();
	generic_transaction_t *get_mem_op ();

	void set_mem_op (generic_transaction_t *_mem_op);
	bool can_stall ();

	void set_user_ptr (void *ptr);
	void* get_user_ptr (void);

	v9_memory_transaction_t *get_v9_mem_op ();
	void set_bypass_value (uint64 val);
	void set_bypass_blk_value (uint8 *val);
	
	bool is_speculative (void);

	bool unimpl_checks (void);
	bool is_side_effect (void);
	
	static bool is_unsafe_asi(int64 asi);
};

#endif
