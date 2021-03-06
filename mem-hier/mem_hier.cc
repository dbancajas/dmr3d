/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    main class for the memory hierarchy
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: mem_hier.cc,v 1.13.2.63 2006/12/12 18:36:57 pwells Exp $");

#include "definitions.h"
#include "config_extern.h"
#include "mem_hier.h"
#include "device.h"
#include "link.h"
#include "statemachine.h"
#include "proc.h"
#include "mainmem.h"
#include "cache.h"
#include "tcache.h"
#include "iodev.h"
#include "transaction.h"
#include "mem_hier_handle.h"
#include "window.h"
#include "meventq.h"
#include "MemorySystem.h"
#include "dram.h"

#include "config.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "startup.h"

#include "isa.h"
#include "profiles.h"
#include "dynamic.h"
#include "mem_xaction.h"
#include "trans_rec.h"
#include "random_tester.h"
#include "mem_tracer.h"
#include "spinlock.h"
#include "codeshare.h"
#include "stackheap.h"
#include "sequencer.h"
#include "chip.h"
#include "proc_stats.h"
#include "shb.h"
#include "function_profile.h"

#include "mai_instr.h"
#include "mai.h"

// Definitions of static objects
mem_hier_handle_iface_t *mem_hier_t::mhh;
mem_hier_t *mem_hier_t::g_ptr;

mem_hier_t::mem_hier_t(proc_object_t *_module)
	: module_obj(_module)
{
	num_deferred = 0;
	num_io_writes = 0;
	stall_delta = 1;
	runtime_config_file = "";
	config = NULL;
	is_initialized = false;

	num_devices = 0;
	num_links = 0;
	links = NULL;
	devices = NULL;
	
	eventq = NULL;
	g_cycles = 0;
	dramptr = new dram_t<prot_t, simple_mainmem_msg_t>("dram", 9);    
    
    mem_trans_array = new mem_trans_t *[g_conf_mem_trans_count];
    ASSERT(mem_trans_array);
    mem_trans_t *trans;
    for (uint32 i = 0; i < g_conf_mem_trans_count; i++)
    {
        trans = new mem_trans_t();
        ASSERT (trans);
        trans->recycle();
        trans->completed = true;
        mem_trans_array[i] = trans;
        trans->unique_id = i;
    }
    mem_trans_array_index = 0;
    profiles = 0;
    
    stats = NULL;
    external::post_event(this);
    
	
}

