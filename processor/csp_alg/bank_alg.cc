/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id $
 *
 * description: Base Algorithm to work with num_threads > num_ctxt	
 * initial author: Koushik Chakraborty 
 *
*/

//  #include "simics/first.h"
// RCSID("$Id: bank_alg.cc,v 1.1.2.1 2006/07/28 01:30:08 pwells Exp $");

#include "definitions.h"
#include "isa.h"
#include "proc_stats.h"
#include "chip.h"
#include "stats.h"
#include "counter.h"
#include "mai_instr.h"
#include "sequencer.h"
#include "verbose_level.h"
#include "config_extern.h"
#include "histogram.h"
#include "mai.h"
#include "sequencer.h"
#include "checkp_util.h"
#include "thread_scheduler.h"
#include "csp_alg.h"
#include "bank_alg.h"
#include "mem_hier.h"

bank_algorithm_t::bank_algorithm_t(string name, hw_context_t **hwc,
   thread_scheduler_t *_t_sched, chip_t *_p, uint32 _num_thr, uint32 _num_ctxt) 
	: csp_algorithm_t(name, hwc, _t_sched, _p, _num_thr, _num_ctxt)
{
	ASSERT(g_conf_scheduling_algorithm == BANK_SCHEDULING_POLICY);
	
	// TODO: Only one thread is actually scheduled below though...
	ASSERT(num_threads <= num_ctxt);
	
	// For only one thread, put it in the middle of the caches on core 2
	if (num_threads == 1) {
		hw_context[2]->wait_list.push_back(p->get_mai_object(0));
		vcpu_2_pcpu[0] = 2;
		next_vcpu_2_pcpu[0] = 2;
		if (g_conf_l2_banks == num_ctxt)
			vcpu_2_currentbank[0] = 2;
		else if (g_conf_l2_banks < num_ctxt)
			vcpu_2_currentbank[0] = 2 * g_conf_l2_banks / num_ctxt;
		else
			vcpu_2_currentbank[0] = 2 * g_conf_l2_banks / num_ctxt;
	}		

	else {
		
		for (uint32 i = 0; i < num_threads; i++)
		{
			hw_context[i]->wait_list.push_back(p->get_mai_object(i));
			vcpu_2_pcpu[i] = i;
			next_vcpu_2_pcpu[i] = i;
			if (g_conf_l2_banks == num_ctxt)
				vcpu_2_currentbank[i] = i;
			else if (g_conf_l2_banks < num_ctxt) {
				ASSERT(num_ctxt % g_conf_l2_banks == 0);
				vcpu_2_currentbank[i] = i * g_conf_l2_banks / num_ctxt;
			} else {
				ASSERT(g_conf_l2_banks % num_ctxt == 0);
				vcpu_2_currentbank[i] = i * g_conf_l2_banks / num_ctxt;
			}
		}
	}
	
    for (uint32 i = 0; i < num_ctxt; i++)
    {
        schedule(i);
    }
	
	mp_scheduled_this_quantum = false;
}

void bank_algorithm_t::schedule(uint32 i)
{
	mai_t *mai = NULL;
	if (!hw_context[i]->wait_list.empty()) {
		mai = hw_context[i]->wait_list.front();
		hw_context[i]->wait_list.pop_front();
	}
	p->switch_to_thread(hw_context[i]->seq, hw_context[i]->ctxt, mai);
}


mai_t * bank_algorithm_t::find_thread_for_ctxt(hw_context_t *ctxt, 
	bool desire_user_ctxt)
{
	if (ctxt->wait_list.size() == 0) {
		// DEBUG_OUT("pcpu%d goes idle\n", ctxt->seq->get_id());
		return 0;
	}
	
	mai_t *mai = ctxt->wait_list.front();
	ctxt->wait_list.pop_front();
	uint32 tid = mai->get_id();
	ASSERT(mai);
	// DEBUG_OUT("pcpu%d will run vcpu%u @ %llu\n", ctxt->seq->get_id(),
	// 	mai->get_id(), p->get_g_cycles());
	
	ASSERT(vcpu_2_pcpu[tid] == ~0U);
	vcpu_2_pcpu[tid] = ctxt->seq->get_id();
	return mai;
}

