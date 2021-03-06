/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 */

/* description:    translates between the simics 'timing-model' and mem-hier
 *                 interfaces
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: mem_driver.cc,v 1.4.4.24 2006/12/12 18:36:57 pwells Exp $");
 
#include "definitions.h"
#include "transaction.h"
#include "profiles.h"
#include "mem_hier_handle.h"
#include "mem_driver.h"
#include "verbose_level.h"
#include "profiles.h"
#include "mem_hier.h"
#include "external.h"
#include "startup.h"
#include "shb.h"
#include "sequencer.h"
#include "config.h"
#include "config_extern.h"
#include "chip.h"
#include "sequencer.h"
#include "mai.h"
#include "warmup_trace.h"

#include <signal.h>


static chip_t *static_chip_object;

static uint32 get_cpu_from_mmu(conf_object_t *obj)
{
    uint32 num_threads = SIM_number_processors();
    for (uint32 i = 0; i < num_threads; i++)
    {
        attr_value_t val = SIM_get_attribute(SIM_proc_no_2_ptr(i), "mmu");
        if (val.u.object == obj)
            return i;
    }
    FAIL;
    return 0;
}

static exception_type_t static_handle_asi (conf_object_t *obj,
		generic_transaction_t *mem_op)
{
    uint32 src = get_cpu_from_mmu(obj);
    mem_driver_t *mem_driver = static_chip_object->get_mem_driver();
    
    if ((mem_op->logical_address & 0x3fff) == 0x70) {
        //uint64 target = (mem_op->logical_address & 0x3c000) >> 14;
        //return Sim_PE_No_Exception;
        uint64 target = (mem_op->logical_address & 0xffc000) >> 14;
        ASSERT(target < 24);
        if (g_conf_paused_vcpu_interrupt && !mem_driver->is_bootstrap()) {
            // DEBUG_OUT("src : vcpu%u target: %s vcpu%u @ %llu\n", src, 
            //     mem_driver->is_paused((uint32) target) ? "paused" : "running" , (uint32) target, 
            //     static_chip_object->get_g_cycles());
            mem_driver->handle_synchronous_interrupt(src, (uint32) target);
        }
        static_chip_object->record_synchronous_interrupt(src,target);
    }
    addr_t val = SIM_get_mem_op_value_cpu(mem_op);
    bool print = //(mem_op->logical_address == 0x50 || 
        //mem_op->logical_address == 0x60 ||
        (mem_op->logical_address == 0x40 && val != 0x100a12c) ;
    if (mem_op->logical_address == 0x40) {
        static_chip_object->update_cross_call_handler(val);
        mem_driver->cross_call_handler.insert(val);
    }
    if (print) 
    DEBUG_OUT("src vcpu%u logical %llx val %llx PC %llx\n", src, mem_op->logical_address,
        val, SIM_get_program_counter(SIM_proc_no_2_ptr(src)));
    
    //return Sim_PE_No_Exception;
    return Sim_PE_Default_Semantics;
    
}


mem_driver_t::mem_driver_t(proc_object_t *_processor) :
    proc(_processor)
{
	mem_hier = new mem_hier_t(_processor);
	mem_hier->set_ptr(mem_hier);	
	mem_hier->set_handler(this);
    runtime_config_file = "Nothing";
	transactions = new hash_map < int, mem_trans_t *> [SIM_number_processors()+1];
    break_sim_called = false;
    advance_memhier_cycles = false;
    pending_checkpoint = false;
    paused = NULL;   
    cached_trans_id = NULL;
    paused_time = NULL;
    bootstrap = true;
    static_chip_object = proc->chip;
	last_fetch_cache_addr = new addr_t[SIM_number_processors()];

	if (g_conf_read_warmup_trace || g_conf_write_warmup_trace)
		warmup_trace = new warmup_trace_t(mem_hier, proc);
}


void mem_driver_t::auto_advance_memhier()
{
    uint32 count = 100;
    uint32 steps = 0;
    proc->chip->stall_front_end();
    while (!mem_hier->is_quiet()) {
        for (uint32 i = 0; i < count; i++)
            mem_hier->advance_cycle();
        steps++;
    }
    advance_memhier_cycles = false;
    
}


cycles_t 
mem_driver_t::timing_operate(conf_object_t *proc_obj, conf_object_t *space, 
	map_list_t *map, generic_transaction_t *mem_op)
{
	v9_memory_transaction_t *v9_trans = (v9_memory_transaction_t *) mem_op;
	v9_trans->s.block_STC = 1;
    sequencer_t *seq = NULL;
	
    // OS pause bootstrap
    if (bootstrap && g_conf_no_os_pause)
        return bootstrap_os_pause(proc_obj, mem_op);
	
	if (g_conf_read_warmup_trace || g_conf_write_warmup_trace)
		return warmup_trace->operate(proc_obj, mem_op);
        
	// To indicate proper CPU or some I/O device
	uint8 proc_id = SIM_mem_op_is_from_cpu(&(v9_trans->s)) ?
				SIM_get_proc_no(v9_trans->s.ini_ptr) : SIM_number_processors();

	mem_trans_t *trans;
	trans = (transactions[proc_id])[v9_trans->s.id];

    if (!trans) {
		// Not found, create new one and add
        trans = mem_hier->get_mem_trans();
		trans->copy(v9_trans);
		// don't call for unstallable transactions or prefetches
		// except for the second atomic if its a read (eg ldda)
		// except that u2 doesn't mark these as atomic like u3, so
		// hack some more crap in too 
		if ((!trans->may_stall || trans->hw_prefetch) &&
			(!trans->atomic || trans->write) &&
			(trans->io_device || !trans->read))
		{
			trans->call_complete_request = false;
		}
        ASSERT(trans->id >= 0);
        
        if (!trans->io_device && paused[trans->cpu_id] && trans->may_stall) {
            if (trans->id == cached_trans_id[trans->cpu_id]) {
                (transactions[proc_id])[trans->id] = trans;
                trans->pending_messages++;
                cached_trans_id[trans->cpu_id] = -1;
                mem_hier->update_leak_stat(trans->cpu_id);
                goto done;
            }
            cached_trans_id[trans->cpu_id] = trans->id;
            trans->completed = true;
            goto stall_indefinitely;
        }
        (transactions[proc_id])[trans->id] = trans;
		
		if (g_conf_ino_decode_instr_info && trans->ifetch) {
			seq = proc->chip->get_sequencer_from_thread(trans->cpu_id);
			//seq->ino_decode_instr_info(trans->phys_addr, trans->cpu_id);
		}
		
		// set ifm to instr-trace-mode so we can decode all the instr above
		// but don't want to send to mem hier unless it is to a different cache
		// line than last time
		if (g_conf_ino_fake_cache_trace_mode && trans->ifetch) {
			addr_t ifetch_addr = trans->phys_addr & (~(g_conf_l1i_lsize-1));
			
			if (last_fetch_cache_addr[trans->cpu_id] == ifetch_addr) {
				trans->completed = true;
				trans->pending_messages++;
				goto done;
			}
			else
				last_fetch_cache_addr[trans->cpu_id] = ifetch_addr;
		}
		
		if (g_conf_cache_data) {
			if (trans->write) {
				// Copy write data to trans
				trans->data = new uint8[trans->size];
				SIM_c_get_mem_op_value_buf(&v9_trans->s, (char *)trans->data);  

				VERBOSE_OUT(verb_t::data, "%10s @ %12llu 0x%016llx: storing\n",
			    	        "mem-driver", external::get_current_cycle(), trans->phys_addr);
				VERBOSE_DATA("mem-driver", external::get_current_cycle(), trans->phys_addr,
				             trans->size, trans->data);
			}
			// Reads checked in snoop_operate
		}

		// Make a new request to the memory hierarchy
		// TODO: ensure ini_ptr is a cpu
        if (pending_checkpoint)
        {
            if (advance_memhier_cycles) auto_advance_memhier();
            if (mem_hier->is_quiet() && !break_sim_called) {
                mem_hier->write_checkpoint(g_conf_mem_hier_checkpoint_out);
                if (g_conf_processor_checkpoint_out != "") {
                    FILE *file = fopen(g_conf_processor_checkpoint_out.c_str(), "w");
                    proc->chip->write_checkpoint(file);
                    fclose(file);
                }
                //SIM_write_configuration_to_file(g_conf_workload_checkpoint.c_str());
                //SIM_quit(0);
                proc->raise_checkpoint_hap();
                SIM_break_simulation("Ready to take warmup checkpoint");
                break_sim_called = true;
                trans->completed = 1;
            } else {
                goto done;
            }
        } else {
            trans->mark_pending(PROC_REQUEST);
            trans->pending_messages++;
            if (!trans->io_device)
                seq = proc->chip->get_sequencer_from_thread(trans->cpu_id);
            ASSERT(seq || trans->io_device || !trans->may_stall);
            
            if (seq) { 
                trans->set_sequencer(seq);
                if (!trans->ifetch && g_conf_spin_check_interval &&
                        g_conf_spinloop_threshold) 
                    seq->get_spin_heuristic()->observe_mem_trans(trans, &v9_trans->s);
                mem_hier->make_request((conf_object_t *)trans->mem_hier_seq, trans);
            } else if (trans->io_device) {
                mem_hier->make_request(trans->ini_ptr, trans);
            } else if (!trans->may_stall) {
                // Can't send this down to mem_hier as we do not know
                // which cache should respond
                ASSERT(paused[trans->cpu_id]);
                trans->completed = true;
            }
        }
		// If this was a hit, complete_request was called already, but has no
		// effect because the transaction isn't stalling yet (this function
		// hasn't returned) so be done with it
		if (trans->call_complete_request) {
			ASSERT(trans->completed || trans->may_stall || g_conf_copy_transaction);

			if (trans->completed) goto done;
			else return max_stall;

		} else {
            trans->completed = 1;
            trans->clear_pending_event(PROC_REQUEST);
			goto done;
		}
		
	} else {
		// Old transaction found, delete if completed, otherwise stall
        if (trans->completed || g_conf_copy_transaction) {
			goto done;
		} else {
			fprintf(stderr, "@ %12llu Reissued Transaction 0x%016llx id: %d cpu %u\n",
					external::get_current_cycle(), trans->phys_addr, trans->id, trans->cpu_id);
			ASSERT_MSG(0, "Trans not completed after 100000 cycles");
		}
	}
	
	return max_stall; 

done:
	//if (pending_checkpoint && trans->ifetch) return 1000000000;
	if (g_conf_check_silent_stores) check_silent_stores(trans, mem_op);
    return 0;
    
stall_indefinitely:
    return 1000000000;
    
}	
	
cycles_t 
mem_driver_t::snoop_operate(conf_object_t *proc_obj, conf_object_t *space, 
		map_list_t *map, generic_transaction_t *mem_op)
{
	v9_memory_transaction_t *v9_trans = (v9_memory_transaction_t *) mem_op;

	if (g_conf_read_warmup_trace || g_conf_write_warmup_trace)
		return warmup_trace->snoop_operate(proc_obj, mem_op);
        
	// To indicate proper CPU or some I/O device
	uint8 proc_id = SIM_mem_op_is_from_cpu((&v9_trans->s)) ?
				SIM_get_proc_no(v9_trans->s.ini_ptr) : SIM_number_processors();
	mem_trans_t *trans;
	trans = (transactions[proc_id])[v9_trans->s.id];
	
	ASSERT(trans);
	
	// Check if load values were correct
	if (g_conf_cache_data) {
		if (trans->read && trans->data && !trans->sw_prefetch) {
			uint8 *databuf = new uint8[trans->size];
			SIM_c_get_mem_op_value_buf(&v9_trans->s, (char *)databuf);
			
			// Output data
			VERBOSE_OUT(verb_t::data, "%10s @ %12llu 0x%016llx: recieved\n",
				"mem-driver", mem_hier->get_g_cycles(), trans->phys_addr);
			VERBOSE_DATA("mem-driver", mem_hier->get_g_cycles(),
				trans->phys_addr, trans->size, trans->data);
			
			VERBOSE_OUT(verb_t::data, "%10s @ %12llu 0x%016llx: simics\n",
				"mem-driver", mem_hier->get_g_cycles(), trans->phys_addr);
			VERBOSE_DATA("mem-driver", mem_hier->get_g_cycles(),
				trans->phys_addr, trans->size, databuf);
						 
			// Compare data
			int retval = memcmp(trans->data, databuf, trans->size);
			//ASSERT_MSG(retval == 0, "Simics/mem-hier data mismatch");
			ASSERT_WARN(retval == 0);
			if (g_conf_break_data_mismatch && retval != 0) {
				SIM_break_cycle(get_cpu(), 0);
			}
			delete [] databuf;
		}
		else if (trans->read) {
			ASSERT(trans->io_device || trans->is_io_space() ||
			       trans->sw_prefetch ||
				   // FIXME: hack to identify flush
				   SIM_get_mem_op_type(&v9_trans->s) == Sim_Trans_Cache
				   );
		}
			
	}

	transactions[proc_id].erase(trans->id);
    trans->pending_messages--;
    trans->completed = true;
	return 0;
}
	
void
mem_driver_t::complete_request(conf_object_t *obj, 
	conf_object_t *cpu, mem_trans_t *trans)
{
    if (trans->vcpu_state_transfer) {
		trans->sequencer->complete_switch_request(trans);
		trans->clear_pending_event(PROC_REQUEST);
		return;
	}
	
    if (g_conf_paused_vcpu_interrupt)
        inspect_interrupt();
	
	ASSERT(trans->completed);
    trans->clear_pending_event(PROC_REQUEST);
    if (trans->io_device || !paused[SIM_get_proc_no(trans->ini_ptr)]) 
        SIM_release_stall(trans->ini_ptr, trans->id);
    else {
        cached_trans_id[SIM_get_proc_no(trans->ini_ptr)] = trans->id;
    }
    //if (trans->ifetch && SIM_get_proc_no(trans->ini_ptr) == 10)
    //    DEBUG_OUT("vcpu10 : PC %llx\n", trans->virt_addr);
}	

void 
mem_driver_t::invalidate_address (conf_object_t *obj, conf_object_t *cpu, 
		invalidate_addr_t *invalid_addr)
{ }

void
mem_driver_t::set_cpus(conf_object_t **cpus, uint32 num_cpus, uint32 _num_threads)
{
    num_threads = _num_threads;
	mem_hier->set_cpus(cpus, num_cpus);
    paused = new bool[num_threads];
    cached_trans_id = new int[num_threads];
    paused_time = new tick_t[num_threads];
    raise_early_interrupt = new bool[num_threads];
    bootstrap_count = num_threads;
    for (uint32 i = 0; i < num_threads; i++) {
        paused[i] = (proc->chip->get_sequencer_from_thread(i) == NULL);
        cached_trans_id[i] = -1;
        raise_early_interrupt[i] = true;
        
        // This is experimental code to handle interrupts early 
        //if (g_conf_no_os_pause && g_conf_paused_vcpu_interrupt) {
            sparc_v9_interface_t *v9_interface;
            conf_object_t *cpu = proc->chip->get_mai_object(i)->get_cpu_obj();
            v9_interface = (sparc_v9_interface_t *) 
		    SIM_get_interface(cpu,SPARC_V9_INTERFACE);
            
            v9_interface->install_user_class_asi_handler(cpu, static_handle_asi, 
            0x77); 
           
        //}
        paused_time[i] = 0;
    }
}

conf_object_t *
mem_driver_t::get_cpu()
{
	return mem_hier->get_cpu_object(0);
}

void
mem_driver_t::set_runtime_config(string config)
{
	runtime_config_file = config; 
	mem_hier->read_runtime_config_file(config);
}

string
mem_driver_t::get_runtime_config()
{
	return runtime_config_file; 
}

void
mem_driver_t::clear_stats()
{
    mem_hier->clear_stats();
}

void
mem_driver_t::stall_mem_hier()
{
	pending_checkpoint = true;
    advance_memhier_cycles = true;
}

bool
mem_driver_t::is_mem_hier_quiet()
{
    return (mem_hier->quiet_and_ready());
}

void
mem_driver_t::print_stats()
{
	mem_hier->print_stats();
    set<addr_t>::iterator sit;
    for (sit = cross_call_handler.begin(); sit != cross_call_handler.end(); sit++)
        DEBUG_LOG("handler : %llx\n", *sit);
}

mem_hier_t * mem_driver_t::get_mem_hier()
{
    return mem_hier;
}

void mem_driver_t::release_proc_stall(uint32 t_id)
{
    if (cached_trans_id[t_id] >= 0) 
        SIM_release_stall(SIM_proc_no_2_ptr(t_id), cached_trans_id[t_id]);
    cached_trans_id[t_id] = -1;
    paused[t_id] = false;
    raise_early_interrupt[t_id] = false;
    mem_hier->reset_spin_commit(t_id);
}

void mem_driver_t::stall_thread(uint32 id)
{
    paused[id] = true;
    paused_time[id] = mem_hier->get_g_cycles();
    raise_early_interrupt[id] = true;
}

void mem_driver_t::leak_transactions()
{
    tick_t curr_cycle = mem_hier->get_g_cycles();
    for (uint32 i = 0; i < proc->chip->get_num_cpus(); i++)
    {
        if (paused[i] && cached_trans_id[i] >= 0 && 
            curr_cycle - paused_time[i] >= (tick_t)g_conf_transaction_leak_interval)
            SIM_release_stall(SIM_proc_no_2_ptr(i), cached_trans_id[i]);
    }
}

void mem_driver_t::inspect_interrupt()
{
    for (uint32 i = 0; i < num_threads; i++)
    {
        if (paused[i] && raise_early_interrupt[i])
        {
            attr_value_t val = SIM_get_attribute(SIM_proc_no_2_ptr(i), "pending_interrupt");
            ASSERT(val.kind != Sim_Val_Invalid);
            if (val.u.integer > 0 &&
                (!g_conf_software_interrupt_only || val.u.integer == 0x60)) {
                //DEBUG_OUT("Raising Interrupt on paused vcpu%u %llx @ %llu\n", i,
                //    val.u.integer, proc->chip->get_g_cycles());
                proc->chip->set_interrupt(i, val.u.integer);
                raise_early_interrupt[i] = false;
            }
        }
    }
}

void mem_driver_t::handle_synchronous_interrupt(uint32 src, uint32 target)
{
    if (paused[target] && raise_early_interrupt[target])
    {
        // DEBUG_OUT("From %u target is %u @ %llu\n", src, target, 
        //         proc->chip->get_g_cycles()); 
        proc->chip->handle_synchronous_interrupt(src, target);
        raise_early_interrupt[target] = false;
    }
}

cycles_t mem_driver_t::bootstrap_os_pause(conf_object_t *proc_obj, 
    generic_transaction_t *mem_op)
{
    cycles_t ret = 0;
    v9_memory_transaction_t *v9_trans = (v9_memory_transaction_t *) mem_op;
    uint8 proc_id = SIM_mem_op_is_from_cpu(&(v9_trans->s))?
				SIM_get_proc_no(v9_trans->s.ini_ptr) : SIM_number_processors();

	
    if (SIM_mem_op_is_from_cpu(&(v9_trans->s))) {
        attr_value_t val = SIM_get_attribute(mem_op->ini_ptr, "pending_interrupt");
        if (SIM_cpu_privilege_level(mem_op->ini_ptr) == 0 && val.u.integer == 0)
        {
            ret = 1000000000;
            bootstrap_count--;
            cached_trans_id[proc_id] = mem_op->id;
            if (bootstrap_count == 0) {
                bootstrap = false;
                DEBUG_OUT("Everyone back to USERLAND!!!\n");
                SIM_release_stall(SIM_proc_no_2_ptr(0), cached_trans_id[0]);
                cached_trans_id[0] = -1;
                if (proc_id == 0)
                    ret = 0;
            }
        }
    }
    if (!ret) {
        mem_trans_t *trans = mem_hier->get_mem_trans();
        trans->copy(v9_trans);
        trans->completed = true;
        (transactions[proc_id])[trans->id] = trans;
        trans->pending_messages++;
    }
    if (bootstrap) inspect_bootstrap_interrupt();
		
    return ret;
                
}

void mem_driver_t::inspect_bootstrap_interrupt()
{
    for (uint32 i = 0; i < num_threads; i++)
    {
        conf_object_t *cpu = SIM_proc_no_2_ptr(i);
        attr_value_t val = SIM_get_attribute(cpu, "pending_interrupt");
        ASSERT(val.kind != Sim_Val_Invalid);
        if (val.u.integer > 0 && SIM_stalled_until(cpu)) {
            //DEBUG_OUT("Interrupt on paused cpu%u %llx\n", i, val.u.integer);
            SIM_release_stall(cpu, cached_trans_id[i]);
            cached_trans_id[i] = -1;
            bootstrap_count++;
        }
    }
}

bool mem_driver_t::is_paused(uint32 id)
{
    return paused[id];
}

bool mem_driver_t::is_bootstrap()
{
    return bootstrap;
}

void mem_driver_t::check_silent_stores(mem_trans_t *trans,
	generic_transaction_t *mem_op)
{
    /*
	if (g_conf_check_silent_stores && trans->write && !trans->io_device && 
		!trans->is_io_space())
	{
		
		if (trans->size <= 8) {
			uint64 trans_data = SIM_get_mem_op_value_cpu(mem_op);
			uint64 mem_data = SIM_read_phys_memory(trans->ini_ptr,
				trans->phys_addr, trans->size);
            
			if (trans_data == mem_data) {
				// silent store
				//DEBUG_OUT("%02d silent: 0x%016llx 0x%016llx\n", trans->cpu_id, trans_data, mem_data); 
			} else {
				//DEBUG_OUT("%02d not si: 0x%016llx 0x%016llx\n", trans->cpu_id, trans_data, mem_data); 
				sequencer_t *seq = proc->chip->get_sequencer_from_thread(trans->cpu_id);
				//if (seq)
				//	seq->ino_non_silent_store(trans->phys_addr, trans->cpu_id);
			}
            
		}				
		else {	
			// Copy write data to trans
			// uint8 data[trans->size];
			// SIM_c_get_mem_op_value_buf(&v9_trans->s, (char *)data);  
			
			// Assume non-silent for now
			//sequencer_t *seq = proc->chip->get_sequencer_from_thread(trans->cpu_id);
			//if (seq)
			//	seq->ino_non_silent_store(trans->phys_addr, trans->cpu_id);
		}
	}
    */
}
