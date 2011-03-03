/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: ctrl_flow.cc,v 1.4.6.10 2006/08/18 14:30:19 kchak Exp $");

#include "definitions.h"
#include "window.h"
#include "iwindow.h"
#include "sequencer.h"
#include "fu.h"
#include "isa.h"
#include "chip.h"
#include "eventq.h"
#include "dynamic.h"
#include "mai_instr.h"
#include "mai.h"
#include "ctrl_flow.h"
#include "yags.h"
#include "cascade.h"
#include "ras.h"
#include "proc_stats.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "config_extern.h"
#include "thread_scheduler.h"


bpred_state_t::bpred_state_t () {
	direct_state = 0;
	indirect_state = 0;
	ras_state = new ras_state_t;
}

void
bpred_state_t::clear () {
	direct_state = 0;
	indirect_state = 0;
	ras_state->clear ();
}

bpred_state_t::~bpred_state_t () {
	delete ras_state;
}

void
bpred_state_t::copy_state (bpred_state_t *s, bool ras_copy) {
	direct_state = s->direct_state;
	indirect_state = s->indirect_state;
	if (ras_copy) ras_state->copy_state (s->ras_state);
}

ctrl_flow_t::ctrl_flow_t (sequencer_t *_seq, addr_t pc, addr_t npc, uint32 _ctx, 
    yags_t *_y, cascade_t *_cas) : 
	seq (_seq), p (_seq->get_chip()),  t_ctxt(_ctx) {

	gpc = new gpc_t (pc, npc); 
	spec_bpred = new bpred_state_t;
	commit_bpred = new bpred_state_t;

	//unified_directbp = new yags_t (g_conf_yags_choice_bits,
	//	g_conf_yags_exception_bits, g_conf_yags_tag_bits);

    unified_directbp = _y;
    
	if (g_conf_separate_kbp)
		k_directbp = new yags_t (g_conf_yags_choice_bits,
			g_conf_yags_exception_bits, g_conf_yags_tag_bits);

	//indirectbp = new cascade_t (g_conf_cas_filter_bits,
	//	g_conf_cas_exception_bits, g_conf_cas_exception_shift);

    indirectbp = _cas;
    
    ras = new ras_t (g_conf_ras_table_bits, g_conf_ras_filter_bits);
	//ras = new ras_t (g_conf_ras_table_bits);
    compute_migrate = false;
    call_depth = 0;

}

ctrl_flow_t::~ctrl_flow_t () {
	delete gpc;
	delete spec_bpred;
	delete commit_bpred;

	delete unified_directbp;
	delete k_directbp;

	delete indirectbp;
}

addr_t
ctrl_flow_t::get_pc () {
	return gpc->pc;
}

addr_t
ctrl_flow_t::get_npc () {
	return gpc->npc;
}

void
ctrl_flow_t::advance_gpc (dynamic_instr_t *d_instr) {
	mai_t *mai = seq->get_mai_object (t_ctxt);
	yags_t *directbp = (mai->is_supervisor () && g_conf_separate_kbp) ? k_directbp : unified_directbp;

    proc_stats_t *pstats = seq->get_pstats (t_ctxt);
	proc_stats_t *tstats = seq->get_tstats (t_ctxt);
	
    
	ASSERT (d_instr->get_type () != IT_TYPES);
	if (d_instr->get_type () == IT_CONTROL) 
		ASSERT (d_instr->get_branch_type () != BRANCH_TYPES);

	switch (d_instr->get_branch_type ()) {

	case BRANCH_COND: 
		if (directbp) direct_pred (d_instr); else increment_gpc ();
		break;

	case BRANCH_PCOND:
		if (directbp) direct_pred (d_instr); else increment_gpc ();
		break;
	
	case BRANCH_INDIRECT:
		if (indirectbp) indirect_pred (d_instr); else increment_gpc ();
		break;
	
	case BRANCH_UNCOND:
		unconditional (d_instr);
		break;

	case BRANCH_CALL:
        call_depth++;
        pstats->stat_call_depth->inc_total(1 , call_depth);
        tstats->stat_call_depth->inc_total(1 , call_depth);
		if (ras) call_pred (d_instr);
		break;
	
	case BRANCH_RETURN:
        if (call_depth) call_depth--;
		if (ras) return_pred (d_instr);
		break;
	
	default:
		increment_gpc ();
		break;
	}

	d_instr->set_pred_o_gpc (gpc);
	d_instr->set_bpred_state (spec_bpred);
}

