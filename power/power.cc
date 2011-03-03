
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Implementation file for power estimation in ms2sim
 * initial author: Koushik Chakraborty
 *
 */ 



/* This file contains code which is part of the WATTCH power estimation
 * extenstions to SimpleScalar.  WATTCH is the work of David Brooks with
 * assistance from Vivek Tiwari and under the direction of Professor
 * Margaret Martonosi of Princeton University.
 *
 * WATTCH was adapted for use in dynamic SimpleScalar by James Dinan.
 */

/* I inclued this copyright since we're using Cacti for some stuff */

/*------------------------------------------------------------
 *  Copyright 1994 Digital Equipment Corporation and Steve Wilton
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-
 * free right and license under any changes, enhancements or extensions
 * made to the core functions of the software, including but not limited to
 * those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to Digital any
 * such changes, enhancements or extensions that they make and inform Digital
 * of noteworthy uses of this software.  Correspondence should be provided
 * to Digital at:
 *
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/

 

//  #include "simics/first.h"
// RCSID("$Id: power.cc,v 1.10.2.17 2006/03/02 23:58:42 kchak Exp $");

#include "definitions.h"
#include "config_extern.h" 
 

#include <math.h>
#include "isa.h"
#include "dynamic.h"
#include "counter.h"
#include "stats.h"
#include "profiles.h"
#include "basic_circuit.h"

#include "dsswattch_regfile.h"
#include "power_def.h"
#include "power.h"
#include "power_util.h"
#include "verbose_level.h"


#define SensePowerfactor (Mhz)*(Vdd/2)*(Vdd/2)
#define Sense2Powerfactor (Mhz)*(2*.3+.1*Vdd)
#define Powerfactor (Mhz)*Vdd*Vdd
#define LowSwingPowerfactor (Mhz)*.2*.2
/* set scale for crossover (vdd->gnd) currents */
double crossover_scaling = 1.2;
/* set non-ideal turnoff percentage */
double turnoff_factor = 0.1;



/*----------------------------------------------------------------------*/

/* static power model results */
power_result_type power;

int pow2(int x) {
  return((int)pow(2.0,(double)x));
}

double logfour(double x)
{
  if (x<=0) VERBOSE_OUT(verb_t::requests, "%e\n",x);
  return( (double) (log(x)/log(4.0)) );
}

/* safer pop count to validate the fast algorithm */
int pop_count_slow(uint64 bits)
{
  int count = 0; 
  uint64 tmpbits = bits; 
  while (tmpbits) { 
    if (tmpbits & 1) ++count; 
    tmpbits >>= 1; 
  } 
  return count; 
}

inline int pop_count32(unsigned int bits)
{
  /* Population Count: Count the number of ones in a binary number
   * 
   * ONES produces a variable of size T that is all ones.
   * TWO  calculates 2^k.
   * CYCL produces numbers with bits that alternate is groups.  eg:
   *        CYCL(0) = 0x55555555 
   *        CYCL(1) = 0x33333333
   *        CYCL(2) = 0x0F0F0F0F
   *        CYCL(3) = 0x00FF00FF
   *        CYCL(4) = 0x0000FFFF
   * BSUM takes a variable, x, and counts the number of bits in each group of
   *      size 2^k, packing this value into groups of size 2^(k+1). 
   */
#define T unsigned int
#define ONES ((T)(~0)) 
#define TWO(k) ((T)1 << (k)) 
#define CYCL(k) (ONES/(1 + (TWO(TWO(k))))) 
#define BSUM(x,k) ((x) = (((x) & CYCL(k)) + (((x) >> TWO(k)) & CYCL(k))))
  unsigned int x = (unsigned int) bits;
  BSUM(x,0);
  BSUM(x,1);
  BSUM(x,2);
  BSUM(x,3);
  BSUM(x,4);
  /* BSUM(x,5); */ /* Uncomment me to make this 64-bit, change T too */
  return x;
#undef T
#undef ONES
#undef TWO
#undef CYCL
#undef BSUM
}

inline int pop_count64(uint64 bits)
{
#define T uint64
#define ONES ((T)(~0)) 
#define TWO(k) ((T)1 << (k)) 
#define CYCL(k) (ONES/(1 + (TWO(TWO(k))))) 
#define BSUM(x,k) ((x) = (((x) & CYCL(k)) + (((x) >> TWO(k)) & CYCL(k))))
  uint64 x = bits;
  BSUM(x,0);
  BSUM(x,1);
  BSUM(x,2);
  BSUM(x,3);
  BSUM(x,4);
  BSUM(x,5);
  return x;
#undef T
#undef ONES
#undef TWO
#undef CYCL
#undef BSUM
}


// extern int g_conf_max_fetch;
// extern int g_conf_max_issue;
// extern int g_conf_max_commit;
// extern int g_conf_window_size;
// extern int g_conf_lsq_load_size;
// extern int res_ialu;
// extern int res_fpalu;
// extern int res_memport;

int nvreg_width;
int npreg_width;

// extern int bimod_config[];

// extern struct cache_t *cache_dl1;
// extern struct cache_t *cache_il1;
// extern struct cache_t *cache_dl2;

// extern struct cache_t *dtlb;
// extern struct cache_t *itlb;

// /* 2-level predictor config (<l1size> <l2size> <hist_size> <xor>) */
// extern int twolev_config[];

// /* combining predictor config (<meta_table_size> */
// extern int comb_config[];


/* BTB predictor config (<num_sets> <associativity>) */
//extern int btb_config[];

// transferred to power_util
//double global_clockcap;


static double last_single_total_cycle_power_cc1 = 0.0;
static double last_single_total_cycle_power_cc2 = 0.0;
static double last_single_total_cycle_power_cc3 = 0.0;
static double current_total_cycle_power_cc1;
static double current_total_cycle_power_cc2;
static double current_total_cycle_power_cc3;

static double max_cycle_power_cc1 = 0.0;
static double max_cycle_power_cc2 = 0.0;
static double max_cycle_power_cc3 = 0.0;




power_t::power_t(string name)
    : profile_entry_t (name)
{
    
    total_rename_access= stats->COUNTER_BASIC("total_rename_access", " ");
    total_bpred_access=0;
    total_window_access=0;
    total_lsq_access=0;
    total_regfile_access=0;
    total_icache_access=0;
    total_dcache_access=0;
    total_dcache2_access=0;
    total_alu_access=0;
    total_resultbus_access=0;
    bct = new basic_circuit_t();    
    
     rename_power = stats->COUNTER_DOUBLE("rename_power", "Rename power");
     bpred_power = stats->COUNTER_DOUBLE("bpred_power", "Branch Predictor power");
     window_power = stats->COUNTER_DOUBLE("window_power", "Window power");
     lsq_power = stats->COUNTER_DOUBLE("lsq_power", "LSQ power");
     regfile_power = stats->COUNTER_DOUBLE("regfile_power", "Power consumed in the register file");
     icache_power = stats->COUNTER_DOUBLE("icache_power", "Power consumed in the Instruction Cache");
     dcache_power = stats->COUNTER_DOUBLE("dcache_power", "Power Consumed in the Data Cache");
     dcache2_power = stats->COUNTER_DOUBLE("dcache2_power", "Power consumed in the Data Cache 2");
     alu_power = stats->COUNTER_DOUBLE("alu_power", "Power consumed in the ALU");
     falu_power = stats->COUNTER_DOUBLE("falu_power", "Power consumed in the Float ALU");
     resultbus_power = stats->COUNTER_DOUBLE("resultbus_power", "Power consumed in the resultbus");
     clock_power = stats->COUNTER_DOUBLE("clock_power", "Power consumed in the clocking network");
     
     rename_power_cc1  = stats->COUNTER_DOUBLE("rename_power_cc1 ", " Power in rename_power_cc1 ");
     bpred_power_cc1  = stats->COUNTER_DOUBLE("bpred_power_cc1 ", " Power in bpred_power_cc1 ");
     window_power_cc1  = stats->COUNTER_DOUBLE("window_power_cc1 ", " Power in window_power_cc1 ");
     lsq_power_cc1  = stats->COUNTER_DOUBLE("lsq_power_cc1 ", " Power in lsq_power_cc1 ");
     regfile_power_cc1  = stats->COUNTER_DOUBLE("regfile_power_cc1 ", " Power in regfile_power_cc1 ");
     icache_power_cc1  = stats->COUNTER_DOUBLE("icache_power_cc1 ", " Power in icache_power_cc1 ");
     dcache_power_cc1  = stats->COUNTER_DOUBLE("dcache_power_cc1 ", " Power in dcache_power_cc1 ");
     dcache2_power_cc1  = stats->COUNTER_DOUBLE("dcache2_power_cc1 ", " Power in dcache2_power_cc1 ");
     alu_power_cc1  = stats->COUNTER_DOUBLE("alu_power_cc1 ", " Power in alu_power_cc1 ");
     resultbus_power_cc1  = stats->COUNTER_DOUBLE("resultbus_power_cc1 ", " Power in resultbus_power_cc1 ");
     clock_power_cc1 = stats->COUNTER_DOUBLE("clock_power_cc1", " Power in clock_power_cc1");


     rename_power_cc2  = stats->COUNTER_DOUBLE("rename_power_cc2 ", " Power in rename_power_cc2 ");
     bpred_power_cc2  = stats->COUNTER_DOUBLE("bpred_power_cc2 ", " Power in bpred_power_cc2 ");
     window_power_cc2  = stats->COUNTER_DOUBLE("window_power_cc2 ", " Power in window_power_cc2 ");
     lsq_power_cc2  = stats->COUNTER_DOUBLE("lsq_power_cc2 ", " Power in lsq_power_cc2 ");
     regfile_power_cc2  = stats->COUNTER_DOUBLE("regfile_power_cc2 ", " Power in regfile_power_cc2 ");
     icache_power_cc2  = stats->COUNTER_DOUBLE("icache_power_cc2 ", " Power in icache_power_cc2 ");
     dcache_power_cc2  = stats->COUNTER_DOUBLE("dcache_power_cc2 ", " Power in dcache_power_cc2 ");
     dcache2_power_cc2  = stats->COUNTER_DOUBLE("dcache2_power_cc2 ", " Power in dcache2_power_cc2 ");
     alu_power_cc2  = stats->COUNTER_DOUBLE("alu_power_cc2 ", " Power in alu_power_cc2 ");
     resultbus_power_cc2  = stats->COUNTER_DOUBLE("resultbus_power_cc2 ", " Power in resultbus_power_cc2 ");
     clock_power_cc2  = stats->COUNTER_DOUBLE("clock_power_cc2 ", " Power in clock_power_cc2 ");


     rename_power_cc3  = stats->COUNTER_DOUBLE("rename_power_cc3 ", " Power in rename_power_cc3 ");
     bpred_power_cc3  = stats->COUNTER_DOUBLE("bpred_power_cc3 ", " Power in bpred_power_cc3 ");
     window_power_cc3  = stats->COUNTER_DOUBLE("window_power_cc3 ", " Power in window_power_cc3 ");
     lsq_power_cc3  = stats->COUNTER_DOUBLE("lsq_power_cc3 ", " Power in lsq_power_cc3 ");
     regfile_power_cc3  = stats->COUNTER_DOUBLE("regfile_power_cc3 ", " Power in regfile_power_cc3 ");
     icache_power_cc3  = stats->COUNTER_DOUBLE("icache_power_cc3 ", " Power in icache_power_cc3 ");
     dcache_power_cc3  = stats->COUNTER_DOUBLE("dcache_power_cc3 ", " Power in dcache_power_cc3 ");
     dcache2_power_cc3  = stats->COUNTER_DOUBLE("dcache2_power_cc3 ", " Power in dcache2_power_cc3 ");
     alu_power_cc3  = stats->COUNTER_DOUBLE("alu_power_cc3 ", " Power in alu_power_cc3 ");
     resultbus_power_cc3  = stats->COUNTER_DOUBLE("resultbus_power_cc3 ", " Power in resultbus_power_cc3 ");
     clock_power_cc3  = stats->COUNTER_DOUBLE("clock_power_cc3 ", " Power in clock_power_cc3 ");
     total_cycle_power = stats->COUNTER_DOUBLE("total_cycle_power", " Power in total_cycle_power");
     total_cycle_power_cc1 = stats->COUNTER_DOUBLE("total_cycle_power_cc1", " Power in total_cycle_power_cc1");
     total_cycle_power_cc2 = stats->COUNTER_DOUBLE("total_cycle_power_cc2", " Power in total_cycle_power_cc2");
     total_cycle_power_cc3 = stats->COUNTER_DOUBLE("total_cycle_power_cc3", " Power in total_cycle_power_cc3");


     last_single_total_cycle_power_cc1   = stats->COUNTER_DOUBLE("last_single_total_cycle_power_cc1  ", " Power in last_single_total_cycle_power_cc1  ");
     last_single_total_cycle_power_cc2   = stats->COUNTER_DOUBLE("last_single_total_cycle_power_cc2  ", " Power in last_single_total_cycle_power_cc2  ");
     last_single_total_cycle_power_cc3   = stats->COUNTER_DOUBLE("last_single_total_cycle_power_cc3  ", " Power in last_single_total_cycle_power_cc3  ");


     current_total_cycle_power_cc1 = stats->COUNTER_DOUBLE("current_total_cycle_power_cc1", " Power in current_total_cycle_power_cc1");
     current_total_cycle_power_cc2 = stats->COUNTER_DOUBLE("current_total_cycle_power_cc2", " Power in current_total_cycle_power_cc2");
     current_total_cycle_power_cc3 = stats->COUNTER_DOUBLE("current_total_cycle_power_cc3", " Power in current_total_cycle_power_cc3");

     max_cycle_power_cc1   = stats->COUNTER_DOUBLE("max_cycle_power_cc1  ", " Power in max_cycle_power_cc1  ");
     max_cycle_power_cc2   = stats->COUNTER_DOUBLE("max_cycle_power_cc2  ", " Power in max_cycle_power_cc2  ");
     max_cycle_power_cc3   = stats->COUNTER_DOUBLE("max_cycle_power_cc3  ", " Power in max_cycle_power_cc3  ");
    
    
}


