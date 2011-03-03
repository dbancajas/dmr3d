/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id $
 *
 * description:    Store History Buffer algorithm
 * initial author: Koushik Chakraborty 
 *
 */
 

#ifndef SPIN_HEURISTIC_H
#define SPIN_HEURISTIC_H


enum spin_alg_t {
    STORE_HISTORY_BUFFER,
    LOAD_HISTORY_BUFFER,
    ATOMIC_HISTORY_BUFFER,
    MAX_HISTORY_BUFFER
};

enum mem_op_type {
    MEMOP_TYPE_STORE,
    MEMOP_TYPE_LOAD,
    MEMOP_TYPE_ATOMIC,
    MEMOP_TYPE_MAX
};

class store_record_t {
    private:
        addr_t phys_addr;
        uint32 size;
        int64 value;
        uint32 dw_bytemask;
    public:
        
        store_record_t(addr_t p_a, uint32 _s, int64 _v, uint32 _bmask) ;
        void copy(addr_t p_a, uint32 _s, int64 _v, uint32 _bmask);
        bool is_identical(addr_t p_a, uint32 _s, int64 _v, uint32 _bmask);
        void debug();
        
        
};

 
class spin_heuristic_t {
    private:
        sequencer_t       *seq;
        uint32            tid;
        store_record_t    **store_history;
        addr_t           *load_history;
        uint32            load_index;
        uint32            store_index;
        uint32            store_count;
        addr_t           *atomic_pc;
        uint32            atomic_index;
        uint32            atomic_count;
        uint32            last_spin_index;
        tick_t            last_spin_cycle;
        uint64            last_spin_step;
        uint32            spin_count;
        uint32            load_threshold;
        uint32            saturating_counter;
        bool              proc_mode_change;
        
        void search_and_insert_store(addr_t pa, int64 val, uint32 bytemask,
            uint32 size);
        void search_and_insert_load(addr_t pa);
        void search_and_insert_atomic(addr_t pa, addr_t pc);
        
        
    
    public:
        spin_heuristic_t(sequencer_t *seq, uint32 tid);
        
        void observe_memop(addr_t PC, addr_t pa, int64 val, uint32 bytemask,
            uint32 size, mem_op_type mtype);
        void observe_d_instr(dynamic_instr_t *d_instr);    
        bool check_spinloop() ;
        void reset();
        void emit_store_history(mai_t *mai);
        void emit_load_history(mai_t *mai);
        void detected_recognized_spin(addr_t PC, uint32 thread_id);
        void observe_mem_trans(mem_trans_t *, generic_transaction_t * mem_op);
        void register_proc_mode_change();
        void observe_commit_pc(dynamic_instr_t *d_instr);
        
};
        
#endif 
