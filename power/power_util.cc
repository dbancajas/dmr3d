
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Implementation file for the power utility class
 * initial author: Koushik Chakraborty
 *
 */ 


//  #include "simics/first.h"
// RCSID("$Id: power_util.cc,v 1.10.2.17 2006/03/02 23:58:42 kchak Exp $");

#include "definitions.h"
#include "config_extern.h" 
#include "verbose_level.h"

#include "profiles.h"
#include "circuit_param.h"
#include "cacti_def.h"
#include "cacti_interface.h"
#include "basic_circuit.h"
#include "power_defs.h"
#include "power_util.h"

power_util_t::power_util_t(uint32 _ia, uint32 _fa, basic_circuit_t *_bct)
: int_alu_count(_ia), fp_alu_count(_fa), bct(_bct)
{
    // Initialilation
    global_clockcap = 0;
    
    //Powerfactor = Mhz * circuit_param_t::cparam_VddPow * circuit_param_t::cparam_VddPow;
    Powerfactor = Powerfactor_ORIG;
    
}
 
/*======================================================================*/



/* 
 * This part of the code contains routines for each section as
 * described in the tech report.  See the tech report for more details
 * and explanations */

/*----------------------------------------------------------------------*/

double power_util_t::driver_size(double driving_cap, double desiredrisetime) 
{
    double nsize, psize;
    double Rpdrive; 
    
    Rpdrive = desiredrisetime/(driving_cap*log(VSINV)*-1.0);
    psize = bct->restowidth(Rpdrive,PCH);
    nsize = bct->restowidth(Rpdrive,NCH);
    if (psize > circuit_param_t::cparam_Wworddrivemax) {
        psize = circuit_param_t::cparam_Wworddrivemax;
    }
    if (psize < 4.0 * LSCALE)
        psize = 4.0 * LSCALE;
    
    return (psize);
    
}


uint64 power_util_t::pop_count64(uint64 bits)
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


/* very rough clock power estimates */
double power_util_t::total_clockpower(double die_length)
{
    
    double clocklinelength;
    double Cline,Cline2,Ctotal;
    double pipereg_clockcap=0;
    double global_buffercap = 0;
    double Clockpower;
    
    double num_piperegs;
    
    int npreg_width = (int)ceil(bct->logtwo((double)g_conf_window_size));
    
    /* Assume say 8 stages (kinda low now).
    FIXME: this could be a lot better; user could input
    number of pipestages, etc  */
    
    /* assume 8 pipe stages and try to estimate bits per pipe stage */
    /* pipe stage 0/1 */
    num_piperegs = g_conf_max_issue*g_conf_inst_length + g_conf_data_width;
    /* pipe stage 1/2 */
    num_piperegs += g_conf_max_issue*(g_conf_inst_length + 3 * g_conf_window_size);
    /* pipe stage 2/3 */
    num_piperegs += g_conf_max_issue*(g_conf_inst_length + 3 * g_conf_window_size);
    /* pipe stage 3/4 */
    num_piperegs += g_conf_max_issue*(3 * npreg_width + bct->pow2((double)g_conf_opcode_length));
    /* pipe stage 4/5 */
    num_piperegs += g_conf_max_issue*(2*g_conf_data_width + bct->pow2((double)g_conf_opcode_length));
    /* pipe stage 5/6 */
    num_piperegs += g_conf_max_issue*(g_conf_data_width + bct->pow2((double)g_conf_opcode_length));
    /* pipe stage 6/7 */
    num_piperegs += g_conf_max_issue*(g_conf_data_width + bct->pow2((double)g_conf_opcode_length));
    /* pipe stage 7/8 */
    num_piperegs += g_conf_max_issue*(g_conf_data_width + bct->pow2((double)g_conf_opcode_length));
    
    /* assume 50% extra in control signals (rule of thumb) */
    num_piperegs = num_piperegs * 1.5;
    
    pipereg_clockcap = num_piperegs * 4*bct->gatecap(10.0,0);
    
    /* estimate based on 3% of die being in clock metal */
    Cline2 = Default_Cmetal * (.03 * die_length * die_length/BitlineSpacing) * 1e6 * 1e6;
    
    /* another estimate */
    clocklinelength = die_length*(.5 + 4 * (.25 + 2*(.25) + 4 * (.125)));
    Cline = 20 * Default_Cmetal * (clocklinelength) * 1e6;
    global_buffercap = 12*bct->gatecap(1000.0,10.0)+16*bct->gatecap(200,10.0)+
        16*8*2*bct->gatecap(100.0,10.00) + 2*bct->gatecap(.29*1e6,10.0);
    /* global_clockcap is computed within each array structure for pre-charge tx's*/
    Ctotal = Cline+global_clockcap+pipereg_clockcap+global_buffercap;
    
    
    VERBOSE_OUT(verb_t::power,"num_piperegs == %f powerfactor %f Vdd = %f\n",num_piperegs, Powerfactor,
        Vdd);
    
    /* add I_ADD Clockcap and F_ADD Clockcap */
    Clockpower = Ctotal*Powerfactor + int_alu_count*I_ADD_CLOCK * Powerfactor 
        + fp_alu_count*F_ADD_CLOCK * Powerfactor;
    
    
    VERBOSE_OUT(verb_t::power,"Global Clock Power: %g\n",Clockpower);
    VERBOSE_OUT(verb_t::power," Global Metal Lines   (W): %g\n",Cline*Powerfactor);
    VERBOSE_OUT(verb_t::power," Global Metal Lines (3%%) (W): %g\n",Cline2*Powerfactor);
    VERBOSE_OUT(verb_t::power," Global Clock Buffers (W): %g\n",global_buffercap*Powerfactor);
    VERBOSE_OUT(verb_t::power," Global Clock Cap (Explicit) (W): %g\n",global_clockcap*Powerfactor+I_ADD_CLOCK+F_ADD_CLOCK);
    VERBOSE_OUT(verb_t::power," Global Clock Cap (Implicit) (W): %g\n",pipereg_clockcap*Powerfactor);
    
    return(Clockpower);

}