void power_t::clear_access_stats()
{

  rename_access=0;
  bpred_access=0;
  window_access=0;
  lsq_access=0;
  icache_access=0;
  dcache_access=0;
  dcache2_access=0;
  alu_access=0;
  ialu_access=0;
  falu_access=0;
  resultbus_access=0;

  window_preg_access=0;
  window_selection_access=0;
  window_wakeup_access=0;
  lsq_store_data_access=0;
  lsq_load_data_access=0;
  lsq_wakeup_access=0;
  lsq_preg_access=0;

  window_total_pop_count_cycle=0;
  window_num_pop_count_cycle=0;
  lsq_total_pop_count_cycle=0;
  lsq_num_pop_count_cycle=0;
  resultbus_total_pop_count_cycle=0;
  resultbus_num_pop_count_cycle=0;

  for (uint32 i = 0; i < NUM_REGFILES; i++)
  {
    wattch_regfiles[i].regfile_access = 0;
    wattch_regfiles[i].total_pop_count_cycle = 0;
    wattch_regfiles[i].num_pop_count_cycle = 0;
  }
}

/* compute bitline activity factors which we use to scale bitline power 
   Here it is very important whether we assume 0's or 1's are
   responsible for dissipating power in pre-charged stuctures. (since
   most of the bits are 0's, we assume the design is power-efficient
   enough to allow 0's to _not_ discharge 
*/
/* compute power statistics on each cycle, for each conditional clocking style.  Obviously
most of the speed penalty comes here, so if you don't want per-cycle power estimates
you could post-process 

See README.wattch for details on the various clock gating styles.

*/
void power_t::update_power_stats()
{
  double window_af_b, lsq_af_b, resultbus_af_b;
  int i;

#ifdef DYNAMIC_AF
  window_af_b = power_util->compute_af(window_num_pop_count_cycle,window_total_pop_count_cycle,g_conf_data_width);
  lsq_af_b = power_util->compute_af(lsq_num_pop_count_cycle,lsq_total_pop_count_cycle,g_conf_data_width);
  resultbus_af_b = power_util->compute_af(resultbus_num_pop_count_cycle,resultbus_total_pop_count_cycle,g_conf_data_width);

  for (i = 0; i < NUM_REGFILES; i++)
  {
    wattch_regfiles[i].af = power_util->compute_af(wattch_regfiles[i].num_pop_count_cycle,
      wattch_regfiles[i].total_pop_count_cycle, wattch_regfiles[i].data_width);
  }
#endif
  
  rename_power->inc_total(power.rename_power);
  bpred_power->inc_total(power.bpred_power);
  window_power->inc_total(power.window_power);
  lsq_power->inc_total(power.lsq_power);
  icache_power->inc_total(power.icache_power+power.itlb);
  dcache_power->inc_total(power.dcache_power+power.dtlb);
  dcache2_power->inc_total(power.dcache2_power);
  alu_power->inc_total(power.ialu_power + power.falu_power);
  falu_power->inc_total(power.falu_power);
  resultbus_power->inc_total(power.resultbus);
  clock_power->inc_total(power.clock_power);

  total_rename_access+=rename_access;
  total_bpred_access+=bpred_access;
  total_window_access+=window_access;
  total_lsq_access+=lsq_access;
  total_icache_access+=icache_access;
  total_dcache_access+=dcache_access;
  total_dcache2_access+=dcache2_access;
  total_alu_access+=alu_access;
  total_resultbus_access+=resultbus_access;

  max_rename_access=MAX(rename_access,max_rename_access);
  max_bpred_access=MAX(bpred_access,max_bpred_access);
  max_window_access=MAX(window_access,max_window_access);
  max_lsq_access=MAX(lsq_access,max_lsq_access);
  max_icache_access=MAX(icache_access,max_icache_access);
  max_dcache_access=MAX(dcache_access,max_dcache_access);
  max_dcache2_access=MAX(dcache2_access,max_dcache2_access);
  max_alu_access=MAX(alu_access,max_alu_access);
  max_resultbus_access=MAX(resultbus_access,max_resultbus_access);
 
  total_regfile_access = 0;
  max_regfile_access = 0;
 
  for (i = 0; i < NUM_REGFILES; i++)
  {
    wattch_regfiles[i].regfile_total_power += wattch_regfiles[i].regfile_power;
    wattch_regfiles[i].total_regfile_access += wattch_regfiles[i].regfile_access;
    wattch_regfiles[i].max_regfile_access = MAX(wattch_regfiles[i].regfile_access, wattch_regfiles[i].max_regfile_access);

    total_regfile_access += wattch_regfiles[i].total_regfile_access;
    max_regfile_access = MAX(max_regfile_access, wattch_regfiles[i].max_regfile_access);
  }
        
  if(rename_access) {
    rename_power_cc1->inc_total(power.rename_power);
    rename_power_cc2->inc_total(((double)rename_access/(double)g_conf_max_fetch)*power.rename_power);
    rename_power_cc3->inc_total(((double)rename_access/(double)g_conf_max_fetch)*power.rename_power);
  }
  else 
    rename_power_cc3->inc_total(turnoff_factor*power.rename_power);

  if(bpred_access) {
    if(bpred_access <= 2)
      bpred_power_cc1->inc_total(power.bpred_power);
    else
      bpred_power_cc1->inc_total(((double)bpred_access/2.0) * power.bpred_power);
    bpred_power_cc2->inc_total(((double)bpred_access/2.0) * power.bpred_power);
    bpred_power_cc3->inc_total(((double)bpred_access/2.0) * power.bpred_power);
  }
  else
    bpred_power_cc3->inc_total(turnoff_factor*power.bpred_power);

#ifdef STATIC_AF
  if(window_preg_access) {
    if(window_preg_access <= 3*g_conf_max_issue)
      window_power_cc1+=power.rs_power;
    else
      window_power_cc1+=((double)window_preg_access/(3.0*(double)g_conf_max_issue))*power.rs_power;
    window_power_cc2+=((double)window_preg_access/(3.0*(double)g_conf_max_issue))*power.rs_power;
    window_power_cc3+=((double)window_preg_access/(3.0*(double)g_conf_max_issue))*power.rs_power;
  }
  else
    window_power_cc3+=turnoff_factor*power.rs_power;
#elif defined(DYNAMIC_AF)
  if(window_preg_access) {
    if(window_preg_access <= 3*(uint32)g_conf_max_issue)
      window_power_cc1->inc_total(power.rs_power_nobit + window_af_b*power.rs_bitline);
    else
      window_power_cc1->inc_total(((double)window_preg_access/(3.0*(double)g_conf_max_issue))*(power.rs_power_nobit + window_af_b*power.rs_bitline));
    window_power_cc2->inc_total(((double)window_preg_access/(3.0*(double)g_conf_max_issue))*(power.rs_power_nobit + window_af_b*power.rs_bitline));
    window_power_cc3->inc_total(((double)window_preg_access/(3.0*(double)g_conf_max_issue))*(power.rs_power_nobit + window_af_b*power.rs_bitline));
  }
  else
    window_power_cc3->inc_total(turnoff_factor*power.rs_power);
#else
  panic("no AF-style defined\n");
#endif

  if(window_selection_access) {
    if(window_selection_access <= (uint32) g_conf_max_issue)
      window_power_cc1->inc_total(power.selection);
    else
      window_power_cc1->inc_total(((double)window_selection_access/((double)g_conf_max_issue))*power.selection);
    window_power_cc2->inc_total(((double)window_selection_access/((double)g_conf_max_issue))*power.selection);
    window_power_cc3->inc_total(((double)window_selection_access/((double)g_conf_max_issue))*power.selection);
  }
  else
    window_power_cc3->inc_total(turnoff_factor*power.selection);

  if(window_wakeup_access) {
    if(window_wakeup_access <= (uint32)g_conf_max_issue)
      window_power_cc1->inc_total(power.wakeup_power);
    else
      window_power_cc1->inc_total(((double)window_wakeup_access/((double)g_conf_max_issue))*power.wakeup_power);
    window_power_cc2->inc_total(((double)window_wakeup_access/((double)g_conf_max_issue))*power.wakeup_power);
    window_power_cc3->inc_total(((double)window_wakeup_access/((double)g_conf_max_issue))*power.wakeup_power);
  }
  else
    window_power_cc3->inc_total(turnoff_factor*power.wakeup_power);

  if(lsq_wakeup_access) {
    if(lsq_wakeup_access <= res_memport)
      lsq_power_cc1->inc_total(power.lsq_wakeup_power);
    else
      lsq_power_cc1->inc_total(((double)lsq_wakeup_access/((double)res_memport))*power.lsq_wakeup_power);
    lsq_power_cc2->inc_total(((double)lsq_wakeup_access/((double)res_memport))*power.lsq_wakeup_power);
    lsq_power_cc3->inc_total(((double)lsq_wakeup_access/((double)res_memport))*power.lsq_wakeup_power);
  }
  else
    lsq_power_cc3->inc_total(turnoff_factor*power.lsq_wakeup_power);

#ifdef STATIC_AF
  if(lsq_preg_access) {
    if(lsq_preg_access <= res_memport)
      lsq_power_cc1+=power.lsq_rs_power;
    else
      lsq_power_cc1+=((double)lsq_preg_access/((double)res_memport))*power.lsq_rs_power;
    lsq_power_cc2+=((double)lsq_preg_access/((double)res_memport))*power.lsq_rs_power;
    lsq_power_cc3+=((double)lsq_preg_access/((double)res_memport))*power.lsq_rs_power;
  }
  else
    lsq_power_cc3+=turnoff_factor*power.lsq_rs_power;
#else
  if(lsq_preg_access) {
    if(lsq_preg_access <= res_memport)
      lsq_power_cc1->inc_total(power.lsq_rs_power_nobit + lsq_af_b*power.lsq_rs_bitline);
    else
      lsq_power_cc1->inc_total(((double)lsq_preg_access/((double)res_memport))*(power.lsq_rs_power_nobit + lsq_af_b*power.lsq_rs_bitline));
    lsq_power_cc2->inc_total(((double)lsq_preg_access/((double)res_memport))*(power.lsq_rs_power_nobit + lsq_af_b*power.lsq_rs_bitline));
    lsq_power_cc3->inc_total(((double)lsq_preg_access/((double)res_memport))*(power.lsq_rs_power_nobit + lsq_af_b*power.lsq_rs_bitline));
  }
  else
    lsq_power_cc3->inc_total(turnoff_factor*power.lsq_rs_power);
#endif

#ifdef STATIC_AF
  for (i = 0; i < NUM_REGFILES; i++)
  {
    if(wattch_regfiles[i].regfile_access) {
      if(wattch_regfiles[i].regfile_access <= (3* (uint32) g_conf_max_commit))
        wattch_regfiles[i].regfile_total_power_cc1 ->inc_total( wattch_regfiles[i].regfile_power);
      else
        wattch_regfiles[i].regfile_total_power_cc1 ->inc_total( ((double)wattch_regfiles[i].regfile_access/(3.0*(double)g_conf_max_commit))*wattch_regfiles[i].regfile_power);

      wattch_regfiles[i].regfile_total_power_cc2 ->inc_total( ((double)wattch_regfiles[i].regfile_access/(3.0*(double)g_conf_max_commit))*wattch_regfiles[i].regfile_power);
      wattch_regfiles[i].regfile_total_power_cc3 ->inc_total( ((double)wattch_regfiles[i].regfile_access/(3.0*(double)g_conf_max_commit))*wattch_regfiles[i].regfile_power);
    }
    else
      wattch_regfiles[i].regfile_total_power_cc3 ->inc_total( turnoff_factor*wattch_regfiles[i].regfile_power);
  }
#else
  for (i = 0; i < NUM_REGFILES; i++)
  {
    if(wattch_regfiles[i].regfile_access) {
      if(wattch_regfiles[i].regfile_access <= (3* (uint32) g_conf_max_commit))
        wattch_regfiles[i].regfile_total_power_cc1 += wattch_regfiles[i].regfile_power_nobit + wattch_regfiles[i].af*wattch_regfiles[i].regfile_bitline;
      else
        wattch_regfiles[i].regfile_total_power_cc1 += ((double)wattch_regfiles[i].regfile_access/(3.0*(double)g_conf_max_commit))*(wattch_regfiles[i].regfile_power_nobit + wattch_regfiles[i].af*wattch_regfiles[i].regfile_bitline);

      wattch_regfiles[i].regfile_total_power_cc2 += ((double)wattch_regfiles[i].regfile_access/(3.0*(double)g_conf_max_commit))*(wattch_regfiles[i].regfile_power_nobit + wattch_regfiles[i].af*wattch_regfiles[i].regfile_bitline);
      wattch_regfiles[i].regfile_total_power_cc3 += ((double)wattch_regfiles[i].regfile_access/(3.0*(double)g_conf_max_commit))*(wattch_regfiles[i].regfile_power_nobit + wattch_regfiles[i].af*wattch_regfiles[i].regfile_bitline);
    }
    else
      wattch_regfiles[i].regfile_total_power_cc3 += turnoff_factor*wattch_regfiles[i].regfile_power;
  }
#endif

  if(icache_access) {
    /* don't scale icache because we assume 1 line is fetched, unless fetch stalls */
    icache_power_cc1->inc_total(power.icache_power+power.itlb);
    icache_power_cc2->inc_total(power.icache_power+power.itlb);
    icache_power_cc3->inc_total(power.icache_power+power.itlb);
  }
  else
    icache_power_cc3->inc_total(turnoff_factor*(power.icache_power+power.itlb));

  if(dcache_access) {
    if(dcache_access <= res_memport)
      dcache_power_cc1->inc_total(power.dcache_power+power.dtlb);
    else
      dcache_power_cc1->inc_total(((double)dcache_access/(double)res_memport)*(power.dcache_power +
						     power.dtlb));
    dcache_power_cc2->inc_total(((double)dcache_access/(double)res_memport)*(power.dcache_power +
						   power.dtlb));
    dcache_power_cc3->inc_total(((double)dcache_access/(double)res_memport)*(power.dcache_power +
						   power.dtlb));
  }
  else
    dcache_power_cc3->inc_total(turnoff_factor*(power.dcache_power+power.dtlb));

  if(dcache2_access) {
    if(dcache2_access <= res_memport)
      dcache2_power_cc1->inc_total(power.dcache2_power);
    else
      dcache2_power_cc1->inc_total(((double)dcache2_access/(double)res_memport)*power.dcache2_power);
    dcache2_power_cc2->inc_total(((double)dcache2_access/(double)res_memport)*power.dcache2_power);
    dcache2_power_cc3->inc_total(((double)dcache2_access/(double)res_memport)*power.dcache2_power);
  }
  else
    dcache2_power_cc3->inc_total(turnoff_factor*power.dcache2_power);

  if(alu_access) {
    if(ialu_access)
      alu_power_cc1->inc_total(power.ialu_power);
    else
      alu_power_cc3->inc_total(turnoff_factor*power.ialu_power);
    if(falu_access)
      alu_power_cc1->inc_total(power.falu_power);
    else
      alu_power_cc3->inc_total(turnoff_factor*power.falu_power);

    alu_power_cc2->inc_total(((double)ialu_access/(double)res_ialu)*power.ialu_power +
      ((double)falu_access/(double)res_fpalu)*power.falu_power);
    alu_power_cc3->inc_total(((double)ialu_access/(double)res_ialu)*power.ialu_power +
      ((double)falu_access/(double)res_fpalu)*power.falu_power);
  }
  else
    alu_power_cc3->inc_total(turnoff_factor*(power.ialu_power + power.falu_power));

#ifdef STATIC_AF
  if(resultbus_access) {
    ASSERT(g_conf_max_issue);
    if(resultbus_access <= (uint32) g_conf_max_issue) {
      resultbus_power_cc1->inc_total(power.resultbus);
    }
    else {
      resultbus_power_cc1->inc_total(((double)resultbus_access/(double)g_conf_max_issue)*power.resultbus);
    }
    resultbus_power_cc2->inc_total(((double)resultbus_access/(double)g_conf_max_issue)*power.resultbus);
    resultbus_power_cc3->inc_total(((double)resultbus_access/(double)g_conf_max_issue)*power.resultbus);
  }
  else
    resultbus_power_cc3->inc_total(turnoff_factor*power.resultbus);
#else
  if(resultbus_access) {
    ASSERT(g_conf_max_issue);
    if(resultbus_access <= (uint32) g_conf_max_issue) {
      resultbus_power_cc1->inc_total(resultbus_af_b*power.resultbus);
    }
    else {
      resultbus_power_cc1->inc_total(((double)resultbus_access/(double)g_conf_max_issue)*resultbus_af_b*power.resultbus);
    }
    resultbus_power_cc2->inc_total(((double)resultbus_access/(double)g_conf_max_issue)*resultbus_af_b*power.resultbus);
    resultbus_power_cc3->inc_total(((double)resultbus_access/(double)g_conf_max_issue)*resultbus_af_b*power.resultbus);
  }
  else
    resultbus_power_cc3->inc_total(turnoff_factor*power.resultbus);
#endif

  /* These will be generated each cycle now.. */
  regfile_power = regfile_power_cc1 = regfile_power_cc2 = regfile_power_cc3 = 0;

  for (i = 0; i < NUM_REGFILES; i++)
  {
    /* add the power from each register file together to get the total regfile power. */
    regfile_power ->inc_total( wattch_regfiles[i].regfile_total_power);
    regfile_power_cc1 ->inc_total( wattch_regfiles[i].regfile_total_power_cc1);
    regfile_power_cc2 ->inc_total( wattch_regfiles[i].regfile_total_power_cc2);
    regfile_power_cc3 ->inc_total( wattch_regfiles[i].regfile_total_power_cc3);
  }

  /*
  total_cycle_power = rename_power + bpred_power + window_power + 
    lsq_power + regfile_power + icache_power + dcache_power +
    alu_power + resultbus_power;

  total_cycle_power_cc1 = rename_power_cc1 + bpred_power_cc1 + 
    window_power_cc1 + lsq_power_cc1 + regfile_power_cc1 + 
    icache_power_cc1 + dcache_power_cc1 + alu_power_cc1 + 
    resultbus_power_cc1;

  total_cycle_power_cc2 = rename_power_cc2 + bpred_power_cc2 + 
    window_power_cc2 + lsq_power_cc2 + regfile_power_cc2 + 
    icache_power_cc2 + dcache_power_cc2 + alu_power_cc2 + 
    resultbus_power_cc2;

  total_cycle_power_cc3 = rename_power_cc3 + bpred_power_cc3 + 
    window_power_cc3 + lsq_power_cc3 + regfile_power_cc3 + 
    icache_power_cc3 + dcache_power_cc3 + alu_power_cc3 + 
    resultbus_power_cc3;

  clock_power_cc1+=power.clock_power*(total_cycle_power_cc1/total_cycle_power);
  clock_power_cc2+=power.clock_power*(total_cycle_power_cc2/total_cycle_power);
  clock_power_cc3+=power.clock_power*(total_cycle_power_cc3/total_cycle_power);

  total_cycle_power_cc1 += clock_power_cc1;
  total_cycle_power_cc2 += clock_power_cc2;
  total_cycle_power_cc3 += clock_power_cc3;

  current_total_cycle_power_cc1 = total_cycle_power_cc1
    -last_single_total_cycle_power_cc1;
  current_total_cycle_power_cc2 = total_cycle_power_cc2
    -last_single_total_cycle_power_cc2;
  current_total_cycle_power_cc3 = total_cycle_power_cc3
    -last_single_total_cycle_power_cc3;

  max_cycle_power_cc1 = MAX(max_cycle_power_cc1,current_total_cycle_power_cc1);
  max_cycle_power_cc2 = MAX(max_cycle_power_cc2,current_total_cycle_power_cc2);
  max_cycle_power_cc3 = MAX(max_cycle_power_cc3,current_total_cycle_power_cc3);

  last_single_total_cycle_power_cc1 = total_cycle_power_cc1;
  last_single_total_cycle_power_cc2 = total_cycle_power_cc2;
  last_single_total_cycle_power_cc3 = total_cycle_power_cc3;
  */
}


