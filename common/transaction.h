/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _TRANSACTION_H_
#define _TRANSACTION_H_

/* $Id: transaction.h,v 1.4.2.19 2006/12/12 18:36:57 pwells Exp $
 *
 * description:    a memory transaction
 * initial author: Philip Wells 
 *
 */
 
const uint32 MEM_EVENT_L1_HIT =				1<<0;
const uint32 MEM_EVENT_L1_DEMAND_MISS =		1<<1;
const uint32 MEM_EVENT_L1_COHER_MISS =		1<<2;
const uint32 MEM_EVENT_L1_MSHR_PART_HIT =	1<<3;
const uint32 MEM_EVENT_L1_STALL =			1<<4;
const uint32 MEM_EVENT_L1_WRITE_BACK =		1<<5;

const uint32 MEM_EVENT_L2_HIT =				1<<6;
const uint32 MEM_EVENT_L2_DEMAND_MISS =		1<<7;
const uint32 MEM_EVENT_L2_COHER_MISS =		1<<8;
const uint32 MEM_EVENT_L2_MSHR_PART_HIT = 	1<<9;
const uint32 MEM_EVENT_L2_STALL =			1<<10;
const uint32 MEM_EVENT_L2_WRITE_BACK =		1<<11;
const uint32 MEM_EVENT_L2_REPLACE =			1<<12;
const uint32 MEM_EVENT_L2_REQ_FWD =			1<<13;

const uint32 MEM_EVENT_L1DIR_DEMAND_MISS =	1<<14;
const uint32 MEM_EVENT_L1DIR_COHER_MISS =	1<<15;
const uint32 MEM_EVENT_L1DIR_STALL =		1<<16;
const uint32 MEM_EVENT_L1DIR_WRITE_BACK =	1<<17;
const uint32 MEM_EVENT_L1DIR_REPLACE =		1<<18;
const uint32 MEM_EVENT_L1DIR_REQ_FWD =		1<<19;
const uint32 MEM_EVENT_L1DIR_HIT =			1<<20;

const uint32 MEM_EVENT_L3_HIT =				1<<21;
const uint32 MEM_EVENT_L3_DEMAND_MISS =		1<<22;
const uint32 MEM_EVENT_L3_COHER_MISS =		1<<23;
const uint32 MEM_EVENT_L3_MSHR_PART_HIT = 	1<<24;
const uint32 MEM_EVENT_L3_STALL =			1<<25;
const uint32 MEM_EVENT_L3_REPLACE =			1<<26;
const uint32 MEM_EVENT_L3_REQ_FWD =			1<<27;

const uint32 MEM_EVENT_MAINMEM_WRITE_BACK = 1<<28;
const uint32 MEM_EVENT_DEFERRED =		    1<<29;
const uint32 PROC_REQUEST          =        1<<30;
const uint32 MEM_XACTION_CREATE    =        1<<31;

 
// TODO: Should go somewhere else
// Structure to pass to invalidate_address() 
class invalidate_addr_t {
 public:
	addr_t        address; // Aligned *physical* address
	uint32        size;    // Size of region

	addr_t get_phys_addr () {
		return address;
	}

	uint32 get_size () {
		return size;
	}
};


enum prefetch_type_t {
    NO_PREFETCH,
    FB_PREFETCH,
    LDST_PREFETCH,
    CACHE_PREFETCH,
    MAX_PREFETCH
};

enum kernel_access_type_t {
    LWP_STACK_ACCESS,
    KERNEL_MAP_ACCESS,
    AS_USER_KERNEL_ACCESS,
    OTHER_TYPE_ACCESS,
    MAX_KERNEL_ACCESS
};


// Memory hierarchy transaction generated for every memory request
class mem_trans_t {
	
public:
	// Basic info
	addr_t phys_addr;              // physical address
	addr_t virt_addr;              // virtual address
	uint32 size;                 // request size
	uint8 asi;                   // Address Space ID                    
	uint8 cpu_id;                // starting from 0, no holes