/* very rough global clock power estimates */
double power_util_t::global_clockpower(double die_length)
{
    
    double clocklinelength;
    double Cline,Cline2,Ctotal;
    double global_buffercap = 0;
    
    Cline2 = Default_Cmetal * (.03 * die_length * die_length/BitlineSpacing) * 1e6 * 1e6;
    
    clocklinelength = die_length*(.5 + 4 * (.25 + 2*(.25) + 4 * (.125)));
    Cline = 20 * Default_Cmetal * (clocklinelength) * 1e6;
    global_buffercap = 12*bct->gatecap(1000.0,10.0)+16*bct->gatecap(200,10.0)+16*8*2*bct->gatecap(100.0,10.00) + 2*bct->gatecap(.29*1e6,10.0);
    Ctotal = Cline+global_buffercap;
    
     {
        VERBOSE_OUT(verb_t::power,"Global Clock Power: %g\n",Ctotal*Powerfactor);
        VERBOSE_OUT(verb_t::power," Global Metal Lines   (W): %g\n",Cline*Powerfactor);
        VERBOSE_OUT(verb_t::power," Global Metal Lines (3%%) (W): %g\n",Cline2*Powerfactor);
        VERBOSE_OUT(verb_t::power," Global Clock Buffers (W): %g\n",global_buffercap*Powerfactor);
    }
    
    return(Ctotal*Powerfactor);
    
}



double power_util_t::cam_array(int rows,int cols,int rports,int wports)
{
  return(cam_tagdrive(rows,cols,rports,wports) +
	 cam_tagmatch(rows,cols,rports,wports));
}


