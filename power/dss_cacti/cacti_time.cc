
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
// RCSID("$Id: cacti_time.cc,v 1.10.2.17 2006/03/02 23:58:42 kchak Exp $");

#include "definitions.h"
#include "config_extern.h" 
#include "basic_circuit.h"
#include "cacti_time.h"

cacti_time_t::cacti_time_t ()
{
    bct = new basic_circuit_t();
}


/*======================================================================*/



/* 
 * This part of the code contains routines for each section as
 * described in the tech report.  See the tech report for more details
 * and explanations */

/*----------------------------------------------------------------------*/

/* Decoder delay:  (see section 6.1 of tech report) */


double cacti_time_t::decoder_delay(int C, int B, int A, int Ndwl, int Ndbl, int Nspd, 
    int Ntwl, int Ntbl, int Ntspd, double *Tdecdrive,double *Tdecoder1,
    double *Tdecoder2,double *outrisetime)
{
        double Ceq,Req,Rwire,rows,tf,nextinputtime,vth,tstep,m,a,b,c;
        int numstack;

        /* Calculate rise time.  Consider two inverters */

        Ceq = bct->draincap(Wdecdrivep,PCH,1)+bct->draincap(Wdecdriven,NCH,1) +
              bct->gatecap(Wdecdrivep+Wdecdriven,0.0);
        tf = Ceq*bct->transreson(Wdecdriven,NCH,1);
        nextinputtime = bct->horowitz(0.0,tf,VTHINV100x60,VTHINV100x60,FALL)/
                                  (VTHINV100x60);

        Ceq = bct->draincap(Wdecdrivep,PCH,1)+bct->draincap(Wdecdriven,NCH,1) +
              bct->gatecap(Wdecdrivep+Wdecdriven,0.0);
        tf = Ceq*bct->transreson(Wdecdriven,NCH,1);
        nextinputtime = bct->horowitz(nextinputtime,tf,VTHINV100x60,VTHINV100x60,
                               RISE)/
                                  (1.0-VTHINV100x60);

        /* First stage: driving the decoders */

        rows = C/(8*B*A*Ndbl*Nspd);
        Ceq = bct->draincap(Wdecdrivep,PCH,1)+bct->draincap(Wdecdriven,NCH,1) +
            4*bct->gatecap(Wdec3to8n+Wdec3to8p,10.0)*(Ndwl*Ndbl)+
            Cwordmetal*0.25*8*B*A*Ndbl*Nspd;
        Rwire = Rwordmetal*0.125*8*B*A*Ndbl*Nspd;
        tf = (Rwire + bct->transreson(Wdecdrivep,PCH,1))*Ceq;
        *Tdecdrive = bct->horowitz(nextinputtime,tf,VTHINV100x60,VTHNAND60x90,
                     FALL);
        nextinputtime = *Tdecdrive/VTHNAND60x90;

        /* second stage: driving a bunch of nor gates with a nand */

        numstack =
          ceil((1.0/3.0)*bct->logtwo( (double)((double)C/(double)(B*A*Ndbl*Nspd))));
        if (numstack==0) numstack = 1;
        if (numstack>5) numstack = 5;
        Ceq = 3*bct->draincap(Wdec3to8p,PCH,1) +bct->draincap(Wdec3to8n,NCH,3) +
              bct->gatecap(WdecNORn+WdecNORp,((numstack*40)+20.0))*rows +
              Cbitmetal*rows*8;

        Rwire = Rbitmetal*rows*8/2;
        tf = Ceq*(Rwire+bct->transreson(Wdec3to8n,NCH,3)); 

        /* we only want to charge the output to the threshold of the
           nor gate.  But the threshold depends on the number of inputs
           to the nor.  */

        switch(numstack) {
          case 1: vth = VTHNOR12x4x1; break;
          case 2: vth = VTHNOR12x4x2; break;
          case 3: vth = VTHNOR12x4x3; break;
          case 4: vth = VTHNOR12x4x4; break;
          case 5: vth = VTHNOR12x4x4; break;
          default: printf("error:numstack=%d\n",numstack);
	}
        *Tdecoder1 = bct->horowitz(nextinputtime,tf,VTHNAND60x90,vth,RISE);
        nextinputtime = *Tdecoder1/(1.0-vth);

        /* Final stage: driving an inverter with the nor */

        Req = bct->transreson(WdecNORp,PCH,numstack);
        Ceq = (bct->gatecap(Wdecinvn+Wdecinvp,20.0)+
              numstack*bct->draincap(WdecNORn,NCH,1)+
                     bct->draincap(WdecNORp,PCH,numstack));
        tf = Req*Ceq;
        *Tdecoder2 = bct->horowitz(nextinputtime,tf,vth,VSINV,FALL);
        *outrisetime = *Tdecoder2/(VSINV);
        return(*Tdecdrive+*Tdecoder1+*Tdecoder2);
}


/*----------------------------------------------------------------------*/

/* Decoder delay in the tag array (see section 6.1 of tech report) */


