/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: chip.cc,v 1.1.2.30 2006/08/18 14:30:18 kchak Exp $");

#include "definitions.h"
#include "window.h"
#include "iwindow.h"
#include "sequencer.h"
#include "mai.h"
#include "chip.h"
#include "mem_hier_handle.h"
#include "fastsim.h"
#include "thread_scheduler.h"

#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "proc_stats.h"
#include "config_extern.h"
#include "config.h"
#include "mmu.h"
#include "thread_context.h"
#include "mem_driver.h"
#include "mem_hier.h"
#include "checkp_util.h"
#include "disable_timer.h"
#include "profiles.h"


#ifdef POWER_COMPILE
#include "power_profile.h"
#include "core_power.h"
#include "circuit_param.h"
#include "area_param.h"
#include "power_obj.h"
circuit_param_t *chip_circuit_param = new circuit_param_t();
area_param_t *chip_area_param = new area_param_t();

#endif

chip_t::chip_t (conf_object_t *_cpus[], uint32 _num_cpus, string &_c) {

	num_cpus = _num_cpus;
	set_config (_c);

	// order important.

	config_db = new config_t ();
	config_db->register_entries ();

	if (!get_config ().empty ()) 
		config_db->parse_runtime (get_config ());

    cpus = _cpus;
    mem_hier = NULL;
    mem_driver = NULL;
	initialized = false;
}


void chip_t::init() {



    
    config_db->print();
    DEBUG_FLUSH();

#ifdef POWER_COMPILE
    power_obj = new power_object_t();
    power_entity_list = new power_profile_t *[MAX_POWER_ENTITIES];
    last_power_vals = new double[MAX_POWER_ENTITIES];
    last_switching_power_vals = new double[MAX_POWER_ENTITIES];
    last_leakage_power_vals = new double[MAX_POWER_ENTITIES];
    num_power_entities = 0;
    for (uint32 i = 0; i < MAX_POWER_ENTITIES; i++)
    {
        last_power_vals[i] = 0;
        last_switching_power_vals[i] = 0;
        last_leakage_power_vals[i] = 0;
    }
    if (g_conf_temperature_update_interval) {
        double scaling_correction = 1000 / (double) g_conf_leakage_cycle_adjust;
        double d_ti = scaling_correction * (double) ((1 << g_conf_temperature_update_interval) - 1);
        temperature_interval = (uint64) d_ti;
        ASSERT(temperature_interval);
    }
    if (g_conf_didt_interval) {
        di_dt_interval = (1 << g_conf_didt_interval) - 1;
        previous_didt_power_val = 0;
    }
    
#endif
	
	num_seq = num_cpus + g_conf_extra_seq;
    mai = new mai_t *[num_cpus];
    fastsim = new fastsim_t *[num_cpus];

    t_ctxt = new thread_context_t *[num_cpus];
    
	for (uint32 i = 0; i < num_cpus; i++) {
		mai[i] = new mai_t (cpus[i], this);
        if (!g_conf_use_processor) SIM_stall_cycle(cpus[i], 0);
		fastsim[i] = new fastsim_t(cpus[i]);
        t_ctxt[i] = new thread_context_t (i);
	}
    
    if (g_conf_chip_design[0])
    {
        // Chip with SMT cores!
        num_seq = g_conf_chip_design[0];
        seq = new sequencer_t *[num_seq];

        uint32 tid_offset = 0;
        for (uint32 i = 0; i < num_seq; i++)
        {
            uint32 ctxts = g_conf_chip_design[i + 1];
            uint32 t_ids[ctxts];
            for (uint32 j = 0; j < ctxts; j++)
            {
                if (tid_offset + j < num_cpus)
                    t_ids[j] = t_ctxt[tid_offset + j]->get_id();
                else
                    t_ids[j] = num_cpus;
            
            }
            seq[i] = new sequencer_t(this, i, t_ids, ctxts);
        }
    } else {
        num_seq = num_cpus;
		seq = new sequencer_t *[num_seq];

        uint32 t_ids[1];
        
        for (uint32 i = 0; i < num_seq; i++)
        {
            t_ids[0] = i;
            seq[i] = new sequencer_t (this, i, t_ids, 1);
            t_ctxt[i]->set_sequencer(seq[i]);
            t_ctxt[i]->set_seq_ctxt(0);
            
        }
        
    }
    
    generate_tstats_list ();
	g_cycles = 1;
	// if checkpoint is inside a trap
	for (uint32 i = 0; i < num_cpus; i++) {
		if (mai[i]->get_tl ()) {
			for (uint32 tl = 1; tl <= mai[i]->get_tl (); tl++) 
				mai[i]->setup_trap_context ();
		}
	}
	
	//mmu = new mmu_t (this);   
	scheduler = new thread_scheduler_t(this); 
    enable_core = disable_core = 0;
    disable_timer = new disable_timer_t(num_seq);
	initialized = true;
    last_synch_interrupt = 0;
    init_stats();
    
    for (uint32 i = 0; i < num_seq; i++)
        seq[i]->initialize_function_records ();
    
    
    if (g_conf_heatrun_migration_epoch)
        heatrun_migration_epoch = (1 << g_conf_heatrun_migration_epoch) - 1;
    
    
    
    
}

