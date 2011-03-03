/*
 * dsswattch.h -  code used to interface Wattch with DSS.
 *
 * This file is part of the Dynamic SimpleScalar tool suite released by
 * the Architecture and Language Implementation Laboratory at the
 * University of Massachusetts, Amherst. It is derived from an earlier
 * version of Dynamic SimpleScalar developed at the University of Texas,
 * Austin and the University of Massachusetts, Amherst. This was in turn
 * derived from the SimpleScalar Tool Suite written by Todd Austin.
 * 
 * This file contains code which is part of the WATTCH power estimation
 * extenstions to SimpleScalar.  WATTCH is the work of David Brooks with
 * assistance from Vivek Tiwari and under the direction of Professor
 * Margaret Martonosi of Princeton University.
 *
 * The tool suite is currently maintained by Chris Hoffmann, and 
 * administrative contact is Prof. Eliot Moss, both at the University of
 * Massachusetts, Amherst.
 *
 * Copyright (C) 2004 by Architecture & Language Implementation Lab
 *
 * This file is based on a file in the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 * 
 * Copyright (C) 1994, 1995, 1996, 1997, 1998 by Todd M. Austin
 *
 * This source file is distributed "as is" in the hope that it will be
 * useful.  The tool set comes with no warranty, and no author or
 * distributor accepts any responsibility for the consequences of its
 * use. 
 * 
 * Everyone is granted permission to copy, modify and redistribute
 * this tool set under the following conditions:
 * 
 *    This source code is distributed for non-commercial use only. 
 *    Please contact the maintainer of SimpleScalar at info@simplescalar.com
 *    for restrictions applying to commercial use.
 *
 *    Permission is granted to anyone to make or distribute copies
 *    of this source code, either as received or modified, in any
 *    medium, provided that all copyright notices, permission and
 *    nonwarranty notices are preserved, and that the distributor
 *    grants the recipient permission for further redistribution as
 *    permitted by this document.
 *
 *    Permission is granted to distribute this file in compiled
 *    or executable form under the same conditions that apply for
 *    source code, provided that either:
 *
 *    A. it is accompanied by the corresponding machine-readable
 *       source code,
 *    B. it is accompanied by a written offer, with no time limit,
 *       to give anyone a machine-readable copy of the corresponding
 *       source code in return for reimbursement of the cost of
 *       distribution.  This written offer must permit verbatim
 *       duplication by anyone, or
 *    C. it is distributed by someone who received only the
 *       executable form, and is accompanied by a copy of the
 *       written offer of source code that they received concurrently.
 *
 * In other words, you are welcome to use, share and improve this
 * source file.  You are forbidden to forbid anyone else to use, share
 * and improve what you give them.
 *
 * INTERNET: dss-users@cs.umass.edu
 * WEB: http://www-ali.cs.umass.edu/DSS/index.html
 * US Mail: Computer Science Building, Amherst, MA 01003
 *
 * $Id: dsswattch.h,v 1.11 2004/07/07 20:41:47 jdinan Exp $
 *
 * $Log: dsswattch.h,v $
 * Revision 1.11  2004/07/07 20:41:47  jdinan
 * Fixes and cleanups
 *
 * Revision 1.10  2004/06/28 21:22:23  jdinan
 * Code cleanups.  Removed new table-lookup pop_count because of negligible
 * performance improvement.
 *
 * Revision 1.9  2004/06/28 15:57:58  hoffmann
 * Added missing endif (and a copyright notice).
 *
 *
 */


#ifndef DSSWATTCH_H
#define DSSWATTCH_H

#include "dsswattch_regfile.h"
 
/* Some build-time constraints */
#ifdef BUILD_WATTCH
#ifdef ADV_MEM_MODEL
#error WATTCH Power Simulation is only compatible with the fast memory model.
#endif /* ADV_MEM_MODEL */
#endif /* BUILD_WATTCH */


