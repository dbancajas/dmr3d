/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *

 * $Id: fetch_buffer.cc,v 1.1.2.12 2006/03/03 22:14:41 kchak Exp $
 *
 * description:    Implementation for fetch buffer
 * initial author: Koushik Chakraborty
 *
 */
 
 

//  #include "simics/first.h"
// RCSID("$Id: fetch_buffer.cc,v 1.1.2.12 2006/03/03 22:14:41 kchak Exp $");

#include "definitions.h"
#include "mai.h"
#include "mai_instr.h"
#include "isa.h"
#include "fu.h"
#include "dynamic.h"
#include "window.h"
#include "iwindow.h"
#include "sequencer.h"
#include "fu.h"
#include "eventq.h"
#include "v9_mem_xaction.h"
#include "mem_xaction.h"
#include "chip.h"

#include "proc_stats.h"
#include "stats.h"
#include "counter.h"
#include "mem_hier_iface.h"
#include "transaction.h"
#include "fetch_buffer.h"
#include "mem_hier_handle.h"
#include "mem_hier.h"
#include "startup.h"
#include "config_extern.h"
#include "fastsim.h"
#include "external.h"


fetch_line_entry_t::fetch_line_entry_t(addr_t fetch_addr, bool _pre, mem_trans_t *_tr)
    : fetch_line_addr(fetch_addr), ready(false), prefetch(_pre), trans(_tr)
{
    wait_list.clear();
    insert_time = external::get_current_cycle();
    useful = false; // fix it
}

fetch_line_entry_t::~fetch_line_entry_t()
{
    // if copy transaction is turned on need to mark complete
    if (prefetch && g_conf_copy_transaction)
    {
        ASSERT(!trans->completed);
        trans->completed = true;
    }
}

addr_t fetch_line_entry_t::get_fetch_addr()
{
    return fetch_line_addr;
}

void fetch_line_entry_t::wake_up_squashed()
{
    
    list<dynamic_instr_t *>::iterator wit = wait_list.begin();
    dynamic_instr_t *waiting_fetch_instr;
    while (wit != wait_list.end())
    {
        waiting_fetch_instr = *wit;
        if (waiting_fetch_instr->is_squashed()) {
            waiting_fetch_instr->wakeup();
            wait_list.erase(wit);
            wit = wait_list.begin();
        } else 
            wit++;
    }
    
}

void fetch_line_entry_t::wake_up_waiting()
{
    
    list<dynamic_instr_t *>::iterator wit;
    dynamic_instr_t *waiting_fetch_instr;
    for (wit = wait_list.begin(); wit != wait_list.end(); wit++)    
    {
        waiting_fetch_instr = *wit;
        waiting_fetch_instr->wakeup();
    }
    wait_list.clear();
}

bool fetch_line_entry_t::wait_list_empty()
{
    return wait_list.empty();
}

void fetch_line_entry_t::insert_instr(dynamic_instr_t *d_instr)
{
    
    wait_list.push_back(d_instr);
}

void fetch_line_entry_t::mark_ready()
{
    ready = true;
}

bool fetch_line_entry_t::is_ready()
{
    return ready;
}

void fetch_line_entry_t::debug()
{
    printf("Fetch Addr = %llx wait list size %u ready %d prefetch %u\n", 
        fetch_line_addr, wait_list.size(), ready, prefetch);
}


fetch_buffer_t::fetch_buffer_t(sequencer_t *_seq, uint32 num_ctxt) : seq(_seq)
{
    pred_table_size = (uint64) exp2(g_conf_line_pred_bits);
    line_pred_table = new addr_t[pred_table_size];
    line_pred_tags = new addr_t[pred_table_size];
    for (uint64 i = 0; i < pred_table_size; i++)
    {
        line_pred_table[i] = 0;
        line_pred_tags[i] = 0;
    }
    pred_table_mask = (pred_table_size - 1)  * g_conf_l1i_lsize;
    lsize_bits = (uint32) log2(g_conf_l1i_lsize);
    last_proc_fetch = 0;
	mem_hier_handle = seq->get_chip()->get_mem_hier();
    num_lines = g_conf_fetch_buffer_size * num_ctxt;
}

void fetch_buffer_t::insert_fetch_line(addr_t fetch_addr, bool prefetch, mem_trans_t *trans)
{
    fetch_line_entry_t *fle;
    if (fetch_lines.size() == num_lines) {
        delete_fetch_line();
    }
    fle = new fetch_line_entry_t(fetch_addr, prefetch, trans);
    fetch_lines.push_back(fle);
    
}

void fetch_buffer_t::do_stats(fetch_line_entry_t *fle)
{
    proc_stats_t *pstats = seq->get_pstats(0);
    
    // do stats
    
    if (fle->is_prefetch()) {
        if (fle->is_useful())
            pstats->stat_line_prediction->inc_total(1, 1);
        else
            pstats->stat_line_prediction->inc_total(1, 2);
    } else
        pstats->stat_line_prediction->inc_total(1, 0);
    
    
}

