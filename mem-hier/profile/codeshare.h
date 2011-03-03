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
 * description:    Class for code page profiling
 * initial author: Koushik Chakraborty 
 *
 */
 
#ifndef _CODESHARE_H_
#define _CODESHARE_H_



class codeshare_t : public profile_entry_t {

private:

	map<addr_t, uint64> *code_block_trace;
    map<addr_t, uint64> *page_access_trace;
    
    map<addr_t, uint64>::iterator *fetch_ptr;
    map<addr_t, uint64>::iterator *page_ptr;
    
    set<addr_t> os_instr_blocks;
	// stat stuff
    histo_1d_t *stat_page_share;
    histo_1d_t *stat_code_block_share;
    histo_1d_t *stat_dynamic_code_share;
    histo_1d_t *stat_user_block_share;
    histo_1d_t *stat_os_block_share;
    
    void gather_stats();
    void clear_records();
    addr_t get_mask(const int min, const int max);
    
    uint32 num_procs;
    addr_t icache_mask;
    addr_t page_mask;
	
	
public:
	codeshare_t(string name, uint32 _nump);
	
	void print_stats();
    void record_fetch(mem_trans_t *trans);
    void to_file(FILE *file);
    void from_file(FILE *file);
    
	
	
};

#endif // _CODESHARE_H_