void
ctrl_flow_t::write_checkpoint(FILE *file)
{
    fprintf(file, "%llu %llu %u\n", gpc->pc, gpc->npc, t_ctxt);
    fprintf(file, "%u %u %u %u\n", (uint32)commit_bpred->direct_state, (uint32)commit_bpred->indirect_state, commit_bpred->ras_state->top, commit_bpred->ras_state->next_free);
    fprintf(file, "%u %u %u %u\n", (uint32)spec_bpred->direct_state, (uint32)spec_bpred->indirect_state, spec_bpred->ras_state->top, spec_bpred->ras_state->next_free);
    unified_directbp->write_checkpoint(file);
    if (g_conf_separate_kbp)
        k_directbp->write_checkpoint(file);
    indirectbp->write_checkpoint(file);
    ras->write_checkpoint(file);
}

void
ctrl_flow_t::read_checkpoint(FILE *file)
{
    fscanf(file, "%llu %llu %u\n", &gpc->pc, &gpc->npc, &t_ctxt);
    fscanf(file, "%u %u %u %u\n", &commit_bpred->direct_state, &commit_bpred->indirect_state, &commit_bpred->ras_state->top, &commit_bpred->ras_state->next_free);
    fscanf(file, "%u %u %u %u\n", &spec_bpred->direct_state, &spec_bpred->indirect_state, &spec_bpred->ras_state->top, &spec_bpred->ras_state->next_free);
    unified_directbp->read_checkpoint(file);
    if (g_conf_separate_kbp)
        k_directbp->read_checkpoint(file);
    indirectbp->read_checkpoint(file);
    ras->read_checkpoint(file);
}

bool
ctrl_flow_t::check_advanced_gpc (dynamic_instr_t *d_instr) {
	mai_t *mai = seq->get_mai_object (t_ctxt);
	yags_t *directbp = (mai->is_supervisor () && g_conf_separate_kbp) ? k_directbp : unified_directbp;

	bool match = true;
    gpc_t *p_gpc;
    gpc_t *a_gpc;

	if (!d_instr->match_o_gpc ()) {
		match = false;

        switch (d_instr->get_branch_type ()) {
		case BRANCH_COND: 
			if (directbp) direct_fixup_state (d_instr);
			break;

		case BRANCH_PCOND:
			if (directbp) direct_fixup_state (d_instr);
			break;

		case BRANCH_INDIRECT:
			if (indirectbp) indirect_fixup_state (d_instr);
			break;

		case BRANCH_UNCOND:	
			WARNING;
			break;
		
		case BRANCH_CALL:
			if (ras) call_fixup_state (d_instr);
			break;

		case BRANCH_RETURN:
            p_gpc = d_instr->get_pred_o_gpc ();
            a_gpc = d_instr->get_actual_o_gpc ();
            // DEBUG_OUT("[seq%u] predicted PC %llx NPC %llx : actual PC %llx NPC %llx\n",
            //     seq->get_id(),p_gpc->pc, p_gpc->npc, a_gpc->pc, a_gpc->npc);
            // ras->debug(commit_bpred->ras_state);
            // DEBUG_OUT("spec : ");
            // ras->debug(spec_bpred->ras_state);
            // ras->fix_state(commit_bpred->ras_state, 4, a_gpc->npc);
            // ras->update_filter_pc (a_gpc->pc, a_gpc->npc);
            // ras->debug(commit_bpred->ras_state);
			if (ras) return_fixup_state (d_instr);
			break;

		default:
			break;
		}
	} 

	return match;	
}


// --- this function should be called with empty iq and after the mai
// functions have been called ---
bpred_state_t *ctrl_flow_t::get_bpred_state (bool commit)
{
    if (commit) return commit_bpred;
    return spec_bpred;
}


