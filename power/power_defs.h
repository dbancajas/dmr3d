

/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Header file for power estimation in ms2sim
 * initial author: Koushik Chakraborty
 *
 */ 


/*  The following are things you might want to change
 *  when compiling
 */

 
#ifndef _POWER_DEF_H_
#define _POWER_DEF_H_

// forward declarations
class power_object_t;
class power_profile_t;
class sparc_regfile_t;
class basic_circuit_t;
class power_util_t;


class core_power_t;
class cache_power_t;

// Cacti Interface

class result_type;
class arearesult_type;
class parameter_type;
class area_type;
class cacti_time_t;
class leakage_t;
class hotspot_t;
typedef struct flp_t_st flp_t;
typedef struct  RC_model_t_st RC_model_t;
#ifdef POWER_COMPILE


#define MAX_POWER_ENTITIES 128

/*
 * The output can be in 'long' format, which shows everything, or
 * 'short' format, which is just what a program like 'grap' would
 * want to see
 */
#define LONG 1
#define SHORT 2

#define OUTPUTTYPE LONG

/* Do we want static AFs (STATIC_AF) or Dynamic AFs (DYNAMIC_AF) */
/* #define STATIC_AF */
#define DYNAMIC_AF

/*
 * Address bits in a word, and number of output bits from the cache 
 */
/*
was: #define ADDRESS_BITS 32
now: I'm using 42 bits as in the Power4, 
since that's bigger then the 36 bits on the Pentium 4 
and 40 bits on the Opteron
*/
#define ADDRESS_BITS 42


//#define BITOUT 64


/*===================================================================*/

/*
 * The following are things you probably wouldn't want to change.  
 */


// #define TRUE 1
// #define FALSE 0
#define OK 1
#define ERROR 0
#define BIGNUM 1e30
#define DIVIDE(a,b) ((b)==0)? 0:(a)/(b)
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

/* Used to communicate with the horowitz model */

#define RISE 1
#define FALL 0
#define NCH  1
#define PCH  0

/*
 * The following scale factor can be used to scale between technologies.
 * To convert from 0.8um to 0.5um, make FUDGEFACTOR = 1.6
 */
 

/*===================================================================*/

/*
 * Cache layout parameters and process parameters 
 * Thanks to Glenn Reinman for the technology scaling factors
 */

#define GEN_POWER_FACTOR 1.31

#define TECH_POINT045

#if defined (TECH_POINT045)
#define CSCALE      (170.000)  /* arbitrary */
#define RSCALE      (190.000) /* arbitrary */
#define LSCALE      (0.05625)
#define ASCALE      (LSCALE * LSCALE)
#define VSCALE      (0.2)
#define VTSCALE     (0.35)    /* Arbitrary */
#define SSCALE      0.6       /* Arbitrary */
#define GEN_POWER_SCALE (1/GEN_POWER_FACTOR)
#elif defined (TECH_POINT065)
#define CSCALE      (125.000) /* Arbitrary */
#define RSCAE       (150.000) /* Arbitrary */
#define LSCALE      (0.08125) 
#define ASCALE      (LSCALE*LSCALE)
#define VSCALE      0.26
#define VTSCLE      0.4       /* Arbitrary */
#define SSCALE      0.7       /* Arbitrary */
#define GEN_POWER_SCALE (1/GEN_POWER_FACTOR)
#elif defined(TECH_POINT10)
#define CSCALE		(84.2172)	/* wire capacitance scaling factor */
			/* linear: 51.7172, predicted: 84.2172 */