double cacti_time_t::decoder_tag_delay(int C, int B, int A, int Ndwl, int Ndbl,
    int Nspd, int Ntwl, int Ntbl, int Ntspd, double *Tdecdrive, 
    double *Tdecoder1, double *Tdecoder2,double *outrisetime)
{
        double Ceq,Req,Rwire,rows,tf,nextinputtime,vth,tstep,m,a,b,c;
        int numstack;


        /* Calculate rise time.  Consider two inverters */

        Ceq = bct->draincap(Wdecdrivep,PCH,1)+bct->draincap(Wdecdriven,NCH,1) +
              bct->gatecap(Wdecdrivep+Wdecdriven,0.0);
        tf = Ceq*bct->transreson(Wdecdriven,NCH,1);
        nextinputtime = bct->horowitz(0.0,tf,VTHINV100x60,VTHINV100x60,FALL)/
                                  (VTHINV100x60);

        Ceq = bct->draincap(Wdecdrivep,PCH,1)+bct->draincap(Wdecdriven,NCH,1) +
              bct->gatecap(Wdecdrivep+Wdecdriven,0.0);
        tf = Ceq*bct->transreson(Wdecdriven,NCH,1);
        nextinputtime = bct->horowitz(nextinputtime,tf,VTHINV100x60,VTHINV100x60,
                               RISE)/
                                  (1.0-VTHINV100x60);

        /* First stage: driving the decoders */

        rows = C/(8*B*A*Ntbl*Ntspd);
        Ceq = bct->draincap(Wdecdrivep,PCH,1)+bct->draincap(Wdecdriven,NCH,1) +
            4*bct->gatecap(Wdec3to8n+Wdec3to8p,10.0)*(Ntwl*Ntbl)+
            Cwordmetal*0.25*8*B*A*Ntbl*Ntspd;
        Rwire = Rwordmetal*0.125*8*B*A*Ntbl*Ntspd;
        tf = (Rwire + bct->transreson(Wdecdrivep,PCH,1))*Ceq;
        *Tdecdrive = bct->horowitz(nextinputtime,tf,VTHINV100x60,VTHNAND60x90,
                     FALL);
        nextinputtime = *Tdecdrive/VTHNAND60x90;

        /* second stage: driving a bunch of nor gates with a nand */

        numstack =
          ceil((1.0/3.0)*bct->logtwo( (double)((double)C/(double)(B*A*Ntbl*Ntspd))));
        if (numstack==0) numstack = 1;
        if (numstack>5) numstack = 5;

        Ceq = 3*bct->draincap(Wdec3to8p,PCH,1) +bct->draincap(Wdec3to8n,NCH,3) +
              bct->gatecap(WdecNORn+WdecNORp,((numstack*40)+20.0))*rows +
              Cbitmetal*rows*8;

        Rwire = Rbitmetal*rows*8/2;
        tf = Ceq*(Rwire+bct->transreson(Wdec3to8n,NCH,3)); 

        /* we only want to charge the output to the threshold of the
           nor gate.  But the threshold depends on the number of inputs
           to the nor.  */

        switch(numstack) {
          case 1: vth = VTHNOR12x4x1; break;
          case 2: vth = VTHNOR12x4x2; break;
          case 3: vth = VTHNOR12x4x3; break;
          case 4: vth = VTHNOR12x4x4; break;
          case 5: vth = VTHNOR12x4x4; break;
          case 6: vth = VTHNOR12x4x4; break;
          default: printf("error:numstack=%d\n",numstack);
	}
    
    *Tdecoder1 = bct->horowitz(nextinputtime,tf,VTHNAND60x90,vth,RISE);
    nextinputtime = *Tdecoder1/(1.0-vth);
    
    /* Final stage: driving an inverter with the nor */
    
    Req = bct->transreson(WdecNORp,PCH,numstack);
    Ceq = (bct->gatecap(Wdecinvn+Wdecinvp,20.0)+
        numstack*bct->draincap(WdecNORn,NCH,1)+
        bct->draincap(WdecNORp,PCH,numstack));
    tf = Req*Ceq;
    *Tdecoder2 = bct->horowitz(nextinputtime,tf,vth,VSINV,FALL);
    *outrisetime = *Tdecoder2/(VSINV);
    return(*Tdecdrive+*Tdecoder1+*Tdecoder2);
}


/*----------------------------------------------------------------------*/

/* Data array wordline delay (see section 6.2 of tech report) */