chip_t::~chip_t (void) {
	delete seq;
	delete mai;
	delete [] tstats_list;
	// todo delete rest
}

void 
chip_t::advance_cycle () {

	if (g_conf_periodic_interrupt && (g_cycles % g_conf_periodic_interrupt) == 0) {
		for (uint32 i = 0; i < num_seq ; i++) {
			for (uint32 j = 0; j < seq[i]->get_num_hw_ctxt(); j++) {
				if (seq[i]->get_mai_object(j))
					seq[i]->potential_thread_switch(j, YIELD_LONG_RUNNING);
			}
		}
	}

	for (uint32 i = 0; i < num_seq ; i++) {
		seq[i]->advance_cycle ();
	}
    /*
    if (g_conf_narrow_core_switch && g_cycles % 1000 == 0) {
        // if (seq[0]->get_mai_object(0) && !seq[0]->get_fu_status(0, FETCH_GPC_UNKNOWN) 
        //     && !seq[0]->get_fu_status (0, FETCH_STALL_UNTIL_EMPTY)) 
        //     seq[0]->potential_thread_switch(0, YIELD_NARROW_CORE);
        // else if (seq[1]->get_mai_object(0) && !seq[1]->get_fu_status(0, FETCH_GPC_UNKNOWN)
        //     && !seq[1]->get_fu_status (0, FETCH_STALL_UNTIL_EMPTY))
        //     seq[1]->potential_thread_switch(0, YIELD_NARROW_CORE);
        
        if (seq[0]->get_mai_object(0) && seq[0]->get_fu_status(0) == FETCH_READY) 
            seq[0]->potential_thread_switch(0, YIELD_NARROW_CORE);
        else if (seq[1]->get_mai_object(0) && seq[1]->get_fu_status(0) == FETCH_READY)
            seq[1]->potential_thread_switch(0, YIELD_NARROW_CORE);
        
    }
    */
    
	g_cycles++;
    handle_thread_stat();
    handle_simulation();
    
    if (g_conf_enable_disable_core || g_conf_disable_faulty_cores)
		simulate_enable_disable_core();
	
#ifdef POWER_COMPILE
    if (g_conf_temperature_update_interval &&
        (g_cycles % temperature_interval) == 0) 
        update_core_temperature();
    if (g_conf_didt_interval && ((g_cycles & di_dt_interval) == 0))
        update_di_dt_power();
#endif

    if (g_conf_heatrun_migration_epoch && 
        (g_cycles & heatrun_migration_epoch) == 0)
        scheduler->migrate();
}


void
chip_t::handle_simulation()
{
    uint64 total_commits  = 0;
    uint64 total_user_commits = 0;

    for (uint32 i = 0; i < num_cpus; i++)
    {
        proc_stats_t **tstat_l = get_tstats_list(i);
        total_commits += STAT_GET(tstat_l[0]->stat_commits);
        total_user_commits += STAT_GET(tstat_l[0]->stat_user_commits);
        if (g_conf_kernel_stats) 
            total_commits += STAT_GET(tstat_l[1]->stat_commits);
		
	}
	
	if ((g_conf_run_commits && total_commits >= (uint64) g_conf_run_commits) ||
        (g_conf_run_user_commits && total_user_commits >= (uint64) g_conf_run_user_commits) ||
        (g_conf_run_cycles &&  g_cycles == (uint64) g_conf_run_cycles)) 
    {
        //print_stats ();
        //mem_hier->print_stats();
        mai[0]->break_sim (0);
    }
    
    if (g_conf_cache_only_provision) {
        uint32 seq_cnt[num_seq];
        bzero(seq_cnt, sizeof(uint32) * num_seq);
        for (uint32 i = 0; i < num_seq; i++)
        {
            if (seq[i]->get_mem_hier_seq(0))
                seq_cnt[seq[i]->get_mem_hier_seq(0)->get_id()]++;
        }
        uint32 max_cnt = 0;
        for (uint32 i = 0; i < num_seq; i++)   
        {
            if (seq_cnt[i] > max_cnt) max_cnt = seq_cnt[i];
            stat_load_balance_histo->inc_total(1, seq_cnt[i]);
        }
        stat_max_imbalance_histo->inc_total(1, max_cnt);
    }
    uint32 active_core = 0;
    for (uint32 i = 0; i < num_seq; i++)
    {
        active_core += seq[i]->active_core();
    }
    stat_active_core_histo->inc_total(1, active_core);
    
}


