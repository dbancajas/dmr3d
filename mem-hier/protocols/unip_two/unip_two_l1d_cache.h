/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: unip_two_l1d_cache.h,v 1.1.2.1 2005/08/24 19:20:07 pwells Exp $
 *
 * description:    implementation of a two-level uniprocessor cache
 * initial author: Philip Wells 
 *
 */
 
#ifndef _UNIP_TWO_L1D_CACHE_H_
#define _UNIP_TWO_L1D_CACHE_H_

////////////////////////////////////////////////////////////////////////////////
// unip two l1d cache line
class unip_two_l1d_line_t : public line_t, public unip_two_l1d_protocol_t {

 public:
	void init(cache_t *cache, addr_t idx, uint32 lsize);
	void reinit(addr_t _tag);

	bool is_free();
	bool can_replace();
	bool is_stable();
	
	uint32 get_state();
	void set_state(uint32 _state);

	void from_file(FILE *file);
	void to_file(FILE *file);

 protected:
	uint32 state;
};


////////////////////////////////////////////////////////////////////////////////
// unip two l1i cache line
//   Creates a specialized cache of the appropriate types
class unip_two_l1d_cache_t : public tcache_t<unip_two_l1d_protocol_t> {

 public:
	// Constructor
	unip_two_l1d_cache_t(string name, cache_config_t *conf, 
        uint32 id, uint64 prefetch_alg);
	
	// Returns the action resulting from incomming message
	uint32 get_action(message_t *);
	
	// trigger the transition based on the passed info
	// Next state is updated
	void trigger(tfn_info_t *, tfn_ret_t *);
	
 protected:
	// Unique transition functions
	bool can_send_msg_down(tfn_info_t *t, tfn_ret_t *ret);
	bool send_read_down(tfn_info_t *t, tfn_ret_t *ret);
	bool send_writethrough_down(tfn_info_t *t, tfn_ret_t *ret);
	void send_data_up(tfn_info_t *t, tfn_ret_t *ret);
	void send_request_complete_up(tfn_info_t *t, tfn_ret_t *ret);
	bool enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret);
	void perform_mshr_requests(tfn_info_t *t, tfn_ret_t *ret);
};

#endif /* _UNIP_TWO_L1D_CACHE_H_ */
