/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    unidirectional network 'links' between devices 
 * initial author: Philip Wells 
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: link.cc,v 1.2.10.12 2006/07/28 01:29:55 pwells Exp $");
 
#include "definitions.h"
#include "device.h"
#include "event.h"
#include "link.h"
#include "debugio.h"
#include "verbose_level.h"
#include "mem_hier.h"
#include "interconnect_msg.h"


link_t::link_t(device_t * _source, device_t * _dest)
	: source(_source), dest(_dest)
{ }

stall_status_t
link_t::send(message_t *message, tick_t ignore1, tick_t ignore2, bool ignore3)
{
	VERBOSE_OUT(verb_t::links, "link_t: send 0x%016llx\n", message->address);
	ASSERT(dest);
	return (dest->message_arrival(message));
}

bool
link_t::is_quiet()
{
	return true;
}

void
link_t::set_source(device_t *_source)
{
	source = _source;
}

device_t *
link_t::get_source()
{
	return source;
}

void
link_t::set_dest(device_t *_dest)
{
	dest = _dest;
}

device_t *
link_t::get_dest()
{
   return dest;
}


///////////////////////////////
// Buffered link

buffered_link_t::buffered_link_t(device_t * _source, device_t * _dest,
                                  tick_t _latency, uint32 _size)
	: link_t(_source, _dest), cur_size(0), max_size(_size), latency(_latency)
{ }

stall_status_t
buffered_link_t::send(message_t *message, tick_t ignore1, tick_t ignore2,
	bool ignore3)
{
	VERBOSE_OUT(verb_t::links, "bufferet_link send() 0x%016llx prevsize : %u\n", message->address, cur_size);

	// max_size == 0 is Infinite
	if (max_size && cur_size == max_size) return StallPoll;

	_event_t *e = new _event_t(message, this, latency, event_handler);

	e->enqueue();
	VERBOSE_OUT(verb_t::links, "buffered link send: 0x%016llx %d\n", e->get_data()->address, e->get_data()->size);
	cur_size++;
	return StallNone;
}

// Calls the message arrival function of device -- static fn
void
buffered_link_t::event_handler(_event_t *e)
{
	//VERBOSE_OUT(verb_t::links, "buffered_link event_handler()\n");
	stall_status_t ret;
	buffered_link_t *link = e->get_context();

	VERBOSE_OUT(verb_t::links, "buffered_link  handle: 0x%016llx %d\n", e->get_data()->address, e->get_data()->size);
	ret = link->dest->message_arrival(e->get_data());
	
	if (ret != StallNone) {
		e->requeue(link->delta_cycles);
	} else {
		ASSERT(ret == StallNone);
		link->cur_size--;
		delete e;
	}
}

bool
buffered_link_t::is_quiet()
{
	return (cur_size == 0);
}

uint32 buffered_link_t::get_latency() { return latency; }
uint32 buffered_link_t::get_size() { return cur_size; }

fifo_link_t::fifo_link_t(device_t * _source, device_t * _dest,
                         tick_t _latency, uint32 _size) : 
  buffered_link_t(_source, _dest, _latency, _size)
{
	next_transmit_event = new _event_t(NULL , this, 0,
		event_handler); 
}

stall_status_t
fifo_link_t::send(message_t *msg, tick_t lat, tick_t ignore2, bool ignore3)
{
    
    VERBOSE_OUT(verb_t::links, "fifo_link send() 0x%016llx prevsize : %u\n", msg->address, cur_size);

	// max_size == 0 is Infinite
	if (max_size && cur_size == max_size) return StallPoll;

    message_queue.push_back(msg);
    if (!lat) lat = latency;
    grant_cycle_list.push_back(external::get_current_cycle() + lat);
    if (!next_transmit_event->is_enqueued())
        next_transmit_event->requeue(lat);
    
	//VERBOSE_OUT(verb_t::links, "buffered link send: 0x%016llx %d\n", e->get_data()->address, e->get_data()->size);
	cur_size++;
	return StallNone;
}

