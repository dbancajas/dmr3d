/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: startup.cc,v 1.7.2.27 2006/12/12 18:36:58 pwells Exp $");

#include "definitions.h"
#include "mai.h"
#include "mai_instr.h"
#include "isa.h"
#include "fu.h"
#include "dynamic.h"
#include "window.h"
#include "iwindow.h"
#include "sequencer.h"
#include "shb.h"
#include "chip.h"
#include "stats.h"
#include "proc_stats.h"
#include "fu.h"
#include "eventq.h"
#include "v9_mem_xaction.h"
#include "mem_xaction.h"
#include "lsq.h"
#include "st_buffer.h"
#include "mem_hier_iface.h"
#include "transaction.h"
#include "mem_hier_handle.h"
#include "startup.h"
#include "config_extern.h"
#include "config.h"
#include "mem_driver.h"
#include <signal.h>
#include "mmu.h"

static int mm_id     = -1;
//extern "C" { const char _module_date[] = "Thu Apr  9 10:51:56 MDT 2009"; }
//const char _module_date[];
//= "04/06/2009";
static proc_object_t *gl_proc;

static void
condor_vacate_handler(int sig)
{
	if (g_conf_use_processor) {
		gl_proc->stall_processor();
		gl_proc->get_mem_hier()->stall_mem_hier();
	}
	else {
		gl_proc->get_mem_driver()->stall_mem_hier();
	}
		
    gl_proc->pending_checkpoint = true;
    DEBUG_OUT("Got Signal for checkpointing\n");
}


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

static exception_type_t static_handle_crosscall_asi (conf_object_t *obj,
		generic_transaction_t *mem_op)
{
    exception_type_t ret = Sim_PE_No_Exception;
    uint32 src = get_cpu_from_mmu(obj);
    addr_t val = SIM_get_mem_op_value_cpu(mem_op);
       
    if (mem_op->logical_address == 0x40) {
         if (gl_proc->chip->identify_cross_call_handler(val))
            gl_proc->cross_call_ignore[src] = true;
            else {
                gl_proc->cross_call_ignore[src] = false;
                ret = Sim_PE_Default_Semantics;
               
            }
            gl_proc->chip->update_cross_call_handler(val);
            
    } else {
        ret = gl_proc->cross_call_ignore[src] ? ret : Sim_PE_Default_Semantics;
        if (mem_op->logical_address & 0x3fff == 0x70)
            gl_proc->cross_call_ignore[src] = false;
    }
    // if (ret == Sim_PE_Default_Semantics) 
    //     DEBUG_LOG("Allowing cross call with src : %u handler %llx\n", src, val);
    // else
    //     DEBUG_OUT("Ignoring cross call with src : %u handler %llx\n", src, val);
    return ret;
}

static exception_type_t static_handle_idsr (conf_object_t *obj,
		generic_transaction_t *mem_op)
{
    SIM_set_mem_op_value_cpu(mem_op, 0);
    return Sim_PE_No_Exception;
   // return Sim_PE_Default_Semantics;

}


void
proc_object_t::stall_processor()
{
	chip->stall_front_end();
}

mem_driver_t *
proc_object_t::get_mem_driver()
{
    return mem_driver;   
}
    
void
proc_object_t::set_mem_hier (mem_hier_handle_t *_mem_hier) {
	if (mem_hier && mem_hier != _mem_hier)
		delete mem_hier;

	mem_hier = _mem_hier;
	mem_driver = NULL;

	chip->set_mem_hier (mem_hier);
}

void
proc_object_t::set_mem_driver (mem_driver_t *_mem_driver) {
	if (mem_driver && mem_driver != _mem_driver)
		delete mem_driver;

	mem_hier = NULL;
	mem_driver = _mem_driver;
}
void
proc_object_t::get_stats () {
	string fname = g_conf_stats_log;

	if (!fname.empty ())
		debugio_t::open_log (fname.c_str (), g_conf_overwrite_stats);

	//if (g_conf_use_processor) {
		chip->print_stats ();
	//}
	if (g_conf_use_mem_hier) {
		if (g_conf_use_processor) mem_hier->print_stats ();
		else mem_driver->print_stats ();
	}

	if (!fname.empty ()) 
		debugio_t::close_log ();
}