void fetch_buffer_t::delete_fetch_line()
{
    fetch_line_entry_t *fle;
    list<fetch_line_entry_t *>::iterator it;
    
    fle = fetch_lines.front();
    
        
    // remove entry     
    if (!fle->is_ready())
        fle->wake_up_squashed();
    
    if (fle->wait_list_empty()) {
        do_stats(fle);
        fetch_lines.pop_front(); 
        delete fle;
        return;
    } else {
        // Need to iterate ... SMT requirement
        it = fetch_lines.begin();
        for (it++; it != fetch_lines.end(); it++)
        {
            fle = *it;
            if (!fle->is_ready())
                fle->wake_up_squashed();
            
            if (fle->wait_list_empty()) {
                do_stats(fle);
                fetch_lines.erase(it); 
                delete fle;
                return;
            }
        }
    }
    
    FAIL_MSG ("Inadequeate Number of Fetch Buffers");
    
}

void fetch_buffer_t::initiate_prefetch(addr_t fetch_addr, mem_trans_t *trans)
{
    addr_t next_line = next_line_prediction(fetch_addr);
    fetch_line_entry_t *fle = get_line_entry(next_line);
    if (!fle) {
        // NO Entry Found
        mem_trans_t *new_trans = mem_hier_t::ptr()->get_mem_trans();
        new_trans->copy(*trans);
        new_trans->dinst = NULL;
		new_trans->sequencer = seq;
        new_trans->phys_addr = next_line;
        new_trans->request_time = 0;
        new_trans->completed = 0;
        new_trans->hw_prefetch = 1;
        new_trans->prefetch_type = FB_PREFETCH;
        new_trans->clear_all_events();
        new_trans->mark_pending(PROC_REQUEST);
		new_trans->pending_messages = 0;
        insert_fetch_line(next_line, true, new_trans);
        mem_hier_handle->make_request(new_trans);
    }
}


uint32 fetch_buffer_t::initiate_fetch(addr_t fetch_addr, dynamic_instr_t *d_instr,
                                        mem_trans_t *trans)
{
 
    if (last_proc_fetch != fetch_addr)
        update_prediction_table(last_proc_fetch, fetch_addr);
    last_proc_fetch = fetch_addr;
    fetch_line_entry_t *fle = get_line_entry(fetch_addr);
    if (fle) {
        // Entry Found ... initiate next prefetch if not done already
        fle->mark_useful();
        if (fle->is_ready())
        {
            if (g_conf_line_prefetch && fle->is_prefetch())
            {
               initiate_prefetch(fle->get_fetch_addr(), trans);
            }
            return 0;
        }
        
		ASSERT(trans->request_time == 0);
		trans->request_time = seq->get_chip()->get_g_cycles();
        d_instr->set_initiate_fetch(false);
        fle->insert_instr(d_instr);
        return g_conf_max_latency;
    }
    
    insert_fetch_line(fetch_addr, false, trans);
    
    d_instr->set_initiate_fetch(true);
    return g_conf_max_latency;
}

addr_t fetch_buffer_t::next_line_prediction(addr_t line_addr)
{
    if (g_conf_straight_line_prefetch)
        return line_addr + g_conf_l1i_lsize;
    
    addr_t index = (line_addr & pred_table_mask) >> lsize_bits;
    addr_t tag = line_addr >> lsize_bits;
    if (line_pred_tags[index] == tag) return line_pred_table[index];
    
    return line_addr + g_conf_l1i_lsize;
}

bool fetch_buffer_t::contiguous_fetch(addr_t prev, addr_t next)
{
    return (next > prev && (next - prev ) == (addr_t) g_conf_l1i_lsize);     
}

void fetch_buffer_t::update_prediction_table(addr_t fetch_addr, addr_t next_line_addr)
{
    if (g_conf_straight_line_prefetch) return;
    
    addr_t index = (fetch_addr & pred_table_mask) >> lsize_bits;
    addr_t tag = fetch_addr >> lsize_bits;
    if (!contiguous_fetch(fetch_addr, next_line_addr))
    {
        line_pred_tags[index] = tag;
        line_pred_table[index] = next_line_addr;
    }
    else if (line_pred_tags[index] == tag)
        line_pred_tags[index] = 0;
    
}


fetch_line_entry_t *fetch_buffer_t::get_line_entry(addr_t fetch_addr)
{
    fetch_line_entry_t *fle;
    list<fetch_line_entry_t *>::iterator it;
    for (it = fetch_lines.begin(); it != fetch_lines.end(); it++)
    {
        fle = *it;
        if (fle->get_fetch_addr() == fetch_addr)
            return fle;
    }
    return NULL;
}


void fetch_buffer_t::complete_fetch(addr_t fetch_addr, mem_trans_t *trans)
{
     
    ASSERT(trans->get_pending_events());
    fetch_line_entry_t *fle = get_line_entry(fetch_addr);
    
    if (fle) {
        fle->mark_ready();
        fle->wake_up_waiting();
    }
    
    // Initiate Next Fetch if it was not a wrong path fetch
    if (g_conf_line_prefetch && trans->dinst && !trans->dinst->is_squashed()) {
        initiate_prefetch(fetch_addr, trans);
    }
    
}