double power_util_t::selection_power(int win_entries)
{
  double Ctotal, Cor, Cpencode;
  int num_arbiter=1;

  Ctotal=0;

  while(win_entries > 4)
    {
      win_entries = (int)ceil((double)win_entries / 4.0);
      num_arbiter += win_entries;
    }

  Cor = 4 * bct->draincap(WSelORn,NCH,1) + bct->draincap(WSelORprequ,PCH,1);

  Cpencode = bct->draincap(WSelPn,NCH,1) + bct->draincap(WSelPp,PCH,1) + 
    2*bct->draincap(WSelPn,NCH,1) + bct->draincap(WSelPp,PCH,2) + 
    3*bct->draincap(WSelPn,NCH,1) + bct->draincap(WSelPp,PCH,3) + 
    4*bct->draincap(WSelPn,NCH,1) + bct->draincap(WSelPp,PCH,4) + 
    4*bct->gatecap(WSelEnn+WSelEnp,20.0) + 
    4*bct->draincap(WSelEnn,NCH,1) + 4*bct->draincap(WSelEnp,PCH,1);

  Ctotal += g_conf_max_issue * num_arbiter*(Cor+Cpencode);

  return(Ctotal*Powerfactor*STATIC_ACTIVITY_FACTORY);
}




double power_util_t::compute_resultbus_power()
{
  double Ctotal, Cline;

  double regfile_height;

  /* compute size of result bus tags */
  int npreg_width = (int)ceil(bct->logtwo((double)g_conf_window_size));

  Ctotal=0;

  regfile_height = g_conf_window_size * (RegCellHeight + 
			       WordlineSpacing * 3 * g_conf_max_issue); 

  /* assume num alu's == ialu  (FIXME: generate a more detailed result bus network model*/
  Cline = Default_Cmetal * (regfile_height + .5 * int_alu_count * 3200.0 * LSCALE);

  /* or use result bus length measured from 21264 die photo */
  /*  Cline = Default_Cmetal * 3.3*1000;*/

  /* Assume g_conf_max_issue result busses -- power can be scaled linearly
     for number of result busses (scale by writeback_access) */
  Ctotal += 2.0 * (g_conf_data_width + npreg_width) * (g_conf_max_issue)* Cline;

#ifdef STATIC_AF
  return(Ctotal*Powerfactor*STATIC_ACTIVITY_FACTORY);
#else
  return(Ctotal*Powerfactor);
#endif
  
}



/* power of depency check logic */
double power_util_t::dcl_compare_power(int compare_bits)
{
  double Ctotal;
  int num_comparators;
  
  num_comparators = (g_conf_max_fetch - 1) * (g_conf_max_fetch);

  Ctotal = num_comparators * compare_cap(compare_bits);

  return(Ctotal*Powerfactor*STATIC_ACTIVITY_FACTORY);
}


double power_util_t::simple_array_power(int rows,int cols,int rports,int wports,int cache)
{
  if(cache==0)
    return( simple_array_decoder_power(rows,cols,rports,wports,cache)+
	    simple_array_wordline_power(rows,cols,rports,wports,cache)+
	    simple_array_bitline_power(rows,cols,rports,wports,cache));
  else
    return( simple_array_decoder_power(rows,cols,rports,wports,cache)+
	    simple_array_wordline_power(rows,cols,rports,wports,cache)+
	    simple_array_bitline_power(rows,cols,rports,wports,cache)+
	    senseamp_power(cols));
}


double power_util_t::cam_tagdrive(int rows,int cols,int rports,int wports)

{
    double Ctotal, Ctlcap, Cblcap, Cwlcap;
    double taglinelength;
    double wordlinelength;
    double nsize, psize;
    int ports;
    Ctotal=0;
    
    ports = rports + wports;
    
    taglinelength = rows * 
    (CamCellHeight + ports * MatchlineSpacing);
    
    wordlinelength = cols * 
    (CamCellWidth + ports * TaglineSpacing);
    
    /* Compute tagline cap */
    Ctlcap = Default_Cmetal * taglinelength + 
    rows * bct->gatecappass(Wcomparen2,2.0) +
    bct->draincap(Wcompdrivern,NCH,1)+bct->draincap(Wcompdriverp,PCH,1);
    
    /* Compute bitline cap (for writing new tags) */
    Cblcap = Default_Cmetal * taglinelength +
    rows * bct->draincap(Wmemcellr,NCH,2);
    
    /* autosize wordline driver */
    psize = driver_size(Default_Cmetal * wordlinelength + 2 * cols * bct->gatecap(Wmemcellr,2.0),Period/8);
    nsize = psize * circuit_param_t::cparam_Wdecinvn/circuit_param_t::circuit_param_t::cparam_Wdecinvp; 
    
    /* Compute wordline cap (for writing new tags) */
    Cwlcap = Default_Cmetal * wordlinelength + 
    bct->draincap(nsize,NCH,1)+bct->draincap(psize,PCH,1) +
    2 * cols * bct->gatecap(Wmemcellr,2.0);
    
    Ctotal += (rports * cols * 2 * Ctlcap) + 
    (wports * ((cols * 2 * Cblcap) + (rows * Cwlcap)));
    
    return(Ctotal*Powerfactor*STATIC_ACTIVITY_FACTORY);
}