/*
chip_t*
proc_object_t::search_chip (conf_object_t *cpu, bool &shadow) {
	for (uint32 i = 0; i < num_cpus; i++) {
		chip_t *chip = p [i];
		mai_t     *mai     = chip->get_mai_object ();

		if (mai->get_cpu_obj () == cpu) return chip;
		if (mai->get_shadow_cpu_obj () == cpu) {
			shadow = true;
			return chip;
		}
	}
	FAIL;
	return 0;
}
*/

mem_hier_handle_t*
proc_object_t::get_mem_hier (void) {
	return mem_hier;
}

void 
run_cycle (chip_t *chip) {
	chip->advance_cycle ();
    if (g_conf_stats_print_cycles &&
        chip->get_g_cycles() % g_conf_stats_print_cycles == 0)
    {
        gl_proc->get_stats();
    }
}

void proc_object_t::clear_stats()
{
    if (g_conf_use_processor) chip->get_mem_hier()->clear_stats();
    else mem_driver->clear_stats();
}

tick_t proc_object_t::get_chip_cycles()
{
    return chip->get_g_cycles();
}

void proc_object_t::do_finalize()
{
    
    if (g_conf_use_processor) {
        mem_hier_handle_t *mem_hier = new mem_hier_handle_t (NULL, NULL, this);
        set_mem_hier (mem_hier);
    } else {
        mem_driver_t *mem_driver = new mem_driver_t (this);
        set_mem_driver (mem_driver);
	
        //mem_driver->set_cpus (cpus, proc->num_cpus);
    }			

        
    if (!g_conf_use_processor) {
		attr_value_t one = SIM_make_attr_integer(1);
		SIM_set_attribute(SIM_get_object("sim"), "cpu_switch_time", &one);
		chip->set_mem_driver(mem_driver);
	}
	chip->init();
	
	// After configs read in and chip created
	conf_object_t **cpus = new conf_object_t * [chip->get_num_sequencers()];
	uint32 i;
	for (i = 0; i < chip->get_num_sequencers(); i++) {
		cpus [i] = (conf_object_t *) chip->get_sequencer(i);
	}
	if (g_conf_use_processor) 
        chip->get_mem_hier()->set_cpus (cpus, i);
    else
        mem_driver->set_cpus(cpus, i, chip->get_num_cpus());
	
	tl_reg = SIM_get_register_number(SIM_proc_no_2_ptr(0), "tl");
    if (g_conf_override_idsr_read) {
        sparc_v9_interface_t *v9_interface;
        for (uint32 i = 0; i < chip->get_num_cpus(); i++)
        {
            conf_object_t *cpu = chip->get_mai_object(i)->get_cpu_obj();
            v9_interface = (sparc_v9_interface_t *) 
		    SIM_get_interface(cpu,SPARC_V9_INTERFACE);
            v9_interface->install_user_class_asi_handler(cpu, static_handle_idsr, 
                0x48); 
            if (g_conf_use_processor)
                v9_interface->install_user_class_asi_handler(cpu, static_handle_crosscall_asi, 
                    0x77);
        }
    }
            
            
}


static void handle_write_checkpoint(conf_object_t *cpu)
{
    // Check if the Window is empty for each processor  
	if (!gl_proc->chip->ready_for_checkpoint())
		return;
    
	// Check if memory hierarchy is quiet
    mem_hier_handle_t *mem_hier_handle = gl_proc->get_mem_hier();
    if (mem_hier_handle->is_mem_hier_quiet())
    {
        // Take Checkpoint
        if (g_conf_processor_checkpoint_out != "") {
            FILE *file = fopen(g_conf_processor_checkpoint_out.c_str(), "w");
				gl_proc->chip->write_checkpoint(file);
            fclose(file);
        }
        gl_proc->raise_checkpoint_hap();
        SIM_break_cycle(cpu, 0);
    }
    
}


void proc_object_t::read_mem_hier_config(string c)
{
    if (!mem_hier_config_read) {
        config_t *config_db = new config_t ();
        config_db->register_entries ();
        config_db->parse_runtime (c);
        mem_hier_config_read = true;   
    }
        
        
        
}
void proc_object_t::raise_checkpoint_hap()
{
    hap_type_t hap = SIM_hap_get_number("MS2_checkpoint_on_exit");
    SIM_c_hap_occurred(hap, (conf_object_t *) this, 0);
    
}


