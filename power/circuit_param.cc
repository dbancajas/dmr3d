/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* 
 *
 * description:    definition of circuit param variables
 * initial author: Koushik Chakraborty 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: circuit_param.cc,v 1.1.2.12 2006/12/12 19:38:31 kchak Exp $");

#include "circuit_param.h"

double circuit_param_t::cparam_Cndiffarea;
double circuit_param_t::cparam_Cpdiffarea;
double circuit_param_t::cparam_Cndiffside;
double circuit_param_t::cparam_Cpdiffside;
double circuit_param_t::cparam_Cndiffovlp;
double circuit_param_t::cparam_Cpdiffovlp;
double circuit_param_t::cparam_Cnoxideovlp;
double circuit_param_t::cparam_Cpoxideovlp;
double circuit_param_t::cparam_Leff;
double circuit_param_t::cparam_inv_Leff;
double circuit_param_t::cparam_Cgate;	
double circuit_param_t::cparam_Cgatepass;		
double circuit_param_t::cparam_Cpolywire;			 
double circuit_param_t::cparam_Rnchannelstatic;
double circuit_param_t::cparam_Rpchannelstatic;
double circuit_param_t::cparam_Rnchannelon;
double circuit_param_t::cparam_Rpchannelon;

 
double circuit_param_t::cparam_FUDGEFACTOR;
double circuit_param_t::cparam_FEATURESIZE;





double circuit_param_t::cparam_Wdecdrivep;
double circuit_param_t::cparam_Wdecdriven;
double circuit_param_t::cparam_Wworddrivemax;
double circuit_param_t::cparam_Waddrdrvn1;
double circuit_param_t::cparam_Waddrdrvp1;
double circuit_param_t::cparam_Waddrdrvn2;
double circuit_param_t::cparam_Waddrdrvp2;


double circuit_param_t::cparam_Wtdecdrivep_second;
double circuit_param_t::cparam_Wtdecdriven_second;
double circuit_param_t::cparam_Wtdecdrivep_first;
double circuit_param_t::cparam_Wtdecdriven_first;
double circuit_param_t::cparam_WtdecdrivetreeN[10];
double circuit_param_t::cparam_Ctdectreesegments[10];
double circuit_param_t::cparam_Rtdectreesegments[10];
int    circuit_param_t::cparam_nr_tdectreesegments;
double circuit_param_t::cparam_Wtdec3to8n ;
double circuit_param_t::cparam_Wtdec3to8p ;
double circuit_param_t::cparam_WtdecNORn  ;
double circuit_param_t::cparam_WtdecNORp  ;
double circuit_param_t::cparam_Wtdecinvn  ;
double circuit_param_t::cparam_Wtdecinvp  ;
double circuit_param_t::cparam_WtwlDrvn ;
double circuit_param_t::cparam_WtwlDrvp ;



double circuit_param_t::cparam_Wpch;
double circuit_param_t::cparam_Wiso;
double circuit_param_t::cparam_WsenseEn;
double circuit_param_t::cparam_WsenseN;
double circuit_param_t::cparam_WsenseP;
double circuit_param_t::cparam_WsPch;
double circuit_param_t::cparam_WoBufN;
double circuit_param_t::cparam_WoBufP;

double circuit_param_t::cparam_WpchDrvp;
double circuit_param_t::cparam_WpchDrvn;
double circuit_param_t::cparam_WisoDrvp;
double circuit_param_t::cparam_WisoDrvn;
double circuit_param_t::cparam_WspchDrvp;
double circuit_param_t::cparam_WspchDrvn;
double circuit_param_t::cparam_WsenseEnDrvp;
double circuit_param_t::cparam_WsenseEnDrvn;

double circuit_param_t::cparam_WwrtMuxSelDrvn;
double circuit_param_t::cparam_WwrtMuxSelDrvp;
double circuit_param_t::cparam_WtwrtMuxSelDrvn;
double circuit_param_t::cparam_WtwrtMuxSelDrvp;

