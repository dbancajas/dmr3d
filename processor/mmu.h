/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mmu.h,v 1.1.2.2 2005/11/02 22:59:57 kchak Exp $
 *
 * description:    mmu class
 * initial author: Philip Wells 
 *
 */
 
#ifndef _MMU_H_
#define _MMU_H_

class mmu_info_t {
public:
	mmu_info_t ();
	mmu_info_t (v9_memory_transaction_t *mem_op, bool orig_inv_endian,
		exception_type_t ex);
	
	addr_t              virt_addr;
	addr_t              phys_addr;
	bool                cache_phys;
	bool                cache_virt;
	bool                side_effect;
	bool                inv_endian_flip;
	exception_type_t    except;
};
	

class mmu_t {
	
public:

	mmu_t (sequencer_t *_seq, uint32);

	// MMU call intercept routines
	exception_type_t do_access (v9_memory_transaction_t *mem_op);

	int current_context (conf_object_t *cpu);

	void reset (conf_object_t *cpu, exception_type_t exc_no);

	exception_type_t undefined_asi (v9_memory_transaction_t *mem_op);

	exception_type_t set_error (conf_object_t *cpu_obj, exception_type_t ex,
		uint64 addr, int ft, int asi, read_or_write_t r_or_w,
		data_or_instr_t d_or_i, int atomic, int size, int priv);

	void set_error_info (mmu_error_info_t *ei);
	
	exception_type_t handle_asi (v9_memory_transaction_t *mem_op);

	// Simics MMU callbacks
	static exception_type_t static_do_access (conf_object_t *obj,
		v9_memory_transaction_t *mem_op);
		
	static int static_current_context (conf_object_t *obj, conf_object_t *cpu);
	
	static void static_reset (conf_object_t *obj, conf_object_t *cpu,
		exception_type_t exc_no);
	
	static exception_type_t static_undefined_asi (conf_object_t *mmu_obj,
		v9_memory_transaction_t *mem_op);

	static exception_type_t static_set_error (conf_object_t *mmu_obj,
		conf_object_t *cpu_obj, exception_type_t ex, uint64 addr, int ft, 
		int asi, read_or_write_t r_or_w, data_or_instr_t d_or_i, int atomic,
		int size, int priv);
	
	static void static_set_error_info (mmu_error_info_t *ei);

	static exception_type_t static_handle_asi (conf_object_t *obj,
		generic_transaction_t *mem_op);

	// Init functions
	static void register_asi_handlers (conf_object_t *cpu);

	void set_simics_mmu (conf_object_t *simics_mmu);

private:

	// Helper functions
	void fill_transaction (v9_memory_transaction_t *mem_op, mmu_info_t *mmui);

	exception_type_t handle_mmu_replay (v9_memory_transaction_t *mem_op,
		dynamic_instr_t *dinst);

	void hw_fill_from_linux_pt (v9_memory_transaction_t *mem_op);

	void insert_tlb_entry (v9_memory_transaction *mem_op, uint64 pte);

	v9_memory_transaction_t *fill_inquiry_transaction (addr_t addr, uint8 asi);
	

	conf_object_t   *simics_mmu_obj;
	mmu_interface_t *simics_mmu_iface;
	mmu_asi_interface_t *simics_mmu_asi_iface;
	
	sequencer_t *seq;
    
    uint32      thread_id;                
	
	v9_memory_transaction_t *mmu_fill_op;
	v9_memory_transaction_t *mmu_inq_op;
	
	public_iq_interface_t *cpu_public_iq_iface;
};

#endif // _MMU_H_
