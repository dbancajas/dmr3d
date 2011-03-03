/* Copyright (c) 2004 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: ms2_mmu_iface.h,v 1.1 2005/02/03 16:11:41 pwells Exp $
 *
 * description:    CMP MMU interface definition
 * initial author: Philip Wells 
 *
 */
 
#ifndef _MS2_MMU_IFACE_H_
#define _MS2_MMU_IFACE_H_

// Alternate Interface to mmu ASI handlers 
#if defined(__cplusplus)
class mmu_asi_interface_t {
public:

#else // !__cplusplus
typedef struct {
#endif // !__cplusplus

	exception_type_t (*mmu_handle_asi) (conf_object_t *mmu_obj,
		generic_transaction_t *mem_op);
	exception_type_t (*mmu_data_in_asi) (conf_object_t *mmu_obj,
		generic_transaction_t *mem_op);
	exception_type_t (*mmu_data_access_asi) (conf_object_t *mmu_obj,
		generic_transaction_t *mem_op);
	exception_type_t (*mmu_tag_read_asi) (conf_object_t *mmu_obj,
		generic_transaction_t *mem_op);
	exception_type_t (*mmu_demap_asi) (conf_object_t *mmu_obj,
		generic_transaction_t *mem_op);

#if defined(__cplusplus)
};
#else // !__cplusplus
} mmu_asi_interface_t;
#endif // !__cplusplus

#endif // _MS2_MMU_IFACE_H_