// Must be called after the simics configuration file has set all required
// attributes
void
mem_hier_t::init() {

	// Create eventq
	eventq = new meventq_t(2 * g_conf_main_mem_latency+g_conf_random_mm_dist+2);
	
	// Init random generator
	srandom(g_conf_random_seed);

    num_threads = SIM_number_processors();
    vcpus = new conf_object_t *[num_threads];
	currently_idle = new bool[num_processors];
	idle_start = new tick_t[num_processors];
    last_step_count = new uint64[num_threads];
    spin_step_count = new uint64[num_threads];
	for (uint32 i = 0; i < num_processors; i++) {
		currently_idle[i] = false;
		idle_start[i] = 0;
	}
    
    for (uint32 i = 0; i < num_threads; i++)
    {
        vcpus[i] = SIM_proc_no_2_ptr(i);
        last_step_count[i] = SIM_step_count(vcpus[i]);
        spin_step_count[i] = last_step_count[i];
    }

    if (g_conf_mem_hier_response_tracking)
        tr_rec = new trans_rec_t(num_processors);
    
    if (g_conf_random_addresses)
        random_tester = new random_tester_t(this);
	// Create stats
	stats = new stats_t();
	stats_print_enqueued = false;
	
	stat_cycles = stats->COUNTER_BASIC("mh_cycles", "# of cycles");
	stat_num_requests = stats->COUNTER_BASIC("num_requests", "# of requests received by mem hier");
	stat_num_reads = stats->COUNTER_BASIC("num_reads", "# of read requests");
	stat_num_writes = stats->COUNTER_BASIC("num_writes", "# of write requests");
	stat_num_atomics = stats->COUNTER_BASIC("num_atomics", "# of atomic requests");
	stat_num_io_space = stats->COUNTER_BASIC("num_io_space", "# of requests to/from I/O space");
	stat_num_io_device = stats->COUNTER_BASIC("num_io_device", "# of requests from a I/O device");
	stat_num_io_device_phys_space = stats->COUNTER_BASIC("num_io_device_phys_space", "# of requests from a I/O device to phys space");
	stat_num_uncache = stats->COUNTER_BASIC("num_uncache", "# of uncacheable requests");
	stat_num_uncache_io_space = stats->COUNTER_BASIC("num_uncache_io_space", "# of uncacheable requests to/from I/O space");
	stat_num_uncache_io_device = stats->COUNTER_BASIC("num_uncache_io_device", "# of uncacheable requests from I/O device");
	stat_num_may_not_stall = stats->COUNTER_BASIC("num_may_not_stall", "# of requests with may_stall unset");
	stat_num_squash = stats->COUNTER_BASIC("num_squash", "# of requests squashed before issued to mem-hier");
    stat_num_commits = stats->COUNTER_BASIC("num_commits", "# of committed instructions");
    stat_user_commits = stats->COUNTER_BASIC("user_commits", "# of committed instructions for user");
    stat_thread_user_commits = stats->COUNTER_GROUP("thread_user_commit", "# of committed instructions for users", num_threads);
    stat_thread_user_cycles = stats->COUNTER_GROUP("thread_user_cycles", "# of cycles in user code", num_threads);
    
	stat_request_size = stats->HISTO_EXP2("request_size", "distribution of request sizes",
	                               8, 1, 1);
								   
	stat_requesttime_histo = stats->HISTO_EXP2("request_time",
											   "distribution of miss latencies",
											   10,1,1);
	stat_requesttime_histo2 = stats->HISTO_EXP2("request_time_no_cache",
											   "distribution of miss latencies after L2 cache",
											   10,1,18);
    stat_data_reqtime_histo = stats->HISTO_EXP2("data_request_time",
											   "distribution of miss latencies (data)",
											   10,1,1);
    stat_instr_reqtime_histo = stats->HISTO_EXP2("instruction_request_time",
											   "distribution of miss latencies (instructions)",
											   10,1,1);
	stat_vcpu_state_requesttime = stats->HISTO_EXP2("vcpu_state_req_time",
											   "distribution of VCPU state miss latencies",
											   10,1,1);
	stat_cpu_histo = stats->HISTO_UNIFORM("cpu_histo",
										  "distribution of requests from each cpu",
										  num_processors,1,0);
//	stat_asi_histo = stats->HISTO_UNIFORM("asi_histo",
//										  "distribution of requests from each ASI",
//										  0xff,1,0);
	stat_idle_cycles_histo = stats->COUNTER_GROUP("idle_cycles",
		"cycles each cpu spends in idle loop", num_processors);

	stat_l1_hit = stats->COUNTER_BASIC("l1_hit", "# of l1 hits");
	stat_l1_miss = stats->COUNTER_BASIC("l1_miss", "# of l1 demand+coherence misses");
	stat_l1_coher_miss = stats->COUNTER_BASIC("l1_coher_miss", "# of l1 coherence misses");
	stat_l1_mshr_part_hit = stats->COUNTER_BASIC("l1_mshr_part_hit", "# of l1 mshr hits");
	stat_l1_stall = stats->COUNTER_BASIC("l1_stall", "# of l1 stalls");

	stat_l2_hit = stats->COUNTER_BASIC("l2_hit", "# of l2 hits");
	stat_l2_miss = stats->COUNTER_BASIC("l2_miss", "# of l2 demand+coherence misses");
	stat_l2_coher_miss = stats->COUNTER_BASIC("l2_coher_miss", "# of l2 coherence misses");
	stat_l2_mshr_part_hit = stats->COUNTER_BASIC("l2_mshr_part_hit", "# of l2 mshr hits");
	stat_l2_req_fwd = stats->COUNTER_BASIC("l2_req_fwd", "# of l2 request forwards");
	stat_l2_stall = stats->COUNTER_BASIC("l2_stall", "# of l2 stalls");
    stat_leaked_trans = stats->COUNTER_GROUP("stat_leaked_trans", "Leaked trans stat",
        num_threads);
    
    stat_memlinklatency_histo = stats->HISTO_EXP2("mem_link_latency",
											   "distribution of mem link latencies",
											   10,1,1);
	

	create_network();

    profiles = new profiles_t(string(get_name()) + "-profiles");
	if (g_conf_profile_mem_addresses) {
		address_profiler = new mem_tracer_t();
		profiles->enqueue(address_profiler);
	}
	if (g_conf_profile_spin_locks) {
		spin_lock_profiler = new profile_spinlock_t();
		profiles->enqueue(spin_lock_profiler);
	}
	
    if (g_conf_profile_codeshare) {
        codeshare = new codeshare_t("codeshare", num_processors);
        profiles->enqueue(codeshare);
    }
    
    if (g_conf_stackheap_profile || g_conf_optimize_stack_write) {
        stackheap_profile = new stackheap_t("stackheap");
        profiles->enqueue(stackheap_profile);
    }
    
	is_initialized = true;
    // Checkpoint initiliazation
    checkpoint_taken = false;
    if (g_conf_mem_hier_checkpoint_in != "") {
        read_checkpoint(g_conf_mem_hier_checkpoint_in);
    }

	l1_misses_to_bank = new uint64 *[num_processors];
	l2_misses_to_bank = new uint64 *[num_processors];
	for (uint32 i = 0; i < num_processors; i++) {
		l1_misses_to_bank[i] = new uint64[g_conf_l2_banks];
		bzero(l1_misses_to_bank[i], (sizeof(uint64) * g_conf_l2_banks));
		l2_misses_to_bank[i] = new uint64[g_conf_l2_banks];
		bzero(l2_misses_to_bank[i], (sizeof(uint64) * g_conf_l2_banks));
	}
}

mem_hier_handle_iface_t *
mem_hier_t::get_handler() {
	return mhh;
}

void
mem_hier_t::set_handler(mem_hier_handle_iface_t *_mhh) {
	mhh = _mhh;
}

void
mem_hier_t::set_cpus(conf_object_t **_cpus, uint32 _num) {
	cpus = _cpus;
	num_processors = _num;
}

conf_object_t **
mem_hier_t::get_cpus() {
	return cpus;
}

config_t *
mem_hier_t::get_config_db() {
	return config;
}

meventq_t *
mem_hier_t::get_eventq() {
	return eventq;
}

tick_t
mem_hier_t::get_g_cycles() {
	return g_cycles;
}

conf_object_t *
mem_hier_t::get_cpu_object(uint32 id) {
	ASSERT(id < num_processors);
	return (cpus[id]);
}

generic_proc_t *
mem_hier_t::get_proc_object(conf_object_t *cpu) {
	for (uint32 i = 0; i < num_processors; i++) {
		if (cpus[i] == cpu) return static_cast <generic_proc_t *> (devices[i]);
	}
	FAIL_MSG("cpu object not found: %s", cpu->name);
}

uint32
mem_hier_t::get_num_processors() {
	return num_processors;
}

void
mem_hier_t::read_runtime_config_file(string file) {

    
	// if (config) delete config;

	// config = new config_t();
	// config->register_entries();
	// if (file != "") config->parse_runtime(file);
    
    // if (!mem_trans_array) {
    //     mem_trans_array_index = 0;
    //     mem_trans_array = new mem_trans_t *[g_conf_mem_trans_count];
    //     mem_trans_t *trans;
    //     for (uint32 i = 0; i < g_conf_mem_trans_count; i++)
    //     {
    //         trans = new mem_trans_t();
    //         trans->recycle();
    //         trans->completed = true;
    //         mem_trans_array[i] = trans;
    //     }
        
    // }
    
}