	unsigned read:1;             // 1 load, 0 not
	unsigned write:1;            // 1 store, 0 not
	unsigned atomic:1;           // part of a atomic LD/ST pair
	unsigned hw_prefetch:1;      // HARDWARE generated prefetch
	unsigned ifetch:1;           // 1 if instruction fetch
	unsigned control:1;          // 1 if a control instruction (eg flush)
	unsigned speculative:1;      // Access is speculative
	unsigned supervisor:1;       // CPU in supervisor (kernel) mode
	unsigned priv:1;             // Access to priveledged ASI
	
	// Interface Info
	unsigned int call_complete_request:1; // Should call complete_req for trans?

	// Sparc V9 specific info
	unsigned sw_prefetch:1;      // Software prefetch instruction
	unsigned int prefetch_fcn;   // Prefetch field from instruction (SW prefetch)
	unsigned cache_phys:1;       // TLBs CP bit
	unsigned cache_virt:1;       // TLBs CV bit
	unsigned side_effect:1;      // Set if page has side effects
	
	// Data related info
	unsigned inverse_endian:1;   // Inverse byte order from normal
	uint32 page_cross;
	uint8 *data;                  // Data of this transaction

	// Membars
	unsigned int membar_mflag;   // memory: loadload, loadstore, etc
	unsigned int membar_cflag;   // control: lookaside, mem_issue, sync, etc

	// Simics specific info
	int id;                      // simics mem_op ID
	conf_object_t *ini_ptr;
	unsigned int io_device:1;    // Initiator is not a proc
	unsigned int may_stall:1;    // Will simics accept a stall?
	
	// Processor info
	dynamic_instr_t *dinst;      // pointer to dynamic instruction 
	
	// LSQ Info
	//lsq_entry_t lsq_entry;       // LSQ Status entry
	//uint32 lsq_slot;             // LSQ Location
	
	// Memory Hierarchy info
	cycles_t request_time;       // Cycle first request was made (to mem hier)
	unsigned completed:1;        // Set when transaction is done

	uint32 events;				// Events that occured to this trans
	uint32 pending_events;		// Pending events for this trans
	uint32 pending_messages;    // number of pending messages for this transaction
	
	// I-TLB information
	bool itlb_miss;              // indicates if the instruction caused I-TLB miss 
    bool cache_prefetch;        // cache prefetch
    bool random_trans;          // transaction generated by the tester
    prefetch_type_t prefetch_type;
	
	sequencer_t *sequencer;  // sequencer that initiated transaction
    sequencer_t *mem_hier_seq; // sequencer connecting the caches serving this trans
	
	bool vcpu_state_transfer;
	uint32 sequencer_ctxt;  // only valid for vcpu state transfers
    
    uint32 unique_id; // Unique ID to ease tracking
	
public:

	// Constructors
	mem_trans_t();
	mem_trans_t(mem_trans_t &);
	mem_trans_t(v9_memory_transaction* trans);
	// Destructor
	~mem_trans_t();

	void clear();
	void copy(v9_memory_transaction* trans);          // Copy info from v9
    void copy(mem_trans_t &);                         // Copy from mem_trans

	bool is_cacheable();
	bool is_normal_asi();
	bool is_io_space();
	bool is_prefetch();

	void mark_pending(uint32 event);			// Mark an event as occured (and pending)
	void mark_event(uint32 event);	// Mark an event as occured
	void clear_pending_event(uint32 event);	// Mark an event as non-pending
	bool occurred(uint32 event);			// Did event occur?
	bool is_pending(uint32 event);			// Is event pending?
	
	uint32 get_events();
	uint32 get_pending_events();
	
	bool get_itlb_miss();
	void set_itlb_miss(bool);
    void clear_all_events();
    void recycle();
    void debug();
    
    void report_event(bool pending = false);
    prefetch_type_t get_prefetch_type();
	
	void set_dinst(dynamic_instr_t *dinst);
	void merge_changes(mem_trans_t *old_trans);
    bool is_as_user_asi();
    
    kernel_access_type_t get_access_type();
    void set_sequencer(sequencer_t *seq);
    bool is_thread_state();
	
	bool kernel() { return supervisor; }
};

#endif // _TRANSACTION_H_
