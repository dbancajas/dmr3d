





#ifndef _CIRCUIT_PARAM_H_
#define _CIRCUIT_PARAM_H_



/* Used to communicate with the horowitz model */

#define RISE 1
#define FALL 0
#define NCH  1
#define PCH  0


class circuit_param_t {
    
    public:
    
      // All parameters are publicly accessible and to be set and used once
      
      
      
      
      //v4.1: Making all constants static variables. Initially these variables are based
      //off 0.8 micron process values; later on in init_tech_params function of leakage.c 
      //they are scaled to input tech node parameters 
      
      static double cparam_Cndiffarea;
      
      /* fF/um2 at 1.5V */
      //#define Cpdiffarea    0.343e-15
      
      static double cparam_Cpdiffarea;
      
      /* fF/um at 1.5V */
      //#define Cndiffside    0.275e-15
      
      static double cparam_Cndiffside;
      
      /* fF/um at 1.5V */
      //#define Cpdiffside    0.275e-15
      static double cparam_Cpdiffside;
      
      /* fF/um at 1.5V */
      //#define Cndiffovlp    0.138e-15
      static double cparam_Cndiffovlp;
      
      /* fF/um at 1.5V */
      //#define Cpdiffovlp    0.138e-15
      static double cparam_Cpdiffovlp;
      
      /* fF/um assuming 25% Miller effect */
      //#define Cnoxideovlp   0.263e-15
      static double cparam_Cnoxideovlp;
      
      /* fF/um assuming 25% Miller effect */
      //#define Cpoxideovlp   0.338e-15
      static double cparam_Cpoxideovlp;
      
      /* um */
      //#define Leff          (0.8)
      static double cparam_Leff;
      
      //#define inv_Leff	  1.25
      static double cparam_inv_Leff;
      
      /* fF/um2 */
      //#define Cgate         1.95e-15
      static double cparam_Cgate;	
      
      /* fF/um2 */
      //#define Cgatepass     1.45e-15
      static double cparam_Cgatepass;		
      
      /* note that the value of Cgatepass will be different depending on 
      whether or not the source and drain are at different potentials or
      the same potential.  The two values were averaged */
      
      /* fF/um */
      //#define Cpolywire	(0.25e-15)	
      static double cparam_Cpolywire;			 
      
      /* ohms*um of channel width */
      //#define Rnchannelstatic	(25800)
      static double cparam_Rnchannelstatic;
      
      /* ohms*um of channel width */
      //#define Rpchannelstatic	(61200)
      static double cparam_Rpchannelstatic;
      
      //#define Rnchannelon	(8751)
      static double cparam_Rnchannelon;
      
      //#define Rpchannelon	(20160)
      static double cparam_Rpchannelon;
      
      
      static double cparam_Wdecdrivep;
      static double cparam_Wdecdriven;
      static double cparam_Wworddrivemax;
      static double cparam_Waddrdrvn1;
      static double cparam_Waddrdrvp1;
      static double cparam_Waddrdrvn2;
      static double cparam_Waddrdrvp2;
      
      
      static double cparam_Wtdecdrivep_second;
      static double cparam_Wtdecdriven_second;
      static double cparam_Wtdecdrivep_first;
      static double cparam_Wtdecdriven_first;
      static double cparam_WtdecdrivetreeN[10];
      static double cparam_Ctdectreesegments[10];
      static double cparam_Rtdectreesegments[10];
      static int    cparam_nr_tdectreesegments;
      static double cparam_Wtdec3to8n ;
      static double cparam_Wtdec3to8p ;
      static double cparam_WtdecNORn  ;
      static double cparam_WtdecNORp  ;
      static double cparam_Wtdecinvn  ;
      static double cparam_Wtdecinvp  ;
      static double cparam_WtwlDrvn ;
      static double cparam_WtwlDrvp ;
      
      
      static double cparam_Wpch;
      static double cparam_Wiso;
      static double cparam_WsenseEn;
      static double cparam_WsenseN;
      static double cparam_WsenseP;
      static double cparam_WsPch;
      static double cparam_WoBufN;
      static double cparam_WoBufP;
      
      static double cparam_WpchDrvp;
      static double cparam_WpchDrvn;
      static double cparam_WisoDrvp;
      static double cparam_WisoDrvn;
      static double cparam_WspchDrvp;
      static double cparam_WspchDrvn;
      static double cparam_WsenseEnDrvp;
      static double cparam_WsenseEnDrvn;
      
      static double cparam_WwrtMuxSelDrvn;
      static double cparam_WwrtMuxSelDrvp;
      static double cparam_WtwrtMuxSelDrvn;
      static double cparam_WtwrtMuxSelDrvp;
      
      static double cparam_Wtbitpreequ;
      static double cparam_Wtpch;
      static double cparam_Wtiso;
      static double cparam_WtsenseEn;
      static double cparam_WtsenseN;
      static double cparam_WtsenseP;
      static double cparam_WtoBufN;
      static double cparam_WtoBufP;
      static double cparam_WtsPch;
      
      static double cparam_WtpchDrvp;
      static double cparam_WtpchDrvn;
      static double cparam_WtisoDrvp;
      static double cparam_WtisoDrvn;
      static double cparam_WtspchDrvp;
      static double cparam_WtspchDrvn;
      static double cparam_WtsenseEnDrvp;
      static double cparam_WtsenseEnDrvn;
      
