/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cache_bank.h,v 1.1.2.3 2005/03/22 13:11:27 kchak Exp $
 *
 * description:    generic cache/mem bank class
 *                 regulates request bandwidth
 * initial author: Philip Wells 
 *
 */
 
#ifndef _BANK_H_
#define _BANK_H_


class cache_bank_t {
public:
	cache_bank_t(cache_t *_cache, uint32 _cycles_per_request,
		uint32 request_queue_size, uint32 wait_queue_size, uint32 bank_id);

	stall_status_t message_arrival(message_t *);

	void wakeup_all();
	void wakeup_set(addr_t cache_index);
    bool is_quiet();
private:	
	typedef event_t<tick_t, cache_bank_t> _event_t;

	void process_message();
	void wakeup(addr_t cache_index, bool all);

	static void static_event_handler(_event_t *e);

	list<message_t *> *request_queue;
	list<message_t *> *replay_queue;
	list<message_t *> *wait_list;
	uint32 max_request_queue;
	uint32 max_replay_queue;
	uint32 max_wait_list;
	
	_event_t *next_request_event;
	tick_t request_next;            // cycle bank available for next request
	
	tick_t cycles_per_request;
	cache_t *cache;
	uint32 bank_id;
};

#endif // _BANK_H_