sequencer_t*
chip_t::get_sequencer (uint32 sequencer_id) {
	return seq[sequencer_id];
}

mai_t*
chip_t::get_mai_object (uint32 cpu_id) {
    if (cpu_id < num_cpus)
        return mai[cpu_id];
    else 
        return NULL;
}

mai_t *
chip_t::get_mai_from_thread(uint32 tid)
{
    return mai[tid];
}

fastsim_t*
chip_t::get_fastsim (uint32 thread_id) {
    // TODO FIX fast sim
    if (thread_id < num_cpus)
        return fastsim[thread_id];
    else
        return fastsim[num_cpus - 1];
}

tick_t
chip_t::get_g_cycles () {
	return g_cycles;
}

proc_stats_t*
chip_t::get_tstats (uint32 thread_id) {
	ASSERT (tstats_list);

	if (thread_id >= num_cpus)
		return NULL;
    
	mai_t *maiobj = get_mai_object(thread_id);
	if (g_conf_kernel_stats && maiobj && maiobj->is_supervisor ()) 
		return tstats_list[thread_id][1];
	else
		return tstats_list[thread_id][0];
}

proc_stats_t**
chip_t::get_tstats_list (uint32 thread_id) {
	return tstats_list[thread_id];
}

void
chip_t::generate_tstats_list () {

	tstats_list = new proc_stats_t ** [num_cpus];

	for (uint32 s = 0; s < num_cpus; s++) {

		if (g_conf_kernel_stats) 
			tstats_list[s] = new proc_stats_t * [3];
		else
			tstats_list[s] = new proc_stats_t * [2];

		char name[32];
		sprintf(name, "thread_stats%u", s);

		uint32 p = 0;
		tstats_list[s][p++] = new proc_stats_t (string(name));
		
		if (g_conf_kernel_stats) {
			sprintf(name, "kthread_stats%u", s);
			tstats_list[s][p++] = new proc_stats_t (string(name));
		}
		
		tstats_list[s][p] = 0;
	}
}

bool
chip_t::ready_for_checkpoint() 
{
	for (uint32 s = 0; s < num_seq; s++) {
		if (!seq[s]->ready_for_checkpoint())
			return false;
	}
	return true;
}

void
chip_t::set_interrupt (uint32 thread_id, int64 vector) {

	if (vector) {
        mai[thread_id]->set_interrupt(vector);
    }
    if (g_conf_paused_vcpu_interrupt && g_cycles > 1) {
        if (t_ctxt[thread_id]->get_sequencer() == NULL) {
            sequencer_t *handler_seq = scheduler->handle_interrupt_thread(thread_id);
            if (handler_seq)
                handler_seq->potential_thread_switch(0, YIELD_PAUSED_THREAD_INTERRUPT);
        } 
        // No need to send interrupt to running thread as the can_preempt
        // function will return false in OS mode with pending interrupt
        /*
        else {
            t_ctxt[thread_id]->get_sequencer()->interrupt_on_running_thread(0);
        }
        */
        
    }
}

/*
void 
chip_t::set_shadow_interrupt (int64 vector) {
	ASSERT (seq);
	seq->set_shadow_interrupt (vector);
}
*/

void
chip_t::set_config (string &c) {
	config = c;
}

string
chip_t::get_config (void) {
	return config;
}

config_t *
chip_t::get_config_db (void) {
	return config_db;
}

void
chip_t::set_mem_hier (mem_hier_handle_t *_mem_hier) {
	ASSERT (_mem_hier);
	mem_hier = _mem_hier;
}

mem_hier_handle_t*
chip_t::get_mem_hier (void) {
	return mem_hier;
}

void
chip_t::print_stats () {
    
#ifdef POWER_COMPILE
    if (g_conf_didt_interval) update_power_sd();
#endif    
	stats->print();
	for (uint32 s = 0; s < num_seq; s++)
		seq[s]->print_stats();

	for (uint32 s = 0; s < num_cpus; s++) {
		uint32 i = 0;
		while (tstats_list[s][i] != 0) {
			tstats_list[s][i]->print ();

			i++;
		}
	}
    
    scheduler->print_stats();

}