double power_util_t::cam_tagmatch(int rows,int cols,int rports,int wports)
{
    double Ctotal, Cmlcap;
    double matchlinelength;
    int ports;
    Ctotal=0;
    
    ports = rports + wports;
    
    matchlinelength = cols * 
    (CamCellWidth + ports * TaglineSpacing);
    
    Cmlcap = 2 * cols * bct->draincap(Wcomparen1,NCH,2) + 
    Default_Cmetal * matchlinelength + bct->draincap(Wmatchpchg,NCH,1) +
    bct->gatecap(Wmatchinvn+Wmatchinvp,10.0) +
    bct->gatecap(Wmatchnandn+Wmatchnandp,10.0);
    
    Ctotal += rports * rows * Cmlcap;
    
    global_clockcap += rports * rows * bct->gatecap(Wmatchpchg,5.0);
    
    /* noring the nanded match lines */
    if(g_conf_max_issue >= 8)
        Ctotal += 2 * bct->gatecap(Wmatchnorn+Wmatchnorp,10.0);
    
    return(Ctotal*Powerfactor*STATIC_ACTIVITY_FACTORY);
}




/* this routine takes the number of rows and cols of an array structure
   and attemps to make it make it more of a reasonable circuit structure
   by trying to make the number of rows and cols as close as possible.
   (scaling both by factors of 2 in opposite directions).  it returns
   a scale factor which is the amount that the rows should be divided
   by and the columns should be multiplied by.
*/
int power_util_t::squarify(int rows, int cols)
{
  int scale_factor = 1;

  if(rows == cols)
    return 1;

  /*
  printf("init rows == %d\n",rows);
  printf("init cols == %d\n",cols);
  */

  while(rows > cols) {
    rows = rows/2;
    cols = cols*2;

    /*
    printf("rows == %d\n",rows);
    printf("cols == %d\n",cols);
    printf("scale_factor == %d (2^ == %d)\n\n",scale_factor,(int)pow(2.0,(double)scale_factor));
    */

    if (rows/2 <= cols)
      return((int)pow(2.0,(double)scale_factor));
    scale_factor++;
  }

  return 1;
}

/* could improve squarify to work when rows < cols */

double power_util_t::squarify_new(int rows, int cols)
{
  double scale_factor = 0.0;

  if(rows==cols)
    return(pow(2.0,scale_factor));

  while(rows > cols) {
    rows = rows/2;
    cols = cols*2;
    if (rows <= cols)
      return(pow(2.0,scale_factor));
    scale_factor++;
  }

  while(cols > rows) {
    rows = rows*2;
    cols = cols/2;
    if (cols <= rows)
      return(pow(2.0,scale_factor));
    scale_factor--;
  }

  return 1;

}



double power_util_t::simple_array_bitline_power(int rows,int cols,int rports,int wports,int cache)
{
  double bitlinelength;

  int ports = rports + wports;

  bitlinelength = rows * (RegCellHeight + ports * WordlineSpacing);

  return (array_bitline_power(rows,cols,bitlinelength,rports,wports,cache));

}

/* estimate senseamp power dissipation in cache structures (Zyuban's method) */
double power_util_t::senseamp_power(int cols)
{
  return((double)cols * Vdd/8 * .5e-3);
}

/* estimate comparator power consumption (this comparator is similar
   to the tag-match structure in a CAM */
