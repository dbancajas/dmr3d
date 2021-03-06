/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    a memory transaction
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: transaction.cc,v 1.4.2.25 2006/12/12 18:36:56 pwells Exp $");

#include "definitions.h"
#include "config_extern.h"
#include "transaction.h"
#include "isa.h"
#include "dynamic.h"
#include "sequencer.h"
#include "mai.h"
#include "mai_instr.h"
#include "mem_hier.h"

#include "config_extern.h"


 
addr_t LWP_STACK_ACCESS_MASK =    0x2a10000;
addr_t KERNEL_MAP_ACCESS_LOWER =  0x3000000;
addr_t KERNEL_MAP_ACCESS_UPPER =  0x3010000;
addr_t KERNEL_MASK  =            0xfff0000;

// default constructor
mem_trans_t::mem_trans_t()
{
	// Probably a waste of time.  Need to fix this class
	// Just don't want to use fields before they are initialized
	bzero(this, sizeof(*this));
	itlb_miss = false;
}

// Copy constructor
// Need to copy data, so can't use default copy const.
mem_trans_t::mem_trans_t(mem_trans_t &trans)
{
	copy(trans);
}

void mem_trans_t::copy(mem_trans_t &trans)
{
	phys_addr = trans.phys_addr;
	virt_addr = trans.virt_addr;
	size = trans.size;

	asi = trans.asi;
	ini_ptr = trans.ini_ptr;

	read = trans.read;
	write = trans.write;
	atomic = trans.atomic;
	hw_prefetch = trans.hw_prefetch;
	ifetch = trans.ifetch;
	control = trans.control;
	speculative = trans.speculative;
	supervisor = trans.supervisor;
	priv = trans.priv;

	call_complete_request = trans.call_complete_request;
	
	sw_prefetch = trans.sw_prefetch;
	prefetch_fcn = trans.prefetch_fcn;
	cache_phys = trans.cache_phys;
	cache_virt = trans.cache_virt;
	
	inverse_endian = trans.inverse_endian;
	page_cross = trans.page_cross;
	
	if (trans.data) {
		data = new uint8[size];
		memcpy(data, trans.data, size); 
	} else {
		data = NULL;
	}
	
	membar_mflag = trans.membar_mflag;
	membar_cflag = trans.membar_cflag;
	
	id = trans.id;
	cpu_id = trans.cpu_id;
	io_device = trans.io_device;
    
    //ASSERT(io_device || cpu_id <  SIM_number_processors());
	may_stall = trans.may_stall;

	dinst = trans.dinst;

	completed = trans.completed;
	request_time = trans.request_time;

	events = trans.events;
	pending_events = trans.pending_events;
	pending_messages = trans.pending_messages;
	itlb_miss = trans.itlb_miss;
    cache_prefetch = trans.cache_prefetch;
    random_trans = trans.random_trans;
    prefetch_type = trans.prefetch_type;
    
    sequencer = trans.sequencer;
    mem_hier_seq = trans.mem_hier_seq;
	vcpu_state_transfer = trans.vcpu_state_transfer;
	sequencer_ctxt = trans.sequencer_ctxt;
}

void mem_trans_t::clear()
{
	bzero(this, sizeof(this));
}


mem_trans_t::~mem_trans_t()
{
	//if (g_conf_cache_date && data) delete [] data;
}


// Constructor passing simics v9_trans
mem_trans_t::mem_trans_t(v9_memory_transaction *trans)
{
	copy(trans);
}

bool
mem_trans_t::is_cacheable()
{
	if (!cache_phys) return false;
	
	return true;
}

bool
mem_trans_t::get_itlb_miss()
{
	return itlb_miss;
}

void 
mem_trans_t::set_itlb_miss(bool val)
{
	itlb_miss = val;
}

bool
mem_trans_t::is_normal_asi()
{
	if (
	(asi >= 0x04 && asi <= 0x11) ||
	(asi >= 0x18 && asi <= 0x19) ||
	(asi >= 0x24 && asi <= 0x2c) ||
	(asi >= 0x70 && asi <= 0x73) ||
	(asi >= 0x78 && asi <= 0x79) ||
	(asi >= 0x80 /*&& asi <= 0xff */)
	) return true;
	
	return false;
}