hw_context_t *bank_algorithm_t::find_ctxt_for_thread(mai_t *thread, 
	bool desire_user_seq, uint32 syscall)
{
	uint32 tid = thread->get_id();
	uint32 hwc_id = vcpu_2_pcpu[tid];
	
	uint32 best_hwc_id = next_vcpu_2_pcpu[tid];
	
	if (num_threads == 1) {
		// Assume single VCPU for now, so no one else should be on other PCPU
		ASSERT(best_hwc_id != hwc_id);
		ASSERT(!hw_context[best_hwc_id]->seq->
				get_mai_object(hw_context[best_hwc_id]->ctxt));
		
		DEBUG_OUT("vcpu%u will run on pcpu%u @ %llu\n", thread->get_id(), 
			best_hwc_id, p->get_g_cycles());
	
		vcpu_2_pcpu[tid] = best_hwc_id;
		
		return hw_context[best_hwc_id];
	}
	else {
		
		// Yield shouldn't return true if not moving
		ASSERT(best_hwc_id != hwc_id);
		
		ASSERT(mp_scheduled_this_quantum > 0);
		mp_scheduled_this_quantum--;
		
		// If new ctxt is occupied, put on wait list		
		if (hw_context[best_hwc_id]->seq->
			get_mai_object(hw_context[best_hwc_id]->ctxt))
		{
			ASSERT(hw_context[best_hwc_id]->wait_list.empty());
			hw_context[best_hwc_id]->wait_list.push_back(p->get_mai_object(tid));
			
			DEBUG_OUT("    vcpu%u waiting for pcpu%u @ %llu\n", tid, 
				best_hwc_id, p->get_g_cycles());
	
			vcpu_2_pcpu[tid] = ~0U;
			return NULL;
		}
		else {

			DEBUG_OUT("    vcpu%u scheduled on pcpu%u @ %llu\n", tid, 
				best_hwc_id, p->get_g_cycles());
	
			vcpu_2_pcpu[tid] = best_hwc_id;
			return hw_context[best_hwc_id];
		}
	}
}


   
bool bank_algorithm_t::thread_yield(sequencer_t *seq, uint32 ctxt, mai_t *mai, 
	ts_yield_reason_t why)
{
	if (num_threads > 1) {
		if (mp_scheduled_this_quantum == 0 && 
			(why == YIELD_LONG_RUNNING || why == YIELD_EXTRA_LONG_RUNNING))
		{
			DEBUG_OUT("schedule init by pcpu%u, vcpu%d @ %llu\n", 
				seq->get_id(), mai->get_id(), p->get_g_cycles());
			schedule_mp_vcpus();
		}
		uint32 tid = mai->get_id();
		// We already scheduled this thread; return true until it migrates
		if (vcpu_2_pcpu[tid] != next_vcpu_2_pcpu[tid]) return true;
		else return false;
	}
	else {	
		uint32 tid = mai->get_id();
		// We already scheduled this thread; return true until it migrates
		if (vcpu_2_pcpu[tid] != next_vcpu_2_pcpu[tid]) return true; 

		return ((why == YIELD_LONG_RUNNING || why == YIELD_EXTRA_LONG_RUNNING) &&
			get_best_pcpu_unip(mai) != vcpu_2_pcpu[tid]);
	}
}
	
	   
void bank_algorithm_t::read_checkpoint(FILE *file)
{
	for (uint32 i = 0; i < num_ctxt; i++)
	{
		uint32 count;
		fscanf(file, "%u\n", &count);
		hw_context[i]->wait_list.clear();
		for (uint32 j = 0; j < count; j++)
		{
			uint32 id;
			fscanf(file, "%u\n", &id);
			hw_context[i]->wait_list.push_back(p->get_mai_object(id));
		}
		fscanf(file, "\n");
	}
	checkpoint_util_t *checkp = new checkpoint_util_t();
	checkp->map_uint32_uint32_from_file(vcpu_2_pcpu, file);
	checkp->map_uint32_uint32_from_file(next_vcpu_2_pcpu, file);
	checkp->map_uint32_uint32_from_file(vcpu_2_currentbank, file);
}
	
	   
void bank_algorithm_t::write_checkpoint(FILE *file)
{
	for (uint32 i = 0; i < num_ctxt; i++)
	{
		list<mai_t *>::iterator it = hw_context[i]->wait_list.begin();
		fprintf(file, "%u\n", hw_context[i]->wait_list.size());
		while (it != hw_context[i]->wait_list.end())
		{
			mai_t *candidate = *it;
			fprintf(file, "%u ", candidate->get_id());
			it++;
		}
		fprintf(file, "\n");
	}
	checkpoint_util_t *checkp = new checkpoint_util_t();
	checkp->map_uint32_uint32_to_file(vcpu_2_pcpu, file);
	checkp->map_uint32_uint32_to_file(next_vcpu_2_pcpu, file);
	checkp->map_uint32_uint32_to_file(vcpu_2_currentbank, file);
}
	
	
void bank_algorithm_t::silent_switch(sequencer_t *seq, uint32 ctxt, mai_t *mai)
{
}	
	
