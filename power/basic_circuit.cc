
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 *

 * description:    Implementation file basic circuits in Cacti
 * initial author: Koushik Chakraborty
 *
 */ 



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


 
//  #include "simics/first.h"
// RCSID("$Id: basic_circuit.cc,v 1.10.2.17 2006/03/02 23:58:42 kchak Exp $");

#include "definitions.h"
#include "config_extern.h" 
#include "profiles.h"
#include "circuit_param.h"
#include "basic_circuit.h"

basic_circuit_t::basic_circuit_t()
{
    
}

int basic_circuit_t::powers (int base, int n)
{
    int i, p;
    
    p = 1;
    for (i = 1; i <= n; ++i)
        p *= base;
    return p;
}


double basic_circuit_t::logtwo (double x)
{
    if (x <= 0)
        printf ("RIGHT HERE %e\n", x);
    return ((double) (log (x) / log (2.0)));
}

// returns gate capacitance in Farads 
// width: gate width in um (length is circuit_param_t::cparam_Leff) 
// wirelength: poly wire length going to gate in lambda 

double basic_circuit_t::gatecap (double width,double  wirelength)	
{
    return (width * circuit_param_t::cparam_Leff * circuit_param_t::cparam_Cgate +
        wirelength * circuit_param_t::cparam_Cpolywire * circuit_param_t::cparam_Leff);
}

double basic_circuit_t::gatecappass (double width,double  wirelength)	
{
    return (width * circuit_param_t::cparam_Leff * circuit_param_t::cparam_Cgatepass +
        wirelength * circuit_param_t::cparam_Cpolywire * circuit_param_t::cparam_Leff);
}


/*----------------------------------------------------------------------*/

/* Routine for calculating drain capacitances.  The draincap routine
 * folds transistors larger than 10um */

 
 /* returns drain cap in Farads */
 /* width: in um */
 /* nchannel: whether n or p-channel (boolean) */
 /* stack: number of transistors in series that are on */

double basic_circuit_t::draincap (double width,int nchannel,int stack)	
{
    double Cdiffside, Cdiffarea, Coverlap, cap;
    
    Cdiffside = (nchannel) ? circuit_param_t::cparam_Cndiffside : circuit_param_t::cparam_Cpdiffside;
    Cdiffarea = (nchannel) ? circuit_param_t::cparam_Cndiffarea : circuit_param_t::cparam_Cpdiffarea;
    Coverlap = (nchannel) ? (circuit_param_t::cparam_Cndiffovlp + circuit_param_t::cparam_Cnoxideovlp) :
    (circuit_param_t::cparam_Cpdiffovlp + circuit_param_t::cparam_Cpoxideovlp);
    /* calculate directly-connected (non-stacked) capacitance */
    /* then add in capacitance due to stacking */
    if(stack > 1) {
        if (width >= 10/circuit_param_t::cparam_FUDGEFACTOR) {
            cap = 3.0 * circuit_param_t::cparam_Leff * width / 2.0 * Cdiffarea + 6.0 * circuit_param_t::cparam_Leff * Cdiffside +
            width * Coverlap;
            cap += (double) (stack - 1) * (circuit_param_t::cparam_Leff * width * Cdiffarea +
                4.0 * circuit_param_t::cparam_Leff * Cdiffside +
                2.0 * width * Coverlap);
        }
        else {
            cap =
            3.0 * circuit_param_t::cparam_Leff * width * Cdiffarea + (6.0 * circuit_param_t::cparam_Leff + width) * Cdiffside +
            width * Coverlap;
            cap +=
            (double) (stack - 1) * (circuit_param_t::cparam_Leff * width * Cdiffarea +
                2.0 * circuit_param_t::cparam_Leff * Cdiffside +
                2.0 * width * Coverlap);
        }
    }
    else {
        if (width >= 10/circuit_param_t::cparam_FUDGEFACTOR) {
            cap = 3.0 * circuit_param_t::cparam_Leff * width / 2.0 * Cdiffarea + 
            6.0 * circuit_param_t::cparam_Leff * Cdiffside +  width * Coverlap;
        }
        else {
            cap = 3.0 * circuit_param_t::cparam_Leff * width * Cdiffarea + 
            (6.0 * circuit_param_t::cparam_Leff + width) * Cdiffside + width * Coverlap;
        }
    }
    return (cap);
}

/*----------------------------------------------------------------------*/

/* The following routines estimate the effective resistance of an
   on transistor as described in the tech report.  The first routine
   gives the "switching" resistance, and the second gives the 
   "full-on" resistance 
   
*/   

/* returns resistance in ohms */
/* width: in um */
/* nchannel: whether n or p-channel (boolean) */
/* stack: number of transistors in series */
double basic_circuit_t::transresswitch (double width,int nchannel,int stack)	
{
  double restrans;
  restrans = (nchannel) ? (circuit_param_t::cparam_Rnchannelstatic) : (circuit_param_t::cparam_Rpchannelstatic);
  /* calculate resistance of stack - assume all but switching trans
     have 0.8X the resistance since they are on throughout switching */
  return ((1.0 + ((stack - 1.0) * 0.8)) * restrans / width);
}

/*----------------------------------------------------------------------*/


/* returns resistance in ohms */
/* width: in um */
/* nchannel: whether n or p-channel (boolean) */
/* stack: number of transistors in series */
double basic_circuit_t::transreson (double width,int nchannel,int stack)	
{
    double restrans;
    restrans = (nchannel) ? circuit_param_t::cparam_Rnchannelon : circuit_param_t::cparam_Rpchannelon;
    
    /* calculate resistance of stack.  Unlike transres, we don't
    multiply the stacked transistors by 0.8 */
    return (stack * restrans / width);

}


/* This routine operates in reverse: given a resistance, it finds
 * the transistor width that would have this R.  It is used in the
 * data wordline to estimate the wordline driver size. */
 
 /* returns width in um */
 /* res: resistance in ohms */
 /* nchannel: whether N-channel or P-channel */ 
 
double basic_circuit_t::restowidth (double res,int nchannel)	
{
    double restrans;
    
    restrans = (nchannel) ? circuit_param_t::cparam_Rnchannelon : circuit_param_t::cparam_Rpchannelon;
    
    return (restrans / res);
    
}

/*----------------------------------------------------------------------*/

/* inputramptime: input rise time */
/* tf: time constant of gate */
/* vs1, vs2: threshold voltages */
/* rise: whether INPUT rise or fall (boolean) */

double basic_circuit_t::horowitz (double inputramptime,double  tf,double  vs1,double  vs2,int rise)

{
    double a, b, td;
    
    a = inputramptime / tf;
    if (rise == RISE)
    {
        b = 0.5;
        td = tf * sqrt (log (vs1) * log (vs1) + 2 * a * b * (1.0 - vs1)) +
        tf * (log (vs1) - log (vs2));
    }
    else
    {
        b = 0.4;
        td = tf * sqrt (log (1.0 - vs1) * log (1.0 - vs1) + 2 * a * b * (vs1)) +
        tf * (log (1.0 - vs1) - log (1.0 - vs2));
    }
    return (td);
}
