/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: proc.cc,v 1.4.2.9 2006/03/02 23:58:42 pwells Exp $
 *
 * description:    processor device of memory hierarchy
 * initial author: Philip Wells 
 *
 */
 
#include "definitions.h"
#include "protocols.h"
#include "mem_hier.h"
#include "transaction.h"
#include "external.h"
#include "debugio.h"
#include "verbose_level.h"
#include "sms.h"

template <class prot_sm_t, class msg_t>
proc_t<prot_sm_t, msg_t>::proc_t(string name, conf_object_t *_cpu)
	: generic_proc_t(name, 4), cpu(_cpu)  // Four links
{ 
    sms = new sms_t();
}

template <class prot_sm_t, class msg_t>
mem_return_t
proc_t<prot_sm_t, msg_t>::make_request(conf_object_t *cpu, mem_trans_t *trans)
{
	VERBOSE_OUT(verb_t::requests, 
		"%10s @ %12llu 0x%016llx: proc_t::make_request()\n", 
		get_cname(), external::get_current_cycle(), trans->phys_addr);

	ASSERT(this->cpu == cpu);
			  
	// If not cacheable, return
	if (g_conf_ignore_uncacheable && !trans->is_cacheable()) {
		VERBOSE_OUT(verb_t::requests, 
			"%10s @ %12llu 0x%016llx: proc_t::make_request uncacheable\n", 
			get_cname(),external::get_current_cycle(), trans->phys_addr);
			   
		return MemComplete;
	}

	type_t type;
	if (trans->hw_prefetch && (trans->write || trans->atomic)) {
		type = msg_t::StorePref; 
	}
	else if (trans->read) {
		if (g_conf_single_trans_atomics && trans->atomic)
			type = msg_t::ProcWrite;
		else
			type = msg_t::ProcRead;
	} 
	else if (trans->write) {
		type = msg_t::ProcWrite;
	}
	else {
		ASSERT(trans->control);
		type = msg_t::ProcRead; // Bogus assignment
	}
	

	uint8 *data = NULL;
	
	// Get store data from transaction
	if (g_conf_cache_data) {
		if (trans->write) {
			ASSERT(trans->data);
			data = trans->data;
		}
	}
	
	// Data copied by message constructor
	msg_t *message = new msg_t(trans, trans->phys_addr, trans->size,
		data, type);

	uint32 link = (trans->ifetch) ? icache_link : dcache_link;
	
	if (g_conf_sep_kern_cache) {
		if (trans->ifetch)
			link = trans->kernel() ? k_icache_link : icache_link;
		else
			link = trans->kernel() ? k_dcache_link : dcache_link;
	}
	
	// Send message down L1 Cache link
	VERBOSE_OUT(verb_t::requests, 
		"%10s @ %12llu 0x%016llx: proc_t::make_request() attempt to send to %s\n", 
		get_cname(), external::get_current_cycle(), trans->phys_addr,
		(link == icache_link) ? "ICache" :
		(link == k_icache_link) ? "K-ICache" :
		(link == dcache_link) ? "DCache" : "K-DCache");

	ASSERT_MSG(link != icache_link || links[icache_link],
		"Received Ifetch w/o link to send it");
	ASSERT(links[dcache_link]);

	trans->pending_messages++;
	stall_status_t ret = links[link]->send(message);

	if (ret != StallNone) {
		VERBOSE_OUT(verb_t::requests, 
			"%10s @ %12llu 0x%016llx: proc_t::make_request() MemStall\n", 
			get_cname(),external::get_current_cycle(), trans->phys_addr);
		delete message;  // Need to delete because no one else is responsible
		return MemStall;
	}
	ASSERT(ret == StallNone);
    
    //sms->record_access(trans);
	return MemMiss;
}

template <class prot_sm_t, class msg_t>
stall_status_t
proc_t<prot_sm_t, msg_t>::message_arrival(message_t *message)
{
	msg_t *msg = static_cast <msg_t *> (message);
	
	mem_trans_t *trans = msg->get_trans();
	
	ASSERT(trans->pending_messages);
	
	//send Invalidate to processor if receive invalidate to same addr as a speculative ld/st
	//msg_class_t mclass = msg->get_msg_class();
	
	if (msg->get_type() == msg_t::ProcInvalidate) {
		invalidate_addr_t * inv_msg = new invalidate_addr_t;
		inv_msg->address = msg->address;    //invalidated address
		inv_msg->size = msg->size;
		mem_hier_t::ptr()->invalidate_address(cpu, inv_msg);
        trans->pending_messages--;
		delete inv_msg;
		delete msg;
		return StallNone;
	}
	
	ASSERT(msg->get_type() == msg_t::DataResp ||
	       msg->get_type() == msg_t::ReqComplete);
	
	trans->clear_pending_event(MEM_EVENT_L1_HIT | MEM_EVENT_L2_HIT);
    ASSERT(!trans->cache_prefetch || trans->ifetch);
    ASSERT(!trans->completed);
	trans->completed = true;
	
	// Copy read data to transaction
	if (g_conf_cache_data && trans->read) {
		ASSERT(msg->data);
		ASSERT(trans->data == NULL);
		ASSERT(msg->size == trans->size);
		trans->data = new uint8[trans->size]; 
		memcpy(trans->data, msg->data, trans->size);
	}

	mem_hier_t::ptr()->complete_request(cpu, trans);
	trans->pending_messages--;
	
	delete msg;
	return StallNone;
}

template <class prot_sm_t, class msg_t>
void
proc_t<prot_sm_t, msg_t>::to_file(FILE *file)
{
	// Output class name
	fprintf(file, "%s\n", typeid(this).name());
}

template <class prot_sm_t, class msg_t>
void
proc_t<prot_sm_t, msg_t>::from_file(FILE *file)
{
    char classname[g_conf_max_classname_len];
    bool condor_checkpoint = mem_hier_t::ptr()->is_condor_checkpoint();
    if (!condor_checkpoint) 
        mem_hier_t::ptr()->cleanup_previous_stats(file, typeid(this).name());
	
	fscanf(file, "%s\n", classname);
    
    if (strcmp(classname, typeid(this).name()) != 0)
        cout << "Read " << classname << " expecting " << get_name() << endl;
	ASSERT(strcmp(classname, typeid(this).name()) == 0);
}

template <class prot_sm_t, class msg_t>
bool
proc_t<prot_sm_t, msg_t>::is_quiet()
{
	// Proc doesn't currently know about any outstanding transactions
	return true;
}
