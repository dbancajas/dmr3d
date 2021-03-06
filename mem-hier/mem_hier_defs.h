/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mem_hier_defs.h,v 1.5.2.10 2006/12/12 18:36:57 pwells Exp $
 *
 * description:    definitions for mem-hier types
 * initial author: Philip Wells 
 *
 */
 
#ifndef _MEM_DEFINITIONS_H_
#define _MEM_DEFINITIONS_H_

// Type definitions
#ifndef __SIZE_TYPE__
typedef uint32 size_t;
#endif

// module declarations
class mem_hier_t;
class mem_driver_t;

// Interface
class mem_hier_interface_t;
class proc_interface_t;
class mem_trans_t;
class dynamic_instr_t;  // Processor's
class invalidate_addr_t;

// cache classes
class cache_t;
class cache_bank_t;
class line_t;
class unip_two_l1d_prot_sm_t;
class cmp_incl_l2_protocol_t;
template <class prot_t> class tcache_t;
template <class type_t> class mshr_t;
template <class type_t> class mshrs_t;
template <class cache_t> class prefetch_engine_t;
struct cache_config_t;

// network classes
class link_t;
class device_t;
class message_t;

// Devices
class generic_proc_t;
template <class prot_t, class msg_t> class proc_t;
template <class prot_t, class msg_t> class main_mem_t;
template <class prot_t, class msg_t> class dram_t;

class interconnect_t;
class mesh_router_t;
class mesh_simple_t;
class mesh_sandwich_t;

// messages
class simple_mainmem_msg_t;

// statemachine classes
template <typename action_t, typename state_t,
          class prot_line_t> class transfn_info_t;
template <typename state_t, class prot_line_t> class transfn_return_t;
template <class prot_t> class statemachine_t;

// Events
template <class data_type, class context_type> class event_t;
class generic_event_t;
class meventq_t;
class trans_rec_t;
class random_tester_t;

// Profilers
class mem_tracer_t;
class profile_spinlock_t;
class codeshare_t;
class stackheap_t;

class sms_t;

class warmup_trace_t;

// Enumerations
enum stall_status_t {
	StallNone,
	StallSetEvent,
	StallOtherEvent,
	StallPoll,
	StallMax
};

enum mem_return_t {
	MemComplete,      // Mem transaction hit and is ready
	MemMiss,          // Mem transaction missed
	MemStall,         // could not accept memory request
	MemReturnMax
};


#endif // _MEM_DEFINITIONS_H_