double circuit_param_t::cparam_Wtbitpreequ;
double circuit_param_t::cparam_Wtpch;
double circuit_param_t::cparam_Wtiso;
double circuit_param_t::cparam_WtsenseEn;
double circuit_param_t::cparam_WtsenseN;
double circuit_param_t::cparam_WtsenseP;
double circuit_param_t::cparam_WtoBufN;
double circuit_param_t::cparam_WtoBufP;
double circuit_param_t::cparam_WtsPch;

double circuit_param_t::cparam_WtpchDrvp;
double circuit_param_t::cparam_WtpchDrvn;
double circuit_param_t::cparam_WtisoDrvp;
double circuit_param_t::cparam_WtisoDrvn;
double circuit_param_t::cparam_WtspchDrvp;
double circuit_param_t::cparam_WtspchDrvn;
double circuit_param_t::cparam_WtsenseEnDrvp;
double circuit_param_t::cparam_WtsenseEnDrvn;

double circuit_param_t::cparam_Wcompinvp1;
double circuit_param_t::cparam_Wcompinvn1;
double circuit_param_t::cparam_Wcompinvp2;
double circuit_param_t::cparam_Wcompinvn2;
double circuit_param_t::cparam_Wcompinvp3;
double circuit_param_t::cparam_Wcompinvn3;
double circuit_param_t::cparam_Wevalinvp;
double circuit_param_t::cparam_Wevalinvn;

double circuit_param_t::cparam_Wfadriven;
double circuit_param_t::cparam_Wfadrivep;
double circuit_param_t::cparam_Wfadrive2n;
double circuit_param_t::cparam_Wfadrive2p;
double circuit_param_t::cparam_Wfadecdrive1n;
double circuit_param_t::cparam_Wfadecdrive1p;
double circuit_param_t::cparam_Wfadecdrive2n;
double circuit_param_t::cparam_Wfadecdrive2p;
double circuit_param_t::cparam_Wfadecdriven;
double circuit_param_t::cparam_Wfadecdrivep;
double circuit_param_t::cparam_Wfaprechn;
double circuit_param_t::cparam_Wfaprechp;
double circuit_param_t::cparam_Wdummyn;
double circuit_param_t::cparam_Wdummyinvn;
double circuit_param_t::cparam_Wdummyinvp;
double circuit_param_t::cparam_Wfainvn;
double circuit_param_t::cparam_Wfainvp;
double circuit_param_t::cparam_Waddrnandn;
double circuit_param_t::cparam_Waddrnandp;
double circuit_param_t::cparam_Wfanandn;
double circuit_param_t::cparam_Wfanandp;
double circuit_param_t::cparam_Wfanorn;
double circuit_param_t::cparam_Wfanorp;
double circuit_param_t::cparam_Wdecnandn;
double circuit_param_t::cparam_Wdecnandp;

double circuit_param_t::cparam_Wcompn;
double circuit_param_t::cparam_Wcompp;
double circuit_param_t::cparam_Wmuxdrv12n;
double circuit_param_t::cparam_Wmuxdrv12p;



double circuit_param_t::cparam_Wdecdrivep_second;
double circuit_param_t::cparam_Wdecdriven_second;
double circuit_param_t::cparam_Wdecdrivep_first;
double circuit_param_t::cparam_Wdecdriven_first;
double circuit_param_t::cparam_WdecdrivetreeN[10];
double circuit_param_t::cparam_Cdectreesegments[10];
double circuit_param_t::cparam_Rdectreesegments[10];
int    circuit_param_t::cparam_nr_dectreesegments;
double circuit_param_t::cparam_Wdec3to8n ;
double circuit_param_t::cparam_Wdec3to8p ;
double circuit_param_t::cparam_WdecNORn  ;
double circuit_param_t::cparam_WdecNORp  ;
double circuit_param_t::cparam_Wdecinvn  ;
double circuit_param_t::cparam_Wdecinvp  ;
double circuit_param_t::cparam_WwlDrvn ;
double circuit_param_t::cparam_WwlDrvp ;

double circuit_param_t::cparam_Wcomppreequ;