sequencer_t *bank_algorithm_t::handle_interrupt_thread(uint32 thread_id)
{
	return NULL;
}

sequencer_t *bank_algorithm_t::handle_synchronous_interrupt(uint32 src, uint32 tgt)
{
	return handle_interrupt_thread(tgt);
}

void bank_algorithm_t::disable_core(sequencer_t *seq)
{
	FAIL;
}

void bank_algorithm_t::enable_core(sequencer_t *seq)
{
	FAIL;
}

ts_yield_reason_t bank_algorithm_t::prioritize_yield_reason(ts_yield_reason_t new_val,
	   		ts_yield_reason_t old_val)
{
	return old_val;
}

uint32 bank_algorithm_t::get_best_pcpu_unip(mai_t *thread)
{
	uint32 tid = thread->get_id();
	uint64 *misses = mem_hier_t::ptr()->
		get_l1_misses_to_bank(tid);
	
	uint32 bank = 0;
	uint64 most_misses = 0;
	uint64 tot_misses = 0;
	for (uint32 i = 0; i < g_conf_l2_banks; i++) {
		if (misses[i] > most_misses) {
			bank = i;
			most_misses = misses[i];
			tot_misses += misses[i];
		}
	}
	
	uint32 cur_bank = vcpu_2_currentbank[tid];
	uint64 cur_misses = misses[cur_bank];
	
	// if (cur_bank != bank)
	// 	DEBUG_OUT("vcpu%u prefers bank %u by %llu\n", tid, bank,
	// 		most_misses - cur_misses);
	
	// Heuristic to periodically reset miss data
	// The idea here is that I want to reset more often when I still prefer my
	// current bank, to allow quick response to chaning behavior.  But I want to 
	// reset less frequently when I don't prefer my current bank but havn't yet
	// decided to switch, to allow the difference between banks to build up
	if (cur_bank == bank) {
		if (tot_misses > (uint64) g_conf_l2stripe_threshold * 2)
			mem_hier_t::ptr()->clear_l1_misses_to_bank(tid);
	} else {
		if (tot_misses > (uint64) g_conf_l2stripe_threshold * 10)
			mem_hier_t::ptr()->clear_l1_misses_to_bank(tid);
	}
	
	// Heuristic to refrain from switching until appropriate expected benefit 
	if (most_misses - cur_misses < (uint64) g_conf_l2stripe_threshold)
		return vcpu_2_pcpu[tid];
	
//	DEBUG_OUT("vcpu%u prefers bank %u\n", thread->get_id(), bank);

	// Always clear stats when we switch
	mem_hier_t::ptr()->clear_l1_misses_to_bank(tid);
	
	uint32 pcpu = get_nearest_pcpu(bank);
	
	next_vcpu_2_pcpu[tid] = pcpu;
	vcpu_2_currentbank[tid] = bank;
	
	
	return pcpu;
}