void chip_t::update_power_sd()
{
    if (!g_conf_didt_interval) return;
    uint32 power_val_index = power_vals.size();
    double t_power = 0;
    double min = power_vals[0];
    double max = 0;
    for (uint32 i = 0; i < power_val_index; i++) {
        t_power += power_vals[i];
        if (power_vals[i] < min) min = power_vals[i];
        if (power_vals[i] > max) max = power_vals[i];
    }
    double mean = t_power/power_val_index;
    double sq_diff = 0;
    for (uint32 i = 0; i < power_val_index; i++)
        sq_diff += (power_vals[i] - mean) * (power_vals[i] - mean);
    stat_didt_sd->set(sqrt(sq_diff/power_val_index));
    stat_didt_mean->set(mean);
    stat_didt_max->set(max);
    stat_didt_min->set(min);
    
    double max_differential = 0;
    double delta_total = 0;
    
    for (uint32 i = 0; i < (power_val_index - 1); i++) {
        double diff;
        if (power_vals[i] < power_vals[i + 1]) {
            diff = power_vals[i+1] - power_vals[i];
        } else {
            diff = power_vals[i] - power_vals[i+1];
        }
        ASSERT(diff >= 0);
        if (max_differential < diff) max_differential = diff;
        delta_total += diff;
    }
    stat_avg_delta->set(delta_total / (power_val_index - 1));
    stat_max_delta->set(max_differential);
        
    
}

void
chip_t::stall_front_end()
{
	for (uint32 i = 0; i < num_cpus; i++)
    {
        if (t_ctxt[i]->get_sequencer())
            t_ctxt[i]->get_sequencer()->prepare_for_checkpoint(t_ctxt[i]->get_seq_ctxt());
    }
}

void
chip_t::print_stats (proc_stats_t *pstats) {

}

mmu_t *
chip_t::get_mmu (uint32 thread_id) {
	FAIL;
	return NULL;
}

void
chip_t::write_checkpoint(FILE *file) 
{
	fprintf(file, "%llu\n", g_cycles);
	fprintf(file, "%u %u\n", disable_core, enable_core);
    fprintf(file, "%llu\n", last_synch_interrupt);
	stats->to_file(file);
    int32 seq_id;
	
    // thread context
    for (uint32 i = 0; i < num_cpus; i++)
    {
        seq_id = t_ctxt[i]->get_sequencer() ? (int32)t_ctxt[i]->get_sequencer()->get_id() :
            -1;
        fprintf(file, "%d %u\n", seq_id, t_ctxt[i]->get_seq_ctxt());   
    }
    
    
	for (uint32 s = 0; s < num_cpus; s++) {
		uint32 i = 0;
		while (tstats_list[s][i] != 0) {
			tstats_list[s][i]->stats->to_file(file);
			i++;
		}
	}
	checkpoint_util_t *checkp = new checkpoint_util_t();
    checkp->map_uint64_uint64_from_file(lwp_syscall, file);
    
	for (uint32 i = 0; i < num_cpus; i++)
		mai[i]->write_checkpoint(file);
	for (uint32 i = 0; i < num_seq; i++)
		seq[i]->write_checkpoint(file);
    
    
    scheduler->write_checkpoint(file);
    disable_timer->write_checkpoint(file);
    
#ifdef POWER_COMPILE
    fprintf(file, "%u\n", num_power_entities);
    for (uint32 i = 0; i < num_power_entities; i++)
        fprintf(file, "%lf\n", last_power_vals[i]);
    uint32 power_val_index = power_vals.size();
    fprintf(file, "%u\n", power_val_index);
    for (uint32 i = 0; i < power_val_index; i++)
        fprintf(file, "%lf\n", power_vals[i]);
    fprintf(file, "%lf\n", previous_didt_power_val);
#endif     
}

void
chip_t::read_checkpoint(FILE *file)
{
    // Check pointing need to be fixed
	fscanf(file, "%llu\n", &g_cycles);
    fscanf(file, "%u %u\n", &enable_core, &disable_core);
    fscanf(file, "%llu\n", &last_synch_interrupt);
	stats->from_file(file);

    uint32 seq_ctxt;
    int32 seq_id;
	for (uint32 i = 0; i < num_cpus; i++)
    {
        fscanf(file, "%d %u\n", &seq_id, &seq_ctxt);
        if (seq_id >= 0) {
            t_ctxt[i]->set_sequencer(seq[seq_id]);
            t_ctxt[i]->set_seq_ctxt(seq_ctxt);
            if (!g_conf_use_processor) mem_driver->release_proc_stall(i);
        } else {
            t_ctxt[i]->set_sequencer(NULL);
            if (!g_conf_use_processor) mem_driver->stall_thread(i);
        }
    }
	
	for (uint32 s = 0; s < num_cpus; s++) {
		uint32 i = 0;
		while (tstats_list[s][i] != 0) {
			tstats_list[s][i]->stats->from_file(file);
			i++;
		}
	}

    checkpoint_util_t *checkp = new checkpoint_util_t();
    checkp->map_uint64_uint64_to_file(lwp_syscall, file);
    
	for (uint32 i = 0; i < num_cpus; i++)
		mai[i]->read_checkpoint(file);
	for (uint32 i = 0; i < num_seq; i++)
		seq[i]->read_checkpoint(file);

    scheduler->read_checkpoint(file);
    disable_timer->read_checkpoint(file);
    
    
#ifdef POWER_COMPILE
    uint32 tmp_power_entities;
    fscanf(file, "%u\n", &tmp_power_entities);
    for (uint32 i = 0; i < tmp_power_entities; i++)
        fscanf(file, "%lf\n", last_power_vals + i);
    uint32 power_val_index;
    fscanf(file, "%u\n", &power_val_index);
    double p_val;
    power_vals.clear();
    for (uint32 i = 0; i < power_val_index; i++) {
        fscanf(file, "%lf\n", &p_val);
        power_vals.push_back(p_val);
    }
    fscanf(file, "%lf\n", &previous_didt_power_val);
#endif 
    
}