void
mem_hier_t::set_ptr(mem_hier_t *_ptr) {
	g_ptr = _ptr;
}

mem_hier_t *
mem_hier_t::ptr() {
	return g_ptr;
}


// Create the links and devices for the memory hierarchy network
//   TODO: This should all be parameterized or read in from a file
void
mem_hier_t::create_network() 
{
	init_network();
	//create_unip_two_default();
       	 create_cmp_incl(false);
	return;//worst idea. get rid of this asap

	if (g_conf_memory_topology == "unip-one-default")
		create_unip_one_default();
	else if (g_conf_memory_topology == "unip-one-dram"){
		create_unip_one_dram();
		cout<<"dram constructed!!!!!!!!!!!!!!!!!!\n"<<endl;;
	}
	else if (g_conf_memory_topology == "minimal-default")
		create_minimal_default();
	else if (g_conf_memory_topology == "mp-minimal-default")
		create_mp_minimal_default();
	else if (g_conf_memory_topology == "unip-one-bus")
		create_unip_one_bus();
	else if (g_conf_memory_topology == "msi-one-bus")
		create_mp_bus();
	else if (g_conf_memory_topology == "unip-one-mp-msi")
		create_unip_one_mp_msi();
	else if (g_conf_memory_topology == "unip-two-default")
		create_unip_two_default();
	else if (g_conf_memory_topology == "msi-two-bus")
		create_two_level_mp_bus();
	else if (g_conf_memory_topology == "msi-two-split")
		create_two_level_split_mp();
	else if (g_conf_memory_topology == "unip-one-cachedata")
		//create_unip_one_cachedata(); no equiv function for this
		create_two_level_split_mp();//dummy only
	else if (g_conf_memory_topology == "cmp_incl")
        	create_cmp_incl(false);
        else if (g_conf_memory_topology == "cmp_incl_wt")
       	 	create_cmp_incl(true);
	else if (g_conf_memory_topology == "cmp_excl")
        	create_cmp_excl();
 	else if (g_conf_memory_topology == "cmp_excl_3l")
        	create_cmp_excl_3l();
    	else if (g_conf_memory_topology == "cmp_incl_3l")
       		create_cmp_incl_3l();
	else if (g_conf_memory_topology == "cmp_incl_l2banks")
        	create_cmp_incl_l2banks();
    	else
		ASSERT_MSG(0, "UNDEFINED PROTOCOL");
}

void
mem_hier_t::advance_cycle()
{
	if (stats) STAT_INC(stat_cycles);
	g_cycles++;
	if (eventq) eventq->advance_cycle();
	if (is_initialized) {
        if (!g_conf_use_processor && g_conf_stats_print_cycles &&
            g_cycles % g_conf_stats_print_cycles == 0)
        {
            get_module_obj()->get_stats();
        }
        
        if (g_conf_mem_hier_response_tracking && 
            g_cycles % g_conf_mem_hier_check_frequency == 0)
        {
            tr_rec->detect_error(g_cycles);
        }
        
        if (g_conf_mem_hier_warmup && g_cycles > (uint64)g_conf_mem_hier_warmup)
        {
            clear_stats();
            g_conf_mem_hier_warmup = 0;
        }
	}
    if (g_conf_hasdram){
	if(dramptr==NULL)
		ASSERT_MSG(0,"DRAM not initialized!");

	dramptr->advance_cycle();
    }
    if (!g_conf_use_processor) {
        if (is_initialized) {
            handle_simulation();
            get_module_obj()->chip->check_for_thread_yield();
        }
    }
    
    if (g_conf_power_profile) {
        for (uint32 i = 0; i < num_devices; i++) 
            devices[i]->cycle_end();
    }
        

	if (g_conf_print_l1miss_bank_trace && 
		(get_g_cycles() % g_conf_print_l1miss_bank_trace == 0))
	{
		char affinity_str[255] = {0};
		char mm_affinity_str[255] = {0};
		char temp_str[255] = {0};
		uint64 tot_misses = 0;
		uint64 tot_l2_misses = 0;
		for (uint32 p = 0; p < num_processors; p++) {
			for (uint32 b = 0; b < g_conf_l2_banks; b++) {
				tot_misses += (l1_misses_to_bank[p])[b];
				tot_l2_misses += (l2_misses_to_bank[p])[b];
			}
		}
		
		for (uint32 p = 0; p < num_processors; p++) {
			uint64 misses = 0;
			uint64 l2_misses = 0;
			for (uint32 b = 0; b < g_conf_l2_banks; b++) {
				misses += (l1_misses_to_bank[p])[b];
				l2_misses += (l2_misses_to_bank[p])[b];
			}			
			
			snprintf(temp_str, 255, "%s %llu", affinity_str, misses);
			strncpy(affinity_str, temp_str, 255);
			snprintf(temp_str, 255, "%s %llu", mm_affinity_str, l2_misses);
			strncpy(mm_affinity_str, temp_str, 255);
			for (uint32 b = 0; b < g_conf_l2_banks; b++) {
				snprintf(temp_str, 255, "%s %3.3f", affinity_str,
					(l1_misses_to_bank[p])[b] / (1.0 * tot_misses));
				strncpy(affinity_str, temp_str, 255);
				snprintf(temp_str, 255, "%s %3.3f", mm_affinity_str,
					(l2_misses_to_bank[p])[b] / (1.0 * tot_l2_misses));
				strncpy(mm_affinity_str, temp_str, 255);
			}
		}
		
		DEBUG_LOG("$$l2affin %llu %s\n", get_g_cycles(), affinity_str);
		DEBUG_LOG("$$mmaffin %llu %s\n", get_g_cycles(), mm_affinity_str);

		// If we're not actually doing any migrations, clear stats
		if (g_conf_periodic_interrupt == 0) {
			for (uint32 i = 0; i < num_processors; i++) {
				clear_l1_misses_to_bank(i);
			}
		}
	}

}


