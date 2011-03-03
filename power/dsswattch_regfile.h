/*
 * dsswattch_regfile.h - Implementation of DSSWattch's register files
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

#ifndef _DSSWATTCH_REGFILE_H
#define _DSSWATTCH_REGFILE_H


#define NUM_REGFILES 2
#define WATTCH_INT_REGFILE wattch_regfiles[0]
#define WATTCH_FP_REGFILE wattch_regfiles[1]

class wattch_regfile {
    
    public:
    
	/* Configurable Parameters */
	char * longname;
	char * shortname;
	int data_width;
	int num_regs;
	/* Per-Cycle Counters */
	counter_t regfile_access;
	counter_t total_pop_count_cycle;
	counter_t num_pop_count_cycle;
	/* Per-Cycle parameters */
	double af; /* Dynamic Activity Factor */
	/* Power parameters */
	double regfile_driver;
	double regfile_decoder;
	double regfile_wordline;
	double regfile_bitline;
	double regfile_senseamp;
	double regfile_power;
	double regfile_power_nobit;
	/* Overall Statistics */
	counter_t total_regfile_access;
	counter_t max_regfile_access;
	double regfile_total_power;
	double regfile_total_power_cc1;
	double regfile_total_power_cc2;
	double regfile_total_power_cc3;
    
    wattch_regfile();
    wattch_access_regfile();
    
};

extern wattch_regfile wattch_regfiles[NUM_REGFILES];
extern const int int_data_width;
extern const int fp_data_width;
extern int total_num_regs;

void wattch_init_regfiles();
inline void wattch_access_regfile(int reg_num, uint64 value); 

#endif /* _DSSWATTCH_REGFILE_H */