double cacti_time_t::wordline_delay(int B,int A,int Ndwl,int Nspd,
    double inrisetime, double *outrisetime)
{
        double Rpdrive,nextrisetime;
        double desiredrisetime,psize,nsize;
        double tf,nextinputtime,Ceq,Req,Rline,Cline;
        int cols;
        double Tworddrivedel,Twordchargedel;

        cols = 8*B*A*Nspd/Ndwl;

        /* Choose a transistor size that makes sense */
        /* Use a first-order approx */

        desiredrisetime = krise*log((double)(cols))/2.0;
        Cline = (bct->gatecappass(Wmemcella,0.0)+
                 bct->gatecappass(Wmemcella,0.0)+
                 Cwordmetal)*cols;
        Rpdrive = desiredrisetime/(Cline*log(VSINV)*-1.0);
        psize = bct->restowidth(Rpdrive,PCH);
        if (psize > Wworddrivemax) {
           psize = Wworddrivemax;
	}

        /* Now that we have a reasonable psize, do the rest as before */
        /* If we keep the ratio the same as the tag wordline driver,
           the threshold voltage will be close to VSINV */

        nsize = psize * Wdecinvn/Wdecinvp;

        Ceq = bct->draincap(Wdecinvn,NCH,1) + bct->draincap(Wdecinvp,PCH,1) +
              bct->gatecap(nsize+psize,20.0);
        tf = bct->transreson(Wdecinvn,NCH,1)*Ceq;

        Tworddrivedel = bct->horowitz(inrisetime,tf,VSINV,VSINV,RISE);
        nextinputtime = Tworddrivedel/(1.0-VSINV);

        Cline = (bct->gatecappass(Wmemcella,(BitWidth-2*Wmemcella)/2.0)+
                 bct->gatecappass(Wmemcella,(BitWidth-2*Wmemcella)/2.0)+
                 Cwordmetal)*cols+
                bct->draincap(nsize,NCH,1) + bct->draincap(psize,PCH,1);
        Rline = Rwordmetal*cols/2;
        tf = (bct->transreson(psize,PCH,1)+Rline)*Cline;
        Twordchargedel = bct->horowitz(nextinputtime,tf,VSINV,VSINV,FALL);
        *outrisetime = Twordchargedel/VSINV;

        return(Tworddrivedel+Twordchargedel);
}

/*----------------------------------------------------------------------*/

/* Tag array wordline delay (see section 6.3 of tech report) */


double cacti_time_t::wordline_tag_delay(int C,int A,int Ntspd,int Ntwl,
    double inrisetime,double *outrisetime)
{
        double tf,m,a,b,c;
        double Cline,Rline,Ceq,nextinputtime;
        int tagbits;
        double Tworddrivedel,Twordchargedel;

        /* number of tag bits */

        tagbits = ADDRESS_BITS+2-(int)bct->logtwo((double)C)+(int)bct->logtwo((double)A);

        /* first stage */

        Ceq = bct->draincap(Wdecinvn,NCH,1) + bct->draincap(Wdecinvp,PCH,1) +
              bct->gatecap(Wdecinvn+Wdecinvp,20.0);
        tf = bct->transreson(Wdecinvn,NCH,1)*Ceq;

        Tworddrivedel = bct->horowitz(inrisetime,tf,VSINV,VSINV,RISE);
        nextinputtime = Tworddrivedel/(1.0-VSINV);

        /* second stage */
        Cline = (bct->gatecappass(Wmemcella,(BitWidth-2*Wmemcella)/2.0)+
                 bct->gatecappass(Wmemcella,(BitWidth-2*Wmemcella)/2.0)+
                 Cwordmetal)*tagbits*A*Ntspd/Ntwl+
                bct->draincap(Wdecinvn,NCH,1) + bct->draincap(Wdecinvp,PCH,1);
        Rline = Rwordmetal*tagbits*A*Ntspd/(2*Ntwl);
        tf = (bct->transreson(Wdecinvp,PCH,1)+Rline)*Cline;
        Twordchargedel = bct->horowitz(nextinputtime,tf,VSINV,VSINV,FALL);
        *outrisetime = Twordchargedel/VSINV;
        return(Tworddrivedel+Twordchargedel);

}

/*----------------------------------------------------------------------*/

/* Data array bitline: (see section 6.4 in tech report) */