/* Build-Conditional Macros */
#ifdef BUILD_WATTCH
#define WATTCH_INC(v) ((v)++);
#define WATTCH_ASSIGN(var,val) ((var) = (val));
#define WATTCH_ASSIGN_REG(var,val) ((var) = fetch_reg(val));
#else
#define WATTCH_INC(v) 
#define WATTCH_ASSIGN(var,val) 
#define WATTCH_ASSIGN_REG(var,val)
#endif /* BUILD_WATTCH */


/* Operand Handling Snippets */
#ifdef BUILD_WATTCH

#define SOME_OPERANDS_READY(RS)							\
	((RS)->idep_ready[0] || (RS)->idep_ready[1] || (RS)->idep_ready[2] ||	\
	 (RS)->idep_ready[3] || (RS)->idep_ready[4])

#endif /* BUILD_WATTCH */


/* Dynamic Activity Factor (AF) Macros */
#ifdef DYNAMIC_AF
#define POP_COUNT(bus_name, input)					\
	SYMCAT(bus_name,_total_pop_count_cycle) += pop_count64(input);	\
	SYMCAT(bus_name,_num_pop_count_cycle)++;
#define IF_DYNAMIC_AF(foo) foo
#else
#define POP_COUNT(bus_name, input)
#define IF_DYNAMIC_AF(foo)
#endif /* DYNAMIC_AF */


/* Power counting handlers for each pipe-stage */
#ifdef BUILD_WATTCH

/*********************************************************/
/* DISPATCH: Count power for IDEPs in the dispatch stage */
#define WATTCH_DISPATCH_IDEP(rs, num)			\
	if ((rs)->wattch_ideps[(num)] > 0 && (rs)->wattch_ideps[(num)] <= 32) {	\
		/* int registers */			\
		window_access++;			\
		window_preg_access++;			\
							\
		IF_DYNAMIC_AF(				\
			WATTCH_INT_REGFILE.total_pop_count_cycle += pop_count32((rs)->wattch_inputs[(num)]);	\
			WATTCH_INT_REGFILE.num_pop_count_cycle++;	\
		) 					\
	} else if ((rs)->wattch_ideps[(num)] <= 64) {	\
		  /* fp registers */			\
		  window_access++;			\
		  window_preg_access++;			\
							\
		IF_DYNAMIC_AF(				\
			WATTCH_FP_REGFILE.total_pop_count_cycle += pop_count64((rs)->wattch_inputs[(num)]);	\
			WATTCH_FP_REGFILE.num_pop_count_cycle++;	\
		) 					\
	} else {					\
		/* Control register access */		\
	}						\

#define WATTCH_DISPATCH_IDEP_IF_READY(rs, num) if ((rs)->idep_ready[(num)]) { WATTCH_DISPATCH_IDEP((rs),(num)); }
	

/*************************************************************/
/* ISSUE: Count power for ideps in the issue stage           */
#define WATTCH_ISSUE_IDEP(rs, num)						\
	/* If this is an integer or fp register, lets count it */		\
	if ((rs)->wattch_ideps[(num)] > 0 && (rs)->wattch_ideps[(num)] <= 64) {	\
		window_preg_access++;						\
		POP_COUNT(window, (rs)->wattch_inputs[(num)]);			\
	}

	
/**************************************************************/
/* WRITEBACK: Update power counters as odeps are written back */
#define WATTCH_WRITEBACK_ODEP(rs, num)						\
	/* If this is an integer or fp register, lets count it */		\
	if ((rs)->wattch_ideps[(num)] > 0 && (rs)->wattch_ideps[(num)] <= 64) {	\
		window_access++;						\
		window_preg_access++;						\
		window_wakeup_access++;						\
		resultbus_access++;						\
										\
		POP_COUNT(window, (rs)->wattch_outputs[(num)]);			\
		POP_COUNT(resultbus, (rs)->wattch_outputs[(num)]);		\
	}

#endif /* BUILD_WATTCH */




#endif /* DSSWATTCH_H */