static void handle_read_checkpoint()
{
    if (g_conf_processor_checkpoint_in != "") {
        FILE *fileHandle = fopen(g_conf_processor_checkpoint_in.c_str(), "r");
        if (fileHandle) {
			gl_proc->chip->read_checkpoint(fileHandle);
            fclose(fileHandle);
        }

        g_conf_processor_checkpoint_in = "";
    }
    
}


static void 
proc_event_handler (conf_object_t *cpu, chip_t *chip) {
	
    if (g_conf_processor_checkpoint_in != "") handle_read_checkpoint();
   
	// Run them all with CPU 0
	if (SIM_get_proc_no(cpu) == 0)
		run_cycle (chip);
	
	SIM_time_post_cycle (cpu, 1, Sim_Sync_Processor, 
	(event_handler_t) proc_event_handler, chip);
    
    if (gl_proc->pending_checkpoint) handle_write_checkpoint(cpu);
}




static void 
proc_interrupt_handler (void *obj, conf_object_t *cpu, int64 vector) {
	proc_object_t    *proc = (proc_object_t *) obj;

    uint32 cpu_id = SIM_get_proc_no((conf_object_t *)cpu);
		
	if (g_conf_use_processor) {
		bool is_shadow = false;
		ASSERT(cpu_id < proc->num_cpus);
		
		if (!is_shadow) {
			proc->chip->set_interrupt (cpu_id, vector);
		} else {
			//proc->chip->set_shadow_interrupt (cpu_id, vector);
		}
	} else {
        proc->chip->set_interrupt(cpu_id, vector);
    }
}

static void
proc_exception (void *obj, conf_object_t *cpu, int64 exception) {
	proc_object_t    *proc = (proc_object_t *) obj;

	if (!g_conf_use_processor) {
		uint32 cpu_id = SIM_get_proc_no(cpu);
		ASSERT(cpu_id < proc->num_cpus);
		
		proc->chip->get_mai_from_thread(cpu_id)->ino_setup_trap_context(exception);
        sequencer_t *seq = proc->chip->get_sequencer_from_thread(cpu_id);
        if (seq) seq->potential_thread_switch(0, YIELD_EXCEPTION);
	}
}

static void
proc_exception_return (void *obj, conf_object_t *cpu, int64 exception) {
	proc_object_t    *proc = (proc_object_t *) obj;

	if (!g_conf_use_processor) {
		uint32 cpu_id = SIM_get_proc_no(cpu);
		ASSERT(cpu_id < proc->num_cpus);
		
		proc->chip->get_mai_from_thread(cpu_id)->ino_finish_trap_context();
        sequencer_t *seq = proc->chip->get_sequencer_from_thread(cpu_id);
        if (seq) seq->potential_thread_switch(0, YIELD_DONE_RETRY);
	}
}

static void
proc_mode_change (void *obj, void *cpu, 
	int64 old_mode, int64 new_mode) {
    proc_object_t    *proc = (proc_object_t *) obj;
    uint32 cpu_id = SIM_get_proc_no((conf_object_t *)cpu);
    if (!g_conf_use_processor && g_conf_no_os_pause)
        proc->chip->handle_mode_change(cpu_id);
    sequencer_t *seq = proc->chip->get_sequencer_from_thread(cpu_id);
    if (seq) seq->get_spin_heuristic()->register_proc_mode_change();
}

static void
proc_control_reg_write (void *obj, conf_object_t *cpu, int64 reg_num, int64 new_value)
{
	proc_object_t    *proc = (proc_object_t *) obj;

	if (reg_num == proc->tl_reg) { // wrpr to tl
		if (!g_conf_use_processor) {
			uint32 cpu_id = SIM_get_proc_no(cpu);
			ASSERT(cpu_id < proc->num_cpus);
	
			if (new_value == 1)
				proc->chip->get_mai_from_thread(cpu_id)->
					ino_setup_trap_context(0, true, new_value);
			else if (new_value == 0)
				proc->chip->get_mai_from_thread(cpu_id)->ino_finish_trap_context();
			else {
//				WARNING;
				//DEBUG_OUT("write: cpu%d %llu %llx\n", cpu_id, reg_num, new_value);
			}
		}
	}	
}

static void
proc_magic_instruction (void *obj, conf_object_t *cpu, int64 magic_value)
{
	proc_object_t    *proc = (proc_object_t *) obj;
	proc->chip->magic_instruction(cpu, magic_value);
}


