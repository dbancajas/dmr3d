/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_excl_3l_l1_cache.h,v 1.1.2.3 2006/03/04 18:55:40 kchak Exp $
 *
 * description:    implementation of a CMP exclusive cache
 * initial author: Philip Wells 
 *
 */
 
#ifndef _CMP_EXCL_3L_L1_CACHE_H_
#define _CMP_EXCL_3L_L1_CACHE_H_

#include "cmp_excl_3l_l1.h"

////////////////////////////////////////////////////////////////////////////////
// CMP exclusive L3 cache line
class cmp_excl_3l_l1_line_t : public line_t, public cmp_excl_3l_l1_protocol_t {

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

	uint32 get_state();
	void set_state(uint32 _state);

	
protected:
	uint32 state;
};


////////////////////////////////////////////////////////////////////////////////
// CMP exclusive L3 cache
//   Creates a specialized cache of the appropriate types
class cmp_excl_3l_l1_cache_t : public tcache_t<cmp_excl_3l_l1_protocol_t>
{

public:
	cmp_excl_3l_l1_cache_t(string name, cache_config_t *conf, uint32 id, 
        bool _icache, conf_object_t * _cpu, uint64 prefetch_alg);
	
	// Returns action resulting from incoming message
	uint32 get_action(message_t *);
	
	// Trigger the transition based on the passed info
	// Next state is updated
	void trigger(tfn_info_t *, tfn_ret_t *);
    
	void prefetch_helper(addr_t addr, mem_trans_t *trans, bool read);
    
protected:
    
	conf_object_t *cpu;
	bool icache;

	// Cache specific transition functions
	void enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret);
	void perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret);
	void send_invalidate_up(tfn_info_t *t, tfn_ret_t *ret);
    void send_protocol_msg_down(tfn_info_t *t, tfn_ret_t *ret, bool datafwd, 
		uint32 msg_type, bool addr_msg);
    void send_protocol_msg_down(tfn_info_t *t, tfn_ret_t *ret, bool datafwd, 
		uint32 msg_type, bool addr_msg, mem_trans_t *trans);
      
    void send_protocol_msg_up(tfn_info_t *t, tfn_ret_t *ret, uint32 msg_type,
        mem_trans_t *trans);
    mem_trans_t *get_enqueued_trans(tfn_info_t *t, tfn_ret_t *ret);
	
};

#endif /* _CMP_EXCL_3L_L1_CACHE_H_ */
