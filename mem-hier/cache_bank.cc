/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    generic cache/mem bank class
 *                 regulates request bandwidth
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: cache_bank.cc,v 1.1.2.10 2006/02/10 21:08:53 pwells Exp $");

#include "definitions.h"
#include "config.h"
#include "device.h"
#include "cache.h"
#include "window.h"
#include "cache_bank.h"
#include "external.h"
#include "event.h"
#include "stats.h"
#include "counter.h"
#include "histogram.h"
#include "mem_hier.h"


cache_bank_t::cache_bank_t(cache_t *_cache, uint32 _cycles_per_request,
	uint32 request_queue_size, uint32 wait_queue_size, uint32 _bank_id)
{
	cache = _cache;
	request_next = 0;
	cycles_per_request = _cycles_per_request;
	bank_id = _bank_id;
	
	next_request_event = new _event_t(new tick_t(0), this, 0,
		static_event_handler); 
	
	request_queue = new list<message_t *>;
	replay_queue = new list<message_t *>;
	wait_list = new list<message_t *>;

	max_request_queue = request_queue_size;
	max_replay_queue = wait_queue_size;
	max_wait_list = wait_queue_size;
}

stall_status_t
cache_bank_t::message_arrival(message_t *message)
{
	bool only = request_queue->empty() && replay_queue->empty();

	VERBOSE_OUT(verb_t::banks, 
		"    bank%02d @ %12llu 0x%016llx: message_arrival %d %d %d\n",
		bank_id, external::get_current_cycle(), message->address,
		request_queue->size(), replay_queue->size(), wait_list->size());
	
	if (max_request_queue && request_queue->size() == max_request_queue)
		return StallPoll;

	request_queue->push_back(message);
	message->bank_insert();

	if (!only)
		return StallNone;

	if (request_next <= external::get_current_cycle()) {
		process_message();
		return StallNone;
	}
	else {
		tick_t delay = request_next - external::get_current_cycle();
		ASSERT(delay > 0);
		ASSERT(!next_request_event->is_enqueued());
		next_request_event->requeue(delay);
		
		return StallNone;
	}
}

void
cache_bank_t::wakeup_all()
{
	wakeup(0, true);
}

void
cache_bank_t::wakeup_set(addr_t cache_index)
{
	wakeup(cache_index, false);
}

void
cache_bank_t::wakeup(addr_t cache_index, bool all)
{
	if (wait_list->empty()) return;

	VERBOSE_OUT(verb_t::banks, 
		"    bank%02d @ %12llu 0x%016llx: wakeup %s replay_q %d %d %d\n",
		bank_id, external::get_current_cycle(), cache_index, all ? "all" : "addr",
		request_queue->size(), replay_queue->size(), wait_list->size());

	bool only = request_queue->empty() && replay_queue->empty();
	bool replayed = false;
	
	list<message_t *>::iterator itr = wait_list->begin();
	list<message_t *>::iterator cur;

	// insert all msgs matching set_addr (or all) into replay queue
	while (itr != wait_list->end()) {
		cur = itr++;
		message_t *msg = *cur;
		if (all || cache->get_index(msg->address) == cache_index) {
			replayed = true;
			if (!max_replay_queue || replay_queue->size() < max_replay_queue) {
				wait_list->erase(cur);
				replay_queue->push_back(msg);
			}
			// else replay queue will wakup again when it becomes non-full
		}
	}
	
	if (!only) return;
	if (!replayed) return;
	
	tick_t delay;
	if (request_next <= external::get_current_cycle())
		delay = 1;  // next cycle
	else
		delay = request_next - external::get_current_cycle();
	ASSERT(delay > 0);
	ASSERT(!next_request_event->is_enqueued());
	next_request_event->requeue(delay);
}

void
cache_bank_t::process_message()
{
	stall_status_t ret;
	message_t *message;
	
	bool need_wakeup = false;
	// Priority to replayed messages
	if (!replay_queue->empty()) {
		
		// Wakeup again if wakeup could have blocked by full replay queue
		if (max_replay_queue && replay_queue->size() == max_replay_queue)
			need_wakeup = true;

		message = replay_queue->front();

		VERBOSE_OUT(verb_t::banks, 
			"    bank%02d @ %12llu 0x%016llx: process_msg replay_q %d %d %d\n",
			bank_id, external::get_current_cycle(), message->address,
			request_queue->size(), replay_queue->size(), wait_list->size());
			
		ret = cache->process_message(message);

		// must be after cache->process_message()
		replay_queue->pop_front();

		if (ret == StallSetEvent || ret == StallOtherEvent) {
			ASSERT(!max_wait_list || wait_list->size() < max_wait_list);
			wait_list->push_back(message);
		}
		else if (ret == StallPoll) {
			ASSERT(!max_replay_queue || replay_queue->size() < max_replay_queue);
			replay_queue->push_back(message);
			need_wakeup = false; // no room now
		}
		else {
			// processed by cache
			ASSERT(ret == StallNone);
			delete message;
		}
	}
	else {
		ASSERT(!request_queue->empty());
		message = request_queue->front();

		VERBOSE_OUT(verb_t::banks, 
			"    bank%02d @ %12llu 0x%016llx: process_mesg request_q %d %d %d\n",
			bank_id, external::get_current_cycle(), message->address,
			request_queue->size(), replay_queue->size(), wait_list->size());
			
		if (!mem_hier_t::is_thread_state(message->address))
			cache->stat_bank_q_lat_histo->inc_total(1, message->get_bank_latency());
		
		ret = cache->process_message(message);
		
		if (ret == StallNone) {
			request_queue->pop_front();
			delete message;
		}
		else if ((ret == StallSetEvent || ret == StallOtherEvent)
			&& (!max_wait_list || wait_list->size() < max_wait_list))
		{
			request_queue->pop_front();
			wait_list->push_back(message);
		}
		else if (ret == StallPoll && 
			(!max_replay_queue || replay_queue->size() < max_replay_queue))
		{
			request_queue->pop_front();
			replay_queue->push_back(message);
		}
		else {
			// In order to allow finite queues and avoid deadlock for some
			// protocols, we need separate request/response queues and separate
			// virtual channels in the network.  If someone cares they can
			// implement...
			FAIL;

			// have to leave on request queue, others full
			ASSERT(ret != StallNone);
		}
	}
	
	if (!replay_queue->empty() || !request_queue->empty()) {
		ASSERT(!next_request_event->is_enqueued());
		next_request_event->requeue(cycles_per_request);
	}
	
	ASSERT(request_next <= external::get_current_cycle());
	request_next = external::get_current_cycle() + cycles_per_request;
	
	if (need_wakeup) wakeup_all();
}

bool
cache_bank_t::is_quiet()
{
    return (replay_queue->empty() && request_queue->empty() && wait_list->empty());
}

void
cache_bank_t::static_event_handler(_event_t *e)
{
	cache_bank_t *bank = static_cast<cache_bank_t *> (e->get_context());
	bank->process_message();
}