void mem_hier_t::handle_simulation()
{
    uint64 total_cycles = g_cycles;
    uint64 proc_commit;

    // SPIN LOOP CHECK
    for (uint32 i = 0; i < num_processors; i++) {
        sequencer_t *seq = module_obj->chip->get_sequencer(i);
        uint32 thread_id = seq->get_thread(0); //TODO SMT
        if (thread_id == num_threads) continue;
        uint64 curr_step_count = SIM_step_count(vcpus[thread_id]);
        proc_commit = curr_step_count - spin_step_count[thread_id];
        if (SIM_cpu_privilege_level(vcpus[thread_id]) == false)
            stat_thread_user_cycles->inc_total(1, thread_id);
        if (g_conf_spin_check_interval && g_conf_spinloop_threshold && 
            proc_commit > (uint64) g_conf_spin_check_interval) {
            // TODO -- SMT
            bool spinloop = seq->get_spin_heuristic()->check_spinloop();
            if (spinloop) {
                if (g_conf_trigger_spin)
                    seq->potential_thread_switch(0, YIELD_MUTEX_LOCKED);
                // DEBUG_OUT("thread %u is spinning on cpu %u with PC %llx %u\n", thread_id, i,
                //     SIM_get_program_counter(SIM_proc_no_2_ptr(thread_id)), 
                //     SIM_cpu_privilege_level(SIM_proc_no_2_ptr(thread_id)));
            }
            spin_step_count[thread_id] = curr_step_count;
        }
            
    }
    
    // COMMIT COUNT
    for (uint32 i = 0; i < num_threads; i++) {
        uint64 curr_step_count = SIM_step_count(vcpus[i]);
        proc_commit = curr_step_count - last_step_count[i];
        proc_stats_t *tstat = module_obj->chip->get_tstats(i);
        if (proc_commit) {
            tstat->stat_commits->inc_total(proc_commit);
            //stat_num_commits->inc_total(proc_commit);
            stat_num_commits->inc_total(1);
            if (SIM_cpu_privilege_level(vcpus[i]) == 0) {
                stat_user_commits->inc_total(proc_commit);
                stat_thread_user_commits->inc_total(proc_commit, i);
            }
        }
        last_step_count[i] = curr_step_count;
    }
    
    uint64 total_commits = STAT_GET(stat_num_commits);
    uint64 user_commits = STAT_GET(stat_user_commits);
        
    if ((g_conf_run_commits && total_commits >= (uint64)g_conf_run_commits) ||
        (g_conf_run_cycles && total_cycles >= (uint64) g_conf_run_cycles) || 
        (g_conf_run_user_commits && user_commits >= (uint64) g_conf_run_user_commits))
    {
	//cout<<"simulation done!!!!!!!!!!!!!!!!!!!!!\n"<<endl;
        module_obj->get_stats();
        SIM_break_simulation("Done");
    }
}


void
mem_hier_t::complete_request(conf_object_t *cpu, mem_trans_t *trans)
{
	VERBOSE_OUT(verb_t::requests, 
			  "%10s @ %12llu 0x%016llx: mem_hier_t::complete_request()\n", 
			  "mem_hier", external::get_current_cycle(), trans->phys_addr);

			  
	complete_stats(trans);
    ASSERT(trans->completed && (trans->get_pending_events() ||
		trans->cache_prefetch || !trans->call_complete_request ||
		trans->random_trans));
							  
	if (g_conf_handle_io_devices && trans->io_device && trans->write) {
		num_io_writes--;
	}
	
    if (g_conf_stackheap_profile && !trans->ifetch && !trans->io_device)
        stackheap_profile->record_access(trans);
    
	if (trans->call_complete_request) {
        if (g_conf_mem_hier_response_tracking)
            tr_rec->delete_record(trans->cpu_id, trans);
        if (trans->random_trans) {
            ASSERT(g_conf_random_addresses);
            random_tester->complete_random_transaction(trans);
        } else {
		    mhh->complete_request((conf_object_t *) get_module_obj(), cpu, trans);
        }
	} else {
		ASSERT(!trans->may_stall || trans->hw_prefetch || trans->cache_prefetch
			|| trans->random_trans);
	}
}

bool
mem_hier_t::is_condor_checkpoint()
{
    return (module_obj->get_chip_cycles() > 100);
}

void
mem_hier_t::invalidate_address(conf_object_t *cpu, invalidate_addr_t *ia)
{
	VERBOSE_OUT(verb_t::requests, 
			  "%10s @ %12llu 0x%016llx: mem_hier_t::invalidate_address() size %u\n", 
			  "mem_hier", external::get_current_cycle(), ia->address,
			  ia->size);

	mhh->invalidate_address((conf_object_t *) get_module_obj(), cpu, ia);
}