sequencer_t *
chip_t::get_sequencer_from_thread(uint32 thread_id)
{
    return t_ctxt[thread_id]->get_sequencer();
}

uint32
chip_t::get_num_sequencers()
{
	return num_seq;
}

uint32
chip_t::get_num_cpus()
{
	return num_cpus;
}

thread_scheduler_t *
chip_t::get_scheduler()
{
	return scheduler;
}

void
chip_t::switch_to_thread(sequencer_t *new_seq, uint32 ctxt, mai_t *thread)
{
	ASSERT(new_seq);
	int32 thread_id = thread ? thread->get_id() : -1; 
    if (thread) {
        /*
        for (uint32 i = 0; i < num_seq; i++)
            ASSERT(g_cycles < 100 || seq[i]->get_mai_object(0) != thread || 
            seq[i] == new_seq );
        */    
        t_ctxt[thread_id]->set_sequencer(new_seq);
        t_ctxt[thread_id]->set_seq_ctxt(ctxt);
        tick_t last_evict = thread->get_last_eviction();
        tick_t wait = g_cycles - last_evict;
        if (wait && last_evict) histo_wait_distr->inc_total(1, wait);
            
    }
    new_seq->switch_to_thread(thread, ctxt, !initialized);
    
}


uint8 chip_t::get_seq_ctxt(uint32 tid)
{
    return t_ctxt[tid]->get_seq_ctxt();
}

void chip_t::idle_thread_context (uint32 id)
{
    t_ctxt[id]->set_sequencer(NULL);
}

void chip_t::invalidate_address(sequencer_t *_s, invalidate_addr_t *invalid_addr)
{
    for (uint32 i = 0; i < num_seq; i++)
        seq[i]->invalidate_address(_s, invalid_addr);
}


uint64
chip_t::get_lwp_syscall(uint64 kstack_region)
{
    if (kstack_region && lwp_syscall.find(kstack_region) != lwp_syscall.end())
        return lwp_syscall[kstack_region];
    return 0;
}


void
chip_t::set_lwp_syscall(uint64 kstack_region, uint64 syscall)
{
   lwp_syscall[kstack_region] = syscall;
}


void chip_t::check_for_thread_yield()
{
    if (g_conf_processor_checkpoint_in != "" ) {
        FILE *fileHandle = fopen(g_conf_processor_checkpoint_in.c_str(), "r");
        if (fileHandle) {
            read_checkpoint(fileHandle);
            fclose(fileHandle);
        }
        g_conf_processor_checkpoint_in = "";
    }
    
	if (g_conf_periodic_interrupt && (g_cycles % g_conf_periodic_interrupt) == 0) {
		for (uint32 i = 0; i < num_seq ; i++) {
			for (uint32 j = 0; j < seq[i]->get_num_hw_ctxt(); j++) {
				if (seq[i]->get_mai_object(j))
					seq[i]->potential_thread_switch(j, YIELD_LONG_RUNNING);
			}
		}
	}

    g_cycles++;
    for (uint32 i = 0; i < num_seq; i++) {
        seq[i]->check_for_thread_switch();
    }
    if (g_conf_enable_disable_core || g_conf_disable_faulty_cores)
		simulate_enable_disable_core();
    handle_thread_stat();
    
}

void chip_t::set_mem_driver(mem_driver_t *_mem_driver)
{
    mem_driver = _mem_driver;
}

mem_driver_t *chip_t::get_mem_driver()
{
    return mem_driver;
}

void chip_t::handle_mode_change(uint32 thread_id)
{
    mai[thread_id]->mode_change();
    /*
    if (mai[thread_id]->is_supervisor() == false)
    {
        sequencer_t *seq = t_ctxt[thread_id]->get_sequencer();
        if (seq) seq->potential_thread_switch(t_ctxt[thread_id]->get_seq_ctxt(), YIELD_PROC_MODE_CHANGE);
    }
    */
}