#define RSCALE		(80.0000)	/* wire resistance scaling factor */
#define LSCALE		0.1250		/* length (feature) scaling factor */
#define ASCALE		(LSCALE*LSCALE)	/* area scaling factor */
#define VSCALE		0.38		/* voltage scaling factor */
#define VTSCALE		0.49		/* threshold voltage scaling factor */
#define SSCALE		0.80		/* sense voltage scaling factor */
#define GEN_POWER_SCALE (1/GEN_POWER_FACTOR)
#elif defined(TECH_POINT18)
#define CSCALE		(19.7172)	/* wire capacitance scaling factor */
#define RSCALE		(20.0000)	/* wire resistance scaling factor */
#define LSCALE		0.2250		/* length (feature) scaling factor */
#define ASCALE		(LSCALE*LSCALE)	/* area scaling factor */
#define VSCALE		0.4		/* voltage scaling factor */
// #define VTSCALE		0.5046		/* threshold voltage scaling factor */
// #define SSCALE		0.85		/* sense voltage scaling factor */
#define GEN_POWER_SCALE 1
#elif defined(TECH_POINT25)
#define CSCALE		(10.2197)	/* wire capacitance scaling factor */
#define RSCALE		(10.2571)	/* wire resistance scaling factor */
#define LSCALE		0.3571		/* length (feature) scaling factor */
#define ASCALE		(LSCALE*LSCALE)	/* area scaling factor */
#define VSCALE		0.45		/* voltage scaling factor */
#define VTSCALE		0.5596		/* threshold voltage scaling factor */
#define SSCALE		0.90		/* sense voltage scaling factor */
#define GEN_POWER_SCALE GEN_POWER_FACTOR
#elif defined(TECH_POINT35a)
#define CSCALE		(5.2197)	/* wire capacitance scaling factor */
#define RSCALE		(5.2571)	/* wire resistance scaling factor */
#define LSCALE		0.4375		/* length (feature) scaling factor */
#define ASCALE		(LSCALE*LSCALE)	/* area scaling factor */
#define VSCALE		0.5		/* voltage scaling factor */
#define VTSCALE		0.6147		/* threshold voltage scaling factor */
#define SSCALE		0.95		/* sense voltage scaling factor */
#define GEN_POWER_SCALE (GEN_POWER_FACTOR*GEN_POWER_FACTOR)
#elif defined(TECH_POINT35)
#define CSCALE		(5.2197)	/* wire capacitance scaling factor */
#define RSCALE		(5.2571)	/* wire resistance scaling factor */
#define LSCALE		0.4375		/* length (feature) scaling factor */
#define ASCALE		(LSCALE*LSCALE)	/* area scaling factor */
#define VSCALE		0.5		/* voltage scaling factor */
#define VTSCALE		0.6147		/* threshold voltage scaling factor */
#define SSCALE		0.95		/* sense voltage scaling factor */
#define GEN_POWER_SCALE (GEN_POWER_FACTOR*GEN_POWER_FACTOR)
#elif defined(TECH_POINT40)
#define CSCALE		1.0		/* wire capacitance scaling factor */
#define RSCALE		1.0		/* wire resistance scaling factor */
#define LSCALE		0.5		/* length (feature) scaling factor */
#define ASCALE		(LSCALE*LSCALE)	/* area scaling factor */
#define VSCALE		1.0		/* voltage scaling factor */
#define VTSCALE		1.0		/* threshold voltage scaling factor */
#define SSCALE		1.0		/* sense voltage scaling factor */
#define GEN_POWER_SCALE (GEN_POWER_FACTOR*GEN_POWER_FACTOR*GEN_POWER_FACTOR)
#else /* TECH_POINT80 */
/* scaling factors */
#define CSCALE		1.0		/* wire capacitance scaling factor */
#define RSCALE		1.0		/* wire resistance scaling factor */
#define LSCALE		1.0		/* length (feature) scaling factor */
#define ASCALE		(LSCALE*LSCALE)	/* area scaling factor */
#define VSCALE		1.0		/* voltage scaling factor */
#define VTSCALE		1.0		/* threshold voltage scaling factor */
#define SSCALE		1.0		/* sense voltage scaling factor */
#define GEN_POWER_SCALE (GEN_POWER_FACTOR*GEN_POWER_FACTOR*GEN_POWER_FACTOR*GEN_POWER_FACTOR)
#endif

/*
 * CMOS 0.8um model parameters
 *   - from Appendix II of Cacti tech report
 */


// /* corresponds to 8um of m3 @ 225ff/um */
// #define Cwordmetal    (1.8e-15 * (CSCALE * ASCALE))

