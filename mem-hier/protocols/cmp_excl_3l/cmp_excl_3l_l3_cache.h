/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_excl_3l_l3_cache.h,v 1.1.2.5 2006/02/13 23:20:29 pwells Exp $
 *
 * description:    implementation of a CMP exclusive cache
 * initial author: Philip Wells 
 *
 */
 
#ifndef _CMP_EXCL_3L_L3_CACHE_H_
#define _CMP_EXCL_3L_L3_CACHE_H_

#include "cmp_excl_3l_l3.h"

////////////////////////////////////////////////////////////////////////////////
// CMP exclusive L3 cache line
class cmp_excl_3l_l3_line_t : public line_t, public cmp_excl_3l_l3_protocol_t {

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
class cmp_excl_3l_l3_cache_t : public tcache_t<cmp_excl_3l_l3_protocol_t>
{

public:
	cmp_excl_3l_l3_cache_t(string name, cache_config_t *conf, uint32 id, 
        uint64 prefetch_alg);
	
	// Returns action resulting from incoming message
	uint32 get_action(message_t *);
	
	// Trigger the transition based on the passed info
	// Next state is updated
	void trigger(tfn_info_t *, tfn_ret_t *);
	
	bool is_request_message(message_t *message);
    void prefetch_helper(addr_t addr, mem_trans_t *trans, bool read);

protected:

	group_counter_t *stat_cpu_ld_requests;
	group_counter_t *stat_cpu_ld_misses;
	group_counter_t *stat_cpu_st_requests;
	group_counter_t *stat_cpu_st_misses;
	
	base_counter_t *stat_num_writebacks;
	
	// Cache specific transition functions
	void send_msg_down(tfn_info_t *t, tfn_ret_t *ret, uint32 type);
	void send_msg_up(tfn_info_t *t, tfn_ret_t *ret, uint32 type);

    bool wakeup_signal(message_t* msg, prot_line_t *line, uint32 action);
    
	void profile_transition(tfn_info_t *t, tfn_ret_t *ret);
};

#endif /* _CMP_EXCL_3L_L3_CACHE_H_ */