static conf_object_t*
proc_new_instance (parse_object_t *p) {
	proc_object_t    *proc;

	proc = MM_ZALLOC (1, proc_object_t);
    proc->cross_call_ignore = new bool[SIM_number_processors()];
    memset(proc->cross_call_ignore, 0, sizeof(bool) * SIM_number_processors());
	proc->timing_iface = MM_ZALLOC (1, timing_model_interface_t);
	proc->timing_iface->operate = mem_hier_handle_t::static_lsq_operate;

	proc->snoop_iface = MM_ZALLOC (1, timing_model_interface_t);
	proc->snoop_iface->operate = mem_hier_handle_t::static_snoop_operate;

	proc->proc_iface = MM_ZALLOC (1, proc_interface_t);
	proc->proc_iface->complete_request   = mem_hier_handle_t::static_complete_request;
	proc->proc_iface->invalidate_address = mem_hier_handle_t::static_invalidate_address;

	proc->mmu_interface = MM_ZALLOC (1, mmu_interface_t);
	proc->mmu_interface->logical_to_physical = mmu_t::static_do_access;
	proc->mmu_interface->get_current_context = mmu_t::static_current_context;
	proc->mmu_interface->cpu_reset = mmu_t::static_reset;
	proc->mmu_interface->undefined_asi = mmu_t::static_undefined_asi;
	proc->mmu_interface->set_error = mmu_t::static_set_error;
	proc->mmu_interface->set_error_info = mmu_t::static_set_error_info;
    
    proc->mem_hier_config_read = false;
	
	conf_class_t *conf_class = SIM_get_class ("ms2sim");
	ASSERT (conf_class);

	SIM_register_interface (conf_class, "timing-model", proc->timing_iface);
	SIM_register_interface (conf_class, "snoop-memory", proc->snoop_iface);
	SIM_register_interface (conf_class, "proc-iface", proc->proc_iface);
	SIM_register_interface (conf_class, "mmu", proc->mmu_interface);

	SIM_object_constructor ( (conf_object_t *) proc, p); 

	SIM_hap_add_callback("Core_Asynchronous_Trap", 	
		(obj_hap_func_t) proc_interrupt_handler, (void *) proc);

	FE_EXCEPTION_CHECK; 

	SIM_hap_add_callback ("Core_Exception", 
		(obj_hap_func_t) proc_exception, (void *) proc);

	FE_EXCEPTION_CHECK; 

	SIM_hap_add_callback ("Core_Exception_Return",
		(obj_hap_func_t) proc_exception_return, (void *) proc);

	FE_EXCEPTION_CHECK; 

	SIM_hap_add_callback ("Core_Mode_Change", 
		(obj_hap_func_t) proc_mode_change, (void *) proc);

	FE_EXCEPTION_CHECK; 

	SIM_hap_add_callback ("Core_Control_Register_Write", 
		(obj_hap_func_t) proc_control_reg_write, (void *) proc);

	FE_EXCEPTION_CHECK; 

	SIM_hap_add_callback ("Core_Magic_Instruction", 
		(obj_hap_func_t) proc_magic_instruction, (void *) proc);

    proc->pending_checkpoint = false;    
    gl_proc = proc;
    signal(SIGTSTP, condor_vacate_handler);
    
	FE_EXCEPTION_CHECK; 

	return (conf_object_t *) proc;
}

static int 
proc_delete_instance (conf_object_t *obj) {
	MM_FREE (obj);
	return 0;
}

/* CLI format: mp.set_param([str param_name, str param_value]) 
 * Note: for now it only works with INTEGER, STRING, and BOOL. 
 * XXX Such values will be set to all processors! XXX
 */
static set_error_t
set_config_value (void *dont_care, conf_object_t *obj, attr_value_t *val, 
		attr_value *idx) {
	proc_object_t *proc = (proc_object_t *) obj;

	if (idx->kind == Sim_Val_Nil) {
		if (val->kind != Sim_Val_List)
			return Sim_Set_Need_List;
		
		/* Two params required
		 * 	string param: the name of the config parameter;
		 * 	string value: the value for the config parameter.
		 */
		ASSERT (val->u.list.size == 2);
		if (val->u.list.vector [0].kind != Sim_Val_String)
			return Sim_Set_Illegal_Value;
		if (val->u.list.vector [1].kind != Sim_Val_String)
			return Sim_Set_Illegal_Value;
		const char *param = val->u.list.vector [0].u.string;
		const char *value = val->u.list.vector [1].u.string;

		/* get config_t ptr from the chip and set new value */
		config_t * cdb = proc->chip->get_config_db ();
		ASSERT (cdb != NULL);

		char reason [128];
		if (cdb->set_runtime_value (param, value, reason) == false) {
			ERROR_OUT ("Error setting config value: %s %s %s\n", param, reason, value);
			FAIL;
		}
	} else {
		return Sim_Set_Illegal_Value;
	}

	return Sim_Set_Ok;
}