// void
// power_reg_stats(struct stat_sdb_t *sdb)	/* stats database */
// {
//   stat_reg_double(sdb, "rename_power", "total power usage of rename unit", &rename_power, 0, NULL);

//   stat_reg_double(sdb, "bpred_power", "total power usage of bpred unit", &bpred_power, 0, NULL);

//   stat_reg_double(sdb, "window_power", "total power usage of instruction window", &window_power, 0, NULL);

//   stat_reg_double(sdb, "lsq_power", "total power usage of load/store queue", &lsq_power, 0, NULL);

//   stat_reg_double(sdb, "regfile_power", "total power usage of arch. regfile", &regfile_power, 0, NULL);

// {
// 	int i;
// 	char * desc;
// 	char * name;

// 	for (i = 0; i < NUM_REGFILES; i++)
// 	{
// 		if ((! (desc = malloc(256))) || (! (name = malloc(128))))
// 		{
// 			perror(__FUNCTION__);
// 			exit(1);
// 		}

// 		snprintf(name, 128, "%s_total_power", wattch_regfiles[i].shortname);
// 		snprintf(desc, 256, "total power usage of %s", wattch_regfiles[i].longname);

// 		stat_reg_double(sdb, name, desc, &wattch_regfiles[i].regfile_total_power, 0, NULL);
// 	}

// }

//   stat_reg_double(sdb, "icache_power", "total power usage of icache", &icache_power, 0, NULL);

//   stat_reg_double(sdb, "dcache_power", "total power usage of dcache", &dcache_power, 0, NULL);

//   stat_reg_double(sdb, "dcache2_power", "total power usage of dcache2", &dcache2_power, 0, NULL);

//   stat_reg_double(sdb, "alu_power", "total power usage of alu", &alu_power, 0, NULL);

//   stat_reg_double(sdb, "falu_power", "total power usage of falu", &falu_power, 0, NULL);

//   stat_reg_double(sdb, "resultbus_power", "total power usage of resultbus", &resultbus_power, 0, NULL);

//   stat_reg_double(sdb, "clock_power", "total power usage of clock", &clock_power, 0, NULL);

//   stat_reg_formula(sdb, "avg_rename_power", "avg power usage of rename unit", "rename_power/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_bpred_power", "avg power usage of bpred unit", "bpred_power/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_window_power", "avg power usage of instruction window", "window_power/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_lsq_power", "avg power usage of lsq", "lsq_power/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_regfile_power", "avg power usage of arch. regfile", "regfile_power/sim_cycle",  NULL);

// {
// 	int i;
// 	char * desc;
// 	char * name;
// 	char * formula;

// 	for (i = 0; i < NUM_REGFILES; i++)
// 	{
// 		if ((! (desc = malloc(256))) || (! (name = malloc(128))) || (! (formula = malloc(256))))
// 		{
// 			perror(__FUNCTION__);
// 			exit(1);
// 		}

// 		snprintf(name, 128, "avg_%s_power", wattch_regfiles[i].shortname);
// 		snprintf(desc, 256, "avg power usage of %s", wattch_regfiles[i].longname);
// 		snprintf(formula, 256, "%s_total_power/sim_cycle", wattch_regfiles[i].shortname);

// 		stat_reg_formula(sdb, name, desc, formula, NULL);
// 	}

// }

//   stat_reg_formula(sdb, "avg_icache_power", "avg power usage of icache", "icache_power/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_dcache_power", "avg power usage of dcache", "dcache_power/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_dcache2_power", "avg power usage of dcache2", "dcache2_power/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_alu_power", "avg power usage of alu", "alu_power/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_falu_power", "avg power usage of falu", "falu_power/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_resultbus_power", "avg power usage of resultbus", "resultbus_power/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_clock_power", "avg power usage of clock", "clock_power/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "fetch_stage_power", "total power usage of fetch stage", "icache_power + bpred_power", NULL);

//   stat_reg_formula(sdb, "dispatch_stage_power", "total power usage of dispatch stage", "rename_power", NULL);

//   stat_reg_formula(sdb, "issue_stage_power", "total power usage of issue stage", "resultbus_power + alu_power + dcache_power + dcache2_power + window_power + lsq_power", NULL);

//   stat_reg_formula(sdb, "avg_fetch_power", "average power of fetch unit per cycle", "(icache_power + bpred_power)/ sim_cycle", /* format */NULL);

//   stat_reg_formula(sdb, "avg_dispatch_power", "average power of dispatch unit per cycle", "(rename_power)/ sim_cycle", /* format */NULL);

//   stat_reg_formula(sdb, "avg_issue_power", "average power of issue unit per cycle", "(resultbus_power + alu_power + dcache_power + dcache2_power + window_power + lsq_power)/ sim_cycle", /* format */NULL);

//   stat_reg_formula(sdb, "total_power", "total power per cycle","(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power  + resultbus_power + clock_power + alu_power + dcache_power + dcache2_power)", NULL);

//   stat_reg_formula(sdb, "avg_total_power_cycle", "average total power per cycle","(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power + resultbus_power + clock_power + alu_power + dcache_power + dcache2_power)/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_total_power_cycle_nofp_nod2", "average total power per cycle","(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power + resultbus_power + clock_power + alu_power + dcache_power - falu_power )/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_total_power_insn", "average total power per insn","(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power + resultbus_power + clock_power + alu_power + dcache_power + dcache2_power)/sim_total_insn", NULL);

//   stat_reg_formula(sdb, "avg_total_power_insn_nofp_nod2", "average total power per insn","(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power + resultbus_power + clock_power + alu_power + dcache_power - falu_power )/sim_total_insn", NULL);

//   stat_reg_double(sdb, "rename_power_cc1", "total power usage of rename unit_cc1", &rename_power_cc1, 0, NULL);