void bank_algorithm_t::schedule_mp_vcpus()
{
	// Schedule all VCPUs to the best banks
	
	// Heuristic is a greedy algorithm which chooses the thead with the biggest
	// preference for a given bank.
	
	// Preference for a VCPU is calculated as the number of misses to the bank 
	// under consideration
	// compared to the bank with the next-most-misses, that hasn't been scheduled 
	// with a VCPU yet this round.
	
	// No attempt is made to schedule VCPUs at neighboring banks to its preferred
	// bank
	
	ASSERT(mp_scheduled_this_quantum == 0);

	uint32 bank_sched[g_conf_l2_banks];
	bool tid_sched[num_threads];

	uint32 cores_per_bank = num_ctxt / g_conf_l2_banks;
	
	bzero(bank_sched, sizeof(uint32)*g_conf_l2_banks);
	bzero(tid_sched, sizeof(bool)*num_threads);

	for (uint32 sched = 0; sched < num_threads; sched++) {
	
		// Find VCPU with biggest different between threads
		uint64 most_misses_diff = 0;        // biggest diff so far
		uint32 most_misses_bank = 0;        // preferred bank
		uint32 most_misses_tid = 0;         // tid with biggest diff
			
		for (uint32 tid = 0; tid < num_threads; tid++) {
			if (tid_sched[tid]) continue;

			// For this thread, find diff of banks with most and second most
			// misses
			uint64 *misses = mem_hier_t::ptr()->get_l1_misses_to_bank(tid);

			uint64 tid_most_misses = 0;
			uint32 tid_most_misses_bank = 0;
			uint64 tid_second_misses = 0;

			for (uint32 b = 0; b < g_conf_l2_banks; b++) {
				if (bank_sched[b] >= cores_per_bank) continue;
				
				// Bigger diff?
				if (misses[b] >= tid_most_misses) {
					tid_second_misses = tid_most_misses;
					tid_most_misses = misses[b];
					tid_most_misses_bank = b;
				}
			}
			
			// Check if the diff for the most misses is best so far
			if ((tid_most_misses - tid_second_misses) >= most_misses_diff) {
				most_misses_diff = (tid_most_misses - tid_second_misses);
				most_misses_bank = tid_most_misses_bank;
				most_misses_tid = tid;
			}					
			
		}
		
		// Schedule it...
		DEBUG_OUT("vcpu%u prefers bank %u by %llu\n", most_misses_tid,
			most_misses_bank, most_misses_diff);
		
		uint32 best_pcpu = get_nearest_pcpu(most_misses_bank);
		
		// Put vcpu onto the next core for this bank 
		ASSERT(bank_sched[most_misses_bank] < cores_per_bank);
		best_pcpu += bank_sched[most_misses_bank];
		
		next_vcpu_2_pcpu[most_misses_tid] = best_pcpu;
		vcpu_2_currentbank[most_misses_tid] = most_misses_bank;
		
		if (next_vcpu_2_pcpu[most_misses_tid] != vcpu_2_pcpu[most_misses_tid])
			mp_scheduled_this_quantum++;
	
		bank_sched[most_misses_bank]++;
		tid_sched[most_misses_tid] = true;
		
	}
	
	
	// Optimization to make sure that VCPUs within the same bank cluster
	// can stay where they are if the still prefer the same bank
	
	// TODO currently only works for 2 cores per bank
	if (cores_per_bank == 2) {
		for (uint32 tid = 0; tid < num_threads; tid++) {
			if (vcpu_2_pcpu[tid] != next_vcpu_2_pcpu[tid] &&
				vcpu_2_pcpu[tid]/2 == next_vcpu_2_pcpu[tid]/2)
			{
				// Find other VCPU to swap with
				for (uint32 tid2 = 0; tid2 < num_threads; tid2++) {
					if (tid2 == tid) continue;
					if (next_vcpu_2_pcpu[tid]/2 == next_vcpu_2_pcpu[tid2]/2)
					{
						// Swap them
						DEBUG_OUT("swapping vcpu%u and vcpu%u\n", tid, tid2);
						uint32 temp_pcpu = next_vcpu_2_pcpu[tid2];
						next_vcpu_2_pcpu[tid2] = next_vcpu_2_pcpu[tid];
						next_vcpu_2_pcpu[tid] = temp_pcpu;
						
						mp_scheduled_this_quantum--;
						if (vcpu_2_pcpu[tid2] == next_vcpu_2_pcpu[tid2])
							mp_scheduled_this_quantum--;
					}
				}
			}
		}
	}

	// Clear misses
	for (uint32 tid = 0; tid < num_threads; tid++)
		mem_hier_t::ptr()->clear_l1_misses_to_bank(tid);
	
}


uint32 bank_algorithm_t::get_nearest_pcpu(uint32 bank) {
	
	uint32 pcpu;
	
	switch (g_conf_l2_banks) {
	case 4:
		pcpu = bank*2;
		break;
		
	case 8:
		pcpu = bank;
		break;
		
	case 16:
		pcpu = bank >> 1;
		break;
		
	default:
		FAIL;
	}

	return pcpu;
}	

