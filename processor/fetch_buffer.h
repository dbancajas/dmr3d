/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *

 * $Id: fetch_buffer.h,v 1.1.2.5 2005/11/15 13:43:58 kchak Exp $
 *
 * description:    header for fetch buffer
 * initial author: Koushik Chakraborty
 *
 */
 
 

#ifndef _FETCH_BUFFER_H_
#define _FETCH_BUFFER_H_


class fetch_line_entry_t {
    private:
    addr_t fetch_line_addr;
    list<dynamic_instr_t *> wait_list;
    bool ready;
    bool prefetch;
    mem_trans_t *trans;
    bool useful;
    tick_t insert_time;
    public:
    
        fetch_line_entry_t(addr_t, bool, mem_trans_t *);
        
        void insert_instr(dynamic_instr_t *);
        addr_t get_fetch_addr();
        bool wait_list_empty();
        void wake_up_waiting();
        void wake_up_squashed();
        void mark_ready();
        bool is_ready();
        bool is_prefetch() { return prefetch; }
        bool is_useful() { return useful; }
        void debug();
        void mark_useful() { useful = true;}
        
        ~fetch_line_entry_t();
};

class fetch_buffer_t {
    private:
    mem_hier_handle_t *mem_hier_handle;
    list<fetch_line_entry_t *> fetch_lines;
    
    sequencer_t *seq;
    
    addr_t last_proc_fetch;
    addr_t *line_pred_tags;
    addr_t *line_pred_table;
    uint64 pred_table_size;
    addr_t pred_table_mask;
    uint32 lsize_bits;
    uint32 num_lines;
    
    
    
    public:
    
        fetch_buffer_t(sequencer_t *_seq, uint32 num_ctxt);
        fetch_line_entry_t *get_line_entry(addr_t);
        uint32 initiate_fetch(addr_t, dynamic_instr_t *instr, mem_trans_t *trans);
        void complete_fetch(addr_t, mem_trans_t *trans);
        addr_t get_line_prediction(addr_t fetch_addr);
        void insert_fetch_line(addr_t fetch_addr, bool prefetch, mem_trans_t *trans);
        addr_t next_line_prediction(addr_t);
        void update_prediction_table(addr_t, addr_t);
        void initiate_prefetch(addr_t, mem_trans_t *trans);
        void delete_fetch_line();
        bool contiguous_fetch(addr_t, addr_t);
        void do_stats(fetch_line_entry_t *);
        
};


#endif