//   stat_reg_double(sdb, "bpred_power_cc1", "total power usage of bpred unit_cc1", &bpred_power_cc1, 0, NULL);

//   stat_reg_double(sdb, "window_power_cc1", "total power usage of instruction window_cc1", &window_power_cc1, 0, NULL);

//   stat_reg_double(sdb, "lsq_power_cc1", "total power usage of lsq_cc1", &lsq_power_cc1, 0, NULL);

//   stat_reg_double(sdb, "regfile_power_cc1", "total power usage of arch. regfile_cc1", &regfile_power_cc1, 0, NULL);

// {
// 	int i;
// 	char * desc;
// 	char * name;

// 	for (i = 0; i < NUM_REGFILES; i++)
// 	{
// 		if ((! (desc = malloc(256))) || (! (name = malloc(128))))
// 		{
// 			perror(__FUNCTION__);
// 			exit(1);
// 		}

// 		snprintf(name, 128, "%s_total_power_cc1", wattch_regfiles[i].shortname);
// 		snprintf(desc, 256, "total power usage of %s_cc1", wattch_regfiles[i].shortname);

// 		stat_reg_double(sdb, name, desc, &wattch_regfiles[i].regfile_total_power_cc1, 0, NULL);
// 	}

// }

//   stat_reg_double(sdb, "icache_power_cc1", "total power usage of icache_cc1", &icache_power_cc1, 0, NULL);

//   stat_reg_double(sdb, "dcache_power_cc1", "total power usage of dcache_cc1", &dcache_power_cc1, 0, NULL);

//   stat_reg_double(sdb, "dcache2_power_cc1", "total power usage of dcache2_cc1", &dcache2_power_cc1, 0, NULL);

//   stat_reg_double(sdb, "alu_power_cc1", "total power usage of alu_cc1", &alu_power_cc1, 0, NULL);

//   stat_reg_double(sdb, "resultbus_power_cc1", "total power usage of resultbus_cc1", &resultbus_power_cc1, 0, NULL);

//   stat_reg_double(sdb, "clock_power_cc1", "total power usage of clock_cc1", &clock_power_cc1, 0, NULL);

//   stat_reg_formula(sdb, "avg_rename_power_cc1", "avg power usage of rename unit_cc1", "rename_power_cc1/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_bpred_power_cc1", "avg power usage of bpred unit_cc1", "bpred_power_cc1/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_window_power_cc1", "avg power usage of instruction window_cc1", "window_power_cc1/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_lsq_power_cc1", "avg power usage of lsq_cc1", "lsq_power_cc1/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_regfile_power_cc1", "avg power usage of arch. regfile_cc1", "regfile_power_cc1/sim_cycle",  NULL);

// {
// 	int i;
// 	char * desc;
// 	char * name;
// 	char * formula;

// 	for (i = 0; i < NUM_REGFILES; i++)
// 	{
// 		if ((! (desc = malloc(256))) || (! (name = malloc(128))) || (! (formula = malloc(256))))
// 		{
// 			perror(__FUNCTION__);
// 			exit(1);
// 		}

// 		snprintf(name, 128, "avg_%s_power_cc1", wattch_regfiles[i].shortname);
// 		snprintf(desc, 256, "avg power usage of %s_cc1", wattch_regfiles[i].shortname);
// 		snprintf(formula, 256, "%s_total_power_cc1/sim_cycle", wattch_regfiles[i].shortname);

// 		stat_reg_formula(sdb, name, desc, formula, NULL);
// 	}

// }

//   stat_reg_formula(sdb, "avg_icache_power_cc1", "avg power usage of icache_cc1", "icache_power_cc1/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_dcache_power_cc1", "avg power usage of dcache_cc1", "dcache_power_cc1/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_dcache2_power_cc1", "avg power usage of dcache2_cc1", "dcache2_power_cc1/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_alu_power_cc1", "avg power usage of alu_cc1", "alu_power_cc1/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_resultbus_power_cc1", "avg power usage of resultbus_cc1", "resultbus_power_cc1/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_clock_power_cc1", "avg power usage of clock_cc1", "clock_power_cc1/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "fetch_stage_power_cc1", "total power usage of fetch stage_cc1", "icache_power_cc1 + bpred_power_cc1", NULL);

//   stat_reg_formula(sdb, "dispatch_stage_power_cc1", "total power usage of dispatch stage_cc1", "rename_power_cc1", NULL);

//   stat_reg_formula(sdb, "issue_stage_power_cc1", "total power usage of issue stage_cc1", "resultbus_power_cc1 + alu_power_cc1 + dcache_power_cc1 + dcache2_power_cc1 + lsq_power_cc1 + window_power_cc1", NULL);

//   stat_reg_formula(sdb, "avg_fetch_power_cc1", "average power of fetch unit per cycle_cc1", "(icache_power_cc1 + bpred_power_cc1)/ sim_cycle", /* format */NULL);

//   stat_reg_formula(sdb, "avg_dispatch_power_cc1", "average power of dispatch unit per cycle_cc1", "(rename_power_cc1)/ sim_cycle", /* format */NULL);

//   stat_reg_formula(sdb, "avg_issue_power_cc1", "average power of issue unit per cycle_cc1", "(resultbus_power_cc1 + alu_power_cc1 + dcache_power_cc1 + dcache2_power_cc1 + lsq_power_cc1 + window_power_cc1)/ sim_cycle", /* format */NULL);

//   stat_reg_formula(sdb, "total_power_cycle_cc1", "total power per cycle_cc1","(rename_power_cc1 + bpred_power_cc1 + lsq_power_cc1 + window_power_cc1 + regfile_power_cc1 + icache_power_cc1 + resultbus_power_cc1 + clock_power_cc1 + alu_power_cc1 + dcache_power_cc1 + dcache2_power_cc1)", NULL);

//   stat_reg_formula(sdb, "avg_total_power_cycle_cc1", "average total power per cycle_cc1","(rename_power_cc1 + bpred_power_cc1 + lsq_power_cc1 + window_power_cc1 + regfile_power_cc1 + icache_power_cc1 + resultbus_power_cc1 + clock_power_cc1 + alu_power_cc1 + dcache_power_cc1 +dcache2_power_cc1)/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_total_power_insn_cc1", "average total power per insn_cc1","(rename_power_cc1 + bpred_power_cc1 + lsq_power_cc1 + window_power_cc1 + regfile_power_cc1 + icache_power_cc1 + resultbus_power_cc1 + clock_power_cc1 +  alu_power_cc1 + dcache_power_cc1 + dcache2_power_cc1)/sim_total_insn", NULL);

//   stat_reg_double(sdb, "rename_power_cc2", "total power usage of rename unit_cc2", &rename_power_cc2, 0, NULL);

//   stat_reg_double(sdb, "bpred_power_cc2", "total power usage of bpred unit_cc2", &bpred_power_cc2, 0, NULL);

//   stat_reg_double(sdb, "window_power_cc2", "total power usage of instruction window_cc2", &window_power_cc2, 0, NULL);

//   stat_reg_double(sdb, "lsq_power_cc2", "total power usage of lsq_cc2", &lsq_power_cc2, 0, NULL);

//   stat_reg_double(sdb, "regfile_power_cc2", "total power usage of arch. regfile_cc2", &regfile_power_cc2, 0, NULL);

// {
// 	int i;
// 	char * desc;
// 	char * name;

// 	for (i = 0; i < NUM_REGFILES; i++)
// 	{
// 		if ((! (desc = malloc(256))) || (! (name = malloc(128))))
// 		{
// 			perror(__FUNCTION__);
// 			exit(1);
// 		}

// 		snprintf(name, 128, "%s_total_power_cc2", wattch_regfiles[i].shortname);
// 		snprintf(desc, 256, "total power usage of %s_cc2", wattch_regfiles[i].shortname);

// 		stat_reg_double(sdb, name, desc, &wattch_regfiles[i].regfile_total_power_cc2, 0, NULL);
// 	}

// }

//   stat_reg_double(sdb, "icache_power_cc2", "total power usage of icache_cc2", &icache_power_cc2, 0, NULL);

//   stat_reg_double(sdb, "dcache_power_cc2", "total power usage of dcache_cc2", &dcache_power_cc2, 0, NULL);

//   stat_reg_double(sdb, "dcache2_power_cc2", "total power usage of dcache2_cc2", &dcache2_power_cc2, 0, NULL);

//   stat_reg_double(sdb, "alu_power_cc2", "total power usage of alu_cc2", &alu_power_cc2, 0, NULL);

//   stat_reg_double(sdb, "resultbus_power_cc2", "total power usage of resultbus_cc2", &resultbus_power_cc2, 0, NULL);

//   stat_reg_double(sdb, "clock_power_cc2", "total power usage of clock_cc2", &clock_power_cc2, 0, NULL);

//   stat_reg_formula(sdb, "avg_rename_power_cc2", "avg power usage of rename unit_cc2", "rename_power_cc2/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_bpred_power_cc2", "avg power usage of bpred unit_cc2", "bpred_power_cc2/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_window_power_cc2", "avg power usage of instruction window_cc2", "window_power_cc2/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_lsq_power_cc2", "avg power usage of instruction lsq_cc2", "lsq_power_cc2/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_regfile_power_cc2", "avg power usage of arch. regfile_cc2", "regfile_power_cc2/sim_cycle",  NULL);

// {
// 	int i;
// 	char * desc;
// 	char * name;
// 	char * formula;

// 	for (i = 0; i < NUM_REGFILES; i++)
// 	{
// 		if ((! (desc = malloc(256))) || (! (name = malloc(128))) || (! (formula = malloc(256))))
// 		{
// 			perror(__FUNCTION__);
// 			exit(1);
// 		}

// 		snprintf(name, 128, "avg_%s_power_cc2", wattch_regfiles[i].shortname);
// 		snprintf(desc, 256, "avg power usage of %s_cc2", wattch_regfiles[i].shortname);
// 		snprintf(formula, 256, "%s_total_power_cc2/sim_cycle", wattch_regfiles[i].shortname);

// 		stat_reg_formula(sdb, name, desc, formula, NULL);
// 	}

// }

//   stat_reg_formula(sdb, "avg_icache_power_cc2", "avg power usage of icache_cc2", "icache_power_cc2/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_dcache_power_cc2", "avg power usage of dcache_cc2", "dcache_power_cc2/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_dcache2_power_cc2", "avg power usage of dcache2_cc2", "dcache2_power_cc2/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_alu_power_cc2", "avg power usage of alu_cc2", "alu_power_cc2/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_resultbus_power_cc2", "avg power usage of resultbus_cc2", "resultbus_power_cc2/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_clock_power_cc2", "avg power usage of clock_cc2", "clock_power_cc2/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "fetch_stage_power_cc2", "total power usage of fetch stage_cc2", "icache_power_cc2 + bpred_power_cc2", NULL);

//   stat_reg_formula(sdb, "dispatch_stage_power_cc2", "total power usage of dispatch stage_cc2", "rename_power_cc2", NULL);

//   stat_reg_formula(sdb, "issue_stage_power_cc2", "total power usage of issue stage_cc2", "resultbus_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2 + lsq_power_cc2 + window_power_cc2", NULL);

//   stat_reg_formula(sdb, "avg_fetch_power_cc2", "average power of fetch unit per cycle_cc2", "(icache_power_cc2 + bpred_power_cc2)/ sim_cycle", /* format */NULL);

//   stat_reg_formula(sdb, "avg_dispatch_power_cc2", "average power of dispatch unit per cycle_cc2", "(rename_power_cc2)/ sim_cycle", /* format */NULL);

//   stat_reg_formula(sdb, "avg_issue_power_cc2", "average power of issue unit per cycle_cc2", "(resultbus_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2 + lsq_power_cc2 + window_power_cc2)/ sim_cycle", /* format */NULL);

//   stat_reg_formula(sdb, "total_power_cycle_cc2", "total power per cycle_cc2","(rename_power_cc2 + bpred_power_cc2 + lsq_power_cc2 + window_power_cc2 + regfile_power_cc2 + icache_power_cc2 + resultbus_power_cc2 + clock_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2)", NULL);

//   stat_reg_formula(sdb, "avg_total_power_cycle_cc2", "average total power per cycle_cc2","(rename_power_cc2 + bpred_power_cc2 + lsq_power_cc2 + window_power_cc2 + regfile_power_cc2 + icache_power_cc2 + resultbus_power_cc2 + clock_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2)/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_total_power_insn_cc2", "average total power per insn_cc2","(rename_power_cc2 + bpred_power_cc2 + lsq_power_cc2 + window_power_cc2 + regfile_power_cc2 + icache_power_cc2 + resultbus_power_cc2 + clock_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2)/sim_total_insn", NULL);

//   stat_reg_double(sdb, "rename_power_cc3", "total power usage of rename unit_cc3", &rename_power_cc3, 0, NULL);

//   stat_reg_double(sdb, "bpred_power_cc3", "total power usage of bpred unit_cc3", &bpred_power_cc3, 0, NULL);

//   stat_reg_double(sdb, "window_power_cc3", "total power usage of instruction window_cc3", &window_power_cc3, 0, NULL);

//   stat_reg_double(sdb, "lsq_power_cc3", "total power usage of lsq_cc3", &lsq_power_cc3, 0, NULL);

//   stat_reg_double(sdb, "regfile_power_cc3", "total power usage of arch. regfile_cc3", &regfile_power_cc3, 0, NULL);