double circuit_param_t::cparam_WireRscaling;
double circuit_param_t::cparam_WireCscaling;

double circuit_param_t::cparam_Cwordmetal;
double circuit_param_t::cparam_Cbitmetal;
double circuit_param_t::cparam_Rwordmetal;
double circuit_param_t::cparam_Rbitmetal;

double circuit_param_t::cparam_CM3metal = circuit_param_t::cparam_Cbitmetal/16; 
double circuit_param_t::cparam_CM2metal = circuit_param_t::cparam_Cbitmetal/16; 


double circuit_param_t::cparam_TagCwordmetal;
double circuit_param_t::cparam_TagCbitmetal;
double circuit_param_t::cparam_TagRwordmetal;
double circuit_param_t::cparam_TagRbitmetal;

double circuit_param_t::cparam_GlobalCwordmetal;
double circuit_param_t::cparam_GlobalCbitmetal;
double circuit_param_t::cparam_GlobalRwordmetal;
double circuit_param_t::cparam_GlobalRbitmetal;
double circuit_param_t::cparam_FACwordmetal;
double circuit_param_t::cparam_FACbitmetal;
double circuit_param_t::cparam_FARwordmetal;
double circuit_param_t::cparam_FARbitmetal;


double circuit_param_t::cparam_BitWidth;
double circuit_param_t::cparam_BitHeight;
double circuit_param_t::cparam_Cout;
int circuit_param_t::cparam_dualVt ; /// ??
int circuit_param_t::cparam_explore; /// ??
int circuit_param_t::cparam_BITOUT;

double circuit_param_t::cparam_krise;
double circuit_param_t::cparam_tsensedata;
double circuit_param_t::cparam_psensedata;
double circuit_param_t::cparam_tsensescale;
double circuit_param_t::cparam_tsensetag;
double circuit_param_t::cparam_psensetag;
double circuit_param_t::cparam_tfalldata;
double circuit_param_t::cparam_tfalltag;



double circuit_param_t::cparam_Wmemcella;
double circuit_param_t::cparam_Wmemcellpmos;
double circuit_param_t::cparam_Wmemcellnmos;


int circuit_param_t::cparam_Wmemcellbscale;
double circuit_param_t::cparam_Wbitpreequ;
double circuit_param_t::cparam_Wpchmax;



double circuit_param_t::cparam_WmuxdrvNANDn    ;
double circuit_param_t::cparam_WmuxdrvNANDp    ;
double circuit_param_t::cparam_WmuxdrvNORn	;
double circuit_param_t::cparam_WmuxdrvNORp	;
double circuit_param_t::cparam_Wmuxdrv3n	;
double circuit_param_t::cparam_Wmuxdrv3p	;
double circuit_param_t::cparam_Woutdrvseln	;
double circuit_param_t::cparam_Woutdrvselp	;

int circuit_param_t::cparam_muxover;

double circuit_param_t::cparam_VddPow;


double circuit_param_t::cparam_WIRESPACING = (1.6*cparam_FEATURESIZE);
double circuit_param_t::cparam_WIREWIDTH = (1.6*cparam_FEATURESIZE);
      /*dt: I've taken the aspect ratio from the Intel paper on their 65nm process */
double circuit_param_t::cparam_WIREHEIGTHRATIO = 	1.8;
      
double circuit_param_t:: cparam_WIREPITCH = (cparam_WIRESPACING+cparam_WIREWIDTH);



double circuit_param_t::cparam_Woutdrvnandn;
double circuit_param_t::cparam_Woutdrvnandp;
double circuit_param_t::cparam_Woutdrvnorn	;
double circuit_param_t::cparam_Woutdrvnorp	;
double circuit_param_t::cparam_Woutdrivern	;
double circuit_param_t::cparam_Woutdriverp	;


double circuit_param_t::cparam_Wsenseextdrv1p;
double circuit_param_t::cparam_Wsenseextdrv1n;
double circuit_param_t::cparam_Wsenseextdrv2p;
double circuit_param_t::cparam_Wsenseextdrv2n;