void chip_t::handle_thread_stat()
{
	bool pcommits = g_conf_print_commit_stat_trace ?
		((g_cycles % g_conf_print_commit_stat_trace) == 0) : 0;
	bool pl1miss = g_conf_print_l1miss_stat_trace ?
		((g_cycles % g_conf_print_l1miss_stat_trace) == 0) : 0;
	bool pl2miss = g_conf_print_l2miss_stat_trace ?
		((g_cycles % g_conf_print_l2miss_stat_trace) == 0) : 0;
	bool psyncs = g_conf_print_sync_stat_trace ?
		((g_cycles % g_conf_print_sync_stat_trace) == 0) : 0;
		
	char commits_str[256] = {0};
	char l1miss_str[256] = {0};
	char l2miss_str[256] = {0};
	char syncs_str[256] = {0};
	char temp_str[256] = {0};
	
    for (uint32 i = 0; i < num_cpus ; i++) {
		proc_stats_t *tstats = get_tstats (i);
        if (!t_ctxt[i]->get_sequencer()) {
			STAT_INC (tstats->stat_idle_context_cycles);
			
			if (g_conf_csp_stop_tick)
				mai[i]->restore_tick_reg();
		} else {
            STAT_INC (tstats->stat_cycles);
		}
		
		if (pcommits) {
			strncpy(temp_str, commits_str, 255);
			snprintf(commits_str, 255, "%s\t%llu", temp_str,
				STAT_GET(tstats->stat_commits));
		}
		if (pl1miss) {
			strncpy(temp_str, l1miss_str, 255);
			snprintf(l1miss_str, 255, "%s\t%llu", temp_str,
				STAT_GET(tstats->stat_instr_l1_miss) +
				STAT_GET(tstats->stat_load_l1_miss));
		}
		if (pl2miss) {
			strncpy(temp_str, l2miss_str, 255);
			snprintf(l2miss_str, 255, "%s\t%llu", temp_str,
				STAT_GET(tstats->stat_instr_l2_miss) +
				STAT_GET(tstats->stat_load_l2_miss));
		}
		if (psyncs) {
			strncpy(temp_str, syncs_str, 255);
			snprintf(syncs_str, 255, "%s\t%llu", temp_str,
				STAT_GET(tstats->stat_sync1) + STAT_GET(tstats->stat_sync2));
		}
	}
	if (pcommits)
		DEBUG_LOG("$$commits\t%llu%s\n", g_cycles, commits_str);
	if (pl1miss)
		DEBUG_LOG("$$l1miss\t%llu%s\n", g_cycles, l1miss_str);
	if (pl2miss)
		DEBUG_LOG("$$l2miss\t%llu%s\n", g_cycles, l2miss_str);
	if (psyncs)
		DEBUG_LOG("$$syncs\t%llu%s\n", g_cycles, syncs_str);
    
    scheduler->update_cycle_stats();
}

thread_context_t *chip_t::get_thread_ctxt(uint32 thread)
{
    return t_ctxt[thread];
}

uint64 chip_t::closest_lwp_match(uint64 sp)
{
    map<uint64, uint64>::iterator it;
    uint64 ret = 0;
    for (it = lwp_syscall.begin(); it != lwp_syscall.end(); it++)
    {
        if (it->first + 0x100 > sp) break;
        ret = it->first;
    }
    return ret;
}

void chip_t::simulate_enable_disable_core()
{
    for (uint32 i = 0; i < num_seq; i++)
    {
        if (disable_timer->disable_core_now(g_cycles, i))
        {
            seq[i]->potential_thread_switch(0, YIELD_DISABLE_CORE);
            scheduler->disable_core(seq[i]);
        } else if (disable_timer->enable_core_now(g_cycles, i)) {
            scheduler->enable_core(seq[i]);
        }
    }
        
}

uint32 chip_t::get_machine_id(uint32 vcpu_id)
{
	uint32 cpus_per_machine = num_cpus / g_conf_num_machines;
	return (vcpu_id / cpus_per_machine);
	
}

