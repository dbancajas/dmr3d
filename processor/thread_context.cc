/*
 * Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: thread_context.cc,v 1.1.2.3 2006/02/14 15:56:25 kchak Exp $");

#include "definitions.h"
#include "window.h"
#include "thread_context.h"


thread_context_t::thread_context_t(uint32 _id)
    : seq(NULL), id(_id)
    
{
    
    
}
        

uint32 thread_context_t::get_id()
{
    return id;
}

sequencer_t *thread_context_t::get_sequencer()
{
    return seq;
}

void thread_context_t::set_sequencer(sequencer_t *_s)
{
    seq = _s;
}

void thread_context_t::set_seq_ctxt(uint32 _seq_id)
{
    seq_ctxt_id = _seq_id;
}

uint32 thread_context_t::get_seq_ctxt()
{
    return seq_ctxt_id;
}
        
