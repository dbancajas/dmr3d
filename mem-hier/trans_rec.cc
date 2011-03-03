/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */
 
/* $Id: trans_rec.cc,v 1.2.2.1 2005/06/16 20:50:12 kchak Exp $
 *
 * description:    Transaction record for forward progress error detection
 * initial author: Koushik Chakraborty
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: trans_rec.cc,v 1.2.2.1 2005/06/16 20:50:12 kchak Exp $");


#include "definitions.h"
#include "config_extern.h"
#include "protocols.h"
#include "device.h"
#include "cache.h"
#include "line.h"
#include "link.h"
#include "mshr.h"
#include "transaction.h"
#include "statemachine.h"
#include "external.h"
#include "debugio.h"
#include "verbose_level.h"
#include "mem_hier.h"
#include "startup.h"
#include "chip.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "trans_rec.h"

trans_rec_info::trans_rec_info(mem_trans_t *_trans, tick_t _time)
    : trans(_trans), request_cycle(_time)
    
{
    
    
}


trans_rec_t::trans_rec_t(uint32 _num_procs)
    : num_procs(_num_procs)
{
    records = new map<mem_trans_t *, trans_rec_info *>[_num_procs];
}


void trans_rec_t::insert_record(uint32  _proc, mem_trans_t *_trans, tick_t _time)
{
    if (records[_proc].find(_trans) == records[_proc].end()) {
        trans_rec_info *trinfo = new trans_rec_info(_trans, _time);
        records[_proc][_trans] = trinfo; 
    }
}

void trans_rec_t::delete_record(uint32 _proc, mem_trans_t *_trans)
{
    ASSERT(records[_proc].find(_trans) != records[_proc].end());
    trans_rec_info *tr = records[_proc][_trans];
    delete tr;
    records[_proc].erase(_trans);
}

void trans_rec_t::detect_error(tick_t current_time)
{
    for (uint32 i = 0; i < num_procs; i++) {
        map<mem_trans_t *, trans_rec_info *>::iterator list_it;
        for (list_it = records[i].begin(); list_it != records[i].end(); list_it++)
        {
            if (current_time - list_it->second->request_cycle > (tick_t)g_conf_mem_hier_response_threshold)
            {
                printf("%10s @ %12llu Request time %12llu Addr 0x%016llx: Response timeout for mem-hier\n", 
					"mem_hier", current_time, list_it->second->request_cycle, list_it->first->phys_addr);
                list_it->first->debug();   
                printf("Processor cycles before error %llu\n", mem_hier_t::ptr()->get_module_obj()->chip->get_g_cycles());
                FAIL;    
            }
        }
    }
}
    