// /* corresponds to 16um of m2 @ 275ff/um */
// #define Cbitmetal     (4.4e-15 * (CSCALE * ASCALE))

// /* corresponds to 1um of m2 @ 275ff/um */
// #define Cmetal        Cbitmetal/16 



#define Mhz             3*1e9
#define Period          (1/Mhz)

// #define krise		(0.4e-9 * LSCALE)
// #define tsensedata	(5.8e-10 * LSCALE)
// #define tsensetag	(2.6e-10 * LSCALE)
// #define tfalldata	(7e-10 * LSCALE)
// #define tfalltag	(7e-10 * LSCALE)
// #define Vbitpre		(3.3 * SSCALE)
// #define Vt		(1.09 * VTSCALE)
// #define Vbitsense	(0.10 * SSCALE)


#define Vdd 5.0 * VSCALE 
#define Powerfactor_ORIG  Mhz * Vdd * Vdd
#define SensePowerfactor3 (Mhz)*(Vbitsense)*(Vbitsense)
#define SensePowerfactor2 (Mhz)*(Vbitpre-Vbitsense)*(Vbitpre-Vbitsense)
#define SensePowerfactor (Mhz)*(Vdd/2)*(Vdd/2)

#define STATIC_ACTIVITY_FACTORY    .5
#define POPCOUNT_AF  (23.9/64.0)

/* Threshold voltages (as a proportion of Vdd)
   If you don't know them, set all values to 0.5 */

#define Wmemcellr	(4.0 * LSCALE)
#define Wmemcellw	(2.1 * LSCALE)
// #define Wmemcellbscale	2		/* means 2x bigger than Wmemcella */
// #define Wbitpreequ	(10.0 * LSCALE)

#define Wbitmuxn	(10.0 * LSCALE)
#define WsenseQ1to4	(4.0 * LSCALE)


#define Wcompcellpd2    (2.4 * LSCALE)
#define Wcompdrivern    (400.0 * LSCALE)
#define Wcompdriverp    (800.0 * LSCALE)
#define Wcomparen2      (40.0 * LSCALE)
#define Wcomparen1      (20.0 * LSCALE)
#define Wmatchpchg      (10.0 * LSCALE)
#define Wmatchinvn      (10.0 * LSCALE)
#define Wmatchinvp      (20.0 * LSCALE)
#define Wmatchnandn     (20.0 * LSCALE)
#define Wmatchnandp     (10.0 * LSCALE)
#define Wmatchnorn     (20.0 * LSCALE)
#define Wmatchnorp     (10.0 * LSCALE)

#define WSelORn         (10.0 * LSCALE)
#define WSelORprequ     (40.0 * LSCALE)
#define WSelPn          (10.0 * LSCALE)
#define WSelPp          (15.0 * LSCALE)
#define WSelEnn         (5.0 * LSCALE)
#define WSelEnp         (10.0 * LSCALE)


#define RatCellHeight    (40.0 * LSCALE)
#define RatCellWidth     (70.0 * LSCALE)
#define RatShiftRegWidth (120.0 * LSCALE)
#define RatNumShift      4
#define BitlineSpacing   (6.0 * LSCALE)
#define WordlineSpacing  (6.0 * LSCALE)

#define RegCellHeight    (16.0 * LSCALE)
#define RegCellWidth     (8.0  * LSCALE)

#define CamCellHeight    (40.0 * LSCALE)
#define CamCellWidth     (25.0 * LSCALE)
#define MatchlineSpacing (6.0 * LSCALE)
#define TaglineSpacing   (6.0 * LSCALE)

/*===================================================================*/