void
fifo_link_t::transmit_message()
{
    if (!message_queue.size()) return;
    stall_status_t ret;
    message_t *msg = message_queue.front();
    tick_t curr_cycle = external::get_current_cycle();
    long delay = grant_cycle_list.front() - curr_cycle;
    while (delay <= 0) {
        //VERBOSE_OUT(verb_t::links, "link_t: sent 0x%016llx to %s ret %u\n", 
        //    msg->address, dest->get_cname(), ret);
        
        ret = dest->message_arrival(msg);
        
        if (ret == StallNone) {
            message_queue.pop_front();
            grant_cycle_list.pop_front();
			cur_size--;
        } 
        if (ret != StallNone || message_queue.size() == 0)
            break;
        msg = message_queue.front();
        delay = grant_cycle_list.front() - curr_cycle;
    }
    if (delay <= 0) 
        delay = delta_cycles;
    next_transmit_event->requeue(delay);
}


void
fifo_link_t::event_handler(_event_t *e)
{
    fifo_link_t *fifo_link = e->get_context();   
    fifo_link->transmit_message();
}

bandwidth_fifo_link_t::bandwidth_fifo_link_t(device_t * _source, device_t * _dest,
                         tick_t _latency, uint32 _size, tick_t _send_every) : 
  fifo_link_t(_source, _dest, _latency, _size)
{
	next_send_cycle = external::get_current_cycle();
	send_every = _send_every;
}

stall_status_t
bandwidth_fifo_link_t::send(message_t *msg, tick_t lat, tick_t send_cycles,
	bool cutthrough)
{
	// lat is min (i.e. uncontended), absolute (i.e. non-delta) latency
	// send_cycles is number of cycles this message occupies link
	// cutthrough causes a message to be "received" after the first cycle it 
	//   gets through the link rather than the last, though other messages
	//   don't get in until thw whole thing gets through (i.e. virtual cut through)

	tick_t cur_cycle = external::get_current_cycle();
	tick_t extra_lat;
    
	if (!lat) lat = latency;  // default link latency 
	if (!send_cycles) send_cycles = send_every;  // default cycles on link
	
	if (next_send_cycle <= cur_cycle) {
		extra_lat = cutthrough ? 0 : send_cycles - 1;
		next_send_cycle = cur_cycle + send_cycles;
	} else {
		extra_lat = next_send_cycle - cur_cycle;
		extra_lat += cutthrough ? 0 : send_cycles - 1;
		next_send_cycle += send_cycles;
	}
	
    mem_hier_t::ptr()->update_mem_link_latency(lat + extra_lat);
	VERBOSE_OUT(verb_t::links,
		"%10s @ %12llu 0x%016llx: bw_fifo_send: %llu extra %u cursize\n",
		" ", external::get_current_cycle(), msg->address, extra_lat, cur_size);

	return fifo_link_t::send(msg, lat + extra_lat);
}

// Keeps a unique fifo for every dest to the source to allow messages from 
// different sources to pass each other if they are blocked by the cache 
// -- the cache should probably implement this functionality in the long run
passing_fifo_link_t::passing_fifo_link_t(device_t * _source, device_t * _dest,
	tick_t _latency, uint32 _size, uint32 _num_source, uint32 _num_dest,
	device_t ** _dests) :
	link_t(_source, _dest), num_source(_num_source), num_dest(_num_dest)
{
	dests = new device_t * [num_dest];
	if (num_dest == 1) {
		dests[0] = _dest;
	} else {
		for (uint32 i = 0; i < num_dest; i++) 
			dests[i] = _dests[i];
	}
		
	fifos = new fifo_link_t * [num_source*num_dest];
	for (uint32 i = 0; i < num_source * num_dest; i++)
		fifos[i] = new fifo_link_t(_source, dests[i/num_source], _latency, _size);
}

stall_status_t
passing_fifo_link_t::send(message_t *msg, tick_t lat, tick_t send_cycles,
	bool cutthrough)
{
	uint32 source_id = (static_cast <interconnect_msg_t *> (msg))->get_source();
	ASSERT(source_id < num_source);
	uint32 dest_id = (static_cast <interconnect_msg_t *> (msg))->get_dest();
	dest_id = dest_id % num_dest;
	
	return fifos[source_id + num_source*dest_id]->send(msg, lat,
		send_cycles, cutthrough);
}


