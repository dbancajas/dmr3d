/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: mem_hier_iface.h,v 1.3 2005/02/02 17:53:30 chang Exp $
 *
 * description:    memory hierarchy interface definition
 * initial author: Philip Wells 
 *
 */
 
#ifndef MEM_HIER_IFACE_H
#define MEM_HIER_IFACE_H

class mem_trans_t;
class invalidate_addr_t;

#include "simics/core/configuration.h"


// Processor's interface to the memory hierarchy
class mem_hier_interface_t {
public:

    // Called once per memory transaction by the processor
    //    Every request will call completeRequest below, whether
    //    the orig instruction has been squashed or not.
	void (*make_request)(conf_object_t *obj, conf_object_t *cpu, mem_trans_t*);

	/* MS2 check cache before commits, cache returns false if needs retry */
	bool (*check_at_commit)(conf_object_t *obj, mem_trans_t*); 
};


//  Mem hierarchie's interface to the processor
class proc_interface_t {
public:
    
    // Called by the memory hierarchy when the previously requested
    // transaction is complete
	void (*complete_request)(conf_object_t *obj, conf_object_t *cpu, mem_trans_t*);
	
	// Called by mem hier when the a range of addresses is invalidated that
	// may have supplied speculative loads
	// Processor must ensure that any speculative loads (and dependant 
	// instructions) are reissued 
	// invalidate_addr_t* is only valid until the function returns
	void (*invalidate_address)(conf_object_t *obj, conf_object_t *cpu, invalidate_addr_t*);
	
	// Called in response to a checkpoint request
	// processor should cease fetching instructions and drain pipe
	// Returns true if processor is in a checkpointable state
	// This function will be called multiple times until it returns true
	bool (*stall_processor)(conf_object_t *obj);
};


#endif /* MEM_HIER_IFACE_H */

