/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mainmem.cc,v 1.6.2.7 2006/02/24 16:28:54 kchak Exp $
 *
 * description:    implements the main memory
 * initial author: Philip Wells 
 *
 */
 
#include "definitions.h"
#include "protocols.h"
#include "device.h"
#include "link.h"
#include "message.h"
#include "event.h"
#include "debugio.h"
#include "verbose_level.h"
#include "transaction.h"
#include "config_extern.h"
#include "stats.h"
#include "counter.h"
#include "histogram.h"

template <class prot_sm_t, class msg_t>
const uint8 main_mem_t<prot_sm_t, msg_t>::cache_link;  
template <class prot_sm_t, class msg_t>
const uint8 main_mem_t<prot_sm_t, msg_t>::io_link;  

template <class prot_sm_t, class msg_t>
main_mem_t<prot_sm_t, msg_t>::main_mem_t(string name, uint32 num_links)
	: device_t(name, num_links)
{ 
	// processor cycles
	latency = g_conf_randomize_mm ? 
		g_conf_main_mem_latency - g_conf_random_mm_dist/2 :
		g_conf_main_mem_latency;

	num_requests = 0;
	stats = new stats_t();
	stats_print_enqueued = false;
	stat_requests = stats->COUNTER_BASIC("m_mem requests", "# of requests made to main mem");
	stat_memlatency_histo = stats->HISTO_EXP2("m_mem latency", "distribution of main mem latencies",15,1,1);
}

template <class prot_sm_t, class msg_t>
stall_status_t
main_mem_t<prot_sm_t, msg_t>::message_arrival(message_t *message)
{
	msg_t *msg = static_cast <msg_t *> (message);

	VERBOSE_OUT(verb_t::mainmem,
	            "%10s @ %12llu 0x%016llx: mainmem message_arrival() %s\n",
				get_cname(), external::get_current_cycle(), 
				msg->address, msg_t::names[msg->get_type()][0].c_str());
	
	ASSERT(msg->get_type() == msg_t::Read ||
	       msg->get_type() == msg_t::ReadEx || 
		   msg->get_type() == msg_t::WriteBack || msg->get_type() == msg_t::DataResp);

    if (msg->get_type() == msg_t::DataResp) {
        delete msg;
        return StallNone;
    }


	// TODO: contention and banks
	tick_t lat = get_latency(message);
	if (g_conf_randomize_mm) {
		VERBOSE_OUT(verb_t::mainmem,
	            "%10s @ %12llu 0x%016llx: mainmem latency: %d\n",
				get_cname(), external::get_current_cycle(), 
				msg->address, lat);
	}
	
	_event_t *e = new _event_t(msg, this, lat, event_handler);
	e->enqueue();
	num_requests++;
	STAT_INC(stat_requests);	

	lMap[msg->address]=external::get_current_cycle();

	return StallNone;
}

template <class prot_sm_t, class msg_t>
void
main_mem_t<prot_sm_t, msg_t>::event_handler(_event_t *e)
{
	msg_t *msg = e->get_data();
	main_mem_t *mem = e->get_context();
	delete e;
	
	ASSERT(mem->num_requests > 0);
	mem->num_requests--;
	

	mem->inc_address(msg->address);
	mem->inc_address(msg->address);

	if (msg->get_type() == msg_t::WriteBack) {
		if (g_conf_cache_data) {
			mem->write_memory(msg->address, msg->size, msg->data);
		}


		
		// TEMP: if hooked directly to; should fix to better mechanism 
		if (msg->need_writeback_ack()) {
			VERBOSE_OUT(verb_t::mainmem,
	            		"%10s @ %12llu 0x%016llx: mainmem sending writeback ack\n",
						mem->get_cname(), external::get_current_cycle(), 
						msg->address);
			
			msg->set_type(msg_t::WBAck);

			uint32 link = (msg->io_type == IONone) ? cache_link : io_link;
			stall_status_t ret = mem->links[link]->send(msg);
			ASSERT(ret == StallNone);
		} else {
            		msg->get_trans()->clear_pending_event(MEM_EVENT_MAINMEM_WRITE_BACK);
            		msg->get_trans()->pending_messages--;

			delete msg;
		}

	} else {
		
		// Main memory doesn't distinguish between read and read exclusive requests
		ASSERT(msg->data == NULL);
		if (g_conf_cache_data) {
			msg->data = new uint8[msg->size];
			mem->read_memory(msg->address, msg->size, msg->data);
		}
		
		VERBOSE_OUT(verb_t::mainmem,
	            	"%10s @ %12llu 0x%016llx: mainmem sending data reponse\n",
					mem->get_cname(), external::get_current_cycle(), 
					msg->address);

		msg->set_type(msg_t::DataResp);
		//msg->sender_id = 0;
		uint32 link = (msg->io_type == IONone) ? cache_link : io_link;
		stall_status_t ret = mem->links[link]->send(msg);
		ASSERT(ret == StallNone);
	}
}

