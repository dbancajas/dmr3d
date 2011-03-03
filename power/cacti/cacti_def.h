/*------------------------------------------------------------
 *                              CACTI 4.0
 *         Copyright 2005 Hewlett-Packard Development Corporation
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein, and
 * hereby grant back to Hewlett-Packard Company and its affiliated companies ("HP")
 * a non-exclusive, unrestricted, royalty-free right and license under any changes, 
 * enhancements or extensions  made to the core functions of the software, including 
 * but not limited to those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to HP any such changes,
 * enhancements or extensions that they make and inform HP of noteworthy uses of
 * this software.  Correspondence should be provided to HP at:
 *
 *                       Director of Intellectual Property Licensing
 *                       Office of Strategy and Technology
 *                       Hewlett-Packard Company
 *                       1501 Page Mill Road
 *                       Palo Alto, California  94304
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND HP DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL HP 
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/

 
// Forward declaration of classes

#ifndef _CACTI_DEF_H_
#define _CACTI_DEF_H_


class leakage_t;
class cacti_time_t;
class area_t;


/*  The following are things you might want to change
 *  when compiling
 */

/*
 * The output can be in 'long' format, which shows everything, or
 * 'short' format, which is just what a program like 'grep' would
 * want to see
 */

#define LONG 1
#define SHORT 2

#define OUTPUTTYPE LONG

/*
 * Address bits in a word, and number of output bits from the cache 
 */

/*
was: #define BITOUT 64
now: making it a commandline parameter
*/


/*dt: In addition to the tag bits, the tags also include 1 valid bit, 1 dirty bit, 2 bits for a 4-state 
  cache coherency protocoll (MESI), 1 bit for MRU (change this to log(ways) for full LRU). 
  So in total we have 1 + 1 + 2 + 1 = 5 */
#define EXTRA_TAG_BITS 5


/* limits on the various N parameters */

#define MAXDATAN 32          /* Maximum for Ndwl,Ndbl */
#define MAXTAGN 32          /* Maximum for Ndwl,Ndbl */
#define MAXSUBARRAYS 256    /* Maximum subarrays for data and tag arrays */
#define MAXDATASPD 32         /* Maximum for Nspd */
#define MAXTAGSPD 32         /* Maximum for Ntspd */



/*===================================================================*/

/*
 * Cache layout parameters and process parameters 
 */


/*
 * CMOS 0.8um model parameters
 *   - directly from Appendix II of tech report
 */

/*#define WORDWIRELENGTH (8+2*WIREPITCH*(EXTRAWRITEPORTS)+2*WIREPITCH*(EXTRAREADPORTS))*/
/*#define BITWIRELENGTH (16+2*WIREPITCH*(EXTRAWRITEPORTS+EXTRAREADPORTS))*/
/*
was: 
#define WIRESPACING (2*FEATURESIZE)
#define WIREWIDTH (3*FEATURESIZE)
is: width and pitch are taken from the Intel IEDM 2004 paper on their 65nm process.
*/
/*
#define Default_Cmetal 275e-18
*/
/*dt: The old Cmetal was calculated using SiO2 as dielectric (k = 3.9). Going by the Intel 65nm paper, 
low-k dielectics are not at k=2.9. This is a very simple adjustment, as lots of other factors also go into the average
capacitance, but I don't have any better/more up to date data on wire distribution, etc. than what's been done for cacti 1.0.
So I'm doing a simple adjustment by 2.9/3.9 */
//#define Default_Cmetal (2.9/3.9*275e-18)
/*dt: changing this to reflect newer data */
/* 2.9: is the k value for the low-k dielectric used in 65nm Intel process 
   3.9: is the k value of normal SiO2
   --> multiply by 2.9/3.9 to get new C values
   the Intel 130nm process paper mentioned 230fF/mm for M1 through M5 with k = 3.6
   So we get
   230*10^-15/mm * 10^-3 mm/1um * 3.9/3.6
*/
#define Default_Cmetal (2.9/3.9*230e-18*3.9/3.6)
#define Default_Rmetal 48e-3
/* dt: I'm assuming that even with all the process improvements, 
copper will 'only' have 2/3 the sheet resistance of aluminum. */
#define Default_CopperSheetResistancePerMicroM 32e-3