/* get config value: idx is the name of config variable */
static attr_value_t
get_config_value (void *dont_care, conf_object_t *obj, attr_value_t *idx)  {
   	proc_object_t *proc = (proc_object_t *) obj;

	if (idx->kind == Sim_Val_String) {
	   	string conf_var (idx->u.string);

		config_t * cdb;
		string config_val;
		
		cdb = proc->chip->get_config_db ();
		config_val = cdb->get_value(conf_var);
		return SIM_make_attr_string(config_val.c_str());
   	}

	return SIM_make_attr_nil ();
}

/* print config: idx is the file name for output (empty string for stdout) */
static attr_value_t
print_config (void *dont_care, conf_object_t *obj, attr_value_t *idx)  {
   	proc_object_t *proc = (proc_object_t *) obj;

	if (idx->kind == Sim_Val_String) {
	   	string conf_log (idx->u.string);
	   	if (!conf_log.empty ())
		   	debugio_t::open_log (conf_log.c_str (), true);

		config_t * cdb;
	   	for (uint32 i = 0; i < proc->num_cpus; i++) {
		   	cdb = proc->chip->get_config_db ();
		   	cdb->print ();
	   	}

		if (!conf_log.empty ())
		   	debugio_t::close_log ();
   	}

	return SIM_make_attr_nil ();
}

static set_error_t
set_shadow_cpus (void *dont_care, 
	conf_object_t *obj, 
	attr_value_t *val, 
	attr_value_t *idx) {

#if 0
	proc_object_t *proc = (proc_object_t *) obj;

	for (uint32 i = 0; i < val->u.list.size; i++) {
		if (val->u.list.vector [i].kind != Sim_Val_String) 
			return Sim_Set_Illegal_Value;

		conf_object_t *shadow_cpu = SIM_get_object (val->u.list.vector [i].u.string);
		if (shadow_cpu && !SIM_object_is_processor (shadow_cpu)) 
			return Sim_Set_Illegal_Value;

		proc->chip->get_mai_object ()->set_shadow_cpu_obj (shadow_cpu);
//  	SIM_time_post_cycle (shadow_cpu, 0, Sim_Sync_Processor, 
//			(event_handler_t) proc_event_handler, proc->p [i]);
	}
#endif
	return Sim_Set_Ok;
}


static set_error_t
set_cpus (void *dont_care, conf_object_t *obj, attr_value_t *val, attr_value_t *idx) {
	conf_object_t *cpu  = 0;
	proc_object_t *proc = (proc_object_t *) obj;
	conf_object_t **cpus;
	string *configs;

	attr_value_t ms2;  // point to us for mmus
	ms2.kind = Sim_Val_Object;
	ms2.u.object = obj;
	
    
	// shadow
	if (dont_care) 
		return (set_shadow_cpus (dont_care, obj, val, idx));

	// master
	if (idx->kind == Sim_Val_Nil) {
		if (val->kind != Sim_Val_List)
			return Sim_Set_Need_List;

		ASSERT (val->u.list.size % 2 == 0);

		proc->num_cpus = val->u.list.size / 2;
		cpus = new conf_object_t * [proc->num_cpus];
		configs = new string[proc->num_cpus];
		
		for (uint32 i = 0, j = 0; i < proc->num_cpus; i++, j += 2) {

			if (val->u.list.vector [i].kind != Sim_Val_String)
				return Sim_Set_Illegal_Value;

			cpu = SIM_get_object (val->u.list.vector [j].u.string);
			if (!SIM_object_is_processor (cpu)) return Sim_Set_Illegal_Value;

			configs[i] = (val->u.list.vector [j + 1].u.string);

			cpus [i] = cpu;

            if (!g_conf_use_processor) SIM_stall_cycle(cpu, 0);
            
		}
		

		proc->chip = new chip_t(cpus, proc->num_cpus, configs[0]); // fix configs
		
		if (g_conf_use_processor) {
			if (SIM_get_proc_no(cpus[0]) == 0) {
				SIM_time_post_cycle (cpus[0], 0, Sim_Sync_Processor, 
				(event_handler_t) proc_event_handler, proc->chip);
			}
		} 
		
		
	} else {
		return Sim_Set_Illegal_Value;
	}

	return Sim_Set_Ok;
}