double cacti_time_t::bitline_delay(int C, int A, int B, int Ndwl, int Ndbl,
    int Nspd,double inrisetime,double *outrisetime)
{
        double Tbit,Cline,Ccolmux,Rlineb,r1,r2,c1,c2,a,b,c;
        double m,tstep;
        double Cbitrow;    /* bitline capacitance due to access transistor */
        int rows,cols;

        Cbitrow = bct->draincap(Wmemcella,NCH,1)/2.0; /* due to shared contact */
        rows = C/(B*A*Ndbl*Nspd);
        cols = 8*B*A*Nspd/Ndwl;
        if (Ndbl*Nspd == 1) {
           Cline = rows*(Cbitrow+Cbitmetal)+2*bct->draincap(Wbitpreequ,PCH,1);
           Ccolmux = 2*bct->gatecap(WsenseQ1to4,10.0);
           Rlineb = Rbitmetal*rows/2.0;
           r1 = Rlineb;
	} else { 
           Cline = rows*(Cbitrow+Cbitmetal) + 2*bct->draincap(Wbitpreequ,PCH,1) +
                   bct->draincap(Wbitmuxn,NCH,1);
           Ccolmux = Nspd*Ndbl*(bct->draincap(Wbitmuxn,NCH,1))+2*bct->gatecap(WsenseQ1to4,10.0);
           Rlineb = Rbitmetal*rows/2.0;
           r1 = Rlineb + 
                 bct->transreson(Wbitmuxn,NCH,1);
	}
        r2 = bct->transreson(Wmemcella,NCH,1) +
             bct->transreson(Wmemcella*Wmemcellbscale,NCH,1);
        c1 = Ccolmux;
        c2 = Cline;


        tstep = (r2*c2+(r1+r2)*c1)*log((Vbitpre)/(Vbitpre-Vbitsense));

        /* take input rise time into account */

        m = Vdd/inrisetime;
        if (tstep <= (0.5*(Vdd-Vt)/m)) {
              a = m;
              b = 2*((Vdd*0.5)-Vt);
              c = -2*tstep*(Vdd-Vt)+1/m*((Vdd*0.5)-Vt)*
                  ((Vdd*0.5)-Vt);
              Tbit = (-b+sqrt(b*b-4*a*c))/(2*a);
        } else {
              Tbit = tstep + (Vdd+Vt)/(2*m) - (Vdd*0.5)/m;
        }

        *outrisetime = Tbit/(log((Vbitpre-Vbitsense)/Vdd));
        return(Tbit);
}




/*----------------------------------------------------------------------*/

/* Tag array bitline: (see section 6.4 in tech report) */


double cacti_time_t::bitline_tag_delay(int C,int A,int B,int Ntwl,int Ntbl,int Ntspd,
    double inrisetime,double *outrisetime)
{
        double Tbit,Cline,Ccolmux,Rlineb,r1,r2,c1,c2,a,b,c;
        double m,tstep;
        double Cbitrow;    /* bitline capacitance due to access transistor */
        int rows,cols;

        Cbitrow = bct->draincap(Wmemcella,NCH,1)/2.0; /* due to shared contact */
        rows = C/(B*A*Ntbl*Ntspd);
        cols = 8*B*A*Ntspd/Ntwl;
        if (Ntbl*Ntspd == 1) {
           Cline = rows*(Cbitrow+Cbitmetal)+2*bct->draincap(Wbitpreequ,PCH,1);
           Ccolmux = 2*bct->gatecap(WsenseQ1to4,10.0);
           Rlineb = Rbitmetal*rows/2.0;
           r1 = Rlineb;
	} else { 
           Cline = rows*(Cbitrow+Cbitmetal) + 2*bct->draincap(Wbitpreequ,PCH,1) +
                   bct->draincap(Wbitmuxn,NCH,1);
           Ccolmux = Ntspd*Ntbl*(bct->draincap(Wbitmuxn,NCH,1))+2*bct->gatecap(WsenseQ1to4,10.0);
           Rlineb = Rbitmetal*rows/2.0;
           r1 = Rlineb + 
                 bct->transreson(Wbitmuxn,NCH,1);
	}
        r2 = bct->transreson(Wmemcella,NCH,1) +
             bct->transreson(Wmemcella*Wmemcellbscale,NCH,1);

        c1 = Ccolmux;
        c2 = Cline;

        tstep = (r2*c2+(r1+r2)*c1)*log((Vbitpre)/(Vbitpre-Vbitsense));

        /* take into account input rise time */

        m = Vdd/inrisetime;
        if (tstep <= (0.5*(Vdd-Vt)/m)) {
              a = m;
              b = 2*((Vdd*0.5)-Vt);
              c = -2*tstep*(Vdd-Vt)+1/m*((Vdd*0.5)-Vt)*
                  ((Vdd*0.5)-Vt);
              Tbit = (-b+sqrt(b*b-4*a*c))/(2*a);
        } else {
              Tbit = tstep + (Vdd+Vt)/(2*m) - (Vdd*0.5)/m;
        }

        *outrisetime = Tbit/(log((Vbitpre-Vbitsense)/Vdd));
        return(Tbit);
}



/*----------------------------------------------------------------------*/

/* It is assumed the sense amps have a constant delay
   (see section 6.5) */

double cacti_time_t::sense_amp_delay(double inrisetime,double *outrisetime)
{
   *outrisetime = tfalldata;
   return(tsensedata);
}

/*--------------------------------------------------------------*/

double cacti_time_t::sense_amp_tag_delay(double inrisetime,double *outrisetime)
{ 
    *outrisetime = tfalltag;
    return(tsensetag);
}

/*----------------------------------------------------------------------*/

/* Comparator Delay (see section 6.6) */


