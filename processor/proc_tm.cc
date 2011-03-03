/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: proc_tm.cc,v 1.4.2.5 2005/10/28 18:28:02 pwells Exp $");

#include "definitions.h"
#include "window.h"
#include "iwindow.h"
#include "sequencer.h"
#include "mai.h"
#include "chip.h"
#include "mem_hier_handle.h"

#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "proc_stats.h"
#include "config_extern.h"
#include "config.h"
#include "mmu.h"

proc_tm_t::proc_tm_t (uint32 _id, conf_object_t *_cpu, string &_c) {

	set_id (_id);
	set_config (_c);

	// order important.

	config_db = new config_t ();
	config_db->register_entries ();

	if (!get_config ().empty ()) 
		config_db->parse_runtime (get_config ());

	config_db->print ();	

	string st_name = "proc_stats";
	pstats = new proc_stats_t (st_name);

	k_pstats = 0;

	if (g_conf_kernel_stats) {
		string k_st_name = "kernel_stats";
		k_pstats = new proc_stats_t (k_st_name);
	} 
	
    cpu = _cpu;
    mem_hier = NULL;
	
}


void proc_tm_t::init() {
     
    
    mai = new mai_t (cpu, this);
	seq = new sequencer_t (this);
	pstats_list = generate_pstats_list ();

	g_cycles = 1;

	// if checkpoint is inside a trap
	if (mai->get_tl ()) {
		for (uint32 i = 1; i <= mai->get_tl (); i++) 
			mai->setup_trap_context ();
	}
	
    pending_checkpoint = false;
    wait_on_checkpoint = false;
	mmu = new mmu_t (this);   
    
}

proc_tm_t::~proc_tm_t (void) {
	delete seq;
	delete mai;
	delete pstats;
	delete k_pstats;
	delete [] pstats_list;
}

void 
proc_tm_t::set_id (uint32 _id) {
	id = _id;
}

uint32 
proc_tm_t::get_id () {
	return id;
}
	
void 
proc_tm_t::advance_cycle () {

	seq->advance_cycle ();
	g_cycles++;
    mai_t * mai = get_mai_object();
    
    if (wait_on_checkpoint && mai->get_tl() == 0) {
        pending_checkpoint = true;
        wait_on_checkpoint = false;
    }
    if (wait_on_checkpoint || pending_checkpoint) {
        STAT_INC(pstats->stat_checkpoint_penalty);
        iwindow_t *iwindow = seq->get_iwindow();
        if (iwindow->empty() && mai->get_tl() != 0)
        {
            // Trap level changed during execution restart fetch
            // and execute till empty
            pending_checkpoint = false;
            wait_on_checkpoint = true;
        }
        
    }
}

void
proc_tm_t::change_to_kernel_cfg () {
	ASSERT (mai->is_supervisor ());
	if (!get_config ().empty ()) {
		string s = get_config (); s.append ("_k");
		config_db->parse_runtime (s);
	}
}

void
proc_tm_t::change_to_user_cfg () {
	ASSERT (!mai->is_supervisor ());
	if (!get_config ().empty ()) 
		config_db->parse_runtime (get_config ());
}

sequencer_t*
proc_tm_t::get_sequencer () {
	return seq;
}

mai_t*
proc_tm_t::get_mai_object () {
	return mai;
}

tick_t
proc_tm_t::get_g_cycles () {
	return g_cycles;
}

proc_stats_t*
proc_tm_t::get_pstats () {
	ASSERT (pstats);

	if (g_conf_kernel_stats && mai->is_supervisor ()) 
		return k_pstats;
	else 	
		return pstats;
}

proc_stats_t**
proc_tm_t::get_pstats_list () {
	return pstats_list;
}

proc_stats_t**
proc_tm_t::generate_pstats_list () {

	proc_stats_t **pstats_list;

	if (g_conf_kernel_stats) 
		pstats_list = new proc_stats_t *[3];
	else
		pstats_list = new proc_stats_t *[2];

	uint32 i = 0;
	pstats_list [i++] = pstats;
	if (g_conf_kernel_stats) pstats_list [i++] = k_pstats;
	
	pstats_list [i] = 0;

	return pstats_list;
}

bool
proc_tm_t::ready_for_checkpoint() 
{
    iwindow_t *iwindow = seq->get_iwindow();
    bool ready = (!wait_on_checkpoint && pending_checkpoint && iwindow->empty() 
        && get_mai_object()->get_tl() == 0 );
    return ready;
}

void
proc_tm_t::set_interrupt (int64 vector) {
	ASSERT (seq);
	seq->set_interrupt (vector);
}

void 
proc_tm_t::set_shadow_interrupt (int64 vector) {
	ASSERT (seq);
	seq->set_shadow_interrupt (vector);
}

void
proc_tm_t::set_config (string &c) {
	config = c;
}

string
proc_tm_t::get_config (void) {
	return config;
}

config_t *
proc_tm_t::get_config_db (void) {
	return config_db;
}

void
proc_tm_t::set_mem_hier (mem_hier_handle_t *_mem_hier) {
	ASSERT (_mem_hier);
	mem_hier = _mem_hier;
}

mem_hier_handle_t*
proc_tm_t::get_mem_hier (void) {
	return mem_hier;
}

void
proc_tm_t::print_stats () {
	uint32 i = 0;

	while (pstats_list [i] != 0) {
		base_counter_t *stat_elapsed_sim = pstats_list [i]->stat_elapsed_sim;
		base_counter_t *stat_start_sim = pstats_list [i]->stat_start_sim;

		stat_elapsed_sim->set (static_cast <int64> (time (0)) - 
			(int64) stat_start_sim->get_total ());

		pstats_list [i]->print ();

		i++;
	}
}

void
proc_tm_t::stall_front_end()
{
    mai_t *mai = get_mai_object();
    
    if (mai->get_tl() != 0) {
        // Need to wait till trap level goes to 0
        wait_on_checkpoint = true;
    } else {
        pending_checkpoint = true;
    }
    
    STAT_INC(pstats->stat_intermediate_checkpoint);
}

void
proc_tm_t::print_stats (proc_stats_t *pstats) {

}

mmu_t *
proc_tm_t::get_mmu () {
	return mmu;
}

void
proc_tm_t::write_checkpoint(FILE *file) 
{
	fprintf(file, "%llu\n", g_cycles);
	uint32 i = 0;
	while (pstats_list[i] != 0) {
		pstats_list[i]->stats->to_file(file);
		i++;
	}
    seq->write_checkpoint(file);
}

void
proc_tm_t::read_checkpoint(FILE *file)
{
	fscanf(file, "%llu\n", &g_cycles);
	uint32 i = 0;
	while (pstats_list[i] != 0) {
		pstats_list[i]->stats->from_file(file);
		i++;
	}
    seq->read_checkpoint(file);

    DEBUG_OUT("Setting Cycles to %llu\n", g_cycles);
}

