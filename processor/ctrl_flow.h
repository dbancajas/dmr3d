/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _CTRL_PREDICT_H_
#define _CTRL_PREDICT_H_

class gpc_t {
public:	
	addr_t pc;
	addr_t npc;

	gpc_t () { clear (); }
	gpc_t (addr_t _pc, addr_t _npc) : pc (_pc), npc (_npc) {}
	void clear () { pc = 0; npc = 0; }
};

class bpred_state_t {
public:
	direct_state_t   direct_state;
	indirect_state_t indirect_state;
	ras_state_t      *ras_state;

	bpred_state_t ();
	~bpred_state_t ();
	void copy_state (bpred_state_t *s, bool ras_copy = true);
    
	void clear ();
};


class ctrl_flow_t {

private:

	sequencer_t     * const seq;
	chip_t          * const p;

	gpc_t           *gpc;
	bpred_state_t   *spec_bpred;
	bpred_state_t   *commit_bpred;

	yags_t          *unified_directbp;
	yags_t          *k_directbp;

	cascade_t       *indirectbp;
	ras_t           *ras;
    
    uint32          t_ctxt; // thread context in the sequencer
    bool            compute_migrate;
    addr_t          return_pc;
    uint32          call_depth;

public:
	ctrl_flow_t (sequencer_t *_seq, addr_t pc, addr_t npc, uint32 t_ctxt, 
        yags_t *_y, cascade_t *_cas);
	~ctrl_flow_t ();

	addr_t          get_pc (void);
	addr_t          get_npc (void);

	void            advance_gpc (dynamic_instr_t *d_instr);
	bool            check_advanced_gpc (dynamic_instr_t *d_instr);
	void            fix_spec_state (bool ras_copy = true);
	void            fix_spec_state (dynamic_instr_t *d_instr);	
	void            update_commit_state (dynamic_instr_t *d_instr);
	void            increment_gpc ();

	void            direct_pred (dynamic_instr_t *d_instr);
	void            direct_fixup_state (dynamic_instr_t *d_instr);
	void            direct_update (dynamic_instr_t *d_instr);

	void            indirect_pred (dynamic_instr_t *d_instr);
	void            indirect_fixup_state (dynamic_instr_t *d_instr);
	void            indirect_update (dynamic_instr_t *d_instr);	

	void            unconditional (dynamic_instr_t *d_instr);

	void            call_pred (dynamic_instr_t *d_instr);
	void            call_fixup_state (dynamic_instr_t *d_instr);

	void            return_pred (dynamic_instr_t *d_instr);
	void            return_fixup_state (dynamic_instr_t *d_instr);
    
    void            read_checkpoint(FILE *file);
    void            write_checkpoint(FILE *file);
    void            set_ctxt(uint32 _c); 

    void            set_direct_bp(yags_t *_y);
    void            set_indirect_bp(cascade_t *_c);
    yags_t *        get_direct_bp();
    cascade_t *     get_indirect_bp();
    bpred_state_t * get_bpred_state(bool commit);
    addr_t          get_return_top();
    void            push_return_top(addr_t r_addr);
    
    void copy_state (ctrl_flow_t *);
};

#endif