bool 
mem_trans_t::is_io_space() {
	if (vcpu_state_transfer) return true;
	return (phys_addr >= (addr_t) g_conf_num_machines * g_conf_memory_image_size);
}

// Copy relevant info from simics v9_trans
void mem_trans_t::copy(v9_memory_transaction *trans)
{
	generic_transaction_t *gtrans = &(trans->s);
	
	phys_addr = gtrans->physical_address;
	virt_addr = gtrans->logical_address;
	size = gtrans->size;
	
	asi = trans->address_space;
	ini_ptr = gtrans->ini_ptr;

	read = SIM_mem_op_is_read(gtrans) ? true : false;
	write = SIM_mem_op_is_write(gtrans) ? true : false;
	atomic = gtrans->atomic;
	hw_prefetch = false;  // TODO fix for HW prefetch
	ifetch = SIM_mem_op_is_instruction(gtrans);
	control = SIM_mem_op_is_control(gtrans);
	speculative = gtrans->speculative;
	supervisor = SIM_mem_op_is_from_cpu(gtrans) ?
		(0 < SIM_cpu_privilege_level(ini_ptr)) : false;
	priv = trans->priv;

	call_complete_request = true;
	
	sw_prefetch = (gtrans->type == Sim_Trans_Prefetch);
	// FIXME: hack for simics bug
	//if (trans->prefetch_fcn != 0) sw_prefetch = true;
	
	prefetch_fcn = trans->prefetch_fcn;
	cache_phys = trans->cache_physical;
	cache_virt = trans->cache_virtual;
	
	inverse_endian = gtrans->inverse_endian;
	page_cross = gtrans->page_cross;
	data = NULL;
	
	membar_mflag = 0;
	membar_cflag = 0;
	
	id = gtrans->id;
	// To indicate proper CPU or some I/O device
	cpu_id = SIM_mem_op_is_from_cpu(gtrans) ? SIM_get_proc_no(ini_ptr)
	                                         : SIM_number_processors();
	io_device = (SIM_mem_op_is_from_device(gtrans));
	may_stall = gtrans->may_stall;
	
	if (g_conf_num_machines > 1) {
		if (phys_addr > (addr_t) g_conf_memory_image_size)
			phys_addr += g_conf_num_machines * (addr_t) g_conf_memory_image_size;
		else if (io_device) { }
		//else if (g_conf_machines_share_osinstr && ifetch && supervisor) { }
		else // Normal case
			phys_addr += ((addr_t) g_conf_memory_image_size) * mem_hier_t::get_machine_id(cpu_id);
	}

	ASSERT(cpu_id < SIM_number_processors() || io_device);
	
	dinst = NULL; // Must be set later
	request_time = 0;

	// Set these in cache for now
	completed = false;
	request_time = 0;
	
	events = 0;
	pending_events = 0;
	pending_messages = 0;
    cache_prefetch = false;
    random_trans = false;
    prefetch_type = NO_PREFETCH;
	vcpu_state_transfer = false;
	sequencer_ctxt = 0;
}

void mem_trans_t::recycle()
{
    data = NULL;
	
	membar_mflag = 0;
	membar_cflag = 0;
	
	id = 0;
	// To indicate proper CPU or some I/O device
    cpu_id = SIM_number_processors();
	io_device = false;
	may_stall = true;

	dinst = NULL; // Must be set later
	completed = false;
	request_time = 0;
	
	ifetch = 0;
	read = 0;
	write = 0;
	
	events = 0;
	pending_events = 0;
	pending_messages = 0;
    cache_prefetch = false;
    random_trans = false;
    prefetch_type = NO_PREFETCH;
    sequencer = 0;
    mem_hier_seq = 0;
	vcpu_state_transfer = false;
	sequencer_ctxt = 0;
}

void
mem_trans_t::debug()
{
    DEBUG_OUT("Phys Addr : 0x%llx cpu %u ifetch %u Read %u Write %u hw_prefetch %u random_trans %u call complete %u\n",
            phys_addr, (uint32) cpu_id, ifetch, read, write, hw_prefetch, random_trans, call_complete_request);
    report_event();
    DEBUG_FLUSH();        
}

prefetch_type_t
mem_trans_t::get_prefetch_type() {
    return prefetch_type;
}

void
mem_trans_t::mark_pending(uint32 event) {
	events |= event;
	pending_events |= event;
}

void
mem_trans_t::mark_event(uint32 event) {
	events |= event;
}