// {
// 	int i;
// 	char * desc;
// 	char * name;

// 	for (i = 0; i < NUM_REGFILES; i++)
// 	{
// 		if ((! (desc = malloc(256))) || (! (name = malloc(128))))
// 		{
// 			perror(__FUNCTION__);
// 			exit(1);
// 		}

// 		snprintf(name, 128, "%s_total_power_cc3", wattch_regfiles[i].shortname);
// 		snprintf(desc, 256, "total power usage of %s_cc3", wattch_regfiles[i].shortname);

// 		stat_reg_double(sdb, name, desc, &wattch_regfiles[i].regfile_total_power_cc3, 0, NULL);
// 	}

// }

//   stat_reg_double(sdb, "icache_power_cc3", "total power usage of icache_cc3", &icache_power_cc3, 0, NULL);

//   stat_reg_double(sdb, "dcache_power_cc3", "total power usage of dcache_cc3", &dcache_power_cc3, 0, NULL);

//   stat_reg_double(sdb, "dcache2_power_cc3", "total power usage of dcache2_cc3", &dcache2_power_cc3, 0, NULL);

//   stat_reg_double(sdb, "alu_power_cc3", "total power usage of alu_cc3", &alu_power_cc3, 0, NULL);

//   stat_reg_double(sdb, "resultbus_power_cc3", "total power usage of resultbus_cc3", &resultbus_power_cc3, 0, NULL);

//   stat_reg_double(sdb, "clock_power_cc3", "total power usage of clock_cc3", &clock_power_cc3, 0, NULL);

//   stat_reg_formula(sdb, "avg_rename_power_cc3", "avg power usage of rename unit_cc3", "rename_power_cc3/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_bpred_power_cc3", "avg power usage of bpred unit_cc3", "bpred_power_cc3/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_window_power_cc3", "avg power usage of instruction window_cc3", "window_power_cc3/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_lsq_power_cc3", "avg power usage of instruction lsq_cc3", "lsq_power_cc3/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_regfile_power_cc3", "avg power usage of arch. regfile_cc3", "regfile_power_cc3/sim_cycle",  NULL);

// {
// 	int i;
// 	char * desc;
// 	char * name;
// 	char * formula;

// 	for (i = 0; i < NUM_REGFILES; i++)
// 	{
// 		if ((! (desc = malloc(256))) || (! (name = malloc(128))) || (! (formula = malloc(256))))
// 		{
// 			perror(__FUNCTION__);
// 			exit(1);
// 		}

// 		snprintf(name, 128, "avg_%s_power_cc3", wattch_regfiles[i].shortname);
// 		snprintf(desc, 256, "avg power usage of %s_cc3", wattch_regfiles[i].shortname);
// 		snprintf(formula, 256, "%s_total_power_cc3/sim_cycle", wattch_regfiles[i].shortname);

// 		stat_reg_formula(sdb, name, desc, formula, NULL);
// 	}

// }

//   stat_reg_formula(sdb, "avg_icache_power_cc3", "avg power usage of icache_cc3", "icache_power_cc3/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_dcache_power_cc3", "avg power usage of dcache_cc3", "dcache_power_cc3/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_dcache2_power_cc3", "avg power usage of dcache2_cc3", "dcache2_power_cc3/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_alu_power_cc3", "avg power usage of alu_cc3", "alu_power_cc3/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_resultbus_power_cc3", "avg power usage of resultbus_cc3", "resultbus_power_cc3/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_clock_power_cc3", "avg power usage of clock_cc3", "clock_power_cc3/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "fetch_stage_power_cc3", "total power usage of fetch stage_cc3", "icache_power_cc3 + bpred_power_cc3", NULL);

//   stat_reg_formula(sdb, "dispatch_stage_power_cc3", "total power usage of dispatch stage_cc3", "rename_power_cc3", NULL);

//   stat_reg_formula(sdb, "issue_stage_power_cc3", "total power usage of issue stage_cc3", "resultbus_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3 + lsq_power_cc3 + window_power_cc3", NULL);

//   stat_reg_formula(sdb, "avg_fetch_power_cc3", "average power of fetch unit per cycle_cc3", "(icache_power_cc3 + bpred_power_cc3)/ sim_cycle", /* format */NULL);

//   stat_reg_formula(sdb, "avg_dispatch_power_cc3", "average power of dispatch unit per cycle_cc3", "(rename_power_cc3)/ sim_cycle", /* format */NULL);

//   stat_reg_formula(sdb, "avg_issue_power_cc3", "average power of issue unit per cycle_cc3", "(resultbus_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3 + lsq_power_cc3 + window_power_cc3)/ sim_cycle", /* format */NULL);

//   stat_reg_formula(sdb, "total_power_cycle_cc3", "total power per cycle_cc3","(rename_power_cc3 + bpred_power_cc3 + lsq_power_cc3 + window_power_cc3 + regfile_power_cc3 + icache_power_cc3 + resultbus_power_cc3 + clock_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3)", NULL);

//   stat_reg_formula(sdb, "avg_total_power_cycle_cc3", "average total power per cycle_cc3","(rename_power_cc3 + bpred_power_cc3 + lsq_power_cc3 + window_power_cc3 + regfile_power_cc3 + icache_power_cc3 + resultbus_power_cc3 + clock_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3)/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_total_power_insn_cc3", "average total power per insn_cc3","(rename_power_cc3 + bpred_power_cc3 + lsq_power_cc3 + window_power_cc3 + regfile_power_cc3 + icache_power_cc3 + resultbus_power_cc3 + clock_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3)/sim_total_insn", NULL);

//   stat_reg_counter(sdb, "total_rename_access", "total number accesses of rename unit", &total_rename_access, 0, NULL);

//   stat_reg_counter(sdb, "total_bpred_access", "total number accesses of bpred unit", &total_bpred_access, 0, NULL);

//   stat_reg_counter(sdb, "total_window_access", "total number accesses of instruction window", &total_window_access, 0, NULL);

//   stat_reg_counter(sdb, "total_lsq_access", "total number accesses of load/store queue", &total_lsq_access, 0, NULL);

//   stat_reg_counter(sdb, "total_regfile_access", "total number accesses of arch. regfile", &total_regfile_access, 0, NULL);

// {
// 	int i;
// 	char * desc;
// 	char * name;

// 	for (i = 0; i < NUM_REGFILES; i++)
// 	{
// 		if ((! (desc = malloc(256))) || (! (name = malloc(128))))
// 		{
// 			perror(__FUNCTION__);
// 			exit(1);
// 		}

// 		snprintf(name, 128, "total_%s_access", wattch_regfiles[i].shortname);
// 		snprintf(desc, 256, "total number of accesses of %s", wattch_regfiles[i].longname);

// 		stat_reg_counter(sdb, name, desc, &wattch_regfiles[i].total_regfile_access, 0, NULL);
// 	}

// }

//   stat_reg_counter(sdb, "total_icache_access", "total number accesses of icache", &total_icache_access, 0, NULL);

//   stat_reg_counter(sdb, "total_dcache_access", "total number accesses of dcache", &total_dcache_access, 0, NULL);

//   stat_reg_counter(sdb, "total_dcache2_access", "total number accesses of dcache2", &total_dcache2_access, 0, NULL);

//   stat_reg_counter(sdb, "total_alu_access", "total number accesses of alu", &total_alu_access, 0, NULL);

//   stat_reg_counter(sdb, "total_resultbus_access", "total number accesses of resultbus", &total_resultbus_access, 0, NULL);

//   stat_reg_formula(sdb, "avg_rename_access", "avg number accesses of rename unit", "total_rename_access/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_bpred_access", "avg number accesses of bpred unit", "total_bpred_access/sim_cycle", NULL);

//   stat_reg_formula(sdb, "avg_window_access", "avg number accesses of instruction window", "total_window_access/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_lsq_access", "avg number accesses of lsq", "total_lsq_access/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_regfile_access", "avg number accesses of arch. regfile", "total_regfile_access/sim_cycle",  NULL);

// {
// 	int i;
// 	char * desc;
// 	char * name;
// 	char * formula;

// 	for (i = 0; i < NUM_REGFILES; i++)
// 	{
// 		if ((! (desc = malloc(256))) || (! (name = malloc(128))) || (! (formula = malloc(256))))
// 		{
// 			perror(__FUNCTION__);
// 			exit(1);
// 		}

// 		snprintf(name, 128, "avg_%s_access", wattch_regfiles[i].shortname);
// 		snprintf(desc, 256, "avg number accesses of %s", wattch_regfiles[i].longname);
// 		snprintf(formula, 256, "total_%s_access/sim_cycle", wattch_regfiles[i].shortname);

// 		stat_reg_formula(sdb, name, desc, formula, NULL);
// 	}

// }

//   stat_reg_formula(sdb, "avg_icache_access", "avg number accesses of icache", "total_icache_access/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_dcache_access", "avg number accesses of dcache", "total_dcache_access/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_dcache2_access", "avg number accesses of dcache2", "total_dcache2_access/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_alu_access", "avg number accesses of alu", "total_alu_access/sim_cycle",  NULL);

//   stat_reg_formula(sdb, "avg_resultbus_access", "avg number accesses of resultbus", "total_resultbus_access/sim_cycle",  NULL);

//   stat_reg_counter(sdb, "max_rename_access", "max number accesses of rename unit", &max_rename_access, 0, NULL);

//   stat_reg_counter(sdb, "max_bpred_access", "max number accesses of bpred unit", &max_bpred_access, 0, NULL);

//   stat_reg_counter(sdb, "max_window_access", "max number accesses of instruction window", &max_window_access, 0, NULL);

//   stat_reg_counter(sdb, "max_lsq_access", "max number accesses of load/store queue", &max_lsq_access, 0, NULL);

//   stat_reg_counter(sdb, "max_regfile_access", "max number accesses of arch. regfile", &max_regfile_access, 0, NULL);

// {
// 	int i;
// 	char * desc;
// 	char * name;

// 	for (i = 0; i < NUM_REGFILES; i++)
// 	{
// 		if ((! (desc = malloc(256))) || (! (name = malloc(128))))
// 		{
// 			perror(__FUNCTION__);
// 			exit(1);
// 		}

// 		snprintf(name, 128, "max_%s_access", wattch_regfiles[i].shortname);
// 		snprintf(desc, 256, "max number accesses of %s", wattch_regfiles[i].longname);

// 		stat_reg_counter(sdb, name, desc, &wattch_regfiles[i].max_regfile_access, 0, NULL);
// 	}

// }

//   stat_reg_counter(sdb, "max_icache_access", "max number accesses of icache", &max_icache_access, 0, NULL);

//   stat_reg_counter(sdb, "max_dcache_access", "max number accesses of dcache", &max_dcache_access, 0, NULL);

//   stat_reg_counter(sdb, "max_dcache2_access", "max number accesses of dcache2", &max_dcache2_access, 0, NULL);

//   stat_reg_counter(sdb, "max_alu_access", "max number accesses of alu", &max_alu_access, 0, NULL);

//   stat_reg_counter(sdb, "max_resultbus_access", "max number accesses of resultbus", &max_resultbus_access, 0, NULL);

//   stat_reg_double(sdb, "max_cycle_power_cc1", "maximum cycle power usage of cc1", &max_cycle_power_cc1, 0, NULL);

//   stat_reg_double(sdb, "max_cycle_power_cc2", "maximum cycle power usage of cc2", &max_cycle_power_cc2, 0, NULL);

//   stat_reg_double(sdb, "max_cycle_power_cc3", "maximum cycle power usage of cc3", &max_cycle_power_cc3, 0, NULL);

// }


// void dump_power_stats(power)
//      power_result_type *power;
// {
//   double total_power;
//   double bpred_power;
//   double rename_power;
//   double rat_power;
//   double dcl_power;
//   double lsq_power;
//   double window_power;
//   double wakeup_power;
//   double rs_power;
//   double lsq_wakeup_power;
//   double lsq_rs_power;
//   double regfile_power;
//   double reorder_power;
//   double icache_power;
//   double dcache_power;
//   double dcache2_power;
//   double dtlb_power;
//   double itlb_power;
//   double ambient_power = 2.0;
//   int i;

//   icache_power = power->icache_power;

//   dcache_power = power->dcache_power;

//   dcache2_power = power->dcache2_power;

//   itlb_power = power->itlb;
//   dtlb_power = power->dtlb;

//   bpred_power = power->btb + power->local_predict + power->global_predict + 
//     power->chooser + power->ras;

//   rat_power = power->rat_decoder + 
//     power->rat_wordline + power->rat_bitline + power->rat_senseamp;

//   dcl_power = power->dcl_compare + power->dcl_pencode;

//   rename_power = power->rat_power + power->dcl_power + power->inst_decoder_power;

//   wakeup_power = power->wakeup_tagdrive + power->wakeup_tagmatch + 
//     power->wakeup_ormatch;
   
//   rs_power = power->rs_decoder + 
//     power->rs_wordline + power->rs_bitline + power->rs_senseamp;

//   window_power = wakeup_power + rs_power + power->selection;

//   lsq_rs_power = power->lsq_rs_decoder + 
//     power->lsq_rs_wordline + power->lsq_rs_bitline + power->lsq_rs_senseamp;

//   lsq_wakeup_power = power->lsq_wakeup_tagdrive + 
//     power->lsq_wakeup_tagmatch + power->lsq_wakeup_ormatch;

