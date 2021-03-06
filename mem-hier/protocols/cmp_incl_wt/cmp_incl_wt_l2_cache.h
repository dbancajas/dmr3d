/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_incl_wt_l2_cache.h,v 1.1.2.16 2005/11/22 19:33:16 kchak Exp $
 *
 * description:    implementation of a CMP inclusive cache
 * initial author: Philip Wells 
 *
 */
 
#ifndef _CMP_INCL_WT_L2_CACHE_H_
#define _CMP_INCL_WT_L2_CACHE_H_

#include "cmp_incl_wt_l2.h"

////////////////////////////////////////////////////////////////////////////////
// CMP inclusive L2 cache line
class cmp_incl_wt_l2_line_t : public line_t, public cmp_incl_wt_l2_protocol_t {

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
	void set_dirty(bool _dirty);
	bool is_dirty();

	uint32 get_state();
	void set_state(uint32 _state);

	void add_sharer(uint32 sharer);
	void remove_sharer(uint32 sharer);
	bool is_sharer(uint32 sharer);
	uint32 get_sharers();
    
    void warmup_state();
    

	static const uint32 MAX_SHARERS = 32;
	
protected:
	bool dirty;
	uint32 state;
	uint32 sharers;
};


////////////////////////////////////////////////////////////////////////////////
// CMP inclusive L2 cache
//   Creates a specialized cache of the appropriate types
class cmp_incl_wt_l2_cache_t : public tcache_t<cmp_incl_wt_l2_protocol_t>
{

public:
	cmp_incl_wt_l2_cache_t(string name, cache_config_t *conf, uint32 id, 
        uint64 prefetch_alg, uint32 id_offset);
	
	// Returns action resulting from incoming message
	uint32 get_action(message_t *);
	
	// Trigger the transition based on the passed info
	// Next state is updated
	void trigger(tfn_info_t *, tfn_ret_t *);
	
	uint32 get_interconnect_id(addr_t addr);
	bool is_request_message(message_t *message);

protected:
	
	uint32 interconnect_id_offset;
    uint32 num_procs;
	
	st_entry_t *stat_num_req_fwd;
    group_counter_t **stat_read_c2c;
	group_counter_t **stat_write_c2c;


	// Cache specific transition functions
	bool can_send_msg_down(tfn_info_t *t, tfn_ret_t *ret);
	void send_msg_down(tfn_info_t *t, tfn_ret_t *ret, uint32 type);
	void send_protocol_message_up(tfn_info_t *t, tfn_ret_t *ret, uint32 type,
		uint32 destination, uint32 requester, mem_trans_t *trans);
	void writeback_if_dirty(tfn_info_t *t, tfn_ret_t *ret);
	void send_data_to_requester(tfn_info_t *t, tfn_ret_t *ret, bool shared);
	void send_ack_up(tfn_info_t *t, tfn_ret_t *ret, uint32 type);
	void mark_requester_as_sharer(tfn_info_t *t, tfn_ret_t *ret);
	void send_update_to_sharers(tfn_info_t *t, tfn_ret_t *ret);
    void send_invalidate_to_sharers(tfn_info_t *t, tfn_ret_t *ret);
	void remove_source_sharer(tfn_info_t *t, tfn_ret_t *ret);
	void remove_only_sharer(tfn_info_t *t, tfn_ret_t *ret);
	void enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret);
	void perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret);
	bool is_only_one_sharer(tfn_info_t *t, tfn_ret_t *ret);
	bool requester_is_only_sharer(tfn_info_t *t, tfn_ret_t *ret);
    bool requester_is_sharer(tfn_info_t *t, tfn_ret_t *ret);

    bool wakeup_signal(message_t* msg, prot_line_t *line, uint32 action);
	void profile_transition(tfn_info_t *t, tfn_ret_t *ret);
};

#endif /* _CMP_INCL_WT_L2_CACHE_H_ */
