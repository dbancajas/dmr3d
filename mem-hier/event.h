/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: event.h,v 1.2.10.2 2005/08/30 14:38:30 pwells Exp $
 *
 * description:    event functionality for mem-hier
 * initial author: Philip Wells 
 *
 */
 
#ifndef _EVENT_H_
#define _EVENT_H_

class generic_event_t {
	friend class meventq_t;
	
 public:
 	generic_event_t();
	
	void enqueue();
	
    virtual void do_event() = 0;
    
    void requeue(tick_t delta);
    tick_t get_delta_cycles();
    tick_t get_cycle();
    
    bool is_enqueued();
    void mark_dequeued();
    
    generic_event_t *tail;
    generic_event_t *next;
    virtual ~generic_event_t() {}

 protected:
	tick_t cycle;
	bool enqueued;
	
};


#include "external.h" 
#include "message.h"
#include "verbose_level.h"

template <class data_type, class context_type>
class event_t : public generic_event_t {
	
 public:
	event_t(data_type *_data, context_type *_context, tick_t delta,
			void (*_handler)(event_t<data_type, context_type> *))
		: generic_event_t(), data(_data), context(_context) {
		
		handler = _handler;
        cycle = delta;
    }
	
    void do_event() {
		VERBOSE_OUT(verb_t::events, "event_t::do_event()\n");
		mark_dequeued();
        handler(this);
    }
	
	data_type *get_data() { return data; }
	context_type *get_context() { return context; }

	
 protected:
    void (*handler)(event_t<data_type, context_type> *);
	data_type *data;
	context_type *context;
	
};

#endif // _EVENT_H_