//   lsq_power = lsq_wakeup_power + lsq_rs_power;

//   reorder_power = power->reorder_decoder + 
//     power->reorder_wordline + power->reorder_bitline + 
//     power->reorder_senseamp;

// /*  The old (one regfile) way: */
// /*  regfile_power = power->regfile_decoder + 
//     power->regfile_wordline + power->regfile_bitline + 
//     power->regfile_senseamp; */

//   regfile_power = 0;

//   for (i = 0; i < NUM_REGFILES; i++)
//     regfile_power += wattch_regfiles[i].regfile_power;

//   total_power = bpred_power + rename_power + window_power +  regfile_power + 
//     power->resultbus + lsq_power + 
//     icache_power + dcache_power + dcache2_power + 
//     dtlb_power + itlb_power + power->clock_power + power->ialu_power +
//     power->falu_power;

//   VERBOSE_OUT(verb_t::requests, "\nProcessor Parameters:\n");
//   VERBOSE_OUT(verb_t::requests, "Issue Width: %d\n",g_conf_max_issue);
//   VERBOSE_OUT(verb_t::requests, "Window Size: %d\n",g_conf_window_size);
//   VERBOSE_OUT(verb_t::requests, "Number of Virtual (ISA) Registers: %d\n",total_num_regs);
//   VERBOSE_OUT(verb_t::requests, "Number of Physical (rename) Registers: %d\n",g_conf_window_size);
//   VERBOSE_OUT(verb_t::requests, "Integer Data Width: %d\n", int_g_conf_data_width);
//   VERBOSE_OUT(verb_t::requests, "Floating-Point Data Width: %d\n", fp_g_conf_data_width);
//   VERBOSE_OUT(verb_t::requests, "Address Data Width: %d\n", g_conf_addr_width);
//   VERBOSE_OUT(verb_t::requests, "Default Data Width: %d\n", g_conf_data_width);

//   for (i = 0; i < NUM_REGFILES; i++)
//   {
//     VERBOSE_OUT(verb_t::requests, "%s\n", wattch_regfiles[i].longname);
//     VERBOSE_OUT(verb_t::requests, " Register Width: %d\n", wattch_regfiles[i].data_width);
//     VERBOSE_OUT(verb_t::requests, " Number of Registers: %d\n", wattch_regfiles[i].num_regs);
//   }

//   VERBOSE_OUT(verb_t::requests, "\nTotal Power Consumption: %g\n",total_power+ambient_power);
//   VERBOSE_OUT(verb_t::requests, "Branch Predictor Power Consumption: %g  (%.3g%%)\n",bpred_power,100*bpred_power/total_power);
//   VERBOSE_OUT(verb_t::requests, " branch target buffer power (W): %g\n",power->btb);
//   VERBOSE_OUT(verb_t::requests, " local predict power (W): %g\n",power->local_predict);
//   VERBOSE_OUT(verb_t::requests, " global predict power (W): %g\n",power->global_predict);
//   VERBOSE_OUT(verb_t::requests, " chooser power (W): %g\n",power->chooser);
//   VERBOSE_OUT(verb_t::requests, " RAS power (W): %g\n",power->ras);
//   VERBOSE_OUT(verb_t::requests, "Rename Logic Power Consumption: %g  (%.3g%%)\n",rename_power,100*rename_power/total_power);
//   VERBOSE_OUT(verb_t::requests, " Instruction Decode Power (W): %g\n",power->inst_decoder_power);
//   VERBOSE_OUT(verb_t::requests, " RAT decode_power (W): %g\n",power->rat_decoder);
//   VERBOSE_OUT(verb_t::requests, " RAT wordline_power (W): %g\n",power->rat_wordline);
//   VERBOSE_OUT(verb_t::requests, " RAT bitline_power (W): %g\n",power->rat_bitline);
//   VERBOSE_OUT(verb_t::requests, " DCL Comparators (W): %g\n",power->dcl_compare);
//   VERBOSE_OUT(verb_t::requests, "Instruction Window Power Consumption: %g  (%.3g%%)\n",window_power,100*window_power/total_power);
//   VERBOSE_OUT(verb_t::requests, " tagdrive (W): %g\n",power->wakeup_tagdrive);
//   VERBOSE_OUT(verb_t::requests, " tagmatch (W): %g\n",power->wakeup_tagmatch);
//   VERBOSE_OUT(verb_t::requests, " Selection Logic (W): %g\n",power->selection);
//   VERBOSE_OUT(verb_t::requests, " decode_power (W): %g\n",power->rs_decoder);
//   VERBOSE_OUT(verb_t::requests, " wordline_power (W): %g\n",power->rs_wordline);
//   VERBOSE_OUT(verb_t::requests, " bitline_power (W): %g\n",power->rs_bitline);
//   VERBOSE_OUT(verb_t::requests, "Load/Store Queue Power Consumption: %g  (%.3g%%)\n",lsq_power,100*lsq_power/total_power);
//   VERBOSE_OUT(verb_t::requests, " tagdrive (W): %g\n",power->lsq_wakeup_tagdrive);
//   VERBOSE_OUT(verb_t::requests, " tagmatch (W): %g\n",power->lsq_wakeup_tagmatch);
//   VERBOSE_OUT(verb_t::requests, " decode_power (W): %g\n",power->lsq_rs_decoder);
//   VERBOSE_OUT(verb_t::requests, " wordline_power (W): %g\n",power->lsq_rs_wordline);
//   VERBOSE_OUT(verb_t::requests, " bitline_power (W): %g\n",power->lsq_rs_bitline);
//   VERBOSE_OUT(verb_t::requests, "Total Arch. Register File Power Consumption: %g  (%.3g%%)\n",regfile_power,100*regfile_power/total_power);
//   for (i = 0; i < NUM_REGFILES; i++)
//   {
//     VERBOSE_OUT(verb_t::requests, " %s\n", wattch_regfiles[i].shortname);
//     VERBOSE_OUT(verb_t::requests, "  regfile_power (W): %g\n",wattch_regfiles[i].regfile_power);
//     VERBOSE_OUT(verb_t::requests, "  decode_power (W): %g\n",wattch_regfiles[i].regfile_decoder);
//     VERBOSE_OUT(verb_t::requests, "  wordline_power (W): %g\n",wattch_regfiles[i].regfile_wordline);
//     VERBOSE_OUT(verb_t::requests, "  bitline_power (W): %g\n",wattch_regfiles[i].regfile_bitline);
//   }

//   VERBOSE_OUT(verb_t::requests, "Result Bus Power Consumption: %g  (%.3g%%)\n",power->resultbus,100*power->resultbus/total_power);
//   VERBOSE_OUT(verb_t::requests, "Total Clock Power: %g  (%.3g%%)\n",power->clock_power,100*power->clock_power/total_power);
//   VERBOSE_OUT(verb_t::requests, "Int ALU Power: %g  (%.3g%%)\n",power->ialu_power,100*power->ialu_power/total_power);
//   VERBOSE_OUT(verb_t::requests, "FP ALU Power: %g  (%.3g%%)\n",power->falu_power,100*power->falu_power/total_power);
//   VERBOSE_OUT(verb_t::requests, "Instruction Cache Power Consumption: %g  (%.3g%%)\n",icache_power,100*icache_power/total_power);
//   VERBOSE_OUT(verb_t::requests, " decode_power (W): %g\n",power->icache_decoder);
//   VERBOSE_OUT(verb_t::requests, " wordline_power (W): %g\n",power->icache_wordline);
//   VERBOSE_OUT(verb_t::requests, " bitline_power (W): %g\n",power->icache_bitline);
//   VERBOSE_OUT(verb_t::requests, " power_util->senseamp_power (W): %g\n",power->icache_senseamp);
//   VERBOSE_OUT(verb_t::requests, " tagarray_power (W): %g\n",power->icache_tagarray);
//   VERBOSE_OUT(verb_t::requests, "Itlb_power (W): %g (%.3g%%)\n",power->itlb,100*power->itlb/total_power);
//   VERBOSE_OUT(verb_t::requests, "Data Cache Power Consumption: %g  (%.3g%%)\n",dcache_power,100*dcache_power/total_power);
//   VERBOSE_OUT(verb_t::requests, " decode_power (W): %g\n",power->dcache_decoder);
//   VERBOSE_OUT(verb_t::requests, " wordline_power (W): %g\n",power->dcache_wordline);
//   VERBOSE_OUT(verb_t::requests, " bitline_power (W): %g\n",power->dcache_bitline);
//   VERBOSE_OUT(verb_t::requests, " power_util->senseamp_power (W): %g\n",power->dcache_senseamp);
//   VERBOSE_OUT(verb_t::requests, " tagarray_power (W): %g\n",power->dcache_tagarray);
//   VERBOSE_OUT(verb_t::requests, "Dtlb_power (W): %g (%.3g%%)\n",power->dtlb,100*power->dtlb/total_power);
//   VERBOSE_OUT(verb_t::requests, "Level 2 Cache Power Consumption: %g (%.3g%%)\n",dcache2_power,100*dcache2_power/total_power);
//   VERBOSE_OUT(verb_t::requests, " decode_power (W): %g\n",power->dcache2_decoder);
//   VERBOSE_OUT(verb_t::requests, " wordline_power (W): %g\n",power->dcache2_wordline);
//   VERBOSE_OUT(verb_t::requests, " bitline_power (W): %g\n",power->dcache2_bitline);
//   VERBOSE_OUT(verb_t::requests, " power_util->senseamp_power (W): %g\n",power->dcache2_senseamp);
//   VERBOSE_OUT(verb_t::requests, " tagarray_power (W): %g\n",power->dcache2_tagarray);
// }


void power_t::pipe_stage_movement(pipe_stage_t prev_pstage, pipe_stage_t curr_pstage,
    dynamic_instr_t *d_inst)
{
    switch(curr_pstage) {
        case PIPE_RENAME:
            rename_access++;
            break;
        case PIPE_DECODE:
            if (d_inst->is_load() || d_inst->is_store())
                lsq_access++;
            break;
        default:
            /* do nothing */
            ;
    }    
            
    
    
}