/*dt: this number is calculated from the 2004 ITRS tables (RC delay and Wire sizes)*/
#define CRatiolocal_to_interm  (1.0/1.4)
/*dt: from ITRS 2004 using wire sizes, aspect ratios and effective resistivities for local and intermediate*/
#define RRatiolocal_to_interm  (1.0/2.04)

#define CRatiointerm_to_global  (1.0/1.9)
/*dt: from ITRS 2004 using wire sizes, aspect ratios and effective resistivities for local and intermediate*/
#define RRatiointerm_to_global  (1.0/3.05)


/* fF/um2 at 1.5V */
//#define Cndiffarea    0.137e-15


//#define Vdd		5

/* Threshold voltages (as a proportion of Vdd)
   If you don't know them, set all values to 0.5 */

#define SizingRatio   0.33
#define VTHNAND       0.561
#define VTHFA1        0.452
#define VTHFA2        0.304
#define VTHFA3        0.420
#define VTHFA4        0.413
#define VTHFA5        0.405
#define VTHFA6        0.452
#define VSINV         0.452   
#define VTHINV100x60  0.438   /* inverter with p=100,n=60 */
#define VTHINV360x240 0.420   /* inverter with p=360, n=240 */
#define VTHNAND60x90  0.561   /* nand with p=60 and three n=90 */
#define VTHNOR12x4x1  0.503   /* nor with p=12, n=4, 1 input */
#define VTHNOR12x4x2  0.452   /* nor with p=12, n=4, 2 inputs */
#define VTHNOR12x4x3  0.417   /* nor with p=12, n=4, 3 inputs */
#define VTHNOR12x4x4  0.390   /* nor with p=12, n=4, 4 inputs */
#define VTHOUTDRINV    0.437
#define VTHOUTDRNOR   0.379
#define VTHOUTDRNAND  0.63
#define VTHOUTDRIVE   0.425
#define VTHCOMPINV    0.437
#define VTHMUXNAND    0.548
#define VTHMUXDRV1    0.406
#define VTHMUXDRV2    0.334
#define VTHMUXDRV3    0.478
#define VTHEVALINV    0.452
#define VTHSENSEEXTDRV  0.438

#define VTHNAND60x120 0.522


#define Vbitpre		(3.3)

#define Vt		(1.09)
/*
was: #define Vbitsense	(0.10)
now: 50mV seems to be the norm as of 2005
*/
#define Vbitsense	(0.05*Vdd)
#define Vbitswing	(0.20*Vdd)

/*===================================================================*/

/*
 * The following are things you probably wouldn't want to change.  
 */

#ifndef NULL
#define NULL 0
#endif
#define OK 1
#define ERROR 0
#define BIGNUM 1e30
#define DIVIDE(a,b) ((b)==0)? 0:(a)/(b)
#define CACTI_MAX(a,b) (((a)>(b))?(a):(b))

#define WAVE_PIPE 3
#define MAX_COL_MUX 16


/* Used to pass values around the program */

/*dt: maximum numbers of entries in the 
      caching structures of the tag calculations
*/
#define MAX_CACHE_ENTRIES 512


#define EPSILON 0.5 //v4.1: This constant is being used in order to fix floating point -> integer
//conversion problems that were occuring within CACTI. Typical problem that was occuring was
//that with different compilers a floating point number like 3.0 would get represented as either 
//2.9999....or 3.00000001 and then the integer part of the floating point number (3.0) would 
//be computed differently depending on the compiler. What we are doing now is to replace 
//int (x) with (int) (x+EPSILON) where EPSILON is 0.5. This would fix such problems. Note that
//this works only when x is an integer >= 0. 

#endif

