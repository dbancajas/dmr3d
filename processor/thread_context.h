/*
 * Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

 
 
class thread_context_t {
    public:
    
        thread_context_t(uint32);
        
        sequencer_t *get_sequencer();
        uint32       get_id();
        void         set_seq_ctxt(uint32 );
        uint32       get_seq_ctxt();
        void         set_sequencer(sequencer_t *_s);
        
        
    private:
        sequencer_t *seq;
        uint32       id; 
        uint32       seq_ctxt_id; // which thread ctxt in the sequencer
        
        
};
        
        
