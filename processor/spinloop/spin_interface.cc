/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id $
 *
 * description:    Base class for spin loop detection heauristics
 * initial author: Koushik Chakraborty 
 *
 */
 
 

//  #include "simics/first.h"
// RCSID("$Id: spin_interface.cc,v 1.7.2.15 2005/11/14 19:44:00 kchak Exp $");

#include "definitions.h"
#include "mai.h"
#include "mai_instr.h"
#include "isa.h"
#include "fu.h"
#include "dynamic.h"
#include "window.h"
#include "iwindow.h"
#include "sequencer.h"
#include "chip.h"
#include "stats.h"
#include "proc_stats.h"
#include "fu.h"
#include "eventq.h"
#include "v9_mem_xaction.h"
#include "mem_xaction.h"
#include "lsq.h"
#include "st_buffer.h"
#include "mem_hier_iface.h"
#include "transaction.h"
#include "mem_hier_handle.h"
#include "startup.h"
#include "config_extern.h"
#include "config.h"
#include "mem_driver.h"
#include "spin_interface.h"


spin_interface_t::spin_interface_t() {
    
    
}