double cacti_time_t::compare_time(int C,int A,int Ntbl,int Ntspd, 
    double inputtime,double *outputtime)
{
   double Req,Ceq,tf,st1del,st2del,st3del,nextinputtime,m;
   double c1,c2,r1,r2,tstep,a,b,c;
   double Tcomparatorni;
   int cols,tagbits;

   /* First Inverter */

   Ceq = bct->gatecap(Wcompinvn2+Wcompinvp2,10.0) +
         bct->draincap(Wcompinvp1,PCH,1) + bct->draincap(Wcompinvn1,NCH,1);
   Req = bct->transreson(Wcompinvp1,PCH,1);
   tf = Req*Ceq;
   st1del = bct->horowitz(inputtime,tf,VTHCOMPINV,VTHCOMPINV,FALL);
   nextinputtime = st1del/VTHCOMPINV;

   /* Second Inverter */

   Ceq = bct->gatecap(Wcompinvn3+Wcompinvp3,10.0) +
         bct->draincap(Wcompinvp2,PCH,1) + bct->draincap(Wcompinvn2,NCH,1);
   Req = bct->transreson(Wcompinvn2,NCH,1);
   tf = Req*Ceq;
   st2del = bct->horowitz(inputtime,tf,VTHCOMPINV,VTHCOMPINV,RISE);
   nextinputtime = st1del/(1.0-VTHCOMPINV);

   /* Third Inverter */

   Ceq = bct->gatecap(Wevalinvn+Wevalinvp,10.0) +
         bct->draincap(Wcompinvp3,PCH,1) + bct->draincap(Wcompinvn3,NCH,1);
   Req = bct->transreson(Wcompinvp3,PCH,1);
   tf = Req*Ceq;
   st3del = bct->horowitz(nextinputtime,tf,VTHCOMPINV,VTHEVALINV,FALL);
   nextinputtime = st1del/(VTHEVALINV);

   /* Final Inverter (virtual ground driver) discharging compare part */
   
   tagbits = ADDRESS_BITS - (int)bct->logtwo((double)C) + (int)bct->logtwo((double)A);
   cols = tagbits*Ntbl*Ntspd;

   r1 = bct->transreson(Wcompn,NCH,2);
   r2 = bct->transresswitch(Wevalinvn,NCH,1);
   c2 = (tagbits)*(bct->draincap(Wcompn,NCH,1)+bct->draincap(Wcompn,NCH,2))+
         bct->draincap(Wevalinvp,PCH,1) + bct->draincap(Wevalinvn,NCH,1);
   c1 = (tagbits)*(bct->draincap(Wcompn,NCH,1)+bct->draincap(Wcompn,NCH,2))
        +bct->draincap(Wcompp,PCH,1) + bct->gatecap(Wmuxdrv12n+Wmuxdrv12p,20.0) +
        cols*Cwordmetal;

   /* time to go to threshold of mux driver */

   tstep = (r2*c2+(r1+r2)*c1)*log(1.0/VTHMUXDRV1);

   /* take into account non-zero input rise time */

   m = Vdd/nextinputtime;

   if ((tstep) <= (0.5*(Vdd-Vt)/m)) {
  	      a = m;
	      b = 2*((Vdd*VTHEVALINV)-Vt);
              c = -2*(tstep)*(Vdd-Vt)+1/m*((Vdd*VTHEVALINV)-Vt)*((Vdd*VTHEVALINV)-Vt);
 	      Tcomparatorni = (-b+sqrt(b*b-4*a*c))/(2*a);
   } else {
	      Tcomparatorni = (tstep) + (Vdd+Vt)/(2*m) - (Vdd*VTHEVALINV)/m;
   }
   *outputtime = Tcomparatorni/(1.0-VTHMUXDRV1);
			
   return(Tcomparatorni+st1del+st2del+st3del);
}




/*----------------------------------------------------------------------*/

/* Delay of the multiplexor Driver (see section 6.7) */


double cacti_time_t::mux_driver_delay(int C, int B,int A,int Ndbl,int Nspd,
    int Ndwl,int Ntbl,int Ntspd,double inputtime,double *outputtime)
{
    double Ceq,Req,tf,nextinputtime;
    double Tst1,Tst2,Tst3;
    
    /* first driver stage - Inverte "match" to produce "matchb" */
    /* the critical path is the DESELECTED case, so consider what
    happens when the address bit is true, but match goes low */
    
    Ceq = bct->gatecap(WmuxdrvNORn+WmuxdrvNORp,15.0)*(8*B/BITOUT) +
    bct->draincap(Wmuxdrv12n,NCH,1) + bct->draincap(Wmuxdrv12p,PCH,1);
    Req = bct->transreson(Wmuxdrv12p,PCH,1);
    tf = Ceq*Req;
    Tst1 = bct->horowitz(inputtime,tf,VTHMUXDRV1,VTHMUXDRV2,FALL);
    nextinputtime = Tst1/VTHMUXDRV2;
    
    /* second driver stage - NOR "matchb" with address bits to produce sel */
    
    Ceq = bct->gatecap(Wmuxdrv3n+Wmuxdrv3p,15.0) + 2*bct->draincap(WmuxdrvNORn,NCH,1) +
    bct->draincap(WmuxdrvNORp,PCH,2);
    Req = bct->transreson(WmuxdrvNORn,NCH,1);
    tf = Ceq*Req;
    Tst2 = bct->horowitz(nextinputtime,tf,VTHMUXDRV2,VTHMUXDRV3,RISE);
    nextinputtime = Tst2/(1-VTHMUXDRV3);
    
    /* third driver stage - invert "select" to produce "select bar" */
    
    Ceq = BITOUT*bct->gatecap(Woutdrvseln+Woutdrvselp+Woutdrvnorn+Woutdrvnorp,20.0)+
    bct->draincap(Wmuxdrv3p,PCH,1) + bct->draincap(Wmuxdrv3n,NCH,1) +
    Cwordmetal*8*B*A*Nspd*Ndbl/2.0;
    Req = (Rwordmetal*8*B*A*Nspd*Ndbl/2)/2 + bct->transreson(Wmuxdrv3p,PCH,1);
    tf = Ceq*Req;
    Tst3 = bct->horowitz(nextinputtime,tf,VTHMUXDRV3,VTHOUTDRINV,FALL);
    *outputtime = Tst3/(VTHOUTDRINV);
    
    return(Tst1 + Tst2 + Tst3);
    
}