      static double cparam_Wcompinvp1;
      static double cparam_Wcompinvn1;
      static double cparam_Wcompinvp2;
      static double cparam_Wcompinvn2;
      static double cparam_Wcompinvp3;
      static double cparam_Wcompinvn3;
      static double cparam_Wevalinvp;
      static double cparam_Wevalinvn;
      
      static double cparam_Wfadriven;
      static double cparam_Wfadrivep;
      static double cparam_Wfadrive2n;
      static double cparam_Wfadrive2p;
      static double cparam_Wfadecdrive1n;
      static double cparam_Wfadecdrive1p;
      static double cparam_Wfadecdrive2n;
      static double cparam_Wfadecdrive2p;
      static double cparam_Wfadecdriven;
      static double cparam_Wfadecdrivep;
      static double cparam_Wfaprechn;
      static double cparam_Wfaprechp;
      static double cparam_Wdummyn;
      static double cparam_Wdummyinvn;
      static double cparam_Wdummyinvp;
      static double cparam_Wfainvn;
      static double cparam_Wfainvp;
      static double cparam_Waddrnandn;
      static double cparam_Waddrnandp;
      static double cparam_Wfanandn;
      static double cparam_Wfanandp;
      static double cparam_Wfanorn;
      static double cparam_Wfanorp;
      static double cparam_Wdecnandn;
      static double cparam_Wdecnandp;
      
      static double cparam_Wcompn;
      static double cparam_Wcompp;
      static double cparam_Wmuxdrv12n;
      static double cparam_Wmuxdrv12p;

      
      
      static double cparam_Wdecdrivep_second;
      static double cparam_Wdecdriven_second;
      static double cparam_Wdecdrivep_first;
      static double cparam_Wdecdriven_first;
      static double cparam_WdecdrivetreeN[10];
      static double cparam_Cdectreesegments[10];
      static double cparam_Rdectreesegments[10];
      static int    cparam_nr_dectreesegments;
      static double cparam_Wdec3to8n ;
      static double cparam_Wdec3to8p ;
      static double cparam_WdecNORn  ;
      static double cparam_WdecNORp  ;
      static double cparam_Wdecinvn  ;
      static double cparam_Wdecinvp  ;
      static double cparam_WwlDrvn ;
      static double cparam_WwlDrvp ;

      static double cparam_Wcomppreequ; 
      
      
      static double cparam_WireRscaling;
      static double cparam_WireCscaling;
      
      static double cparam_Cwordmetal;
      static double cparam_Cbitmetal;
      static double cparam_CM3metal;        
      static double cparam_CM2metal;         

      static double cparam_Rwordmetal;
      static double cparam_Rbitmetal;
      
      static double cparam_TagCwordmetal;
      static double cparam_TagCbitmetal;
      static double cparam_TagRwordmetal;
      static double cparam_TagRbitmetal;
      
      static double cparam_GlobalCwordmetal;
      static double cparam_GlobalCbitmetal;
      static double cparam_GlobalRwordmetal;
      static double cparam_GlobalRbitmetal;
      static double cparam_FACwordmetal;
      static double cparam_FACbitmetal;
      static double cparam_FARwordmetal;
      static double cparam_FARbitmetal;
      
      static double cparam_VddPow;
      
      static double cparam_BitWidth;
      static double cparam_BitHeight;
      static double cparam_Cout;
      static int cparam_dualVt ; /// ??
      static int cparam_explore; /// ??
      
      static int cparam_BITOUT;
      static int cparam_muxover;
      
      
      static double cparam_krise;
      static double cparam_tsensedata;
      static double cparam_psensedata;
      static double cparam_tsensescale;
      static double cparam_tsensetag;
      static double cparam_psensetag;
      static double cparam_tfalldata;
      static double cparam_tfalltag;
      
      
      
      static double cparam_Wmemcella;
      static double cparam_Wmemcellpmos;
      static double cparam_Wmemcellnmos;
      
      
      static int cparam_Wmemcellbscale;
      static double cparam_Wbitpreequ;
      static double cparam_Wpchmax;
      
      static double cparam_WmuxdrvNANDn    ;
      static double cparam_WmuxdrvNANDp    ;
      static double cparam_WmuxdrvNORn	;
      static double cparam_WmuxdrvNORp	;
      static double cparam_Wmuxdrv3n	;
      static double cparam_Wmuxdrv3p	;
      static double cparam_Woutdrvseln	;
      static double cparam_Woutdrvselp	;
      
      
      
      static double cparam_Woutdrvnandn;
      static double cparam_Woutdrvnandp;
      static double cparam_Woutdrvnorn	;
      static double cparam_Woutdrvnorp	;
      static double cparam_Woutdrivern	;
      static double cparam_Woutdriverp	;
      
      
      static double cparam_Wsenseextdrv1p;
      static double cparam_Wsenseextdrv1n;
      static double cparam_Wsenseextdrv2p;
      static double cparam_Wsenseextdrv2n;
      
      static double cparam_WIRESPACING;
      static double cparam_WIREWIDTH;
      /*dt: I've taken the aspect ratio from the Intel paper on their 65nm process */
      static double cparam_WIREHEIGTHRATIO;
      
      static double cparam_WIREPITCH;
      static double cparam_FUDGEFACTOR;
      static double cparam_FEATURESIZE;
      

      
      
};

#endif
