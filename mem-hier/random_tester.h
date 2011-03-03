/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: random_tester.h,v 1.1.2.1 2005/06/16 18:16:41 kchak Exp $
 *
 * description:    Random Tester for Coherence Bug Detection
 * initial author: Koushik Chakraborty 
 *
 */
 
#ifndef _RANDOM_TESTER_H_
#define _RANDOM_TESTER_H_

class random_tester_t {
    private:
    vector<addr_t> random_addr;
    list<mem_trans_t *> delete_list;
    mem_hier_t *mem_hier;
    
    public:
    random_tester_t(mem_hier_t *mem_hier);
    void generate_random_transactions(mem_trans_t *trans);
    void complete_random_transaction(mem_trans_t *trans);
    
};

#endif
