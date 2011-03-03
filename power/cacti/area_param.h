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

 
#ifndef CACTI_AREAPARAM_H_
#define CACTI_AREAPARAM_H_
 


#define tracks_precharge_p    12
#define tracks_precharge_nx2   5 
#define tracks_outdrvselinv_p  3
#define tracks_outdrvfanand_p  6  

#define CONVERT_TO_MMSQUARE 1.0/1000000.0

//// ---- KC addition for compilation ----
#define CACTI_FALSE 0
#define CACTI_TRUE 1


class area_param_t {
    
    public:

    
    static double aparam_area_all_datasubarrays;
    static double aparam_area_all_tagsubarrays;
    static double aparam_area_all_dataramcells;
    static double aparam_area_all_tagramcells;
    static double aparam_faarea_all_subarrays ;
    
    static double aparam_aspect_ratio_data;
    static double aparam_aspect_ratio_tag;
    static double aparam_aspect_ratio_subbank;
    static double aparam_aspect_ratio_total;
    
    /*
    was: #define Widthptondiff 4.0
    now: is 4* feature size, taken from the Intel 65nm process paper
    */
    
    //v4.1: Making all constants static variables. Initially these variables are based
    //off 0.8 micron process values; later on in init_tech_params function of leakage.c 
    //they are scaled to input tech node parameters
    
    //#define Widthptondiff 3.2
    static double aparam_Widthptondiff;
    /* 
    was: #define Widthtrack    3.2
    now: 3.2*FEATURESIZE, i.e. 3.2*0.8
    */
    //#define Widthtrack    (3.2*0.8)
    static double aparam_Widthtrack;
    //#define Widthcontact  1.6
    static double aparam_Widthcontact;
    //#define Wpoly         0.8
    static double aparam_Wpoly;
    //#define ptocontact    0.4
    static double aparam_ptocontact;
    //#define stitch_ramv   6.0 
    static double aparam_stitch_ramv;
    //#define BitHeight16x2 33.6
    //#define BitHeight1x1 (2*7.746*0.8) /* see below */
    static double aparam_BitHeight1x1;
    //#define stitch_ramh   12.0
    static double aparam_stitch_ramh;
    //#define BitWidth16x2  192.8
    //#define BitWidth1x1	  (7.746*0.8) 
    static double aparam_BitWidth1x1;
    /* dt: Assume that each 6-T SRAM cell is 120F^2 and has an aspect ratio of 1(width) to 2(height), than the width is 2*sqrt(60)*F */
    //#define WidthNOR1     11.6
    static double aparam_WidthNOR1;
    //#define WidthNOR2     13.6
    static double aparam_WidthNOR2;
    //#define WidthNOR3     20.8
    static double aparam_WidthNOR3;
    //#define WidthNOR4     28.8
    static double aparam_WidthNOR4;
    //#define WidthNOR5     34.4
    static double aparam_WidthNOR5;
    //#define WidthNOR6     41.6
    static double aparam_WidthNOR6;
    //#define Predec_height1    140.8
    static double aparam_Predec_height1;
    //#define Predec_width1     270.4
    static double aparam_Predec_width1;
    //#define Predec_height2    140.8
    static double aparam_Predec_height2;
    //#define Predec_width2     539.2
    static double aparam_Predec_width2;
    //#define Predec_height3    281.6
    static double aparam_Predec_height3;    
    //#define Predec_width3     584.0
    static double aparam_Predec_width3;
    //#define Predec_height4    281.6
    static double aparam_Predec_height4;  
    //#define Predec_width4     628.8
    static double aparam_Predec_width4; 
    //#define Predec_height5    422.4
    static double aparam_Predec_height5; 
    //#define Predec_width5     673.6
    static double aparam_Predec_width5;
    //#define Predec_height6    422.4
    static double aparam_Predec_height6;
    //#define Predec_width6     718.4
    static double aparam_Predec_width6;
    //#define Wwrite		  1.2
    static double aparam_Wwrite;
    //#define SenseampHeight    152.0
    static double aparam_SenseampHeight;
    //#define OutdriveHeight	  200.0
    static double aparam_OutdriveHeight;
    //#define FAOutdriveHeight  229.2
    static double aparam_FAOutdriveHeight;
    //#define FArowWidth	  382.8
    static double aparam_FArowWidth;
    //#define CAM2x2Height_1p	  48.8
    static double aparam_CAM2x2Height_1p;
    //#define CAM2x2Width_1p	  44.8
    static double aparam_CAM2x2Width_1p;
    //#define CAM2x2Height_2p   80.8 
    static double aparam_CAM2x2Height_2p;   
    //#define CAM2x2Width_2p    76.8
    static double aparam_CAM2x2Width_2p;
    //#define DatainvHeight     25.6
    static double aparam_DatainvHeight;
    //#define Wbitdropv 	  30.0
    static double aparam_Wbitdropv;
    //#define decNandWidth      34.4
    static double aparam_decNandWidth;
    //#define FArowNANDWidth    71.2
    static double aparam_FArowNANDWidth;
    //#define FArowNOR_INVWidth 28.0  
    static double aparam_FArowNOR_INVWidth;
    
    //#define FAHeightIncrPer_first_rw_or_w_port 16.0
    static double aparam_FAHeightIncrPer_first_rw_or_w_port;
    //#define FAHeightIncrPer_later_rw_or_w_port 16.0
    static double aparam_FAHeightIncrPer_later_rw_or_w_port;
    //#define FAHeightIncrPer_first_r_port       12.0
    static double aparam_FAHeightIncrPer_first_r_port;
    //#define FAHeightIncrPer_later_r_port       12.0
    static double aparam_FAHeightIncrPer_later_r_port;
    //#define FAWidthIncrPer_first_rw_or_w_port  16.0 
    static double aparam_FAWidthIncrPer_first_rw_or_w_port;
    //#define FAWidthIncrPer_later_rw_or_w_port  9.6
    static double aparam_FAWidthIncrPer_later_rw_or_w_port;
    //#define FAWidthIncrPer_first_r_port        12.0
    static double aparam_FAWidthIncrPer_first_r_port;
    //#define FAWidthIncrPer_later_r_port        9.6
    static double aparam_FAWidthIncrPer_later_r_port;
    
    
    static int aparam_pure_sram_flag; //Changed from static int to just int as value wasn't getting passed through to 
    //area function in area.c


    
};


#endif
