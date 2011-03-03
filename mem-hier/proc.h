/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: proc.h,v 1.1.1.1.10.3 2006/03/02 23:58:43 pwells Exp $
 *
 * description:    processor device of memory hierarchy
 * initial author: Philip Wells 
 *
 */
 
#ifndef _PROC_H_
#define _PROC_H_

// for polymorphism
class generic_proc_t : public device_t {
 public:
	generic_proc_t(string name, uint32 num_links) : device_t(name, num_links)
	{ }

	// Called to initiate a transaction
	virtual mem_return_t make_request(conf_object_t *cpu, mem_trans_t *trans) = 0;
};


// Virtual processor
template <class prot_sm_t, class msg_t>
class proc_t : public generic_proc_t {

 public:
	//typedef typename prot_sm_t::action_t action_t;
	typedef uint32 type_t;

	proc_t(string name, conf_object_t *_cpu);
	
	// Called to initiate a transaction
	mem_return_t make_request(conf_object_t *cpu, mem_trans_t *trans);

	// Called by network presumable to end a transaction
	stall_status_t message_arrival(message_t *message);

	void from_file(FILE *file);
	void to_file(FILE *file);
	bool is_quiet();

	// Links
	static const uint8 dcache_link = 0;  
	static const uint8 icache_link = 1;  
	static const uint8 k_dcache_link = 2;  
	static const uint8 k_icache_link = 3;  
	
 private:
	conf_object_t *cpu;  // CPU this proc is acting as a proxy for
    sms_t   *sms;

};

#include "proc.cc"

#endif // _PROC_H_