mem_return_t
mem_hier_t::make_request(conf_object_t *cpu, mem_trans_t *trans)
{
	// Bootstrap mem-hier
	// TODO: fix
	if (!is_initialized){
		if (!config) read_runtime_config_file("");
		init();
	}
	
	ASSERT(trans->ini_ptr);
	ASSERT_WARN(trans->is_cacheable() || trans->is_io_space() || 
		trans->io_device);
        
    ASSERT(!trans->completed);    
	
	if (!trans->occurred(MEM_EVENT_DEFERRED)) {
		// Make a copy if we shouldn't call complete request and not deferred already
		if (!trans->random_trans && (g_conf_copy_transaction || !trans->call_complete_request)) {
            mem_trans_t *old_trans = trans;
			trans = get_mem_trans();
            trans->copy(*old_trans);
            trans->clear_all_events();
			trans->pending_messages = 0;
            if (g_conf_copy_transaction) {
                old_trans->clear_pending_event(PROC_REQUEST);
                if (trans->call_complete_request) {
                    trans->mark_pending(PROC_REQUEST);
                }
            }
		}

		ASSERT(trans->request_time == 0);
		trans->request_time = external::get_current_cycle();

		request_stats(cpu, trans);
	}
	
    if (g_conf_mem_hier_response_tracking && trans->call_complete_request)
        tr_rec->insert_record(trans->cpu_id, trans, g_cycles);
	
	mem_return_t ret ;
    if (g_conf_optimize_stack_write && !trans->ifetch && trans->call_complete_request
        && stackheap_profile->stack_access(trans)) {
        ret = MemComplete;
    } 
    //else if (trans->hw_prefetch){
//	cout<<"prefetch returned "<<endl;
//	ret = MemComplete;
    //}
	else if (trans->io_device) {
		ret = handle_io_dev_request(cpu, trans);
	} else { 
		ret = handle_proc_request(cpu, trans);
        if (g_conf_random_addresses && !trans->ifetch && trans->may_stall
            && !trans->random_trans && (g_cycles % g_conf_random_transaction_frequency == 0)) {
            random_tester->generate_random_transactions(trans);
        }
	}

	switch (ret) {
	case MemComplete:
//        ASSERT(!trans->random_trans);
		trans->completed = true;
		complete_request(cpu, trans);
		break;
	
	case MemStall:
		defer_trans(trans);
		break;
	
	case MemMiss:
		// nothing
		break;
	
	default:
		FAIL;
	}
	return ret;
}

mem_return_t
mem_hier_t::handle_proc_request(conf_object_t *cpu, mem_trans_t *trans)
{
	//ASSERT(trans->cpu_id < num_processors);
	// Assume first n-devices are Procs
	//   Multiplex request to appropriate processor
	generic_proc_t *proc = get_proc_object(cpu);
	
	VERBOSE_OUT(verb_t::requests, 
		"%10s @ %12llu 0x%016llx: mem_hier_t::make_request()\n", 
		"mem_hier", external::get_current_cycle(), trans->phys_addr);
		
	if (trans->dinst && trans->dinst->is_squashed()) {
        trans->completed = true;
		STAT_INC(stat_num_squash);
		return MemComplete;
	}

	if (g_conf_perfect_icache && trans->ifetch)
		return MemComplete;
	if (g_conf_perfect_dcache && !trans->ifetch)
		return MemComplete;
	if (g_conf_mem_ignore_os && trans->supervisor)
		return MemComplete;
	if (g_conf_mem_ignore_user && !trans->supervisor)
		return MemComplete;
    if (g_conf_perfect_store_access && 
        ((trans->write || trans->atomic) && !trans->vcpu_state_transfer))
        return MemComplete;
		
	if (!trans->may_stall) {
		if (!trans->atomic) {
			VERBOSE_OUT(verb_t::requests, 
				"%10s @ %12llu 0x%016llx: unstallable, nonatomic processor trans\n", 
				"mem_hier", external::get_current_cycle(), trans->phys_addr);
		}
		
		// Just complete when doing single-transaction atomics
		if (g_conf_single_trans_atomics) return MemComplete;	
	}
	
	if (!g_conf_cache_io_space && trans->is_io_space()) {
		return MemComplete;
	}
	
	if (g_conf_per_syscall_kern_stack && trans->supervisor && !trans->ifetch &&
		trans->get_access_type() == LWP_STACK_ACCESS)
	{
		// map physical addresses to something unused (i.e. >memory_image_size)
		// Note: after check for is_io_space above...
		addr_t seg_addr = trans->virt_addr - 0x000002a100000000ULL;
		
		// Give each sequencer 0x10000000 bytes, which should be more than
		// the amount actually used by all the LWPs 
		ASSERT_WARN(seg_addr < 0x10000000);
		trans->phys_addr = g_conf_memory_image_size + 
			0x10000000 * trans->mem_hier_seq->get_id() + 
			seg_addr;
	}
	if (g_conf_per_syscall_kern_map && trans->supervisor && !trans->ifetch &&
		trans->get_access_type() == KERNEL_MAP_ACCESS)
	{
		// map physical addresses to something unused (i.e. >memory_image_size)
		// Note: after check for is_io_space above...
		addr_t seg_addr = trans->virt_addr - 0x0000030000000000ULL;
		
		// Give each sequencer 0x20000000 bytes, which should be more than
		// the amount actually used by all the LWPs 
		ASSERT_WARN(seg_addr < 0x20000000);
		
		// map these above LWP stacks
		trans->phys_addr = g_conf_memory_image_size + 
			get_num_processors() * 0x10000000,
			0x20000000 * trans->mem_hier_seq->get_id() + 
			seg_addr;
	}

	// Stall new requests if pending io write
	if (g_conf_handle_io_devices && num_io_writes > 0) {
		// But don't stall the second in an atomic pair
		if (!trans->may_stall && !trans->atomic) { } 
		else return MemStall;
	}
	
	ASSERT(proc);
    return proc->make_request(cpu, trans);
}

mem_return_t
mem_hier_t::handle_io_dev_request(conf_object_t *cpu, mem_trans_t *trans)
{
	VERBOSE_OUT(verb_t::requests, 
		"%10s @ %12llu 0x%016llx: mem_hier_t::make_request() I/O %s\n", 
		"mem_hier", external::get_current_cycle(), trans->phys_addr,
		trans->read ? "read" : "write");
		
	if (!g_conf_handle_io_devices) return MemComplete;
	if (!g_conf_handle_iodev_reads && trans->read) return MemComplete;
	
	// Inc # outstanding IO writes (if we havn't done so already for trans)
	if (trans->write && !trans->occurred(MEM_EVENT_DEFERRED)) num_io_writes++;
	
	// io dev should be first device after all procs  FIXME: better way
	generic_io_dev_t *iodev = 
		static_cast<generic_io_dev_t*> (devices[num_processors]);
	
	return iodev->make_request(trans);
}