void
ctrl_flow_t::fix_spec_state (bool ras_copy) {
	// fix the spec bpred to the commit bpred
    
    
	spec_bpred->copy_state (commit_bpred, ras_copy);

	// set pc and npc from mai
	mai_t *mai = seq->get_mai_object (t_ctxt);
	gpc->pc = mai->get_pc ();
	gpc->npc = mai->get_npc ();
    
    seq->ctrl_flow_redirect(t_ctxt);

}

void 
ctrl_flow_t::fix_spec_state (dynamic_instr_t *d_instr) {

    	
    
	
	if (!d_instr) {
		fix_spec_state (); 
	} else {
		spec_bpred->copy_state (d_instr->get_bpred_state ());
		
		if (d_instr->retire_ready ()) {
			gpc->pc  = d_instr->get_actual_o_gpc ()->pc;
			gpc->npc = d_instr->get_actual_o_gpc ()->npc;
		} else {
			gpc->pc  = d_instr->get_pred_o_gpc ()->pc;
			gpc->npc = d_instr->get_pred_o_gpc ()->npc;
		}			
	}
    
    seq->ctrl_flow_redirect(t_ctxt);

}

void
ctrl_flow_t::update_commit_state (dynamic_instr_t *d_instr) {
	mai_t *mai = seq->get_mai_object (t_ctxt);
	yags_t *directbp = (mai->is_supervisor () && g_conf_separate_kbp) ? k_directbp : unified_directbp;

	switch (d_instr->get_branch_type ()) {
	case BRANCH_COND: 
		if (directbp) direct_update (d_instr);
		break;
	case BRANCH_PCOND:
		if (directbp) direct_update (d_instr);
		break;
	case BRANCH_INDIRECT:
		if (indirectbp) indirect_update (d_instr);
		break;
	case BRANCH_UNCOND:
		break;
	case BRANCH_CALL:
		break;
	case BRANCH_RETURN:	
        ras->update_commit(d_instr->get_actual_o_gpc()->npc);
		break;
	default:
		break;
	}

	commit_bpred->copy_state (d_instr->get_bpred_state ());
}

void ctrl_flow_t::increment_gpc () {
	gpc->pc = gpc->npc;
	gpc->npc = gpc->npc + sizeof (uint32);
    
    
    // if (compute_migrate) {
    //     seq->potential_thread_switch(t_ctxt, YIELD_EXCEPTION);
    //     compute_migrate = false;
    // }
}


// commit calls
void
ctrl_flow_t::direct_update (dynamic_instr_t *d_instr) {
	mai_t *mai = seq->get_mai_object (t_ctxt);
	yags_t *directbp = (mai->is_supervisor () && g_conf_separate_kbp) ? k_directbp : unified_directbp;

	directbp->update (d_instr->get_pc (), 
		commit_bpred->direct_state,
		d_instr->is_taken ());
}

// commit calls
void
ctrl_flow_t::indirect_update (dynamic_instr_t *d_instr) {
	gpc_t *o_gpc = d_instr->get_actual_o_gpc ();

	indirectbp->update (d_instr->get_pc (), 
		commit_bpred->indirect_state, o_gpc->npc);
}


void
ctrl_flow_t::call_pred (dynamic_instr_t *d_instr) {
	addr_t target_pc;

	if (d_instr->get_opcode () == i_call) {
		target_pc = gpc->pc + d_instr->get_offset ();
		gpc->pc = gpc->npc;
		gpc->npc = target_pc;
        
        if (d_instr->get_mai_instruction()->is_32bit()) {
			gpc->npc = gpc->npc & 0xffffffff;
		}
        // if (g_conf_scheduling_algorithm == FUNCTION_SCHEDULING_POLICY &&
        //     p->get_g_cycles () > 10000)
        // {
        //     if (p->get_scheduler() && 
        //         p->get_scheduler()->thread_yield(target_pc, seq, t_ctxt))
        //         compute_migrate = true;
        // }
                
	} else if (indirectbp) 
		indirect_pred (d_instr); 
	else 
		increment_gpc ();

	ras->push (d_instr->get_pc () + (sizeof (uint32) * 2), 
		spec_bpred->ras_state);
}

