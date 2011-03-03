/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: statemachine.h,v 1.3.4.3 2005/06/02 19:08:10 pwells Exp $
 *
 * description:    abstract state machine class
 * initial author: Philip Wells 
 *
 */
 
#ifndef _STATEMACHINE_H_
#define _STATEMACHINE_H_

#include "protocols.h"

// Sent to transition functions
template <typename action_t, typename state_t, class prot_line_t>
class transfn_info_t {
 public:
 	transfn_info_t(mem_trans_t *_trans, state_t _curstate, action_t _action, 
		addr_t _address, message_t *_message, prot_line_t *_line)
		: trans(_trans), curstate(_curstate), action(_action),
		address(_address), message(_message), line(_line)
	{ }

	mem_trans_t *trans; // initiating mem_trans
	state_t curstate;   // Current state of line
	action_t action;    // Action triggered
	addr_t address;     // affected address 
	message_t *message; // Message triggering action
	prot_line_t *line;  // affected cache line
};

template <typename state_t, class prot_line_t>
class transfn_return_t {
 public:
	transfn_return_t() : next_state((state_t) 0), block_message(StallNone)
	{ }
 
	state_t next_state;  // Next state to go to
	stall_status_t block_message;  // Block triggering message?
};

 
 
template <class prot_t>
class statemachine_t : public prot_t {

public:
	typedef typename prot_t::tfn_info_t   tfn_info_t;
	typedef typename prot_t::tfn_ret_t    tfn_ret_t;

	// Returns the action resulting from an incomming message
	virtual uint32 get_action(message_t *) = 0;

	// trigger the transition based on the passed info
	// Next state is returned in structure
	virtual void trigger(tfn_info_t *, tfn_ret_t *) = 0;

	// Record state transitions
	virtual void profile_transition(tfn_info_t *, tfn_ret_t *) = 0;
	virtual ~statemachine_t() {}
	
};

#endif // _STATEMACHINE_H_