void
mem_hier_t::request_stats(conf_object_t *cpu, mem_trans_t *trans)
{
	// Stats
	STAT_INC(stat_num_requests);
	stat_request_size->inc_total(1, trans->size);
	if (trans->read) STAT_INC(stat_num_reads);
	if (trans->write) STAT_INC(stat_num_writes);
	if (trans->atomic) STAT_INC(stat_num_atomics);
	stat_cpu_histo->inc_total(1, trans->cpu_id);
	if (!trans->is_cacheable()) {
		STAT_INC(stat_num_uncache);
		if (trans->io_device) STAT_INC(stat_num_uncache_io_device);
		if (trans->is_io_space()) STAT_INC(stat_num_uncache_io_space);
	}
	if (trans->io_device) {
		STAT_INC(stat_num_io_device);
		if (!trans->is_io_space()) STAT_INC(stat_num_io_device_phys_space);
	}
	if (trans->is_io_space()) STAT_INC(stat_num_io_space);
	if (!trans->may_stall) STAT_INC(stat_num_may_not_stall);

	if (g_conf_watch_idle_loop && trans->ifetch) {
		if (trans->phys_addr <= (addr_t) g_conf_idle_max_pc &&
			trans->phys_addr >= (addr_t) g_conf_idle_min_pc)
		{
			if (!currently_idle[trans->cpu_id]) {
				currently_idle[trans->cpu_id] = true;
				idle_start[trans->cpu_id] = external::get_current_cycle();
			}
		}
		else {
			if (currently_idle[trans->cpu_id]) {
				currently_idle[trans->cpu_id] = false;
				stat_idle_cycles_histo->inc_total(
					external::get_current_cycle() - idle_start[trans->cpu_id],
					trans->cpu_id);
			}
		}
	}

	//stat_asi_histo->inc_total(1, trans->asi);
}

void
mem_hier_t::complete_stats(mem_trans_t *trans)
{
	if (g_conf_l2_stripe_size
//		 && !trans->ifetch
		)
	{
		mai_t *mai = trans->dinst ? trans->dinst->get_mai_instruction()->
			get_mai_object() : NULL;
		uint32 vcpu_id = mai ? mai->get_id() : trans->cpu_id;
		if (trans->occurred(MEM_EVENT_L1_DEMAND_MISS) ||
			trans->occurred(MEM_EVENT_L1_COHER_MISS))
		{
				
			if (g_conf_print_l1miss_addr_trace) {
				DEBUG_LOG("$$l1miss%d \%llu %llu %lld 0x%llx %d 0x%llx\n", 
					vcpu_id, get_g_cycles(),
					trans->phys_addr >> g_conf_l2_stripe_size, 
					mai ? mai->get_tl() : 0,
					mai ? (mai->get_tl() > 0 ? mai->get_tt(mai->get_tl()) : 0) : 0,
					mai ? mai->is_supervisor() : 0, 
					trans->dinst ? trans->dinst->get_pc() : 0);
			}
			// calculate bank stripe and affinity
			//		if (!mai || mai->get_tl() == 0
				//|| mai->get_tt(mai->get_tl()) != 0x68
				//		) {
			uint32 bank = (trans->phys_addr >> g_conf_l2_stripe_size) % g_conf_l2_banks;
			(l1_misses_to_bank[vcpu_id])[bank]++;
			//		}
		}
		
		if (trans->occurred(MEM_EVENT_L2_DEMAND_MISS) ||
			trans->occurred(MEM_EVENT_L2_COHER_MISS))
		{
// PA			uint32 bank = (trans->phys_addr >> g_conf_l2_stripe_size) % g_conf_l2_banks;
			uint32 bank = (trans->virt_addr >> g_conf_l2_stripe_size) % g_conf_l2_banks;
			(l2_misses_to_bank[vcpu_id])[bank]++;
		}
	}

	if (trans->cache_prefetch || 
        (trans->hw_prefetch && (trans->prefetch_type != FB_PREFETCH))) 
		return;
	
	if (is_thread_state(trans->phys_addr)) {
		stat_vcpu_state_requesttime->inc_total(1, external::get_current_cycle() - 
			trans->request_time);
		return;
	}
	tick_t req_latency = external::get_current_cycle() - trans->request_time;
	stat_requesttime_histo->inc_total(1, req_latency);
	if (req_latency > 18)
		stat_requesttime_histo2->inc_total(1, req_latency);
		
    if (trans->ifetch)
        stat_instr_reqtime_histo->inc_total(1, req_latency);
    else
        stat_data_reqtime_histo->inc_total(1, req_latency);

	if (trans->occurred(MEM_EVENT_L1_HIT))
		STAT_INC(stat_l1_hit);
	if (trans->occurred(MEM_EVENT_L1_DEMAND_MISS))
		STAT_INC(stat_l1_miss);
	if (trans->occurred(MEM_EVENT_L1_COHER_MISS)) {
		STAT_INC(stat_l1_miss);
		STAT_INC(stat_l1_coher_miss);
	}
	if (trans->occurred(MEM_EVENT_L1_MSHR_PART_HIT))
		STAT_INC(stat_l1_mshr_part_hit);
	if (trans->occurred(MEM_EVENT_L1_STALL))
		STAT_INC(stat_l1_stall);

	if (trans->occurred(MEM_EVENT_L2_HIT))
		STAT_INC(stat_l2_hit);
	if (trans->occurred(MEM_EVENT_L2_DEMAND_MISS))
		STAT_INC(stat_l2_miss);
	if (trans->occurred(MEM_EVENT_L2_COHER_MISS)) {
		STAT_INC(stat_l2_miss);
		STAT_INC(stat_l2_coher_miss);
	}
	if (trans->occurred(MEM_EVENT_L2_MSHR_PART_HIT))
		STAT_INC(stat_l2_mshr_part_hit);
	if (trans->occurred(MEM_EVENT_L2_REQ_FWD))
		STAT_INC(stat_l2_req_fwd);
	if (trans->occurred(MEM_EVENT_L2_STALL))
		STAT_INC(stat_l2_stall);

	if ((verb_t::l1miss_trace & g_conf_debug_level) && 
		(trans->occurred(MEM_EVENT_L1_COHER_MISS) ||
		 trans->occurred(MEM_EVENT_L1_DEMAND_MISS)))
	{
		DEBUG_LOG("%02d 0x%08llx 0x%08llx %s\n", trans->cpu_id, trans->phys_addr,
			trans->virt_addr, trans->ifetch ? "I":"D");
	}		
		
	if (verb_t::access_trace & g_conf_debug_level)
	{
		DEBUG_LOG("%02d 0x%08llx 0x%08llx %s\n", trans->cpu_id, trans->phys_addr,
			trans->virt_addr, trans->ifetch ? "I":"D");
	}		
		
	if (g_conf_profile_mem_addresses)
		address_profiler->mem_request(trans);
    
    if (g_conf_profile_codeshare && trans->ifetch)
        codeshare->record_fetch(trans);

}

