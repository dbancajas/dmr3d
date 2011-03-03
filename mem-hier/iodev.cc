/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: iodev.cc,v 1.2.10.1 2005/03/10 17:46:31 pwells Exp $
 *
 * description:    io_dev device of memory hierarchy
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

template <class prot_sm_t, class msg_t>
io_dev_t<prot_sm_t, msg_t>::io_dev_t(string name, uint32 _num_sharers)
	: generic_io_dev_t(name, 2), // 2 links
	  num_sharers(_num_sharers),
	  outstanding(NULL), state(IOStateNone)
{ }

template <class prot_sm_t, class msg_t>
mem_return_t
io_dev_t<prot_sm_t, msg_t>::make_request(mem_trans_t *trans)
{
	VERBOSE_OUT(verb_t::io, 
			  "%10s @ %12llu 0x%016llx: io_dev_t::make_request()\n", 
			  get_cname(), external::get_current_cycle(), trans->phys_addr);

	if (state != IOStateNone) {
		ASSERT(outstanding);
		VERBOSE_OUT(verb_t::io, 
					"%10s @ %12llu 0x%016llx: io_dev_t::already outstanding request\n", 
					get_cname(), external::get_current_cycle(), trans->phys_addr);
		return MemStall;
	}
	
	ASSERT(!outstanding);

	if (trans->control) {
		WARNING;
		return MemStall;
	}

	type_t type;
	if (trans->read) type = msg_t::Read;
	else if (trans->write) type = msg_t::ReadEx;
	else FAIL;
	
	uint8 *data = NULL;
	// Get store data from transaction
	if (g_conf_cache_data) {
		if (trans->write) {
			ASSERT(trans->data);
			data = trans->data;
		}
	}
	
	// Data copied by message constructor
	msg_t *message = new msg_t(trans, trans->phys_addr, trans->size, data, type);
	message->io_type = IOAccessBegin;
	
	// Send message down link
	VERBOSE_OUT(verb_t::io, 
			  "%10s @ %12llu 0x%016llx: io_dev_t::make_request() attempt to send\n", 
			  get_cname(), external::get_current_cycle(), trans->phys_addr);

	ASSERT(links[0]);

	stall_status_t ret = links[cache_link]->send(message);

	if (ret != StallNone) {
		VERBOSE_OUT(verb_t::io, 
				  "%10s @ %12llu 0x%016llx: io_dev_t::make_request() MemStall\n", 
				  get_cname(),external::get_current_cycle(), trans->phys_addr);
		delete message;  // Need to delete because no one else is responsible
		return MemStall;
	}

	ASSERT(ret == StallNone);
	pending_flush_acks = num_sharers;
	outstanding = trans;  // Set current outstanding trans
	state = IOStateFlushWait;

	return MemMiss;

}

template <class prot_sm_t, class msg_t>
stall_status_t
io_dev_t<prot_sm_t, msg_t>::message_arrival(message_t *message)
{
	msg_t *msg = static_cast <msg_t *> (message);
	
	// Ignore non IO messages and messages from self
	if (msg->io_type == IONone) return StallNone;
	
	ASSERT(outstanding);
	ASSERT(msg->address >= outstanding->phys_addr && 
		   msg->address + msg->size <= outstanding->phys_addr + outstanding->size);

	if (state == IOStateFlushWait) {
		ASSERT(msg->io_type == IOAccessOK);
		pending_flush_acks--;

		if (pending_flush_acks == 0) {

			bool read = outstanding->read;
			
			uint8 *data = NULL;
			// Get store data from transaction
			if (g_conf_cache_data) {
				if (!read) {
					ASSERT(outstanding->data);
					data = outstanding->data;
				}
			}
	
			type_t type = read ? msg_t::Read : msg_t::WriteBack;
			
			// Data copied by message constructor
			msg_t *message = new msg_t(message->get_trans(),
				outstanding->phys_addr, outstanding->size, data, type);
			message->io_type = read ? IOAccessRead : IOAccessWrite;
			message->need_wb_ack = true;

			// Send message down link
			VERBOSE_OUT(verb_t::io, 
				"%10s @ %12llu 0x%016llx: io_dev_t:: sending to main mem\n", 
				get_cname(), external::get_current_cycle(),
				outstanding->phys_addr);

			ASSERT(links[mainmem_link]);
			
			stall_status_t ret = links[mainmem_link]->send(message);
			ASSERT(ret == StallNone);

			state = read ? IOStateReadWait : IOStateWriteWait;
		}

	} else if (state == IOStateReadWait) {
		ASSERT(msg->io_type = IOAccessRead);
		ASSERT(msg->type == msg_t::DataResp);

		// Copy read data to transaction
		if (g_conf_cache_data && outstanding->read) {
			ASSERT(msg->data);
			ASSERT(outstanding->data == NULL);
			ASSERT(msg->size == outstanding->size);
			outstanding->data = new uint8[outstanding->size]; 
			memcpy(outstanding->data, msg->data, outstanding->size);
		}

		// Send IOAccessDone
		type_t type = outstanding->read ? msg_t::Read : msg_t::ReadEx;
		msg_t *message = new msg_t(message->get_trans(), outstanding->phys_addr,
			outstanding->size, NULL, type);
		message->io_type = IOAccessDone;
		
		// Send message down link
		VERBOSE_OUT(verb_t::io, 
					"%10s @ %12llu 0x%016llx: io_dev_t:: sending AccessDone\n", 
					get_cname(), external::get_current_cycle(),
					outstanding->phys_addr);
		
		ASSERT(links[mainmem_link]);
		stall_status_t ret = links[cache_link]->send(message);
		ASSERT(ret == StallNone);
			
		// Complete transaction
		mem_hier_t::ptr()->complete_request(outstanding->ini_ptr, outstanding);
		outstanding = NULL;
		state = IOStateNone;
			   
	} else if (state == IOStateWriteWait) {
		ASSERT(msg->io_type = IOAccessWrite);
		ASSERT(msg->type == msg_t::WBAck);

		// Send IOAccessDone
		type_t type = outstanding->read ? msg_t::Read : msg_t::ReadEx;
		msg_t *message = new msg_t(message->get_trans(), outstanding->phys_addr,
			outstanding->size, NULL, type);
		message->io_type = IOAccessDone;
		
		// Send message down link
		VERBOSE_OUT(verb_t::io, 
			"%10s @ %12llu 0x%016llx: io_dev_t:: sending AccessDone\n", 
			get_cname(), external::get_current_cycle(),
			outstanding->phys_addr);
		
		ASSERT(links[mainmem_link]);
		stall_status_t ret = links[cache_link]->send(message);
		ASSERT(ret == StallNone);
		
		// Complete transaction
		mem_hier_t::ptr()->complete_request(outstanding->ini_ptr, outstanding);
		outstanding = NULL;
		state = IOStateNone;
		
	} else FAIL;

	return StallNone;
}

template <class prot_sm_t, class msg_t>
void
io_dev_t<prot_sm_t, msg_t>::to_file(FILE *file)
{
	// Output class name
	fprintf(file, "%s\n", typeid(this).name());
}

template <class prot_sm_t, class msg_t>
void
io_dev_t<prot_sm_t, msg_t>::from_file(FILE *file)
{
	// Input and check class name
}

template <class prot_sm_t, class msg_t>
bool
io_dev_t<prot_sm_t, msg_t>::is_quiet()
{
	// Proc doesn't currently know about any outstanding transactions
	return true;
}