void chip_t::init_stats()
{
	stats = new stats_t("chip_stats");
	stat_magic_trans = stats->COUNTER_GROUP("stat_magic_transactions",
		"Magic Transactions", g_conf_num_machines);
    
    histo_syncint_interval = stats->HISTO_EXP("histo_syncint_interval", 
        "distribution of sync interrupt interval", 10, 8, 1);
    stat_syncint_src = stats->COUNTER_GROUP("stat_syncint_src", 
        "source of sync int", num_cpus);
    stat_syncint_tgt = stats->COUNTER_GROUP("stat_syncint_tgt",
        "target of sync int", num_cpus);
    string cc_handler_desc [] = {
    "Other", "0x100a12c", "0x11656e8", "0x1033cd0"};
    string cc_handler_name [] = {
    "Other", "scheduler", "TLB Shootdown", "xt_sync_tl1"};
    cc_handler_list[0x100a12c] = 1;
    cc_handler_list[0x11656e8] = 2;
    cc_handler_list[0x1033cd0] = 3;
    //cc_handler_list[0x1033cd4] = 0; // TEMP
    stat_cross_call_handler       = stats->COUNTER_BREAKDOWN ("cross_call_handler",
	"frequency of cross call handler", 4, cc_handler_name, cc_handler_desc);

    histo_wait_distr = stats->HISTO_EXP("histo_wait_distr", 
        "distribution of thread wait", 10, 8, 1);
    stat_load_balance_histo = stats->HISTO_UNIFORM("load_balance_histo", 
        "Load Balance Histo", num_cpus + 1, 1, 0);
    stat_max_imbalance_histo = stats->HISTO_UNIFORM("max_imbalance_histo", 
        "Max Load Imbalance Histo", num_cpus + 1, 1, 0);
    stat_active_core_histo = stats->HISTO_UNIFORM("active_core_histo",
        "Histogram of Core Activity", num_seq + 1, 1, 0);
#ifdef POWER_COMPILE
    stat_interval_power = stats->HISTO_EXP("max_interval_power", "Interval Power", 10, 4, 1);
    stat_interval_power->set_terse_format ();
    stat_interval_switching_power = stats->HISTO_EXP("stat_interval_switching_power", "Interval Dynamic Power", 10, 4, 1);
    stat_interval_switching_power->set_terse_format ();
    stat_interval_leakage_power = stats->HISTO_EXP("stat_interval_leakage_power", "Interval Leakage Power", 10, 4, 1);
    stat_interval_leakage_power->set_terse_format ();
    stat_didt_sd = stats->COUNTER_DOUBLE("stat_didt_sd", "standard deviation of didt interval power");
    stat_didt_mean = stats->COUNTER_DOUBLE("stat_didt_mean", "mean of didt interval power");
    stat_didt_min = stats->COUNTER_DOUBLE("stat_didt_min", "min of didt interval power");
    stat_didt_max = stats->COUNTER_DOUBLE("stat_didt_max", "max of didt interval power");
    stat_max_delta = stats->COUNTER_DOUBLE("stat_max_delta", "max of incremental current differentials");
    stat_avg_delta = stats->COUNTER_DOUBLE("stat_avg_delta", "Average of incremental current differentials");
#endif    
    set_hidden_stats();

}		



void chip_t::set_hidden_stats()
{
    if (!g_conf_override_idsr_read) {
        histo_syncint_interval->set_hidden();
        stat_syncint_src->set_hidden();
        stat_syncint_tgt->set_hidden();
        stat_cross_call_handler->set_hidden();
        histo_wait_distr->set_hidden();
        
    }
    
    histo_syncint_interval->set_hidden();
    stat_syncint_src->set_hidden();
    stat_syncint_tgt->set_hidden();
    
    if (!g_conf_cache_only_provision) {
        histo_wait_distr->set_hidden();
        stat_load_balance_histo->set_hidden();
        stat_max_imbalance_histo->set_hidden();
    }

}    

void chip_t::magic_instruction(conf_object_t *cpu, int64 magic_value)
{
	stat_magic_trans->inc_total(1, get_machine_id(SIM_get_proc_no(cpu)));
}

void chip_t::handle_synchronous_interrupt(uint32 src, uint32 target)
{
    if (t_ctxt[target]->get_sequencer())
        t_ctxt[target]->get_sequencer()->interrupt_on_running_thread(0);
    else {
        sequencer_t *sync_seq = scheduler->handle_synchronous_interrupt(src, target);
        if (sync_seq) {
            sync_seq->potential_thread_switch(0, YIELD_PAUSED_THREAD_INTERRUPT);
        }
    }
}

void chip_t::record_synchronous_interrupt(uint32 src, uint32 target)
{
    // Ignore bootstrap 
    if (g_cycles == 1) return;
    tick_t interval = g_cycles - last_synch_interrupt;
    if (interval > 0) histo_syncint_interval->inc_total(1, interval);
    else histo_syncint_interval->inc_total(1, 1);
    stat_syncint_src->inc_total(1, src);
    stat_syncint_tgt->inc_total(1, target);
    last_synch_interrupt = g_cycles; 
    
}

void chip_t::update_cross_call_handler(addr_t hdl)
{
    stat_cross_call_handler->inc_total(1, cc_handler_list[hdl]);
}

bool chip_t::identify_cross_call_handler(addr_t hndlr)
{
    if (g_conf_allow_tlb_shootdown && hndlr == 0x11656e8)
        return false;
    return (cc_handler_list.find(hndlr) != cc_handler_list.end());
}
        