double power_util_t::compare_cap(int compare_bits)
{
  double c1, c2;
  /* bottom part of comparator */
  c2 = (compare_bits)*(bct->draincap(circuit_param_t::cparam_Wcompn,NCH,1)+
      bct->draincap(circuit_param_t::cparam_Wcompn,NCH,2))+
    bct->draincap(circuit_param_t::cparam_Wevalinvp,PCH,1) + 
    bct->draincap(circuit_param_t::cparam_Wevalinvn,NCH,1);

  /* top part of comparator */
  c1 = (compare_bits)*(bct->draincap(circuit_param_t::cparam_Wcompn,NCH,1)+
      bct->draincap(circuit_param_t::cparam_Wcompn,NCH,2)+
	  bct->draincap(circuit_param_t::cparam_Wcomppreequ,NCH,1)) + 
      bct->gatecap(circuit_param_t::cparam_WdecNORn,1.0)+
      bct->gatecap(circuit_param_t::cparam_WdecNORp,3.0);

  return(c1 + c2);
}



double power_util_t::array_wordline_power(int rows,int cols,double wordlinelength,int rports,
    int wports,int cache)
{
    double Ctotal=0;
    double Ceq=0;
    double Cline=0;
    double Cliner, Clinew=0;
    double desiredrisetime,psize,nsize;
    int ports;
    double colsb;
    
    ports = rports+wports;
    
    colsb = (double)cols;
    
    /* Calculate size of wordline drivers assuming rise time == Period / 8 
    - estimate cap on line 
    - compute min resistance to achieve this with RC 
    - compute width needed to achieve this resistance */
    
    desiredrisetime = Period/16;
    Cline = (bct->gatecappass(Wmemcellr,1.0))*colsb + wordlinelength*circuit_param_t::cparam_CM3metal;
    psize = driver_size(Cline,desiredrisetime);
    
    /* how do we want to do p-n ratioing? -- here we just assume the same ratio 
    from an inverter pair  */
    nsize = psize * circuit_param_t::cparam_Wdecinvn/circuit_param_t::cparam_Wdecinvp; 
    
    
        VERBOSE_OUT(verb_t::power,"Wordline Driver Sizes -- nsize == %f, psize == %f\n",nsize,psize);
    
    Ceq = bct->draincap(circuit_param_t::cparam_Wdecinvn,NCH,1) + bct->draincap(circuit_param_t::circuit_param_t::cparam_Wdecinvp,PCH,1) +
    bct->gatecap(nsize+psize,20.0);
    
    Ctotal+=ports*Ceq;
    
    
        VERBOSE_OUT(verb_t::power,"Wordline -- Inverter -> Driver         == %g\n",ports*Ceq*Powerfactor);
    
    /* Compute caps of read wordline and write wordlines 
    - wordline driver caps, given computed width from above
    - read wordlines have 1 nmos access tx, size ~4
    - write wordlines have 2 nmos access tx, size ~2
    - metal line cap
    */
    
    Cliner = (bct->gatecappass(Wmemcellr,(circuit_param_t::cparam_BitWidth-2*Wmemcellr)/2.0))*colsb+
    wordlinelength*circuit_param_t::cparam_CM3metal+
    2.0*(bct->draincap(nsize,NCH,1) + bct->draincap(psize,PCH,1));
    Clinew = (2.0*bct->gatecappass(Wmemcellw,(circuit_param_t::cparam_BitWidth-2*Wmemcellw)/2.0))*colsb+
    wordlinelength*circuit_param_t::cparam_CM3metal+
    2.0*(bct->draincap(nsize,NCH,1) + bct->draincap(psize,PCH,1));
    
     {
        VERBOSE_OUT(verb_t::power,"Wordline -- Line                       == %g\n",1e12*Cline);
        VERBOSE_OUT(verb_t::power,"Wordline -- Line -- access -- bct->gatecap  == %g\n",1e12*colsb*2*bct->gatecappass(circuit_param_t::cparam_Wmemcella,(circuit_param_t::cparam_BitWidth-2*circuit_param_t::cparam_Wmemcella)/2.0));
        VERBOSE_OUT(verb_t::power,"Wordline -- Line -- driver -- bct->draincap == %g\n",1e12*bct->draincap(nsize,NCH,1) + bct->draincap(psize,PCH,1));
        VERBOSE_OUT(verb_t::power,"Wordline -- Line -- metal              == %g\n",1e12*wordlinelength*circuit_param_t::cparam_CM3metal);
    }
    Ctotal+=rports*Cliner+wports*Clinew;
    
    /* STATIC_ACTIVITY_FACTORY == 1 assuming a different wordline is charged each cycle, but only
    1 wordline (per port) is actually used */
    
    return(Ctotal*Powerfactor);
}

