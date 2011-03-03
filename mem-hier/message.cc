/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    generic messsage
 * initial author: Philip Wells 
 *
 */
 
 
//  // #include "simics/first.h"
// // RCSID("$Id: message.cc,v 1.2.10.1 2005/08/01 16:15:34 pwells Exp $");

#include "definitions.h"
#include "config_extern.h"
#include "message.h"
#include "external.h"

message_t::message_t(mem_trans_t *_trans, addr_t _addr, uint32 _size,
	const uint8 *_data)
	: size(_size), address(_addr), io_type(IONone), trans(_trans),
	bank_insert_time(0)
{
	if (g_conf_cache_data && _data) {
		data = new uint8[size];
		memcpy(data, _data, size);
	} else data = NULL;
}

message_t::message_t(message_t &msg) 
	: size(msg.size), address(msg.address), io_type(msg.io_type),
	trans(msg.trans), bank_insert_time(msg.bank_insert_time)
{
	if (g_conf_cache_data && msg.data) {
		data = new uint8[size];
		memcpy(data, msg.data, size);
	} else data = NULL;
}

// Virtual destructor to delete child class as well
message_t::~message_t()
{
	if (g_conf_cache_data && data) delete [] data;
}

msg_class_t
message_t::get_msg_class()
{
	return InvMsgClass;
}

mem_trans_t *
message_t::get_trans()
{
	return trans;
}

void
message_t::bank_insert()
{
	bank_insert_time = external::get_current_cycle();
}

tick_t
message_t::get_bank_latency()
{
	return external::get_current_cycle() - bank_insert_time;
}