/* ALU POWER NUMBERS for .18um 733Mhz */
/* normalize to cap from W */
#define NORMALIZE_SCALE (1.0/(733.0e6*1.45*1.45))
/* normalize .18um cap to other gen's cap, then xPowerfactor */
//#define POWER_SCALE    (GEN_POWER_SCALE * NORMALIZE_SCALE * Powerfactor)
//#define I_ADD          ((.37 - .091) * POWER_SCALE)
#define I_ADD          ((.37 - .091) * GEN_POWER_SCALE * NORMALIZE_SCALE)
//#define I_ADD32        (((.37 - .091)/2)*POWER_SCALE)
#define I_ADD32        (((.37 - .091)/2)*GEN_POWER_SCALE * NORMALIZE_SCALE)
//#define I_MULT16       ((.31-.095)*POWER_SCALE)
//#define I_SHIFT        ((.21-.089)*POWER_SCALE)
//#define I_LOGIC        ((.04-.015)*POWER_SCALE)
//#define F_ADD          ((1.307-.452)*POWER_SCALE)
#define F_ADD          ((1.307-.452)*GEN_POWER_SCALE * NORMALIZE_SCALE)
//#define F_MULT         ((1.307-.452)*POWER_SCALE)

#define I_ADD_CLOCK    (.091*GEN_POWER_SCALE * NORMALIZE_SCALE)
#define I_MULT_CLOCK   (.095*GEN_POWER_SCALE * NORMALIZE_SCALE)
//#define I_SHIFT_CLOCK  (.089*POWER_SCALE)
//#define I_LOGIC_CLOCK  (.015*POWER_SCALE)
#define F_ADD_CLOCK    (.452*GEN_POWER_SCALE * NORMALIZE_SCALE)
//#define F_MULT_CLOCK   (.452*POWER_SCALE)


#define MSCALE (LSCALE * .624 / .2250)




class core_power_result_type {
    
    public:
        double btb;
        double local_predict;
        double global_predict;
        double chooser;
        double ras;
        double rat_driver;
        double rat_decoder;
        double rat_wordline;
        double rat_bitline;
        double rat_senseamp;
        double dcl_compare;
        double dcl_pencode;
        double inst_decoder_power;
        double wakeup_tagdrive;
        double wakeup_tagmatch;
        double wakeup_ormatch;
        double lsq_wakeup_tagdrive;
        double lsq_wakeup_tagmatch;
        double lsq_wakeup_ormatch;
        double selection;
        double reorder_driver;
        double reorder_decoder;
        double reorder_wordline;
        double reorder_bitline;
        double reorder_senseamp;
        double rs_driver;
        double rs_decoder;
        double rs_wordline;
        double rs_bitline;
        double rs_senseamp;
        double lsq_rs_driver;
        double lsq_rs_decoder;
        double lsq_rs_wordline;
        double lsq_rs_bitline;
        double lsq_rs_senseamp;
        double resultbus;
        
        
        double total_power;
        double ialu_power;
        double falu_power;
        double bpred_power;
        double rename_power;
        double rat_power;
        double dcl_power;
        double window_power;
        double lsq_power;
        double wakeup_power;
        double lsq_wakeup_power;
        double rs_power;
        double rs_power_nobit;
        double lsq_rs_power;
        double lsq_rs_power_nobit;
        double selection_power;
        double result_power;
        
        double itlb_power;
        double dtlb_power;
        
        double clock_power;

};

class cache_power_result_type {
    public:
    
        double decoder;
        double wordline;
        double bitline;
        double senseamp;
        double tagarray;
        
        double cache_power;
        double tlb;
};


/* Used to pass values around the program */

class time_parameter_type {
    public:
        int cache_size;
        int number_of_sets;
        int associativity;
        int block_size;
}; 

class time_result_type  {
    public:
    
        double access_time,cycle_time;
        int best_Ndwl,best_Ndbl;
        int best_Nspd;
        int best_Ntwl,best_Ntbl;
        int best_Ntspd;
        double decoder_delay_data,decoder_delay_tag;
        double dec_data_driver,dec_data_3to8,dec_data_inv;
        double dec_tag_driver,dec_tag_3to8,dec_tag_inv;
        double wordline_delay_data,wordline_delay_tag;
        double bitline_delay_data,bitline_delay_tag;
        double sense_amp_delay_data,sense_amp_delay_tag;
        double senseext_driver_delay_data;
        double compare_part_delay;
        double drive_mux_delay;
        double selb_delay;
        double data_output_delay;
        double drive_valid_delay;
        double precharge_delay;
   
} ;

#endif

#endif
