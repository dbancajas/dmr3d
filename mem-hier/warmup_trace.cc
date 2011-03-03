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
// RCSID("$Id: warmup_trace.cc,v 1.1.2.1 2006/12/12 18:36:58 pwells Exp $");
 
#include "definitions.h"
#include "transaction.h"
#include "profiles.h"
#include "mem_hier_handle.h"
#include "warmup_trace.h"
#include "verbose_level.h"
#include "profiles.h"
#include "mem_hier.h"
#include "sequencer.h"
#include "chip.h"
#include "mai.h"
#include "external.h"
#include "startup.h"
#include "config.h"
#include "config_extern.h"

warmup_trace_t::warmup_trace_t(mem_hier_t *_mem_hier, proc_object_t *_proc) :
    mem_hier(_mem_hier), proc(_proc)
{
	// reset handler to warup
	mem_hier->set_handler(this);
	num_transactions = 0;
	
	recent_trans = new deque<uint32> * [SIM_number_processors()];
	for (int i = 0; i < SIM_number_processors(); i++)
		recent_trans[i] = new deque<uint32>;
	
	// open trace file
	if (g_conf_read_warmup_trace) 
		trace_fp = fopen(g_conf_warmup_trace_file.c_str(), "r");
	else
		trace_fp = fopen(g_conf_warmup_trace_file.c_str(), "w");
	
	trace_fp_closed = false;

	ASSERT(trace_fp);
}


cycles_t 
warmup_trace_t::operate(conf_object_t *proc_obj, generic_transaction_t *mem_op)
{
	if (g_conf_read_warmup_trace) {
		// this will only be called once per processor per 1B cycles
		bool ret = send_next_trans();
		if (!ret) return 0;
		return 1000000000ULL;
	}

	if (num_transactions > g_conf_write_warmup_trace_num) {

		if (!trace_fp_closed) {
			fclose(trace_fp);
			SIM_break_simulation("Ready for checkpoint");
		}
		trace_fp_closed = true;
		return 0;
	}			
	
	ASSERT(g_conf_write_warmup_trace);

	// of 32, we take 1 bit for highest addr bit, 1 for r/w, 1 for ifetch
	ASSERT((uint64) g_conf_memory_image_size <= (1ULL << 33));  // 8GB
	ASSERT(g_conf_l1i_lsize >= (1<<3));
	ASSERT(g_conf_l1d_lsize >= (1<<3));

	if (!SIM_mem_op_is_from_cpu(mem_op))
		return 0;
	
	uint8 cpu_id = SIM_get_proc_no(mem_op->ini_ptr);
	bool ifetch = SIM_mem_op_is_instruction(mem_op);
	bool read = SIM_mem_op_is_read(mem_op) ? true : false;
	uint32 trace_output = (mem_op->physical_address >> 1) & 0xffffffffULL;  // To get up to 8G
	
	if (ifetch) {
		ASSERT(read);
		uint32 lsize_mask = g_conf_l1i_lsize-1;
		lsize_mask >>= 1; // Addr is already shifted
		trace_output = trace_output & (~lsize_mask);
	}
	else {
		uint32 lsize_mask = g_conf_l1d_lsize-1;
		lsize_mask >>= 1; // Addr is already shifted
		trace_output = trace_output & (~lsize_mask);
	}
	
	trace_output = trace_output |
		(read ? 1 : 0) |
		((ifetch ? 1 : 0) << 1);


	DEBUG_OUT("%02d 0x%llx %d %d %x\n",
		cpu_id, mem_op->physical_address, ifetch, read, trace_output);
	num_transactions++;
		
	// Avoid inserting if another recent trans of same type from this proc
	for (uint32 i = 0; i < recent_trans[cpu_id]->size(); i++) {
		if (trace_output == (*recent_trans[cpu_id])[i])
			return 0;
	}
	recent_trans[cpu_id]->push_front(trace_output);

	if (recent_trans[cpu_id]->size() > g_conf_write_warmup_trace_norepeat)
		recent_trans[cpu_id]->pop_back();


	// Write to trace file
	int ret;
	ret = fwrite(&cpu_id, sizeof(uint8), 1, trace_fp);
	ASSERT(ret == 1);

	ret = fwrite(&trace_output, sizeof(uint32), 1, trace_fp);
	ASSERT(ret == 1);

	return 0;
}		
		
		
bool
warmup_trace_t::send_next_trans() {


	uint32 trace_input;
	uint8 proc_id;
	
	int ret;
	ret = fread(&proc_id, sizeof(uint8), 1, trace_fp);
	if (feof(trace_fp)) {
		return false;
	}
	ASSERT(ret == 1);

	ret = fread(&trace_input, sizeof(uint32), 1, trace_fp);
	ASSERT(ret == 1);

	bool read = (trace_input & 0x1);
	bool ifetch = (trace_input & 0x2);

	addr_t physical_address = trace_input;
	physical_address <<= 1;

	addr_t lsize_mask;
	if (ifetch) lsize_mask = g_conf_l1i_lsize-1;
	else lsize_mask = g_conf_l1d_lsize-1;
	physical_address &= (~lsize_mask);

	// Create new mem hier transcation
	mem_trans_t *trans = mem_hier->get_mem_trans();
	trans->clear();
	
	trans->phys_addr = physical_address;
	trans->size = 64;
	trans->read = read;
	trans->write = !read;
	trans->ifetch = ifetch;
	trans->call_complete_request = true;
	trans->cache_phys = true;
	
	trans->ini_ptr = proc->chip->get_mai_from_thread(proc_id)->get_cpu_obj();

	sequencer_t *seq = proc->chip->get_sequencer_from_thread(proc_id);
	ASSERT(seq);
	trans->set_sequencer(seq);  // sets cpu_id too
	
	ASSERT(!g_conf_cache_data);
	
	trans->mark_pending(PROC_REQUEST);
	trans->pending_messages++;
	
	DEBUG_OUT("%02d 0x%llx %d %d %x\n",
		trans->cpu_id, trans->phys_addr, trans->ifetch, trans->read, 
		trace_input);

	mem_hier->make_request((conf_object_t *)trans->mem_hier_seq, trans);
		
	num_transactions++;

	return true;
}	
	
cycles_t 
warmup_trace_t::snoop_operate(conf_object_t *proc_obj,
	generic_transaction_t *mem_op)
{
	return 0;
}
	
void
warmup_trace_t::complete_request(conf_object_t *obj, 
	conf_object_t *cpu, mem_trans_t *trans)
{

	ASSERT(g_conf_read_warmup_trace);
	ASSERT(!g_conf_write_warmup_trace);

	ASSERT(trans->completed);
    trans->clear_pending_event(PROC_REQUEST);
	trans->pending_messages--;

	if (!send_next_trans()) {
		mem_hier->checkpoint_and_quit();
		for (int i = 0; i < SIM_number_processors(); i++)
			SIM_release_stall(SIM_proc_no_2_ptr(i), 0);
	}
}	

void 
warmup_trace_t::invalidate_address (conf_object_t *obj, conf_object_t *cpu, 
		invalidate_addr_t *invalid_addr)
{ }