/*----------------------------------------------------------------------*/

/* Valid driver (see section 6.9 of tech report)
   Note that this will only be called for a direct mapped cache */

double cacti_time_t::valid_driver_delay(int C,int A,int Ntbl,int Ntspd,
    double inputtime)
{
   double Ceq,Tst1,tf;

   Ceq = bct->draincap(Wmuxdrv12n,NCH,1)+bct->draincap(Wmuxdrv12p,PCH,1)+Cout;
   tf = Ceq*bct->transreson(Wmuxdrv12p,PCH,1);
   Tst1 = bct->horowitz(inputtime,tf,VTHMUXDRV1,0.5,FALL);

   return(Tst1);
}


/*----------------------------------------------------------------------*/

/* Data output delay (data side) -- see section 6.8
   This is the time through the NAND/NOR gate and the final inverter 
   assuming sel is already present */

double cacti_time_t::dataoutput_delay(int C, int B, int A, int Ndbl, int Nspd,
    int Ndwl, double inrisetime,double *outrisetime)
{
        double Ceq,Rwire,Rline;
        double aspectRatio;     /* as height over width */
        double ramBlocks;       /* number of RAM blocks */
        double tf;
        double nordel,outdel,nextinputtime;       
        double hstack,vstack;

        /* calculate some layout info */

        aspectRatio = (2.0*C)/(8.0*B*B*A*A*Ndbl*Ndbl*Nspd*Nspd);
        hstack = (aspectRatio > 1.0) ? aspectRatio : 1.0/aspectRatio;
        ramBlocks = Ndwl*Ndbl;
        hstack = hstack * sqrt(ramBlocks/ hstack);
        vstack = ramBlocks/ hstack;

        /* Delay of NOR gate */

        Ceq = 2*bct->draincap(Woutdrvnorn,NCH,1)+bct->draincap(Woutdrvnorp,PCH,2)+
              bct->gatecap(Woutdrivern,10.0);
        tf = Ceq*bct->transreson(Woutdrvnorp,PCH,2);
        nordel = bct->horowitz(inrisetime,tf,VTHOUTDRNOR,VTHOUTDRIVE,FALL);
        nextinputtime = nordel/(VTHOUTDRIVE);

        /* Delay of final output driver */

        Ceq = (bct->draincap(Woutdrivern,NCH,1)+bct->draincap(Woutdriverp,PCH,1))*
              ((8*B*A)/BITOUT) +
              Cwordmetal*(8*B*A*Nspd* (vstack)) + Cout;
        Rwire = Rwordmetal*(8*B*A*Nspd* (vstack))/2;

        tf = Ceq*(bct->transreson(Woutdriverp,PCH,1)+Rwire);
        outdel = bct->horowitz(nextinputtime,tf,VTHOUTDRIVE,0.5,RISE);
        *outrisetime = outdel/0.5;
        return(outdel+nordel);
}

/*----------------------------------------------------------------------*/

/* Sel inverter delay (part of the output driver)  see section 6.8 */

double cacti_time_t::selb_delay_tag_path(double inrisetime,double *outrisetime)
{
   double Ceq,Tst1,tf;

   Ceq = bct->draincap(Woutdrvseln,NCH,1)+bct->draincap(Woutdrvselp,PCH,1)+
         bct->gatecap(Woutdrvnandn+Woutdrvnandp,10.0);
   tf = Ceq*bct->transreson(Woutdrvseln,NCH,1);
   Tst1 = bct->horowitz(inrisetime,tf,VTHOUTDRINV,VTHOUTDRNAND,RISE);
   *outrisetime = Tst1/(1.0-VTHOUTDRNAND);

   return(Tst1);
}


/*----------------------------------------------------------------------*/

/* This routine calculates the extra time required after an access before
 * the next access can occur [ie.  it returns (cycle time-access time)].
 */

