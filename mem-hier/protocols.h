/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: protocols.h,v 1.1.1.1.10.7 2005/08/24 19:20:05 pwells Exp $
 *
 * description:    list of protocol definitions
 * initial author: Philip Wells 
 *
 */
 
#ifndef _PROTOCOLS_H_
#define _PROTOCOLS_H_

#include "default_msg.h"
#include "simple_proc_msg.h"
#include "simple_mainmem_msg.h"

// New
#ifdef COMPILE_CMP_INCL
class cmp_incl_l2_line_t;
class cmp_incl_l2_cache_t;
class cmp_incl_l1_line_t;
class cmp_incl_l1_cache_t;

#include "cmp_incl_msg.h"
#include "cmp_incl_l2.h"
#include "cmp_incl_l1.h"

#endif /* COMPILE_CMP_INCL */

#ifdef COMPILE_CMP_INCL_WT

class cmp_incl_wt_l2_line_t;
class cmp_incl_wt_l2_cache_t;
class cmp_incl_wt_l1_line_t;
class cmp_incl_wt_l1_cache_t;

#include "cmp_incl_wt_msg.h"
#include "cmp_incl_wt_l2.h"
#include "cmp_incl_wt_l1.h"

#endif /* COMPILE_CMP_INCL_WT */


#ifdef COMPILE_CMP_EXCL
class cmp_excl_l2_line_t;
class cmp_excl_l2_cache_t;
class cmp_excl_l1_line_t;
class cmp_excl_l1_cache_t;
class cmp_excl_l1dir_line_t;
class cmp_excl_l1dir_array_t;

#include "cmp_excl_msg.h"
#include "cmp_excl_l2.h"
#include "cmp_excl_l1.h"
#include "cmp_excl_l1dir.h"

#endif /* COMPILE_CMP_EXCL */

#ifdef COMPILE_CMP_EX_3L
class cmp_excl_3l_l3_line_t;
class cmp_excl_3l_l3_cache_t;
class cmp_excl_3l_l2_line_t;
class cmp_excl_3l_l2_cache_t;
class cmp_excl_3l_l1_line_t;
class cmp_excl_3l_l1_cache_t;
class cmp_excl_3l_l2dir_line_t;
class cmp_excl_3l_l2dir_array_t;

#include "cmp_excl_3l_msg.h"
#include "cmp_excl_3l_l3.h"
#include "cmp_excl_3l_l2.h"
#include "cmp_excl_3l_l1.h"
#include "cmp_excl_3l_l2dir.h"

#endif /* COMPILE_CMP_EX_3L */



#ifdef COMPILE_CMP_INCL_3L
class cmp_incl_3l_l3_line_t;
class cmp_incl_3l_l3_cache_t;
class cmp_incl_3l_l2_line_t;
class cmp_incl_3l_l2_cache_t;
class cmp_incl_3l_l1_line_t;
class cmp_incl_3l_l1_cache_t;

#include "cmp_incl_3l_msg.h"
#include "cmp_incl_3l_l3.h"
#include "cmp_incl_3l_l2.h"
#include "cmp_incl_3l_l1.h"

#endif /* COMPILE_CMP_INCL_3L */


#ifdef COMPILE_UNIP_TWO_DRAM
class unip_two_l1d_prot_sm_t;
class unip_two_l1d_cache_t;
class unip_two_l1d_line_t;
class unip_two_l1i_prot_sm_t;
class unip_two_l1i_cache_t;
class unip_two_l1i_line_t;
class unip_two_l2_prot_sm_t;
class unip_two_l2_cache_t;
class unip_two_l2_line_t;

#include "unip_two_msg.h"
#include "unip_two.h"
#include "unip_two_l1i.h"
#include "unip_two_l1d.h"
#include "unip_two_l2.h"

#endif /* COMPILE_UNIP_TWO_DRAM */

#ifdef COMPILE_UNIP_TWO
class unip_two_l1d_prot_sm_t;
class unip_two_l1d_cache_t;
class unip_two_l1d_line_t;
class unip_two_l1i_prot_sm_t;
class unip_two_l1i_cache_t;
class unip_two_l1i_line_t;
class unip_two_l2_prot_sm_t;
class unip_two_l2_cache_t;
class unip_two_l2_line_t;

#include "unip_two_msg.h"
#include "unip_two.h"
#include "unip_two_l1i.h"
#include "unip_two_l1d.h"
#include "unip_two_l2.h"

#endif /* COMPILE_UNIP_TWO */


class unip_one_prot_t;

#endif // _PROTOCOLS_H_
