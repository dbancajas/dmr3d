
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 *

 * description:    Implementation of dss register files
 * initial author: Koushik Chakraborty
 *
 */ 




/*
 * dsswattch_regfile.c - Implementation of DSSWattch's register files
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
 */

 
//  #include "simics/first.h"
// RCSID("$Id: dsswattch_regfiles.cc,v 1.10.2.17 2006/03/02 23:58:42 kchak Exp $");

#include "definitions.h"
#include "config_extern.h" 
#include "dsswattch_regfile.h"

#define MD_NUM_IREGS 32
#define MD_NUM_FREGS 32
#define DFTMP 71

wattch_regfile wattch_regfiles[NUM_REGFILES];
const int int_data_width = 32;
const int fp_data_width = 64;
int total_num_regs = 0;

void wattch_init_regfiles()
{
	int i;

	/* Configure your register file parameters here */
	WATTCH_INT_REGFILE.longname = "Integer Register File";
	WATTCH_FP_REGFILE.longname = "Floating-Point Register File";

	/* The shortnames must be unique - they are used by the stats package */
	WATTCH_INT_REGFILE.shortname = "int_regfile";
	WATTCH_FP_REGFILE.shortname = "fp_regfile";

	WATTCH_INT_REGFILE.data_width = int_data_width;
	WATTCH_FP_REGFILE.data_width = fp_data_width;

	WATTCH_INT_REGFILE.num_regs = MD_NUM_IREGS;
	WATTCH_FP_REGFILE.num_regs = MD_NUM_FREGS;

	WATTCH_INT_REGFILE.regfile_total_power = 0;
	WATTCH_INT_REGFILE.regfile_total_power_cc1 = 0;
	WATTCH_INT_REGFILE.regfile_total_power_cc2 = 0;
	WATTCH_INT_REGFILE.regfile_total_power_cc3 = 0;
	WATTCH_INT_REGFILE.total_regfile_access = 0;
	WATTCH_INT_REGFILE.max_regfile_access = 0;

	WATTCH_FP_REGFILE.regfile_total_power = 0;
	WATTCH_FP_REGFILE.regfile_total_power_cc1 = 0;
	WATTCH_FP_REGFILE.regfile_total_power_cc2 = 0;
	WATTCH_FP_REGFILE.regfile_total_power_cc3 = 0;
	WATTCH_FP_REGFILE.total_regfile_access = 0;
	WATTCH_FP_REGFILE.max_regfile_access = 0;

	for (i = 0; i < NUM_REGFILES; i++)
	{
		total_num_regs += wattch_regfiles[i].num_regs;
	}
}

inline void wattch_access_regfile(int reg_num, uint64 value)
{
	/* This function generates an access to a register file as well as a
	   population count. */
	
	/* Customize this to handle each register file in wattch_regfiles[] */

	if (!reg_num)
		return; /* This is DNA */
	else if (reg_num < 0)
		ASSERT_MSG(0, "wattch_access_regfile(): Register number out of bounds ");
    //FAIL("wattch_access_regfile(): Register number out of bounds %d!", reg_num);
	else if (reg_num <= 32)
	{
		/* Integer Register File */
		WATTCH_INT_REGFILE.regfile_access++;
#ifdef DYNAMIC_AF
		WATTCH_INT_REGFILE.total_pop_count_cycle += pop_count32(value);
		WATTCH_INT_REGFILE.num_pop_count_cycle++;
#endif /* DYNAMIC_AF */
	}
	else if (reg_num <= 64)
	{
		/* Floating-Point Register File */
		WATTCH_FP_REGFILE.regfile_access++;
#ifdef DYNAMIC_AF
		WATTCH_FP_REGFILE.total_pop_count_cycle += pop_count64(value);
		WATTCH_FP_REGFILE.num_pop_count_cycle++;
#endif /* DYNAMIC_AF */
	}
	else if (reg_num <= DFTMP) /* DFTMP is the last special purpose register */
	{
		/* Special-Purpose Registers */
		// Ignored for now ...
	}
	else 
		ASSERT_MSG(0, "wattch_access_regfile(): Register number out of bounds");
}