void chip_t::inspect_register_vals(uint32 cpu_id)
{
    conf_object_t *cpu = mai[cpu_id]->get_cpu_obj();
    attr_value_t regs = SIM_get_all_registers(cpu);
    uint32 zeros, ones, rest;
    zeros = ones = rest = 0;
    attr_value_t item;
    ASSERT(regs.kind == Sim_Val_List);
    for (uint32 i = 0; i < regs.u.list.size; i++)
    {
        item = regs.u.list.vector[i];
        uint64 val = SIM_read_register(cpu, item.u.integer);
        //DEBUG_OUT("REgister name %s number %u\n", SIM_get_register_name(cpu, item.u.integer), item.u.integer);
        if ((item.u.integer >= 48 && item.u.integer <= 82) || 
            (item.u.integer >= 102 && item.u.integer <= 119)) continue;
        if (val == 0) {
            DEBUG_OUT("Register name %s number %llu val %llu\n", SIM_get_register_name(cpu, item.u.integer),
               item.u.integer, val);
            zeros++;
        }
        else if (val == 1) ones++;
        else {
            //DEBUG_OUT("REgister name %s number %llu val %llu\n", SIM_get_register_name(cpu, item.u.integer),
            //    item.u.integer, val);
            rest++;
        }
    }
    
    DEBUG_OUT("Zeros = %u ones = %u rest = %u\n", zeros, ones, rest);
}

uint32 chip_t::current_occupancy(sequencer_t *c_seq, uint32 t_ctxt)
{
    uint32 ret = 0;
    for (uint32 i = 0; i < num_seq; i++)
    {
        ret = (seq[i]->get_mem_hier_seq(t_ctxt) == c_seq) ? ret + 1 : ret;
    }
    return ret;
    
}

void chip_t::narrow_core_switch(sequencer_t *s_seq)
{
    
    cerr << "Narrow Core Switch Happenning now " << g_cycles << endl;
       uint32 vacate_id = s_seq->get_id();
       uint32 fillcore_id = (vacate_id % 2 == 0) ? vacate_id + 1: vacate_id - 1;
       seq[fillcore_id]->narrow_core_switch(s_seq);
       uint32 thread_id = s_seq->get_mai_object(0)->get_id();
       t_ctxt[thread_id]->set_sequencer(seq[fillcore_id]);
       t_ctxt[thread_id]->set_seq_ctxt(0);
    
    
}



sequencer_t *chip_t::get_remote_sequencer(sequencer_t *s_seq)
{
    uint32 s_tid = s_seq->get_id ();
    uint32 t_tid = ((s_tid % 2)  == 0 ) ? (s_tid + 1) : s_tid - 1;
    return seq[t_tid];
}

#ifdef POWER_COMPILE
power_object_t* chip_t::get_power_obj()
{
    return power_obj;
}


void chip_t::register_power_entity(power_profile_t *_pf)
{
    power_entity_list[num_power_entities++] = _pf;
}

void chip_t::update_core_temperature()
{
    double chip_total = 0;
    double chip_leakage_total = 0;
    double chip_switching_total = 0;
    double tmp;
    for (uint32 i = 0; i < num_power_entities; i++)
    {
        tmp = power_entity_list[i]->get_current_power();
        chip_total += (tmp - last_power_vals[i]);
        last_power_vals[i] = tmp;
        
        tmp = power_entity_list[i]->get_current_switching_power();
        chip_switching_total += (tmp - last_switching_power_vals[i]);
        last_switching_power_vals[i] = tmp;
        
        tmp = power_entity_list[i]->get_current_leakage_power();
        chip_leakage_total += (tmp - last_leakage_power_vals[i]);
        last_leakage_power_vals[i] = tmp;
        
    }
    
    stat_interval_power->inc_total(1, (uint64) chip_total);
    stat_interval_switching_power->inc_total(1, (uint64) chip_switching_total);
    stat_interval_leakage_power->inc_total(1, (uint64) chip_leakage_total);
    
    for (uint32 i = 0; i < num_seq; i++) {
        seq[i]->get_core_power_profile()->compute_temperature();
    }
    
}


void chip_t::update_di_dt_power()
{
    if (g_cycles < 1000 * di_dt_interval) return;
    double total_power = 0;
    for (uint32 i = 0; i < num_power_entities; i++)
    {
        total_power += power_entity_list[i]->get_current_switching_power() +
            power_entity_list[i]->get_current_leakage_power();
    }
    if (previous_didt_power_val == 0)
        previous_didt_power_val = total_power;
    else {
        double interval_power = total_power - previous_didt_power_val;
        previous_didt_power_val = total_power;
        power_vals.push_back(interval_power);
    }
    
}

#endif