double cacti_time_t::precharge_delay(double worddata)
{
   double Ceq,tf,pretime;

   /* as discussed in the tech report, the delay is the delay of
      4 inverter delays (each with fanout of 4) plus the delay of
      the wordline */

   Ceq = bct->draincap(Wdecinvn,NCH,1)+bct->draincap(Wdecinvp,PCH,1)+
         4*bct->gatecap(Wdecinvn+Wdecinvp,0.0);
   tf = Ceq*bct->transreson(Wdecinvn,NCH,1);
   pretime = 4*bct->horowitz(0.0,tf,0.5,0.5,RISE) + worddata;

   return(pretime);
}



/*======================================================================*/


/* returns TRUE if the parameters make up a valid organization */
/* Layout concerns drive any restrictions you might add here */
 
int cacti_time_t::organizational_parameters_valid(int rows,int cols,int Ndwl,
    int Ndbl,int Nspd,int Ntwl,int Ntbl,int Ntspd)
{
   /* don't want more than 8 subarrays for each of data/tag */

   if (Ndwl*Ndbl>MAXSUBARRAYS) return(FALSE);
   if (Ntwl*Ntbl>MAXSUBARRAYS) return(FALSE);

   /* add more constraints here as necessary */

   return(TRUE);
}


/*----------------------------------------------------------------------*/


