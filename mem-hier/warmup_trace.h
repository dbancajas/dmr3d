/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: warmup_trace.h,v 1.1.2.1 2006/12/12 18:36:58 pwells Exp $
 *
 * description:    translates between the simics 'timing-model' and mem-hier
 *                 interfaces
 * initial author: Philip Wells 
 *
 */
 
#ifndef _WARMUP_TRACE_H_
#define _WARMUP_TRACE_H_


class warmup_trace_t : public mem_hier_handle_iface_t {
	
 public:
	
	warmup_trace_t(mem_hier_t *_mem_hier, proc_object_t *_proc);
	
	cycles_t operate(conf_object_t *proc_obj, generic_transaction_t *mem_op);
	cycles_t snoop_operate(conf_object_t *proc_obj, generic_transaction_t *mem_op);
		
	// Memhier interface
	void complete_request(conf_object_t *obj, conf_object_t *cpu, 
		mem_trans_t *trans);
	void invalidate_address(conf_object_t *obj, conf_object_t *cpu, 
		invalidate_addr_t *invalid_addr);
	
private:

	bool send_next_trans();

	mem_hier_t *mem_hier;
	proc_object_t *proc;

	deque<uint32> **recent_trans;
	
	uint32 num_transactions;
	
	FILE *trace_fp;
	bool trace_fp_closed;

};

#endif /* _WARMUP_TRACE_H */
