/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: random_tester.cc,v 1.1.2.7 2005/09/13 23:37:51 kchak Exp $
 *
 * description:    Random Tester for Coherence Bug Detection
 * initial author: Koushik Chakraborty 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: random_tester.cc,v 1.1.2.7 2005/09/13 23:37:51 kchak Exp $");
 
#include "definitions.h"
#include "config_extern.h"
#include "transaction.h"
#include "mem_hier.h"
#include "random_tester.h"


random_tester_t::random_tester_t(mem_hier_t *_mem_hier_t)
    : mem_hier(_mem_hier_t)
{
	for (uint32 i = 0; i < g_conf_random_addresses; i++)
		random_addr.insert(random_addr.begin(), i * (1<<30));
}


void random_tester_t::generate_random_transactions(mem_trans_t *trans)
{
    for (uint32 i = 0; i < g_conf_random_transaction_traffic; i++)
    {
        addr_t addr = random_addr[random() % random_addr.size()];
        mem_trans_t *r_trans = mem_hier_t::ptr()->get_mem_trans ();
        r_trans->phys_addr = addr;
        // This can be varied ... presently just meant to increase contention
        r_trans->cpu_id = (trans->cpu_id + i) % mem_hier->get_num_processors();
        r_trans->read = (i % 2 == 0);
        r_trans->write = !(r_trans->read);
        r_trans->ini_ptr = mem_hier_t::ptr()->get_cpu_object(r_trans->cpu_id);
        r_trans->random_trans = 1;
        r_trans->cache_phys = true;
        r_trans->size = 4; // To Avoid Arithmetic Exception in LSQ
        mem_hier->make_request(r_trans->ini_ptr, r_trans);
    }
    
}

void random_tester_t::complete_random_transaction(mem_trans_t *trans)
{
    /*
    delete_list.push_back(trans);
    mem_trans_t *item = delete_list.front();
    while (delete_list.size() && item->get_pending_events() == 0)
    {
        delete item;
        delete_list.pop_front();
        item = delete_list.front();
    }
    */
    
}
