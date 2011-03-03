/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: codeshare.h,v 1.1.2.3 2006/01/30 16:43:35 kchak Exp $
 *
 * description:    Class for stack heap access profile
 * initial author: Koushik Chakraborty 
 *
 */
 
#ifndef _STACKHEAP_H_
#define _STACKHEAP_H_



class stackheap_t : public profile_entry_t {

private:
    group_counter_t *stat_read_reference;
    group_counter_t *stat_read_l1_miss;
    group_counter_t *stat_read_l2_miss;
    
    group_counter_t *stat_write_reference;
    group_counter_t *stat_write_l1_miss;
    group_counter_t *stat_write_l2_miss;
	
	
    group_counter_t *stat_kread_reference;
    group_counter_t *stat_kread_l1_miss;
    group_counter_t *stat_kread_l2_miss;
    
    group_counter_t *stat_kwrite_reference;
    group_counter_t *stat_kwrite_l1_miss;
    group_counter_t *stat_kwrite_l2_miss;
    
    void record_kernel_access(mem_trans_t *trans);
    void record_user_access(mem_trans_t *trans);
    
    bool l1_miss;
    bool l2_miss;
    
public:
	stackheap_t(string name);
    void record_access(mem_trans_t *trans);
    void print_stats();
    bool stack_access(mem_trans_t *trans);
	
	
};

#endif // _STACKHEAP_H_