void
mem_hier_t::defer_trans(mem_trans_t *trans)
{
	ASSERT(!trans->completed);

	trans->mark_pending(MEM_EVENT_DEFERRED);

	// Does not have to be FIFO for SC if:
	//    - protocols all support speculative loads or processor sends only one
	//      load at a time
	//    - processor won't send more than one store at a time
	// TODO: impelment this per/processor or per/cache
	_stall_event_t *e = new _stall_event_t(trans, this, stall_delta,
		stall_event_handler);
	e->enqueue();
	num_deferred++;
}
	

bool
mem_hier_t::quiet_and_ready()
{
    return (is_quiet() && checkpoint_taken);
}


bool
mem_hier_t::is_quiet()
{
	if (num_deferred > 0) return false;

	// Check each device ...
	for (uint32 i = 0; i < num_devices; i++) {
		if (!devices[i]->is_quiet()) return false;
	}

	// ... and each link
	for (uint32 i = 0; i < num_links; i++) {
		if (!links[i]->is_quiet()) return false;
	}
	
	// Quiet!
	// Can have an outstanding print stats event
	ASSERT(external::get_num_events() == 0 || external::get_num_events() == 1);
	return true;
}


void
mem_hier_t::quiesce_event_handler(_quiesce_event_t *e)
{
	// TODO: fix checkpointing

	mem_hier_t *mem = e->get_context();
	
	if (mem->is_quiet()) {
		// We're good to go!
		//external::write_configuration(*filename);
        mem->write_checkpoint(g_conf_mem_hier_checkpoint_out);
		//external::quit(0);

		if (g_conf_read_warmup_trace)   // Dumb hack
			SIM_break_simulation("Warmup memhier checkpoint complete");
			
	} else {
		VERBOSE_OUT(verb_t::checkpoints,
					"[%s] Mem-hier not quiet.  Will try again in %d cycles\n",
					mem->get_name(), 
					g_conf_quiesce_delta);

		// Otherwise wait another while
		e->enqueue();
	}
	
}

void
mem_hier_t::checkpoint_and_quit()
{
	if (g_conf_mem_hier_checkpoint_out == "" || g_conf_workload_checkpoint == "") {
		VERBOSE_OUT(verb_t::checkpoints, 
				  "[%s] ERROR: No condor checkpoint file specified!  Exiting\n",
				  mem_hier_t::ptr()->get_name());

	//	external::quit(0);
        return;
	}

    checkpoint_signal_pending = true;
	
	

	//	get_handler()->stall_processor();
	
	if (is_quiet()) {
        VERBOSE_OUT(verb_t::checkpoints,
			    "[%s] Received SIGTSTP.  Writing checkpoint %s\n",
				mem_hier_t::ptr()->get_name(), g_conf_mem_hier_checkpoint_out.c_str());

		// We're good to go!
		//external::write_configuration(g_conf_workload_checkpoint);
        write_checkpoint(g_conf_mem_hier_checkpoint_out);
		//external::quit(0);
		if (g_conf_read_warmup_trace)   // Dumb hack
			SIM_break_simulation("Warmup memhier checkpoint complete");
	} else {
		VERBOSE_OUT(verb_t::checkpoints,
					"[%s] Mem-hier not quiet.  Will try again in %d cycles\n",
					mem_hier_t::ptr()->get_name(), 
					g_conf_quiesce_delta);

		_quiesce_event_t *e = new _quiesce_event_t(&g_conf_workload_checkpoint, this,
		                                           g_conf_quiesce_delta,
												   quiesce_event_handler);
		e->enqueue();
	}
}


void
mem_hier_t::stall_event_handler(_stall_event_t *e)
{
	mem_hier_t *mem = e->get_context();
	mem_trans_t *trans = e->get_data();
	delete e;
	mem->num_deferred--;
	
	ASSERT(!trans->completed);
	
	// TODO: fix:  should save cpu * somewhere else
	mem->make_request(trans->ini_ptr, trans);
}
	

void
mem_hier_t::clear_stats()
{
	stats->clear();

	for (uint32 i = 0; i < num_devices; i++) {
		devices[i]->clear_stats();
	}
}

void
mem_hier_t::print_stats()
{
    // Flush out idle loop count
	if (g_conf_watch_idle_loop) {
		for (uint32 i = 0; i < num_processors; i++) {
			if (currently_idle[i]) {
				stat_idle_cycles_histo->inc_total(
					external::get_current_cycle() - idle_start[i], i);
			}
			idle_start[i] = external::get_current_cycle();
		}
	}
	
	stats->print();
	for (uint32 i = 0; i < num_devices; i++) {
		devices[i]->print_stats();
	}
    profiles->print();
}
void
mem_hier_t::printStats(){

	//call DRAMSim2 dump stats
	dramptr->printStats();	
}
	