void
mem_trans_t::clear_pending_event(uint32 event) {
	pending_events &= ~event;
}

uint32
mem_trans_t::get_events()
{
	return events;
}

void
mem_trans_t::report_event(bool pending)
{
    
    uint32 test_events = events;
    if (pending) test_events = pending_events;
    
    if (test_events & MEM_EVENT_L1_HIT) 
        cout << " MEM_EVENT_L1_HIT "<< endl ;
    if (test_events & MEM_EVENT_L1_DEMAND_MISS) 
        cout << " MEM_EVENT_L1_DEMAND_MISS "<< endl ;
    if (test_events & MEM_EVENT_L1_COHER_MISS) 
        cout << " MEM_EVENT_L1_COHER_MISS "<< endl ;
    if (test_events & MEM_EVENT_L1_MSHR_PART_HIT) 
        cout << " MEM_EVENT_L1_MSHR_PART_HIT "<< endl ;
    if (test_events & MEM_EVENT_L1_STALL) 
        cout << " MEM_EVENT_L1_STALL "<< endl ;
    if (test_events & MEM_EVENT_L1_WRITE_BACK) 
        cout << " MEM_EVENT_L1_WRITE_BACK "<< endl ;
    if (test_events & MEM_EVENT_L2_HIT) 
        cout << " MEM_EVENT_L2_HIT "<< endl ;
    if (test_events & MEM_EVENT_L2_DEMAND_MISS) 
        cout << " MEM_EVENT_L2_DEMAND_MISS "<< endl ;
    if (test_events & MEM_EVENT_L2_COHER_MISS) 
        cout << " MEM_EVENT_L2_COHER_MISS "<< endl ;
    if (test_events & MEM_EVENT_L2_MSHR_PART_HIT) 
        cout << " MEM_EVENT_L2_MSHR_PART_HIT "<< endl ;
    if (test_events & MEM_EVENT_L2_STALL) 
        cout << " MEM_EVENT_L2_STALL "<< endl ;
    if (test_events & MEM_EVENT_L2_WRITE_BACK) 
        cout << " MEM_EVENT_L2_WRITE_BACK "<< endl ;
    if (test_events & MEM_EVENT_L2_REPLACE) 
        cout << " MEM_EVENT_L2_REPLACE "<< endl ;
    if (test_events & MEM_EVENT_L2_REQ_FWD) 
        cout << " MEM_EVENT_L2_REQ_FWD "<< endl ;
    if (test_events & MEM_EVENT_L1DIR_DEMAND_MISS) 
        cout << " MEM_EVENT_L1DIR_DEMAND_MISS "<< endl ;
    if (test_events & MEM_EVENT_L1DIR_COHER_MISS) 
        cout << " MEM_EVENT_L1DIR_COHER_MISS "<< endl ;
    if (test_events & MEM_EVENT_L1DIR_STALL) 
        cout << " MEM_EVENT_L1DIR_STALL "<< endl ;
    if (test_events & MEM_EVENT_L1DIR_WRITE_BACK) 
        cout << " MEM_EVENT_L1DIR_WRITE_BACK "<< endl ;
    if (test_events & MEM_EVENT_L1DIR_REPLACE) 
        cout << " MEM_EVENT_L1DIR_REPLACE "<< endl ;
    if (test_events & MEM_EVENT_L1DIR_REQ_FWD) 
        cout << " MEM_EVENT_L1DIR_REQ_FWD "<< endl ;
    if (test_events & MEM_EVENT_L1DIR_HIT) 
        cout << " MEM_EVENT_L1DIR_HIT "<< endl ;
    if (test_events & MEM_EVENT_L3_HIT) 
        cout << " MEM_EVENT_L3_HIT "<< endl ;
    if (test_events & MEM_EVENT_L3_DEMAND_MISS) 
        cout << " MEM_EVENT_L3_DEMAND_MISS "<< endl ;
    if (test_events & MEM_EVENT_L3_COHER_MISS) 
        cout << " MEM_EVENT_L3_COHER_MISS "<< endl ;
    if (test_events & MEM_EVENT_L3_MSHR_PART_HIT) 
        cout << " MEM_EVENT_L3_MSHR_PART_HIT "<< endl ;
    if (test_events & MEM_EVENT_L3_STALL) 
        cout << " MEM_EVENT_L3_STALL "<< endl ;
    if (test_events & MEM_EVENT_L3_REPLACE) 
        cout << " MEM_EVENT_L3_REPLACE "<< endl ;
    if (test_events & MEM_EVENT_L3_REQ_FWD) 
        cout << " MEM_EVENT_L3_REQ_FWD "<< endl ;
    if (test_events & MEM_EVENT_MAINMEM_WRITE_BACK) 
        cout << " MEM_EVENT_MAINMEM_WRITE_BACK "<< endl ;
    if (test_events & MEM_EVENT_DEFERRED) 
        cout << " MEM_EVENT_DEFERRED "<< endl ;
    if (test_events & PROC_REQUEST) 
        cout << " PROC_REQUEST "<< endl ;
    if (test_events & MEM_XACTION_CREATE ) 
        cout << " MEM_XACTION_CREATE " << endl;
    
    DEBUG_FLUSH ();
    
}


