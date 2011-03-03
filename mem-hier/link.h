/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: link.h,v 1.1.1.1.10.7 2006/07/28 01:29:55 pwells Exp $
 *
 * description:    unidirectional network 'links' between devices
 * initial author: Philip Wells 
 *
 */
 
#ifndef _LINK_H_
#define _LINK_H_


//   This is a simple interface connecting two devices
//   with no additional functionality
class link_t {

 public:

	// Constructor
	link_t(device_t * _source = NULL, device_t * _dest = NULL);

	// Enqueue a message onto link
	//   deliver directly to destination
	virtual stall_status_t send(message_t *message, tick_t ignore1 = 0, 
		tick_t ignore2 = 0, bool ignore3 = false);
	
	// Is this link quiet?
	virtual bool is_quiet();
	
	void set_source(device_t *_source);
	device_t *get_source();

	void set_dest(device_t *_dest);
	device_t *get_dest();
	
	virtual ~link_t() {}


 protected:

	device_t *source;
	device_t *dest;
	
};


// network link with buffer (not FIFO)
class buffered_link_t : public link_t {
	
 public:
	typedef event_t<message_t, buffered_link_t> _event_t;
	
	// Constructor
	buffered_link_t(device_t * _source = NULL,
					device_t * _dest = NULL,
					tick_t _latency = 0,
					uint32 _size = 1);
	
	// Enqueue a network message and post a receive event 
	virtual stall_status_t send(message_t *message, tick_t ignore1 = 0, 
		tick_t ignore2 = 0, bool ignore3 = false);

	// Is this link quiet?
	virtual bool is_quiet();

	// Calls the message arrival function of device
	static void event_handler(_event_t *);
	
	uint32 get_latency();
	uint32 get_size();
	
	virtual ~buffered_link_t() {}
	
 protected:
	
	static const uint32 delta_cycles = 1;
	
	uint32 cur_size;                // current num messages
	uint32 max_size;                // max num messages (0 == Inf)
	tick_t latency;             // minimum link lantency
    
};

class fifo_link_t : public buffered_link_t {
    public:
    
        fifo_link_t(device_t * _source = NULL,
					device_t * _dest = NULL,
					tick_t _latency = 0,
					uint32 _size = 1);
        typedef event_t<message_t, fifo_link_t> _event_t;
        virtual stall_status_t send(message_t *message, tick_t latency = 0, 
			tick_t ignore2 = 0, bool ignore3 = false);
        static void event_handler(_event_t *);
        void transmit_message();
        virtual ~fifo_link_t() {}
        
    protected:
        list<message_t *> message_queue;
        list<tick_t>      grant_cycle_list; 
        _event_t          *next_transmit_event;
        
   
};

class bandwidth_fifo_link_t : public fifo_link_t {
public:

	bandwidth_fifo_link_t(device_t * _source = NULL,
		device_t * _dest = NULL,
		tick_t _latency = 0,
		uint32 _size = 0,
		tick_t send_every = 1);
		
	virtual stall_status_t send(message_t *message, tick_t latency = 0, 
		tick_t send_cycles = 0, bool cutthrough = false);
	
	virtual ~bandwidth_fifo_link_t() {}

protected:
	tick_t next_send_cycle;
	tick_t send_every;  // allow a message to be send every N cycles
	
};

class passing_fifo_link_t : public link_t {
public:

	passing_fifo_link_t(device_t * _source = NULL,
		device_t * _dest = NULL,
		tick_t _latency = 0,
		uint32 _size = 0,
		uint32 num_source = 1,
		uint32 num_dest = 1,
		device_t **_dests = NULL);
		
	virtual stall_status_t send(message_t *message, tick_t latency = 0, 
		tick_t send_cycles = 0, bool cutthrough = false);
	
	virtual ~passing_fifo_link_t() {}

protected:
	
	fifo_link_t **fifos;
	uint32 num_source;
	uint32 num_dest;
	device_t **dests;
};

#endif // _LINK_H_	