void cacti_time_t::calculate_time(time_result_type *result, 
    time_parameter_type *parameters)
{
   int Ndwl,Ndbl,Nspd,Ntwl,Ntbl,Ntspd,rows,columns,tag_driver_size1,tag_driver_size2;
   double access_time;
   double before_mux,after_mux;
   double decoder_data_driver,decoder_data_3to8,decoder_data_inv;
   double decoder_data,decoder_tag,wordline_data,wordline_tag;
   double decoder_tag_driver,decoder_tag_3to8,decoder_tag_inv;
   double bitline_data,bitline_tag,sense_amp_data,sense_amp_tag;
   double compare_tag,mux_driver,data_output,selb;
   double time_till_compare,time_till_select,driver_cap,valid_driver;
   double cycle_time, precharge_del;
   double outrisetime,inrisetime;   

   rows = parameters->number_of_sets;
   columns = 8*parameters->block_size*parameters->associativity;

   /* go through possible Ndbl,Ndwl and find the smallest */
   /* Because of area considerations, I don't think it makes sense
      to break either dimension up larger than MAXN */

   result->cycle_time = BIGNUM;
   result->access_time = BIGNUM;
   for (Nspd=1;Nspd<=MAXSPD;Nspd=Nspd*2) {
    for (Ndwl=1;Ndwl<=MAXN;Ndwl=Ndwl*2) {
     for (Ndbl=1;Ndbl<=MAXN;Ndbl=Ndbl*2) {
      for (Ntspd=1;Ntspd<=MAXSPD;Ntspd=Ntspd*2) {
       for (Ntwl=1;Ntwl<=1;Ntwl=Ntwl*2) {
	for (Ntbl=1;Ntbl<=MAXN;Ntbl=Ntbl*2) {

	if (organizational_parameters_valid
	    (rows,columns,Ndwl,Ndbl,Nspd,Ntwl,Ntbl,Ntspd)) {


            /* Calculate data side of cache */


	    decoder_data = decoder_delay(parameters->cache_size,parameters->block_size,
			   parameters->associativity,Ndwl,Ndbl,Nspd,Ntwl,Ntbl,Ntspd,
			   &decoder_data_driver,&decoder_data_3to8,
			   &decoder_data_inv,&outrisetime);

            inrisetime = outrisetime;
	    wordline_data = wordline_delay(parameters->block_size,
			    parameters->associativity,Ndwl,Nspd,
                            inrisetime,&outrisetime);
            inrisetime = outrisetime;
	    bitline_data = bitline_delay(parameters->cache_size,parameters->associativity,
			   parameters->block_size,Ndwl,Ndbl,Nspd,
                           inrisetime,&outrisetime);
            inrisetime = outrisetime;
	    sense_amp_data = sense_amp_delay(inrisetime,&outrisetime);
            inrisetime = outrisetime;
	    data_output = dataoutput_delay(parameters->cache_size,parameters->block_size,
			  parameters->associativity,Ndbl,Nspd,Ndwl,
			  inrisetime,&outrisetime);
            inrisetime = outrisetime;


            /* if the associativity is 1, the data output can come right
               after the sense amp.   Otherwise, it has to wait until 
               the data access has been done. */

	    if (parameters->associativity==1) {
	       before_mux = decoder_data + wordline_data + bitline_data +
			    sense_amp_data + data_output;
	       after_mux = 0;
	    } else {   
		  before_mux = decoder_data + wordline_data + bitline_data +
			       sense_amp_data;
		  after_mux = data_output;
	    }


	    /*
	     * Now worry about the tag side.
	     */


	    decoder_tag = decoder_tag_delay(parameters->cache_size,
                          parameters->block_size,parameters->associativity,
			  Ndwl,Ndbl,Nspd,Ntwl,Ntbl,Ntspd,
			  &decoder_tag_driver,&decoder_tag_3to8,
			  &decoder_tag_inv,&outrisetime);
            inrisetime = outrisetime;

	    wordline_tag = wordline_tag_delay(parameters->cache_size,
			   parameters->associativity,Ntspd,Ntwl,
                           inrisetime,&outrisetime);
            inrisetime = outrisetime;

	    bitline_tag = bitline_tag_delay(parameters->cache_size,parameters->associativity,
			  parameters->block_size,Ntwl,Ntbl,Ntspd,
                          inrisetime,&outrisetime);
            inrisetime = outrisetime;

	    sense_amp_tag = sense_amp_tag_delay(inrisetime,&outrisetime);
            inrisetime = outrisetime;

	    compare_tag = compare_time(parameters->cache_size,parameters->associativity,
			  Ntbl,Ntspd,
			  inrisetime,&outrisetime);
            inrisetime = outrisetime;

	    if (parameters->associativity == 1) {
	       mux_driver = 0;
	       valid_driver = valid_driver_delay(parameters->cache_size,
			      parameters->associativity,Ntbl,Ntspd,inrisetime);
	       time_till_compare = decoder_tag + wordline_tag + bitline_tag +
 				   sense_amp_tag;
 
	       time_till_select = time_till_compare+ compare_tag + valid_driver;
	
	 
	       /*
		* From the above info, calculate the total access time
		*/
	  
	       access_time = MAX(before_mux+after_mux,time_till_select);


	    } else {
	       mux_driver = mux_driver_delay(parameters->cache_size,parameters->block_size,
			    parameters->associativity,Ndbl,Nspd,Ndwl,Ntbl,Ntspd,
                            inrisetime,&outrisetime);

               selb = selb_delay_tag_path(inrisetime,&outrisetime);

	       valid_driver = 0;

	       time_till_compare = decoder_tag + wordline_tag + bitline_tag +
 				sense_amp_tag;

	       time_till_select = time_till_compare+ compare_tag + mux_driver
                                  + selb;

	       access_time = MAX(before_mux,time_till_select) +after_mux;
	    }

	    /*
	     * Calcuate the cycle time
	     */

	    precharge_del = precharge_delay(wordline_data);
	     
	    cycle_time = access_time + precharge_del;
	 
	    /*
	     * The parameters are for a 0.8um process.  A quick way to
             * scale the results to another process is to divide all
             * the results by FUDGEFACTOR.  Normally, FUDGEFACTOR is 1.
	     */

	    if (result->cycle_time+1e-11*(result->best_Ndwl+result->best_Ndbl+result->best_Nspd+result->best_Ntwl+result->best_Ntbl+result->best_Ntspd) > cycle_time/FUDGEFACTOR+1e-11*(Ndwl+Ndbl+Nspd+Ntwl+Ntbl+Ntspd)) {
	       result->cycle_time = cycle_time/FUDGEFACTOR;
	       result->access_time = access_time/FUDGEFACTOR;
	       result->best_Ndwl = Ndwl;
	       result->best_Ndbl = Ndbl;
	       result->best_Nspd = Nspd;
	       result->best_Ntwl = Ntwl;
	       result->best_Ntbl = Ntbl;
	       result->best_Ntspd = Ntspd;
	       result->decoder_delay_data = decoder_data/FUDGEFACTOR;
	       result->decoder_delay_tag = decoder_tag/FUDGEFACTOR;
	       result->dec_tag_driver = decoder_tag_driver/FUDGEFACTOR;
	       result->dec_tag_3to8 = decoder_tag_3to8/FUDGEFACTOR;
	       result->dec_tag_inv = decoder_tag_inv/FUDGEFACTOR;
	       result->dec_data_driver = decoder_data_driver/FUDGEFACTOR;
	       result->dec_data_3to8 = decoder_data_3to8/FUDGEFACTOR;
	       result->dec_data_inv = decoder_data_inv/FUDGEFACTOR;
	       result->wordline_delay_data = wordline_data/FUDGEFACTOR;
	       result->wordline_delay_tag = wordline_tag/FUDGEFACTOR;
	       result->bitline_delay_data = bitline_data/FUDGEFACTOR;
	       result->bitline_delay_tag = bitline_tag/FUDGEFACTOR;
	       result->sense_amp_delay_data = sense_amp_data/FUDGEFACTOR;
	       result->sense_amp_delay_tag = sense_amp_tag/FUDGEFACTOR;
	       result->compare_part_delay = compare_tag/FUDGEFACTOR;
	       result->drive_mux_delay = mux_driver/FUDGEFACTOR;
	       result->selb_delay = selb/FUDGEFACTOR;
	       result->drive_valid_delay = valid_driver/FUDGEFACTOR;
	       result->data_output_delay = data_output/FUDGEFACTOR;
	       result->precharge_delay = precharge_del/FUDGEFACTOR;
	    }
	  }
	 }
	}
       }
      }
     }
     }

}
   