bool
mem_trans_t::occurred(uint32 event)
{
	return (events & event);
}

uint32
mem_trans_t::get_pending_events()
{
	return pending_events;
}

void
mem_trans_t::clear_all_events()
{
    events = 0;
    pending_events = 0;    
}

bool
mem_trans_t::is_pending(uint32 event)
{
	return (pending_events & event);
}

bool
mem_trans_t::is_thread_state()
{
    return vcpu_state_transfer;
}

bool
mem_trans_t::is_prefetch()
{
	//return ((hw_prefetch && prefetch_type != FB_PREFETCH) || cache_prefetch || sw_prefetch);
    return cache_prefetch;
}

void
mem_trans_t::set_dinst(dynamic_instr_t *_dinst)
{
	dinst = _dinst;
	sequencer = dinst->get_sequencer();
    mem_hier_seq = sequencer->get_mem_hier_seq(_dinst->get_tid());
    cpu_id = mem_hier_seq->get_id();
	
	if (g_conf_profile_mem_addresses && g_conf_prof_ma_regwin_as_user) {
		mai_t *mai = dinst->get_mai_instruction()->get_mai_object();
		if (mai->is_user_trap() && mai->is_fill_spill(mai->get_tt(mai->get_tl())))
			supervisor = false;
	}
}

void mem_trans_t::set_sequencer(sequencer_t *seq)
{
    sequencer = seq;
    mem_hier_seq = sequencer->get_mem_hier_seq(0); // TODO SMT version
    cpu_id = mem_hier_seq->get_id();
}

// merge stats etc from first trans into new trans 
void
mem_trans_t::merge_changes(mem_trans_t *old_trans)
{
	// Don't merge fetch trans into load trans
	if (ifetch != old_trans->ifetch) return;
	
	// Lddas, etc will be different
	ASSERT(phys_addr>>8 == old_trans->phys_addr>>8 || g_conf_per_syscall_kern_stack
    || g_conf_per_syscall_kern_map);
	ASSERT(virt_addr>>8 == old_trans->virt_addr>>8 || g_conf_per_syscall_kern_stack
     || g_conf_per_syscall_kern_map);
	ASSERT(size == old_trans->size);
	
	request_time = old_trans->request_time;
	events = old_trans->events;
	itlb_miss = old_trans->itlb_miss;
    cache_prefetch = old_trans->cache_prefetch;
    prefetch_type = old_trans->prefetch_type;
	vcpu_state_transfer = old_trans->vcpu_state_transfer;
	sequencer_ctxt = old_trans->sequencer_ctxt;
}

bool mem_trans_t::is_as_user_asi()
{
    return (asi == 0x10 || asi == 0x11 || asi == 0x18 || asi == 0x19 ||
        asi == 0x70 || asi == 0x71 || asi == 0x78 || asi == 0x79);
}

kernel_access_type_t mem_trans_t::get_access_type()
{
    uint64 masked_va = (virt_addr >> 16) & KERNEL_MASK ;
    if (masked_va == LWP_STACK_ACCESS_MASK) return LWP_STACK_ACCESS;
    if (masked_va == KERNEL_MAP_ACCESS_LOWER || masked_va == KERNEL_MAP_ACCESS_UPPER)
        return KERNEL_MAP_ACCESS;
    if (is_as_user_asi()) return AS_USER_KERNEL_ACCESS;
    return OTHER_TYPE_ACCESS;
    
}