double power_util_t::simple_array_wordline_power(int rows,int cols,int rports,int wports,int cache)
{

    double wordlinelength;
    int ports = rports + wports;
    wordlinelength = cols *  (RegCellWidth + 2 * ports * BitlineSpacing);
    return(array_wordline_power(rows,cols,wordlinelength,rports,wports,cache));
}


double power_util_t::array_bitline_power(int rows,int cols,double bitlinelength,int rports,
    int wports,int cache)
{
    double Ctotal=0;
    double Ccolmux=0;
    double Cbitrowr=0;
    double Cbitroww=0;
    double Cprerow=0;
    double Cwritebitdrive=0;
    double Cpregate=0;
    double Cliner=0;
    double Clinew=0;
    int ports;
    double rowsb;
    double colsb;
    
    double desiredrisetime, Cline, psize, nsize;
    
    ports = rports + wports;
    
    rowsb = (double)rows;
    colsb = (double)cols;
    
    /* Bct->draincaps of access tx's */
    
    Cbitrowr = bct->draincap(Wmemcellr,NCH,1);
    Cbitroww = bct->draincap(Wmemcellw,NCH,1);
    
    /* Cprerow -- precharge cap on the bitline
    -simple scheme to estimate size of pre-charge tx's in a similar fashion
    to wordline driver size estimation.
    -FIXME: it would be better to use precharge/keeper pairs, i've omitted this
    from this version because it couldn't autosize as easily.
    */
    
    desiredrisetime = Period/8;
    
    Cline = rowsb*Cbitrowr+circuit_param_t::cparam_CM3metal*bitlinelength;
    psize = driver_size(Cline,desiredrisetime);
    
    /* compensate for not having an nmos pre-charging */
    psize = psize + psize * circuit_param_t::cparam_Wdecinvn/circuit_param_t::circuit_param_t::cparam_Wdecinvp; 
    Cprerow = bct->draincap(psize,PCH,1);
    
    /* Cpregate -- cap due to bct->gatecap of precharge transistors -- tack this
    onto bitline cap, again this could have a keeper */
    Cpregate = 4.0*bct->gatecap(psize,10.0);
    global_clockcap+=rports*cols*2.0*Cpregate;
    
    /* Cwritebitdrive -- write bitline drivers are used instead of the precharge
    stuff for write bitlines
    - 2 inverter drivers within each driver pair */
    
    Cline = rowsb*Cbitroww+circuit_param_t::cparam_CM3metal*bitlinelength;
    
    psize = driver_size(Cline,desiredrisetime);
    nsize = psize * circuit_param_t::cparam_Wdecinvn/circuit_param_t::circuit_param_t::cparam_Wdecinvp; 
    
    Cwritebitdrive = 2.0*(bct->draincap(psize,PCH,1)+bct->draincap(nsize,NCH,1));
    
    /* 
    reg files (cache==0) 
    => single ended bitlines (1 bitline/col)
    => STATIC_ACTIVITY_FACTORYs from pop_count
    caches (cache ==1)
    => double-ended bitlines (2 bitlines/col)
    => STATIC_ACTIVITY_FACTORYs = .5 (since one of the two bitlines is always charging/discharging)
    */
    
    #ifdef STATIC_AF
    if (cache == 0) {
        /* compute the total line cap for read/write bitlines */
        Cliner = rowsb*Cbitrowr+circuit_param_t::cparam_CM3metal*bitlinelength+Cprerow;
        Clinew = rowsb*Cbitroww+circuit_param_t::cparam_CM3metal*bitlinelength+Cwritebitdrive;
        
        /* Bitline inverters at the end of the bitlines (replaced w/ sense amps
            in cache styles) */
        Ccolmux = bct->gatecap(MSCALE*(29.9+7.8),0.0)+bct->gatecap(MSCALE*(47.0+12.0),0.0);
        Ctotal+=(1.0-POPCOUNT_AF)*rports*cols*(Cliner+Ccolmux+2.0*Cpregate);
        Ctotal+=.3*wports*cols*(Clinew+Cwritebitdrive);
    } 
    else { 
        Cliner = rowsb*Cbitrowr+circuit_param_t::cparam_CM3metal*bitlinelength+Cprerow + bct->draincap(Wbitmuxn,NCH,1);
        Clinew = rowsb*Cbitroww+circuit_param_t::cparam_CM3metal*bitlinelength+Cwritebitdrive;
        Ccolmux = (bct->draincap(Wbitmuxn,NCH,1))+2.0*bct->gatecap(WsenseQ1to4,10.0);
        Ctotal+=.5*rports*2.0*cols*(Cliner+Ccolmux+2.0*Cpregate);
        Ctotal+=.5*wports*2.0*cols*(Clinew+Cwritebitdrive);
    }
    #else
    if (cache == 0) {
        /* compute the total line cap for read/write bitlines */
        Cliner = rowsb*Cbitrowr+circuit_param_t::cparam_CM3metal*bitlinelength+Cprerow;
        Clinew = rowsb*Cbitroww+circuit_param_t::cparam_CM3metal*bitlinelength+Cwritebitdrive;
        
        /* Bitline inverters at the end of the bitlines (replaced w/ sense amps
            in cache styles) */
        Ccolmux = bct->gatecap(MSCALE*(29.9+7.8),0.0)+bct->gatecap(MSCALE*(47.0+12.0),0.0);
        Ctotal += rports*cols*(Cliner+Ccolmux+2.0*Cpregate);
        Ctotal += .3*wports*cols*(Clinew+Cwritebitdrive);
    } 
    else { 
        Cliner = rowsb*Cbitrowr+circuit_param_t::cparam_CM3metal*bitlinelength+Cprerow + bct->draincap(Wbitmuxn,NCH,1);
        Clinew = rowsb*Cbitroww+circuit_param_t::cparam_CM3metal*bitlinelength+Cwritebitdrive;
        Ccolmux = (bct->draincap(Wbitmuxn,NCH,1))+2.0*bct->gatecap(WsenseQ1to4,10.0);
        Ctotal+=.5*rports*2.0*cols*(Cliner+Ccolmux+2.0*Cpregate);
        Ctotal+=.5*wports*2.0*cols*(Clinew+Cwritebitdrive);
    }
    #endif
    
     {
        VERBOSE_OUT(verb_t::power,"Bitline -- Precharge                   == %g\n",1e12*Cpregate);
        VERBOSE_OUT(verb_t::power,"Bitline -- Line                        == %g\n",1e12*(Cliner+Clinew));
        VERBOSE_OUT(verb_t::power,"Bitline -- Line -- access bct->draincap     == %g\n",1e12*rowsb*Cbitrowr);
        VERBOSE_OUT(verb_t::power,"Bitline -- Line -- precharge bct->draincap  == %g\n",1e12*Cprerow);
        VERBOSE_OUT(verb_t::power,"Bitline -- Line -- metal               == %g\n",1e12*bitlinelength*circuit_param_t::cparam_CM3metal);
        VERBOSE_OUT(verb_t::power,"Bitline -- Colmux                      == %g\n",1e12*Ccolmux);
        
        VERBOSE_OUT(verb_t::power,"\n");
    }
    
    
    if(cache==0)
        return(Ctotal*Powerfactor);
    else
        return(Ctotal*SensePowerfactor*.4);
    
}