static attr_value_t
get_clear_stats (void *dont_care, conf_object_t *obj, attr_value_t *idx)
{
    proc_object_t *proc = (proc_object_t *) obj;
    proc->clear_stats();
    
    return SIM_make_attr_nil();
}

static set_error_t
set_clear_stats (void *dont_care, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
    proc_object_t *proc = (proc_object_t *) obj;
    proc->clear_stats();
    
    return Sim_Set_Ok;
}


static attr_value_t
get_finalize (void *dont_care, conf_object_t *obj, attr_value_t *idx)
{
    proc_object_t *proc = (proc_object_t *) obj;
    proc->do_finalize();
    
    return SIM_make_attr_nil();
}

static set_error_t
set_finalize (void *dont_care, conf_object_t *obj, attr_value_t *val, attr_value_t *idx)
{
    proc_object_t *proc = (proc_object_t *) obj;
    proc->do_finalize();
    
    return Sim_Set_Ok;
}

static attr_value_t
get_stats (void *dont_care, conf_object_t *obj, attr_value_t *idx)  {

	proc_object_t *proc = (proc_object_t *) obj;

	proc->get_stats ();

	return SIM_make_attr_nil ();
}

static set_error_t 
set_stats (void *dont_care, conf_object_t *obj, attr_value_t *val, attr_value_t *idx) {
	proc_object_t *proc = (proc_object_t *) obj;
	
	if (val->kind == Sim_Val_String) {
		string stats_log (val->u.string);
		g_conf_stats_log = stats_log;
	}

	proc->get_stats ();
	return Sim_Set_Ok;
}


static set_error_t
set_read_only (void *dont_care, conf_object_t *obj, attr_value_t *val, attr_value_t *idx) {
	return Sim_Set_Illegal_Value;
}

static attr_value_t
get_consistency_level (void *dont_care, conf_object_t *obj, attr_value_t *idx) {
	return SIM_make_attr_integer (10);
}

static attr_value_t
get_mem_hier (void *dont_care, conf_object_t *obj, attr_value_t *idx)  {
	// unimplemented
	return SIM_make_attr_nil ();
}

static attr_value_t
get_cpus (void *dont_care, conf_object_t *obj, attr_value_t *idx)  {
	// unimplemented
	return SIM_make_attr_nil ();
}

static set_error_t
set_mem_hier (void *dont_care, conf_object_t *obj, attr_value_t *val, attr_value_t *idx) {
	proc_object_t *proc = (proc_object_t *) obj;
	
	if (val->kind != Sim_Val_Object) 
		return Sim_Set_Need_Object;

	mem_hier_handle_t *mem_hier = new mem_hier_handle_t (
		val->u.object, 
		(mem_hier_interface_t *) (SIM_get_interface (val->u.object, "mem-hier")),
		proc);

	proc->set_mem_hier (mem_hier);

	return Sim_Set_Ok;
}

static attr_value_t
get_checkpoint_file(void *dont_care, conf_object_t *obj, attr_value_t *idx)
{
	attr_value_t file;
	proc_object_t *proc = (proc_object_t *) obj;

	file.kind = Sim_Val_String;
	file.u.string = proc->mem_hier->get_checkpoint_file().c_str();

	return file;
}

static set_error_t
set_checkpoint_file(void *dont_care, conf_object_t *proc_module, attr_value_t *val,
					attr_value_t *idx)
{
	proc_object_t *proc = (proc_object_t *) proc_module;

	if (val->kind != Sim_Val_String) 
		return Sim_Set_Need_String;

	proc->mem_hier->set_checkpoint_file(val->u.string);

	return Sim_Set_Ok;
}

static attr_value_t
get_mem_image(void *dont_care, conf_object_t *obj, attr_value_t *idx)
{
	attr_value_t mem_img;
	proc_object_t *proc = (proc_object_t *) obj;

	if (proc->mem_image) {
		mem_img.kind = Sim_Val_Object;
		mem_img.u.object = proc->mem_image;
	} else {
		mem_img.kind = Sim_Val_Nil;
	}

	return mem_img;
}

