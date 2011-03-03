/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: cmp_mmu_iface.h,v 1.1.2.1 2006/01/13 21:26:19 pwells Exp $
 *
 * description:    interface definition for CMP mmu
 * initial author: Philip Wells 
 *
 */
 
#ifndef _CMP_MMU_IFACE_H_
#define _CMP_MMU_IFACE_H_

#include "simics/core/configuration.h"

typedef struct cmp_mmu_iface {
	// Check TLB for a translation for VA in mem_trans
	// Return true if a translation found, and return daccess_t and tagread_t
	// Don't perform access checks
	int (*external_lookup)(conf_object_t *obj, v9_memory_transaction_t *,
	                        int, void **, void **);
	int (*demap_asi)(conf_object_t *mmu_obj, generic_transaction_t *gen_op);
} cmp_mmu_iface_t;

#endif // _CMP_MMU_IFACE_H_ 