/* Decoder power:  (see section 6.1 of tech report) */

double power_util_t::array_decoder_power(int rows,int cols,double predeclength,
    int rports,int wports,int cache)
{
    double Ctotal=0;
    double Ceq=0;
    int numstack;
    int decode_bits=0;
    int ports;
    double rowsb;
    
    /* read and write ports are the same here */
    ports = rports + wports;
    
    rowsb = (double)rows;
    
    /* number of input bits to be decoded */
    decode_bits=(int)ceil((bct->logtwo(rowsb)));
    
    /* First stage: driving the decoders */
    
    /* This is the capacitance for driving one bit (and its complement).
    -There are #rowsb 3->8 decoders contributing bct->gatecap.
    - 2.0 factor from 2 identical sets of drivers in parallel
    */
    
     
    Ceq = 2.0*(bct->draincap(circuit_param_t::cparam_Wdecdrivep,PCH,1)+
        bct->draincap(circuit_param_t::cparam_Wdecdriven,NCH,1)) +
        bct->gatecap(circuit_param_t::cparam_Wdec3to8n +
            circuit_param_t::cparam_Wdec3to8p,10.0)*rowsb;
    
    /* There are ports * #decode_bits total */
    Ctotal+=ports*decode_bits*Ceq;
    
    VERBOSE_OUT(verb_t::power,"Decoder -- Driving decoders            == %g\n",.3*Ctotal*Powerfactor);
    
    /* second stage: driving a bunch of nor gates with a nand 
    numstack is the size of the nor gates -- ie. a 7-128 decoder has
    3-input NAND followed by 3-input NOR  */
    
    numstack = (int)ceil((1.0/3.0)*bct->logtwo(rows));
    
    if (numstack<=0) numstack = 1;
    if (numstack>5) numstack = 5;
    
    /* There are #rowsb NOR gates being driven*/
    Ceq = (3.0*bct->draincap(circuit_param_t::cparam_Wdec3to8p,PCH,1) +bct->draincap(circuit_param_t::cparam_Wdec3to8n,NCH,3) +
        bct->gatecap(circuit_param_t::cparam_WdecNORn+circuit_param_t::cparam_WdecNORp,((numstack*40)+20.0)))*rowsb;
    
    Ctotal+=ports*Ceq;
    
    
        VERBOSE_OUT(verb_t::power,"Decoder -- Driving nor w/ nand         == %g\n",.3*ports*Ceq*Powerfactor);
    
    /* Final stage: driving an inverter with the nor 
    (inverter preceding wordline driver) -- wordline driver is in the next section*/
    
    Ceq = (bct->gatecap(circuit_param_t::cparam_Wdecinvn+circuit_param_t::circuit_param_t::cparam_Wdecinvp,20.0)+
        numstack*bct->draincap(circuit_param_t::cparam_WdecNORn,NCH,1)+
        bct->draincap(circuit_param_t::cparam_WdecNORp,PCH,numstack));
    
    
        VERBOSE_OUT(verb_t::power,"Decoder -- Driving inverter w/ nor     == %g\n",.3*ports*Ceq*Powerfactor);
    
    Ctotal+=ports*Ceq;
    
    /* assume Activity Factor == .3  */
    
    return(.3*Ctotal*Powerfactor);
}

double power_util_t::simple_array_decoder_power(int rows,int cols,int rports,
    int wports,int cache)

{
    double predeclength=0.0;
    return(array_decoder_power(rows,cols,predeclength,rports,wports,cache));
}



double power_util_t::compute_af(uint64 num_pop_count_cycle,uint64 total_pop_count_cycle,
    int pop_width) 

{
    double avg_pop_count;
    double af,af_b;
    
    if(num_pop_count_cycle)
        avg_pop_count = (double)total_pop_count_cycle / (double)num_pop_count_cycle;
    else
        avg_pop_count = 0;
    
    af = avg_pop_count / (double)pop_width;
    
    af_b = 1.0 - af;
    
    /*  printf("af == %f%%, af_b == %f%%, total_pop == %d, num_pop == %d\n",100*af,100*af_b,total_pop_count_cycle,num_pop_count_cycle); */
    
    return(af_b);
}