template <class prot_sm_t, class msg_t>
void
main_mem_t<prot_sm_t, msg_t>::to_file(FILE *file)
{
	// Output class name
	fprintf(file, "%s\n", typeid(this).name());
}

template <class prot_sm_t, class msg_t>
void
main_mem_t<prot_sm_t, msg_t>::from_file(FILE *file)
{
	// Input and check class name
    bool condor_checkpoint = mem_hier_t::ptr()->is_condor_checkpoint();
    if (!condor_checkpoint) 
        mem_hier_t::ptr()->cleanup_previous_stats(file, typeid(this).name());
	char classname[g_conf_max_classname_len];
	fscanf(file, "%s\n", classname);

    if (strcmp(classname, typeid(this).name()) != 0)
        DEBUG_OUT("Read %s   Require %s file_pos %d\n",classname, 
            typeid(this).name(), ftell(file));
	ASSERT(strcmp(classname, typeid(this).name()) == 0);
}

template <class prot_sm_t, class msg_t>
bool
main_mem_t<prot_sm_t, msg_t>::is_quiet()
{
	return (num_requests == 0);
}

template <class prot_sm_t, class msg_t>
void
main_mem_t<prot_sm_t, msg_t>::write_memory(addr_t addr, size_t size, uint8 *data)
{
	VERBOSE_OUT(verb_t::data,
	            "%10s @ %12llu 0x%016llx: mainmem writing %d bytes\n",
				get_cname(), external::get_current_cycle(), 
				addr, size);
	external::write_to_memory(addr, size, data);
	VERBOSE_DATA(get_cname(), external::get_current_cycle(), 
				addr, size, data);

}

// Reads memory, stores into data array
// assumes data points to an array large enough to hold size bytes
template <class prot_sm_t, class msg_t>
void
main_mem_t<prot_sm_t, msg_t>::read_memory(addr_t addr, size_t size, uint8 *data)
{
	VERBOSE_OUT(verb_t::data,
	            "%10s @ %12llu 0x%016llx: mainmem reading %d bytes\n",
				get_cname(), external::get_current_cycle(), 
				addr, size);
	external::read_from_memory(addr, size, data);
	VERBOSE_DATA(get_cname(), external::get_current_cycle(), 
				addr, size, data);


}

template <class prot_sm_t, class msg_t>
tick_t
main_mem_t<prot_sm_t, msg_t>::get_latency(message_t *message)
{
//	if (g_conf_cache_vcpu_state && message->get_trans()->vcpu_state_transfer)
//		return 1;
	
	return (g_conf_randomize_mm) ?
		(latency + random() % g_conf_random_mm_dist) :
		latency;
}

template <class prot_sm_t, class msg_t>
void
main_mem_t<prot_sm_t, msg_t>::inc_address(addr_t addr){
	ASSERT(external::get_current_cycle() > lMap[addr]);	
	stat_memlatency_histo->inc_total(1, external::get_current_cycle() - lMap[addr]);
	lMap.erase(addr);
	
}