void
ctrl_flow_t::return_pred (dynamic_instr_t *d_instr) {
	proc_stats_t *pstats = seq->get_pstats (t_ctxt);
	proc_stats_t *tstats = seq->get_tstats (t_ctxt);
	gpc->pc = gpc->npc;
    //gpc->npc = ras->filter_pc (gpc->pc);
    // if (gpc->npc)
    //     ras->fix_state (spec_bpred->ras_state, 4, gpc->npc);
    // else
    gpc->npc = ras->pop (spec_bpred->ras_state);

	STAT_INC (pstats->stat_ras_predictions);
	STAT_INC (tstats->stat_ras_predictions);
    mai_t *mai = seq->get_mai_object (t_ctxt);
    if (mai->is_supervisor()) {
        STAT_INC (pstats->stat_ras_predictions_kernel);
        STAT_INC (tstats->stat_ras_predictions_kernel);
    }
}

void
ctrl_flow_t::unconditional (dynamic_instr_t *d_instr) {
	if (!d_instr->is_taken ()) {
		if (d_instr->get_annul ()) {
			gpc->pc = gpc->pc + (2 * sizeof (uint32));
			gpc->npc = gpc->pc + sizeof (uint32);
		} else {
			increment_gpc ();
		}
	} else {
		addr_t target_pc = gpc->pc + d_instr->get_offset ();
		if (d_instr->get_annul ()) {
			gpc->pc = target_pc;
			gpc->npc = gpc->pc + sizeof (uint32);
		} else {
			gpc->pc = gpc->npc;
			gpc->npc = target_pc;
		}
	}
}
	
void
ctrl_flow_t::direct_pred (dynamic_instr_t *d_instr) {
	mai_t *mai = seq->get_mai_object (t_ctxt);
	yags_t *directbp = (mai->is_supervisor () && g_conf_separate_kbp) ? k_directbp : unified_directbp;

	proc_stats_t *pstats = seq->get_pstats (t_ctxt);
	proc_stats_t *tstats = seq->get_tstats (t_ctxt);
	bool prediction;

	STAT_INC (pstats->stat_direct_predictions);
	STAT_INC (tstats->stat_direct_predictions);
    if (mai->is_supervisor())
    {
        STAT_INC (pstats->stat_direct_predictions_kernel);
        STAT_INC (tstats->stat_direct_predictions_kernel);
    }

	prediction = directbp->predict (gpc->pc, spec_bpred->direct_state);
	directbp->update_state (spec_bpred->direct_state, prediction);

	if (prediction) {
		// conditional branch, taken.
		addr_t target_pc = gpc->pc + d_instr->get_offset ();
		gpc->pc = gpc->npc;
		gpc->npc = target_pc;
	} else {
		// conditional branch, not taken.
		if (d_instr->get_annul ()) {
			// annul delay slot
			gpc->pc = gpc->pc + (2 * sizeof (uint32));
			gpc->npc = gpc->pc + sizeof (uint32);
		} else {
			gpc->pc = gpc->npc;
			gpc->npc = gpc->npc + sizeof (uint32);
		}
	}
}

void
ctrl_flow_t::indirect_pred (dynamic_instr_t *d_instr) {
	proc_stats_t *pstats = seq->get_pstats (t_ctxt);
	proc_stats_t *tstats = seq->get_tstats (t_ctxt);
	addr_t pred_npc;

    mai_t *mai = seq->get_mai_object (t_ctxt);
	STAT_INC (pstats->stat_indirect_predictions);
	STAT_INC (tstats->stat_indirect_predictions);
    if (mai->is_supervisor()) {
        STAT_INC (pstats->stat_indirect_predictions_kernel);
        STAT_INC (tstats->stat_indirect_predictions_kernel);
    }

	pred_npc = indirectbp->predict (gpc->pc, spec_bpred->indirect_state);
	indirectbp->update_state (spec_bpred->indirect_state, pred_npc);

	gpc->pc = gpc->npc;
	gpc->npc = pred_npc;
}