static set_error_t
set_mem_image(void *dont_care, conf_object_t *proc_module, attr_value_t *val,
	attr_value_t *idx)
{
	proc_object_t *proc = (proc_object_t *) proc_module;

	if (val->kind != Sim_Val_Object) 
		return Sim_Set_Need_Object;

	proc->mem_image = val->u.object;
	proc->mem_image_iface = (image_interface *) (SIM_get_interface (val->u.object, "image"));

	return Sim_Set_Ok;
}

static attr_value_t
get_mh_runtime_config(void *dont_care, conf_object_t *proc_module,
	attr_value_t *idx)
{
	proc_object_t *proc = (proc_object_t *) proc_module;

	attr_value_t file;
	file.kind = Sim_Val_String;
	if (g_conf_use_processor)
		file.u.string = proc->get_mem_hier()->get_runtime_config().c_str();
	else
		file.u.string = proc->get_mem_driver()->get_runtime_config().c_str();
	return file;
}

static set_error_t
set_mh_runtime_config(void *dont_care, conf_object_t *proc_module, attr_value_t *val,
	attr_value_t *idx)
{
    
    proc_object_t *proc = (proc_object_t *) proc_module;

	if (val->kind != Sim_Val_String) 
		return Sim_Set_Need_String;
    
    proc->read_mem_hier_config(val->u.string);
    
	return Sim_Set_Ok;
}

extern "C" {
	
void 
init_local () {
	class_data_t    class_data;
	conf_class_t    *conf_class;

	memset (&class_data, 0, sizeof (class_data_t));
	class_data.new_instance     = proc_new_instance;
	class_data.delete_instance  = proc_delete_instance;
	class_data.description      = "ms2sim simulator module";

	conf_class = SIM_register_class ("ms2sim", &class_data);

	SIM_register_attribute (conf_class, "param", 
			get_config_value, 0, set_config_value, 0, Sim_Attr_String_Indexed, 
		"Get/set runtime config value for all processors");

	SIM_register_attribute (conf_class, "print_config", 
			print_config, 0, NULL, 0, Sim_Attr_Pseudo, 
		"Print runtime config values");

	SIM_register_attribute (conf_class, "cpu", get_cpus, 0, set_cpus, 0, 
		(attr_attr_t) (Sim_Attr_Required | Sim_Attr_Integer_Indexed), 
		"CPUs");

	bool shadow = true;
	lang_void *ud = static_cast <lang_void *> (&shadow);
	SIM_register_attribute (conf_class, "shadow-cpu", get_cpus, 0, set_cpus, ud, 
		(attr_attr_t) (Sim_Attr_Required | Sim_Attr_Integer_Indexed), 
		"Shadow CPUs");

	SIM_register_attribute (conf_class, "mem-hier", 
		get_mem_hier, 0, set_mem_hier, 0, 
		Sim_Attr_Optional, "memory hierarchy");

	SIM_register_attribute (conf_class, "stats", 
		get_stats, 0, set_stats, 0, 
		Sim_Attr_Pseudo, "print stats");

    SIM_register_attribute (conf_class, "finalize",
        get_finalize, 0, set_finalize, 0, Sim_Attr_Pseudo, "finalize");
        
    SIM_register_attribute (conf_class, "clear_stats",
        get_clear_stats, 0, set_clear_stats, 0, Sim_Attr_Pseudo, "clear_stats");    
        
        
	SIM_register_attribute (conf_class, "consistency-level",
		get_consistency_level, 0, set_read_only, 0, Sim_Attr_Pseudo,
		"Consistency level of this device");

	// MH integration
	// TODO: change these
	SIM_register_attribute (conf_class, "mh-runtime-config", 
		get_mh_runtime_config, 0, set_mh_runtime_config, 0, 
		Sim_Attr_Optional, "mem-hier runtime config file");

	SIM_register_attribute (conf_class, "checkpoint-file", 
		get_checkpoint_file, 0, set_checkpoint_file, 0, 
		Sim_Attr_Optional, "checkpoint file");

	SIM_register_attribute (conf_class, "mem-image", 
		get_mem_image, 0, set_mem_image, 0, 
		Sim_Attr_Optional, "second mem-image for caching data");
}

void 
fini_local () {
}

}