void power_t::calculate_power(power_result_type *power)
{
    double clockpower;
    double predeclength, wordlinelength, bitlinelength;
    int ndwl, ndbl, nspd, ntwl, ntbl, ntspd, c,b,a,cache, rowsb, colsb;
    int trowsb, tcolsb, tagsize;
    int va_size = 48;
    
    int npreg_width = (int)ceil(bct->logtwo((double)g_conf_window_size));
    
    /* these variables are needed to use Cacti to auto-size cache arrays 
    (for optimal delay) */
    time_result_type time_result;
    time_parameter_type time_parameters;
    
    /* used to autosize other structures, like bpred tables */
    int scale_factor;
    
    /* loop index variable */
    int i;
    
    
    cache=0;
    
    
    /* FIXME: ALU power is a simple constant, it would be better
    to include bit AFs and have different numbers for different
    types of operations */
    if (g_conf_data_width == 32)
        power->ialu_power = res_ialu * I_ADD32;
    else
        power->ialu_power = res_ialu * I_ADD;
    
    power->falu_power = res_fpalu * F_ADD;
    
    nvreg_width = (int)ceil(bct->logtwo((double)total_num_regs));
    npreg_width = (int)ceil(bct->logtwo((double)g_conf_window_size));
    
    
    /* RAT has shadow bits stored in each cell, this makes the
    cell size larger than normal array structures, so we must
    compute it here */
    
    predeclength = total_num_regs * 
    (RatCellHeight + 3 * g_conf_max_fetch * WordlineSpacing);
    
    wordlinelength = npreg_width * 
    (RatCellWidth + 
        6 * g_conf_max_fetch * BitlineSpacing + 
        RatShiftRegWidth*RatNumShift);
    
    bitlinelength = total_num_regs * (RatCellHeight + 3 * g_conf_max_fetch * WordlineSpacing);
    
    
    VERBOSE_OUT(verb_t::requests, "rat power stats\n");
    power->rat_decoder = power_util->array_decoder_power(total_num_regs,npreg_width,predeclength,2*g_conf_max_fetch,g_conf_max_fetch,cache);
    power->rat_wordline = power_util->array_wordline_power(total_num_regs,npreg_width,wordlinelength,2*g_conf_max_fetch,g_conf_max_fetch,cache);
    power->rat_bitline = power_util->array_bitline_power(total_num_regs,npreg_width,bitlinelength,2*g_conf_max_fetch,g_conf_max_fetch,cache);
    power->rat_senseamp = 0;
    
    power->dcl_compare = power_util->dcl_compare_power(nvreg_width);
    power->dcl_pencode = 0;
    power->inst_decoder_power = g_conf_max_fetch * power_util->simple_array_decoder_power(g_conf_opcode_length,1,1,1,cache);
    power->wakeup_tagdrive = power_util->cam_tagdrive(g_conf_window_size,npreg_width,g_conf_max_issue,g_conf_max_issue);
    power->wakeup_tagmatch = power_util->cam_tagmatch(g_conf_window_size,npreg_width,g_conf_max_issue,g_conf_max_issue);
    power->wakeup_ormatch =0; 
    
    power->selection = power_util->selection_power(g_conf_window_size);
    
    for (i = 0; i < NUM_REGFILES; i++)
    {
        predeclength = wattch_regfiles[i].num_regs * (RegCellHeight + 3 * g_conf_max_issue * WordlineSpacing);
        wordlinelength = wattch_regfiles[i].data_width * (RegCellWidth + 6 * g_conf_max_issue * BitlineSpacing);
        bitlinelength = wattch_regfiles[i].num_regs * (RegCellHeight + 3 * g_conf_max_issue * WordlineSpacing);
        
        
            VERBOSE_OUT(verb_t::requests, "regfile power stats for regfile %s\n", wattch_regfiles[i].shortname);
        
            wattch_regfiles[i].regfile_decoder = power_util->array_decoder_power(wattch_regfiles[i].num_regs,
                wattch_regfiles[i].data_width, predeclength, 2*g_conf_max_issue, g_conf_max_issue, cache);
            wattch_regfiles[i].regfile_wordline = power_util->array_wordline_power(wattch_regfiles[i].num_regs,
                wattch_regfiles[i].data_width, wordlinelength, 2*g_conf_max_issue, g_conf_max_issue, cache);
            wattch_regfiles[i].regfile_bitline = power_util->array_bitline_power(wattch_regfiles[i].num_regs,
                wattch_regfiles[i].data_width, bitlinelength, 2*g_conf_max_issue, g_conf_max_issue, cache);
            wattch_regfiles[i].regfile_senseamp = 0;
    }
    
    predeclength = g_conf_window_size * (RegCellHeight + 3 * g_conf_max_issue * WordlineSpacing);
    
    wordlinelength = g_conf_data_width * 
    (RegCellWidth + 
        6 * g_conf_max_issue * BitlineSpacing);
    
    bitlinelength = g_conf_window_size * (RegCellHeight + 3 * g_conf_max_issue * WordlineSpacing);
    
    
    VERBOSE_OUT(verb_t::requests, "res station power stats\n");
    power->rs_decoder = power_util->array_decoder_power(g_conf_window_size,g_conf_data_width,predeclength,2*g_conf_max_issue,g_conf_max_issue,cache);
    power->rs_wordline = power_util->array_wordline_power(g_conf_window_size,g_conf_data_width,wordlinelength,2*g_conf_max_issue,g_conf_max_issue,cache);
    power->rs_bitline = power_util->array_bitline_power(g_conf_window_size,g_conf_data_width,bitlinelength,2*g_conf_max_issue,g_conf_max_issue,cache);
    /* no senseamps in reg file structures (only caches) */
    power->rs_senseamp =0;
    
    /* addresses go into lsq tag's */
    power->lsq_wakeup_tagdrive = power_util->cam_tagdrive(g_conf_lsq_load_size,g_conf_addr_width,res_memport,res_memport);
    power->lsq_wakeup_tagmatch = power_util->cam_tagmatch(g_conf_lsq_load_size,g_conf_addr_width,res_memport,res_memport);
    power->lsq_wakeup_ormatch =0; 
    
    wordlinelength = g_conf_data_width * 
    (RegCellWidth + 
        4 * res_memport * BitlineSpacing);
    
    bitlinelength = g_conf_window_size * (RegCellHeight + 4 * res_memport * WordlineSpacing);
    
    /* rs's hold data */
    
    VERBOSE_OUT(verb_t::requests, "lsq station power stats\n");
    power->lsq_rs_decoder = power_util->array_decoder_power(g_conf_lsq_load_size,g_conf_data_width,predeclength,res_memport,res_memport,cache);
    power->lsq_rs_wordline = power_util->array_wordline_power(g_conf_lsq_load_size,g_conf_data_width,wordlinelength,res_memport,res_memport,cache);
    power->lsq_rs_bitline = power_util->array_bitline_power(g_conf_lsq_load_size,g_conf_data_width,bitlinelength,res_memport,res_memport,cache);
    power->lsq_rs_senseamp =0;
    
    power->resultbus = power_util->compute_resultbus_power();
    
    branch_predictor_power();
    
    
    memory_hierarchy_power();
    
    
    power->rat_decoder *= crossover_scaling;
    power->rat_wordline *= crossover_scaling;
    power->rat_bitline *= crossover_scaling;
    
    power->dcl_compare *= crossover_scaling;
    power->dcl_pencode *= crossover_scaling;
    power->inst_decoder_power *= crossover_scaling;
    power->wakeup_tagdrive *= crossover_scaling;
    power->wakeup_tagmatch *= crossover_scaling;
    power->wakeup_ormatch *= crossover_scaling;
    
    power->selection *= crossover_scaling;
    
    for (i = 0; i < NUM_REGFILES; i++)
    {
        wattch_regfiles[i].regfile_decoder *= crossover_scaling;
        wattch_regfiles[i].regfile_wordline *= crossover_scaling;
        wattch_regfiles[i].regfile_bitline *= crossover_scaling;
        wattch_regfiles[i].regfile_senseamp *= crossover_scaling;
    }
    
    power->rs_decoder *= crossover_scaling;
    power->rs_wordline *= crossover_scaling;
    power->rs_bitline *= crossover_scaling;
    power->rs_senseamp *= crossover_scaling;
    
    power->lsq_wakeup_tagdrive *= crossover_scaling;
    power->lsq_wakeup_tagmatch *= crossover_scaling;
    
    power->lsq_rs_decoder *= crossover_scaling;
    power->lsq_rs_wordline *= crossover_scaling;
    power->lsq_rs_bitline *= crossover_scaling;
    power->lsq_rs_senseamp *= crossover_scaling;
    
    power->resultbus *= crossover_scaling;
    
    power->btb *= crossover_scaling;
    power->local_predict *= crossover_scaling;
    power->global_predict *= crossover_scaling;
    power->chooser *= crossover_scaling;
    
    power->dtlb *= crossover_scaling;
    
    power->itlb *= crossover_scaling;
    
    power->icache_decoder *= crossover_scaling;
    power->icache_wordline*= crossover_scaling;
    power->icache_bitline *= crossover_scaling;
    power->icache_senseamp*= crossover_scaling;
    power->icache_tagarray*= crossover_scaling;
    
    power->icache_power *= crossover_scaling;
    
    power->dcache_decoder *= crossover_scaling;
    power->dcache_wordline *= crossover_scaling;
    power->dcache_bitline *= crossover_scaling;
    power->dcache_senseamp *= crossover_scaling;
    power->dcache_tagarray *= crossover_scaling;
    
    power->dcache_power *= crossover_scaling;
    
    power->clock_power *= crossover_scaling;
    
    power->dcache2_decoder *= crossover_scaling;
    power->dcache2_wordline *= crossover_scaling;
    power->dcache2_bitline *= crossover_scaling;
    power->dcache2_senseamp *= crossover_scaling;
    power->dcache2_tagarray *= crossover_scaling;
    
    power->dcache2_power *= crossover_scaling;
    
    power->total_power = power->local_predict + power->global_predict + 
    power->chooser + power->btb +
    power->rat_decoder + power->rat_wordline + 
    power->rat_bitline + power->rat_senseamp + 
    power->dcl_compare + power->dcl_pencode + 
    power->inst_decoder_power +
    power->wakeup_tagdrive + power->wakeup_tagmatch + 
    power->selection +
    /* power->regfile_decoder + power->regfile_wordline + 
    power->regfile_bitline + power->regfile_senseamp +  */
    power->rs_decoder + power->rs_wordline +
    power->rs_bitline + power->rs_senseamp + 
    power->lsq_wakeup_tagdrive + power->lsq_wakeup_tagmatch +
    power->lsq_rs_decoder + power->lsq_rs_wordline +
    power->lsq_rs_bitline + power->lsq_rs_senseamp +
    power->resultbus +
    power->clock_power +
    power->icache_power + 
    power->itlb + 
    power->dcache_power + 
    power->dtlb + 
    power->dcache2_power;
    
    for (i = 0; i < NUM_REGFILES; i++)
    {
        power->total_power += wattch_regfiles[i].regfile_decoder + wattch_regfiles[i].regfile_wordline +
        wattch_regfiles[i].regfile_bitline + wattch_regfiles[i].regfile_senseamp;
    }
    
    power->total_power_nodcache2 = power->local_predict + power->global_predict + 
    power->chooser + power->btb +
    power->rat_decoder + power->rat_wordline + 
    power->rat_bitline + power->rat_senseamp + 
    power->dcl_compare + power->dcl_pencode + 
    power->inst_decoder_power +
    power->wakeup_tagdrive + power->wakeup_tagmatch + 
    power->selection +
    /* power->regfile_decoder + power->regfile_wordline + 
    power->regfile_bitline + power->regfile_senseamp +  */
    power->rs_decoder + power->rs_wordline +
    power->rs_bitline + power->rs_senseamp + 
    power->lsq_wakeup_tagdrive + power->lsq_wakeup_tagmatch +
    power->lsq_rs_decoder + power->lsq_rs_wordline +
    power->lsq_rs_bitline + power->lsq_rs_senseamp +
    power->resultbus +
    power->clock_power +
    power->icache_power + 
    power->itlb + 
    power->dcache_power + 
    power->dtlb + 
    power->dcache2_power;
    
    for (i = 0; i < NUM_REGFILES; i++)
    {
        power->total_power_nodcache2 += wattch_regfiles[i].regfile_decoder + wattch_regfiles[i].regfile_wordline +
        wattch_regfiles[i].regfile_bitline + wattch_regfiles[i].regfile_senseamp;
    }
    
    power->bpred_power = power->btb + power->local_predict + power->global_predict + power->chooser + power->ras;
    
    power->rat_power = power->rat_decoder + 
    power->rat_wordline + power->rat_bitline + power->rat_senseamp;
    
    power->dcl_power = power->dcl_compare + power->dcl_pencode;
    
    power->rename_power = power->rat_power + 
    power->dcl_power + 
    power->inst_decoder_power;
    
    power->wakeup_power = power->wakeup_tagdrive + power->wakeup_tagmatch + 
    power->wakeup_ormatch;
    
    power->rs_power = power->rs_decoder + 
    power->rs_wordline + power->rs_bitline + power->rs_senseamp;
    
    power->rs_power_nobit = power->rs_decoder + 
    power->rs_wordline + power->rs_senseamp;
    
    power->window_power = power->wakeup_power + power->rs_power + 
    power->selection;
    
    power->lsq_rs_power = power->lsq_rs_decoder + 
    power->lsq_rs_wordline + power->lsq_rs_bitline + 
    power->lsq_rs_senseamp;
    
    power->lsq_rs_power_nobit = power->lsq_rs_decoder + 
    power->lsq_rs_wordline + power->lsq_rs_senseamp;
    
    power->lsq_wakeup_power = power->lsq_wakeup_tagdrive + power->lsq_wakeup_tagmatch;
    
    power->lsq_power = power->lsq_wakeup_power + power->lsq_rs_power;
    
    for (i = 0; i < NUM_REGFILES; i++)
    {
        wattch_regfiles[i].regfile_power = wattch_regfiles[i].regfile_decoder + 
        wattch_regfiles[i].regfile_wordline + wattch_regfiles[i].regfile_bitline + 
        wattch_regfiles[i].regfile_senseamp;
        
        wattch_regfiles[i].regfile_power_nobit = wattch_regfiles[i].regfile_decoder + 
        wattch_regfiles[i].regfile_wordline + wattch_regfiles[i].regfile_senseamp;
    }
    
    //dump_power_stats(power);
    
}

uint32 power_t::get_int_alu_count()
{
    return res_ialu;
}

uint32 power_t::get_fp_alu_count()
{
    return res_fpalu;
}


