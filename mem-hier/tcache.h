/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: tcache.h,v 1.4.2.19 2006/03/02 23:58:43 pwells Exp $
 *
 * description:    cache with functionality common to most protocols
 * initial author: Philip Wells 
 *
 */
 
#ifndef _TCACHE_H_
#define _TCACHE_H_

// TEMP
#define DEBUG_TR_FN(_str, _t, _ret, ...) \
    VERBOSE_OUT(verb_t::transfn, "%10s @ %12llu 0x%016llx: " _str "\n", \
	get_cname(), external::get_current_cycle(), \
    get_line_address(_t->address), \
	##__VA_ARGS__)

	
template <class prot_t>
class tcache_t : public cache_t, public statemachine_t<prot_t> {
	
public:
 	typedef typename prot_t::prot_line_t  prot_line_t;
	typedef typename prot_t::mshr_type_t  mshr_type_t;
	typedef typename prot_t::tfn_info_t   tfn_info_t;
	typedef typename prot_t::tfn_ret_t    tfn_ret_t;
    typedef typename prot_t::cache_type_t cache_type_t;
	typedef typename prot_t::upmsg_t      upmsg_t;
    typedef typename prot_t::downmsg_t    downmsg_t;

	tcache_t(string name, uint32 _num_links, cache_config_t *conf, uint32 id, uint64 prefetch_alg);
	~tcache_t();
 
	// Required functions for statemachine_t
	// Returns the action resulting from an incomming message
	virtual uint32 get_action(message_t *) = 0;

	// Required functions for device_t
	// These are virtual in case subclasses want to override it
	virtual stall_status_t message_arrival(message_t *message);
	virtual void from_file(FILE *file);
	virtual void to_file(FILE *file);
	virtual bool is_quiet();
	
	// This drives the statemachine
	virtual stall_status_t process_message(message_t *message);

	// Basic Cache Functions
	line_t *lookup(addr_t addr);
	
	bool flush(addr_t addr, size_t size, message_t *message);
	bool flush_line(line_t *line, message_t *message);
    virtual void prefetch_helper(addr_t addr, mem_trans_t *trans, bool read);
	
	virtual bool is_up_message(message_t *message);
	virtual bool is_down_message(message_t *message);
	virtual bool is_request_message(message_t *message);
	
	void set_kernel_cache(bool val = true);
	
 protected:
 
    uint32 id;                         // Id of the cache
 	prot_line_t **lines;               // Array of cache lines
	mshrs_t<mshr_type_t> *mshrs;       // MSHRs for cache
	
	prot_line_t *wb_lines;             // WriteBack buffer
	bool        *wb_valid;             // valid bits for wbb lines 
	uint32 numwb_buffers_avail;        // number of available WB buffers
	
	prefetch_engine_t<cache_type_t> *prefetch_engine;
	
	cache_bank_t **banks;              // Cache backs
	
	// For dealing with I/O devices
	addr_t cur_io_addr;
	size_t cur_io_size;
	uint32 num_flush_wait; // Number of lines waiting for completed flush
	
	bool kernel_cache;  // cache for kernel data
    
    
	// Private Cache functions
    void initialize_stats();
	line_t *choose_replacement(addr_t idx);
	line_t *find_free_line(addr_t addr);
	stall_status_t send_message(link_t *link, message_t *msg);
	void wakeup_replay_q_set(addr_t line_addr);
	void wakeup_replay_q_all();

    virtual bool wakeup_signal(message_t* msg, prot_line_t *line, uint32 action);
    
	
	stall_status_t handle_io_message(message_t *message);
    void send_protocol_message(tfn_info_t *t, tfn_ret_t *ret, bool up_msg, uint32);

	// Functions to Handle the Write-back Buffer
	void release_writebackbuffer(addr_t _addr);
	bool can_allocate_writebackbuffer();
	bool allocate_writebackbuffer(line_t *line);
	bool search_writebackbuffer(addr_t _addr);
	
	// Required statemachine function
	virtual void profile_transition(tfn_info_t *t, tfn_ret_t *ret);
    virtual bool is_tagarray();
    
    
	// Common Coherence Protocol Transition Functions
	stall_status_t can_insert_new_line(tfn_info_t *t, tfn_ret_t *ret);
	stall_status_t insert_new_line(tfn_info_t *t, tfn_ret_t *ret);
	void mark_completed(tfn_info_t *t, tfn_ret_t *ret);
	bool can_allocate_mshr(tfn_info_t *t, tfn_ret_t *ret);
	bool allocate_mshr(tfn_info_t *t, tfn_ret_t *ret);
	void deallocate_mshr(tfn_info_t *t, tfn_ret_t *ret);
	bool can_enqueue_request_mshr(tfn_info_t *t, tfn_ret_t *ret);
	void set_mshr_valid_bits(tfn_info_t *t, tfn_ret_t *ret);
	bool check_mshr_valid_bits(tfn_info_t *t, tfn_ret_t *ret);
	void store_data_to_cache(tfn_info_t *t, tfn_ret_t *ret);
	void store_data_to_cache_ifnot_valid(tfn_info_t *t, tfn_ret_t *ret);
	void prefetch_heuristic(tfn_info_t *t, tfn_ret_t *ret);
	
	void block_message_set_event(tfn_info_t *t, tfn_ret_t *ret);
	void block_message_other_event(tfn_info_t *t, tfn_ret_t *ret);
	void block_message_poll(tfn_info_t *t, tfn_ret_t *ret);
    void block_message(tfn_info_t *t, tfn_ret_t *ret, stall_status_t stall_type);

	/*
	void stat_read_hit(tfn_info_t *t, tfn_ret_t *ret);
	void stat_read_miss(tfn_info_t *t, tfn_ret_t *ret);
	void stat_read_miss_mshr_hit(tfn_info_t *t, tfn_ret_t *ret);
	void stat_read_hit_mshr_hit(tfn_info_t *t, tfn_ret_t *ret);
	void stat_write_hit(tfn_info_t *t, tfn_ret_t *ret);
	void stat_write_miss(tfn_info_t *t, tfn_ret_t *ret);
	void stat_write_miss_mshr_hit(tfn_info_t *t, tfn_ret_t *ret);
	*/
    
    // checkpoint hacks
    void cmp_incl_read_cache_lines(FILE *file);
    void cmp_3l_read_cache_lines(FILE *file, bool);
};


// Include template CC
#include "tcache.cc"

#endif /* _TCACHE_H_ */