void
ctrl_flow_t::direct_fixup_state (dynamic_instr_t *d_instr) {
	mai_t *mai = seq->get_mai_object (t_ctxt);
	yags_t *directbp;
	directbp = (mai->is_supervisor () && g_conf_separate_kbp) ? k_directbp : unified_directbp;

	proc_stats_t *pstats = seq->get_pstats (t_ctxt);
	proc_stats_t *tstats = seq->get_tstats (t_ctxt);
	directbp->fix_state (d_instr->get_bpred_state ()->direct_state);
	STAT_INC (pstats->stat_direct_mispredicts);
	STAT_INC (tstats->stat_direct_mispredicts);
    if (mai->is_supervisor()) {
        STAT_INC (pstats->stat_direct_mispredicts_kernel);
        STAT_INC (tstats->stat_direct_mispredicts_kernel);
    }
}



void
ctrl_flow_t::indirect_fixup_state (dynamic_instr_t *d_instr) {
    mai_t *mai = seq->get_mai_object (t_ctxt);
	proc_stats_t *pstats = seq->get_pstats (t_ctxt);
	proc_stats_t *tstats = seq->get_tstats (t_ctxt);
	indirectbp->fix_state (
		d_instr->get_bpred_state ()->indirect_state, 
		d_instr->get_actual_o_gpc ()->npc
	);

	STAT_INC (pstats->stat_indirect_mispredicts);
	STAT_INC (tstats->stat_indirect_mispredicts);
    if (mai->is_supervisor()) {
        STAT_INC (pstats->stat_indirect_mispredicts_kernel);
        STAT_INC (tstats->stat_indirect_mispredicts_kernel);
    }
}

void 
ctrl_flow_t::call_fixup_state (dynamic_instr_t *d_instr) {
	if (indirectbp && d_instr->get_opcode () == i_jmpl) 
		indirect_fixup_state (d_instr);
	else if (d_instr->get_opcode () == i_call) 
		WARNING;
}

void
ctrl_flow_t::return_fixup_state (dynamic_instr_t *d_instr) {
    ras->fix_state (
		d_instr->get_bpred_state ()->ras_state, 
		d_instr->get_actual_o_gpc ()->npc,
		d_instr->get_pred_o_gpc ()->npc
	);
    
    
    mai_t *mai = seq->get_mai_object (t_ctxt);
	proc_stats_t *pstats = seq->get_pstats (t_ctxt);
	proc_stats_t *tstats = seq->get_tstats (t_ctxt);
	STAT_INC (pstats->stat_ras_mispredicts);
	STAT_INC (tstats->stat_ras_mispredicts);
    
    if (mai->is_supervisor()) {
        STAT_INC (pstats->stat_ras_mispredicts_kernel);
        STAT_INC (tstats->stat_ras_mispredicts_kernel);
    }
}

void
ctrl_flow_t::set_ctxt(uint32 _c)
{
    t_ctxt = _c;
}

void
ctrl_flow_t::set_direct_bp(yags_t *_y)
{
    // only unified for now ...
    unified_directbp = _y;
}

void
ctrl_flow_t::set_indirect_bp(cascade_t *_c)
{
    indirectbp = _c;
}

yags_t *
ctrl_flow_t::get_direct_bp()
{
    return unified_directbp;
}

cascade_t *
ctrl_flow_t::get_indirect_bp()
{
    return indirectbp;
}

addr_t ctrl_flow_t::get_return_top()
{
    return ras->peek_top(spec_bpred->ras_state);
}

void ctrl_flow_t::push_return_top(addr_t r_addr)
{
    ras->push(r_addr, spec_bpred->ras_state);
}

void ctrl_flow_t::copy_state(ctrl_flow_t *cflow)
{
    spec_bpred = cflow->get_bpred_state(false);
    commit_bpred = cflow->get_bpred_state(true);
    gpc->pc = cflow->get_pc();
    gpc->npc = cflow->get_npc();
    
}