void power_t::memory_hierarchy_power()
{
    
    // tagsize = va_size - ((int)bct->logtwo(cache_dl1->nsets) + (int)bct->logtwo(cache_dl1->bsize));
    
    
    //     VERBOSE_OUT(verb_t::requests, "dtlb predict power stats\n");
    // power->dtlb = res_memport*(cam_array(dtlb->nsets, va_size - (int)bct->logtwo((double)dtlb->bsize),1,1) + power_util->simple_array_power(dtlb->nsets,tagsize,1,1,cache));
    
    // tagsize = va_size - ((int)bct->logtwo(cache_il1->nsets) + (int)bct->logtwo(cache_il1->bsize));
    
    // predeclength = itlb->nsets * (RegCellHeight + WordlineSpacing);
    // wordlinelength = bct->logtwo((double)itlb->bsize) * (RegCellWidth + BitlineSpacing);
    // bitlinelength = itlb->nsets * (RegCellHeight + WordlineSpacing);
    
    
    //     VERBOSE_OUT(verb_t::requests, "itlb predict power stats\n");
    // power->itlb = cam_array(itlb->nsets, va_size - (int)bct->logtwo((double)itlb->bsize),1,1) + power_util->simple_array_power(itlb->nsets,tagsize,1,1,cache);
    
    
    // cache=1;
    
    // time_parameters.cache_size = cache_il1->nsets * cache_il1->bsize * cache_il1->assoc; /* C */
    // time_parameters.block_size = cache_il1->bsize; /* B */
    // time_parameters.associativity = cache_il1->assoc; /* A */
    // time_parameters.number_of_sets = cache_il1->nsets; /* C/(B*A) */
    
    //calculate_time(&time_result,&time_parameters);
    //output_data(stderr,&time_result,&time_parameters);
    
    // ndwl=time_result.best_Ndwl;
    // ndbl=time_result.best_Ndbl;
    // nspd=time_result.best_Nspd;
    // ntwl=time_result.best_Ntwl;
    // ntbl=time_result.best_Ntbl;
    // ntspd=time_result.best_Ntspd;
    
    // c = time_parameters.cache_size;
    // b = time_parameters.block_size;
    // a = time_parameters.associativity;
    
    // rowsb = c/(b*a*ndbl*nspd);
    // colsb = 8*b*a*nspd/ndwl;
    
    // tagsize = va_size - ((int)bct->logtwo(cache_il1->nsets) + (int)bct->logtwo(cache_il1->bsize));
    // trowsb = c/(b*a*ntbl*ntspd);
    // tcolsb = a * (tagsize + 1 + 6) * ntspd/ntwl;
    
    //  {
    //     VERBOSE_OUT(verb_t::requests, "%d KB %d-way cache (%d-byte block size):\n",c,a,b);
    //     VERBOSE_OUT(verb_t::requests, "ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
    //     VERBOSE_OUT(verb_t::requests, "%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
    //     VERBOSE_OUT(verb_t::requests, "tagsize == %d\n",tagsize);
    // }
    
    // predeclength = rowsb * (RegCellHeight + WordlineSpacing);
    // wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
    // bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);
    
    
    //     VERBOSE_OUT(verb_t::requests, "icache power stats\n");
    // power->icache_decoder = ndwl*ndbl*array_decoder_power(rowsb,colsb,predeclength,1,1,cache);
    // power->icache_wordline = ndwl*ndbl*array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache);
    // power->icache_bitline = ndwl*ndbl*array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache);
    // power->icache_senseamp = ndwl*ndbl*senseamp_power(colsb);
    // power->icache_tagarray = ntwl*ntbl*(simple_array_power(trowsb,tcolsb,1,1,cache));
    
    // power->icache_power = power->icache_decoder + power->icache_wordline + power->icache_bitline + power->icache_senseamp + power->icache_tagarray;
    
    // time_parameters.cache_size = cache_dl1->nsets * cache_dl1->bsize * cache_dl1->assoc; /* C */
    // time_parameters.block_size = cache_dl1->bsize; /* B */
    // time_parameters.associativity = cache_dl1->assoc; /* A */
    // time_parameters.number_of_sets = cache_dl1->nsets; /* C/(B*A) */
    
    //calculate_time(&time_result,&time_parameters);
    //output_data(stderr,&time_result,&time_parameters);
    
    // ndwl=time_result.best_Ndwl;
    // ndbl=time_result.best_Ndbl;
    // nspd=time_result.best_Nspd;
    // ntwl=time_result.best_Ntwl;
    // ntbl=time_result.best_Ntbl;
    // ntspd=time_result.best_Ntspd;
    // c = time_parameters.cache_size;
    // b = time_parameters.block_size;
    // a = time_parameters.associativity; 
    
    // cache=1;
    
    // rowsb = c/(b*a*ndbl*nspd);
    // colsb = 8*b*a*nspd/ndwl;
    
    // tagsize = va_size - ((int)bct->logtwo(cache_dl1->nsets) + (int)bct->logtwo(cache_dl1->bsize));
    // trowsb = c/(b*a*ntbl*ntspd);
    // tcolsb = a * (tagsize + 1 + 6) * ntspd/ntwl;
    
    //  {
    //     VERBOSE_OUT(verb_t::requests, "%d KB %d-way cache (%d-byte block size):\n",c,a,b);
    //     VERBOSE_OUT(verb_t::requests, "ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
    //     VERBOSE_OUT(verb_t::requests, "%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
    //     VERBOSE_OUT(verb_t::requests, "tagsize == %d\n",tagsize);
        
    //     VERBOSE_OUT(verb_t::requests, "\nntwl == %d, ntbl == %d, ntspd == %d\n",ntwl,ntbl,ntspd);
    //     VERBOSE_OUT(verb_t::requests, "%d sets of %d rows x %d cols\n",ntwl*ntbl,trowsb,tcolsb);
    // }
    
    // predeclength = rowsb * (RegCellHeight + WordlineSpacing);
    // wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
    // bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);
    
    
    //     VERBOSE_OUT(verb_t::requests, "dcache power stats\n");
    // power->dcache_decoder = res_memport*ndwl*ndbl*array_decoder_power(rowsb,colsb,predeclength,1,1,cache);
    // power->dcache_wordline = res_memport*ndwl*ndbl*array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache);
    // power->dcache_bitline = res_memport*ndwl*ndbl*array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache);
    // power->dcache_senseamp = res_memport*ndwl*ndbl*senseamp_power(colsb);
    // power->dcache_tagarray = res_memport*ntwl*ntbl*(simple_array_power(trowsb,tcolsb,1,1,cache));
    
    // power->dcache_power = power->dcache_decoder + power->dcache_wordline + power->dcache_bitline + power->dcache_senseamp + power->dcache_tagarray;
    
    // clockpower = total_clockpower(.018);
    // power->clock_power = clockpower;
    //  {
    //     VERBOSE_OUT(verb_t::requests, "result bus power == %f\n",power->resultbus);
    //     VERBOSE_OUT(verb_t::requests, "global clock power == %f\n",clockpower);
    // }
    
    // time_parameters.cache_size = cache_dl2->nsets * cache_dl2->bsize * cache_dl2->assoc; /* C */
    // time_parameters.block_size = cache_dl2->bsize; /* B */
    // time_parameters.associativity = cache_dl2->assoc; /* A */
    // time_parameters.number_of_sets = cache_dl2->nsets; /* C/(B*A) */
    
    //calculate_time(&time_result,&time_parameters);
    //output_data(stderr,&time_result,&time_parameters);
    
    // ndwl=time_result.best_Ndwl;
    // ndbl=time_result.best_Ndbl;
    // nspd=time_result.best_Nspd;
    // ntwl=time_result.best_Ntwl;
    // ntbl=time_result.best_Ntbl;
    // ntspd=time_result.best_Ntspd;
    // c = time_parameters.cache_size;
    // b = time_parameters.block_size;
    // a = time_parameters.associativity;
    
    // rowsb = c/(b*a*ndbl*nspd);
    // colsb = 8*b*a*nspd/ndwl;
    
    // tagsize = va_size - ((int)bct->logtwo(cache_dl2->nsets) + (int)bct->logtwo(cache_dl2->bsize));
    // trowsb = c/(b*a*ntbl*ntspd);
    // tcolsb = a * (tagsize + 1 + 6) * ntspd/ntwl;
    
    //  {
    //     VERBOSE_OUT(verb_t::requests, "%d KB %d-way cache (%d-byte block size):\n",c,a,b);
    //     VERBOSE_OUT(verb_t::requests, "ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
    //     VERBOSE_OUT(verb_t::requests, "%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
    //     VERBOSE_OUT(verb_t::requests, "tagsize == %d\n",tagsize);
    // }
    
    // predeclength = rowsb * (RegCellHeight + WordlineSpacing);
    // wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
    // bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);
    
    
    //     VERBOSE_OUT(verb_t::requests, "dcache2 power stats\n");
    // power->dcache2_decoder = power_util->array_decoder_power(rowsb,colsb,predeclength,1,1,cache);
    // power->dcache2_wordline = power_util->array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache);
    // power->dcache2_bitline = power_util->array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache);
    // power->dcache2_senseamp = power_util->senseamp_power(colsb);
    // power->dcache2_tagarray = power_util->simple_array_power(trowsb,tcolsb,1,1,cache);
    
    // power->dcache2_power = power->dcache2_decoder + power->dcache2_wordline + power->dcache2_bitline + power->dcache2_senseamp + power->dcache2_tagarray;
    
}


void power_t::branch_predictor_power()
{
    
 
    // /* Load cache values into what cacti is expecting */
    // time_parameters.cache_size = btb_config[0] * (g_conf_addr_width/8) * btb_config[1]; /* C */
    // time_parameters.block_size = (g_conf_addr_width/8); /* B */
    // time_parameters.associativity = btb_config[1]; /* A */
    // time_parameters.number_of_sets = btb_config[0]; /* C/(B*A) */
    
    // /* have Cacti compute optimal cache config */
    //calculate_time(&time_result,&time_parameters);
    //output_data(stderr,&time_result,&time_parameters);
    
    // /* extract Cacti results */
    // ndwl=time_result.best_Ndwl;
    // ndbl=time_result.best_Ndbl;
    // nspd=time_result.best_Nspd;
    // ntwl=time_result.best_Ntwl;
    // ntbl=time_result.best_Ntbl;
    // ntspd=time_result.best_Ntspd;
    // c = time_parameters.cache_size;
    // b = time_parameters.block_size;
    // a = time_parameters.associativity; 
    
    // cache=1;
    
    // /* Figure out how many rows/cols there are now */
    // rowsb = c/(b*a*ndbl*nspd);
    // colsb = 8*b*a*nspd/ndwl;
    
    //  {
    //     VERBOSE_OUT(verb_t::requests, "%d KB %d-way btb (%d-byte block size):\n",c,a,b);
    //     VERBOSE_OUT(verb_t::requests, "ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
    //     VERBOSE_OUT(verb_t::requests, "%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
    // }
    
    // predeclength = rowsb * (RegCellHeight + WordlineSpacing);
    // wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
    // bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);
    
    
    //     VERBOSE_OUT(verb_t::requests, "btb power stats\n");
    // power->btb = ndwl*ndbl*(power_util->array_decoder_power(rowsb,colsb,predeclength,1,1,cache)
    //     + power_util->array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache)
    //     + power_util->array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache)
    //     + power_util->senseamp_power(colsb));
    
    // cache=1;
    
    // scale_factor = power_util->squarify(twolev_config[0],twolev_config[2]);
    // predeclength = (twolev_config[0] / scale_factor)* (RegCellHeight + WordlineSpacing);
    // wordlinelength = twolev_config[2] * scale_factor *  (RegCellWidth + BitlineSpacing);
    // bitlinelength = (twolev_config[0] / scale_factor) * (RegCellHeight + WordlineSpacing);
    
    
    //     VERBOSE_OUT(verb_t::requests, "local predict power stats\n");
    
    // power->local_predict = power_util->array_decoder_power(twolev_config[0]/scale_factor,twolev_config[2]*scale_factor,predeclength,1,1,cache) + power_util->array_wordline_power(twolev_config[0]/scale_factor,twolev_config[2]*scale_factor,wordlinelength,1,1,cache) + power_util->array_bitline_power(twolev_config[0]/scale_factor,twolev_config[2]*scale_factor,bitlinelength,1,1,cache) + power_util->senseamp_power(twolev_config[2]*scale_factor);
    
    // scale_factor = power_util->squarify(twolev_config[1],3);
    
    // predeclength = (twolev_config[1] / scale_factor)* (RegCellHeight + WordlineSpacing);
    // wordlinelength = 3 * scale_factor *  (RegCellWidth + BitlineSpacing);
    // bitlinelength = (twolev_config[1] / scale_factor) * (RegCellHeight + WordlineSpacing);
    
    
    
    //     VERBOSE_OUT(verb_t::requests, "local predict power stats\n");
    // power->local_predict += power_util->array_decoder_power(twolev_config[1]/scale_factor,3*scale_factor,predeclength,1,1,cache) + power_util->array_wordline_power(twolev_config[1]/scale_factor,3*scale_factor,wordlinelength,1,1,cache) + power_util->array_bitline_power(twolev_config[1]/scale_factor,3*scale_factor,bitlinelength,1,1,cache) + power_util->senseamp_power(3*scale_factor);
    
    
    //     VERBOSE_OUT(verb_t::requests, "bimod_config[0] == %d\n",bimod_config[0]);
    
    // scale_factor = power_util->squarify(bimod_config[0],2);
    
    // predeclength = bimod_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);
    // wordlinelength = 2*scale_factor *  (RegCellWidth + BitlineSpacing);
    // bitlinelength = bimod_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);
    
    
    
    //     VERBOSE_OUT(verb_t::requests, "global predict power stats\n");
    // power->global_predict = power_util->array_decoder_power(bimod_config[0]/scale_factor,2*scale_factor,predeclength,1,1,cache) + power_util->array_wordline_power(bimod_config[0]/scale_factor,2*scale_factor,wordlinelength,1,1,cache) + power_util->array_bitline_power(bimod_config[0]/scale_factor,2*scale_factor,bitlinelength,1,1,cache) + power_util->senseamp_power(2*scale_factor);
    
    // scale_factor = power_util->squarify(comb_config[0],2);
    
    // predeclength = comb_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);
    // wordlinelength = 2*scale_factor *  (RegCellWidth + BitlineSpacing);
    // bitlinelength = comb_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);
    
    
    //     VERBOSE_OUT(verb_t::requests, "chooser predict power stats\n");
    // power->chooser = power_util->array_decoder_power(comb_config[0]/scale_factor,2*scale_factor,predeclength,1,1,cache) + power_util->array_wordline_power(comb_config[0]/scale_factor,2*scale_factor,wordlinelength,1,1,cache) + power_util->array_bitline_power(comb_config[0]/scale_factor,2*scale_factor,bitlinelength,1,1,cache) + power_util->senseamp_power(2*scale_factor);
    
    
    //     VERBOSE_OUT(verb_t::requests, "RAS predict power stats\n");
    // power->ras = power_util->simple_array_power(g_conf_ras_table_bits,g_conf_addr_width,1,1,0);
    
       
    
}