void
mem_hier_t::write_checkpoint(string filename)
{

	VERBOSE_OUT(verb_t::checkpoints, "mem_hier_t::write_checkpoint(): %s\n",
			  filename.c_str());

	FILE * file;
	file = fopen(filename.c_str(), "w");

	if (!file) {
		ERROR_OUT("Unable to open file %s\n!", filename.c_str());
		return;
	}
    
    // Output the counters
    fprintf(file, "%llu\n", g_cycles);
    fprintf(file, "\n");
    
    stats->to_file(file);
	
	for (uint32 i = 0; i < num_devices; i++) {
		devices[i]->to_file(file);
        devices[i]->stats_write_checkpoint(file);
        devices[i]->profiles_write_checkpoint(file);
	}
	
	if (profiles) profiles->to_file(file);
	
	fclose(file);

	VERBOSE_OUT(verb_t::checkpoints, "mem_hier_t::finished writing checkpoint\n");
	checkpoint_taken = true;
}

void
mem_hier_t::read_checkpoint(string filename)
{
	VERBOSE_OUT(verb_t::checkpoints, "mem_hier_t::read_checkpoint(): %s\n", filename.c_str());

	if (filename.length() == 0) {
		VERBOSE_OUT(verb_t::checkpoints, "None to read\n");
		return;
	}

	VERBOSE_OUT(verb_t::checkpoints, "reading...");
	
	FILE * file;
	file = fopen(filename.c_str(), "r");
	if (!file) { 
        FAIL_MSG("unable to open mem_hier checkpoint %s", filename.c_str());
    }
    
    fscanf(file, "%llu\n", &g_cycles);
	if (is_condor_checkpoint()) 
        stats->from_file(file);
    

	for (uint32 i = 0; i < num_devices; i++) {
		devices[i]->from_file(file);
        if (is_condor_checkpoint()) {
            devices[i]->stats_read_checkpoint(file);
        	devices[i]->profiles_read_checkpoint(file);
        }
	}
	
	// Hack not to read profiles for warm-up checkpoints
	if (is_condor_checkpoint())
		profiles->from_file(file);
		
	VERBOSE_OUT(verb_t::checkpoints, "done\n");
	
}

mem_trans_t *mem_hier_t::get_mem_trans()
{
    mem_trans_t *trans;
    if (mem_trans_array_index == g_conf_mem_trans_count) mem_trans_array_index = 0;
    for (uint32 i = 0; i < g_conf_mem_trans_count; i++)
    {
        trans = mem_trans_array[mem_trans_array_index++];
        if (trans->pending_messages == 0)
        {
            if (trans->completed) {
                trans->recycle();
                return trans;
            } 
        }
        if (trans->dinst && trans->call_complete_request && 
            trans->dinst->get_mem_transaction()->get_mem_hier_trans () != trans)
        {
            DEBUG_OUT("WTF!!\n");
        }
        if (mem_trans_array_index == g_conf_mem_trans_count) mem_trans_array_index = 0;
        
    }
    
    FAIL_MSG("Not Enough mem_trans_t");
    
}

	

proc_object_t *
mem_hier_t::get_module_obj()
{
	return module_obj;
}

const char *
mem_hier_t::get_name()
{
	// TODO: fix
	return "mem-hier";
}

void
mem_hier_t::profile_commits(dynamic_instr_t *dinst)
{
	if (g_conf_profile_spin_locks) {
		if (dinst->is_load() || dinst->is_store() || dinst->is_atomic())
			spin_lock_profiler->profile_instr_commits(dinst);
	}
	
}

bool 
mem_hier_t::is_thread_state(addr_t address) 
{
    return (address >= THREAD_STATE_BASE);
	//return ((address >= THREAD_STATE_BASE) && 
	//	(address < (THREAD_STATE_BASE + SIM_number_processors()*THREAD_STATE_OFFSET)));
}

void
mem_hier_t::reset_spin_commit(uint32 thread_id)
{
    if (is_initialized) 
        spin_step_count[thread_id] = SIM_step_count(vcpus[thread_id]);
}

void
mem_hier_t::update_leak_stat(uint32 thread)
{
    stat_leaked_trans->inc_total(1, thread);
}

void mem_hier_t::cleanup_previous_stats(FILE *file, const char *pattern)
{
    char readstring[g_conf_max_classname_len];
    long filepos;
    do {
        filepos = ftell(file);
        if (fscanf(file, "%s", readstring) == EOF) {
            fseek(file, 0, SEEK_SET);
            filepos = 0;
            fscanf(file, "%s", readstring);
        }
    } while (strcmp(pattern, readstring));
    fseek(file, filepos, SEEK_SET);
    
}

uint32 mem_hier_t::get_machine_id(uint32 cpu_id)
{
	if (ptr()->is_initialized)
		return ptr()->get_module_obj()->chip->get_machine_id(cpu_id);
	else 
		return 0;
}

void mem_hier_t::update_mem_link_latency(tick_t lat)
{
    stat_memlinklatency_histo->inc_total(1, lat);
}

base_counter_t *mem_hier_t::get_cycle_counter()
{
    return stat_cycles;   
}

uint64 *mem_hier_t::get_l1_misses_to_bank(uint32 cpu_id)
{
	return l1_misses_to_bank[cpu_id];
}

void mem_hier_t::clear_l1_misses_to_bank(uint32 cpu_id)
{
	bzero(l1_misses_to_bank[cpu_id], (sizeof(uint64) * g_conf_l2_banks));
	bzero(l2_misses_to_bank[cpu_id], (sizeof(uint64) * g_conf_l2_banks));
}

uint64 mem_hier_t::get_commits(){
	//return STAT_GET(stat_num_commits);
	uint64 total_commits=0;
	for (uint32 i=0; i < num_threads;i++){
           if (ptr()->is_initialized){
		  proc_stats_t **tstat_l = ptr()->get_module_obj()->chip->get_tstats_list(i);
		  total_commits += STAT_GET(tstat_l[0]->stat_commits);
	   	  //return STAT_GET(tstat_l[0]->stat_user_commits);
	   }
	}
		  return total_commits;
                //return STAT_GET(ptr()->get_module_obj()->chip->tstats_list[0]->stat_commits);
}
