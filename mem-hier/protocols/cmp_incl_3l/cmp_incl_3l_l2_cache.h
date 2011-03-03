/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_incl_3l_l2_cache.h,v 1.1.2.4 2006/03/04 18:55:40 kchak Exp $
 *
 * description:    implementation of a CMP exclusive cache
 * initial author: Philip Wells 
 *
 */
 
#ifndef _CMP_INCL_3L_L2_CACHE_H_
#define _CMP_INCL_3L_L2_CACHE_H_

#include "cmp_incl_3l_l2.h"

////////////////////////////////////////////////////////////////////////////////
// CMP exclusive L3 cache line
class cmp_incl_3l_l2_line_t : public line_t, public cmp_incl_3l_l2_protocol_t {
friend class cmp_incl_3l_l2_cache_t;

public:
	/// Required by line_t
	void init(cache_t *cache, addr_t idx, uint32 lsize);
	void wb_init(addr_t tag, addr_t idx, line_t *cache_line);
	void reinit(addr_t _tag);
	
	bool is_free();
	bool can_replace();
	bool is_stable();
	
	void from_file(FILE *file);
	void to_file(FILE *file);


	/// Protocol specific
	void set_dirty(bool);
	bool is_dirty();
	void set_icache_incl(bool);
	bool get_icache_incl();
	void set_dcache_incl(bool);
	bool get_dcache_incl();

	uint32 get_state();
	void set_state(uint32 _state);

	void set_requester(uint32);
	uint32 get_requester();
	
protected:
	bool dirty;
	bool icache_incl;
	bool dcache_incl;
	uint32 state;
	
	// transient only; need not checkpt
	uint32 requester;
};


////////////////////////////////////////////////////////////////////////////////
// CMP exclusive L3 cache
//   Creates a specialized cache of the appropriate types
class cmp_incl_3l_l2_cache_t : public tcache_t<cmp_incl_3l_l2_protocol_t>
{

public:
	cmp_incl_3l_l2_cache_t(string name, cache_config_t *conf, uint32 id, 
        conf_object_t * _cpu, uint64 prefetch_alg);
	
	// Returns action resulting from incoming message
	uint32 get_action(message_t *);
	
	// Trigger the transition based on the passed info
	// Next state is updated
	void trigger(tfn_info_t *, tfn_ret_t *);
    void set_l3_cache_ptr(cmp_incl_3l_l3_cache_t *);
    
    bool wakeup_signal(message_t* msg, prot_line_t *line, uint32 action);

	bool is_up_message(message_t *message);
	bool is_down_message(message_t *message);
	bool is_request_message(message_t *message);
    
protected:
    
    cmp_incl_3l_l3_cache_t *l3_cache_ptr;
    conf_object_t *cpu;

	// Cache specific transition functions
	void enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret);
	void perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret);
    void send_protocol_msg_down(tfn_info_t *t, tfn_ret_t *ret, bool datafwd, 
        uint32 msg_type, bool addr_msg);
    void send_protocol_msg_down(tfn_info_t *t, tfn_ret_t *ret, bool datafwd, 
        uint32 msg_type, bool addr_msg, mem_trans_t *trans);
        
    void send_protocol_msg_up(tfn_info_t *t, tfn_ret_t *ret, uint32 msg_type);
    void send_protocol_msg_up(tfn_info_t *t, tfn_ret_t *ret, uint32 msg_type, 
		bool icache);
    void send_protocol_msg_up(tfn_info_t *t, tfn_ret_t *ret, uint32 msg_type, 
		bool icache, mem_trans_t *trans);
    void send_readfwd_up(tfn_info_t *t, tfn_ret_t *ret, uint32 msg_type);
	void send_invalidates_up(tfn_info_t *t, tfn_ret_t *ret, bool always = false);
	void set_inclusion(tfn_info_t *t, tfn_ret_t *ret);
	void unset_inclusion(tfn_info_t *t, tfn_ret_t *ret);
	void set_requester(tfn_info_t *t, tfn_ret_t *ret);
    mem_trans_t *get_enqueued_trans(tfn_info_t *t, tfn_ret_t *ret);
    
	
};

#endif /* _CMP_EXCL_3L_L2_CACHE_H_ */
