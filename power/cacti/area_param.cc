/* Copyright (c) 2005 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id $
 *
 * description: Definitions for area params 
 * initial author: Koushik Chakraborty 
 *
*/

//  #include "simics/first.h"
// RCSID("$Id: area_param.cc,v 1.1.2.13 2006/07/28 01:30:08 kchak Exp $");

#include "area_param.h"




double area_param_t::aparam_area_all_datasubarrays;
double area_param_t::aparam_area_all_tagsubarrays;
double area_param_t::aparam_area_all_dataramcells;
double area_param_t::aparam_area_all_tagramcells;
double area_param_t::aparam_faarea_all_subarrays ;

double area_param_t::aparam_aspect_ratio_data;
double area_param_t::aparam_aspect_ratio_tag;
double area_param_t::aparam_aspect_ratio_subbank;
double area_param_t::aparam_aspect_ratio_total;

/*
was: #define Widthptondiff 4.0
now: is 4* feature size, taken from the Intel 65nm process paper
*/

//v4.1: Making all constants static variables. Initially these variables are based
//off 0.8 micron process values; later on in init_tech_params function of leakage.c 
//they are scaled to input tech node parameters

//#define Widthptondiff 3.2
double area_param_t::aparam_Widthptondiff;
/* 
was: #define Widthtrack    3.2
now: 3.2*FEATURESIZE, i.e. 3.2*0.8
*/
//#define Widthtrack    (3.2*0.8)
double area_param_t::aparam_Widthtrack;
//#define Widthcontact  1.6
double area_param_t::aparam_Widthcontact;
//#define Wpoly         0.8
double area_param_t::aparam_Wpoly;
//#define ptocontact    0.4
double area_param_t::aparam_ptocontact;
//#define stitch_ramv   6.0 
double area_param_t::aparam_stitch_ramv;
//#define BitHeight16x2 33.6
//#define BitHeight1x1 (2*7.746*0.8) /* see below */
double area_param_t::aparam_BitHeight1x1;
//#define stitch_ramh   12.0
double area_param_t::aparam_stitch_ramh;
//#define BitWidth16x2  192.8
//#define BitWidth1x1	  (7.746*0.8) 
double area_param_t::aparam_BitWidth1x1;
/* dt: Assume that each 6-T SRAM cell is 120F^2 and has an aspect ratio of 1(width) to 2(height), than the width is 2*sqrt(60)*F */
//#define WidthNOR1     11.6
double area_param_t::aparam_WidthNOR1;
//#define WidthNOR2     13.6
double area_param_t::aparam_WidthNOR2;
//#define WidthNOR3     20.8
double area_param_t::aparam_WidthNOR3;
//#define WidthNOR4     28.8
double area_param_t::aparam_WidthNOR4;
//#define WidthNOR5     34.4
double area_param_t::aparam_WidthNOR5;
//#define WidthNOR6     41.6
double area_param_t::aparam_WidthNOR6;
//#define Predec_height1    140.8
double area_param_t::aparam_Predec_height1;
//#define Predec_width1     270.4
double area_param_t::aparam_Predec_width1;
//#define Predec_height2    140.8
double area_param_t::aparam_Predec_height2;
//#define Predec_width2     539.2
double area_param_t::aparam_Predec_width2;
//#define Predec_height3    281.6
double area_param_t::aparam_Predec_height3;    
//#define Predec_width3     584.0
double area_param_t::aparam_Predec_width3;
//#define Predec_height4    281.6
double area_param_t::aparam_Predec_height4;  
//#define Predec_width4     628.8
double area_param_t::aparam_Predec_width4; 
//#define Predec_height5    422.4
double area_param_t::aparam_Predec_height5; 
//#define Predec_width5     673.6
double area_param_t::aparam_Predec_width5;
//#define Predec_height6    422.4
double area_param_t::aparam_Predec_height6;
//#define Predec_width6     718.4
double area_param_t::aparam_Predec_width6;
//#define Wwrite		  1.2
double area_param_t::aparam_Wwrite;
//#define SenseampHeight    152.0
double area_param_t::aparam_SenseampHeight;
//#define OutdriveHeight	  200.0
double area_param_t::aparam_OutdriveHeight;
//#define FAOutdriveHeight  229.2
double area_param_t::aparam_FAOutdriveHeight;
//#define FArowWidth	  382.8
double area_param_t::aparam_FArowWidth;
//#define CAM2x2Height_1p	  48.8
double area_param_t::aparam_CAM2x2Height_1p;
//#define CAM2x2Width_1p	  44.8
double area_param_t::aparam_CAM2x2Width_1p;
//#define CAM2x2Height_2p   80.8 
double area_param_t::aparam_CAM2x2Height_2p;   
//#define CAM2x2Width_2p    76.8
double area_param_t::aparam_CAM2x2Width_2p;
//#define DatainvHeight     25.6
double area_param_t::aparam_DatainvHeight;
//#define Wbitdropv 	  30.0
double area_param_t::aparam_Wbitdropv;
//#define decNandWidth      34.4
double area_param_t::aparam_decNandWidth;
//#define FArowNANDWidth    71.2
double area_param_t::aparam_FArowNANDWidth;
//#define FArowNOR_INVWidth 28.0  
double area_param_t::aparam_FArowNOR_INVWidth;

//#define FAHeightIncrPer_first_rw_or_w_port 16.0
double area_param_t::aparam_FAHeightIncrPer_first_rw_or_w_port;
//#define FAHeightIncrPer_later_rw_or_w_port 16.0
double area_param_t::aparam_FAHeightIncrPer_later_rw_or_w_port;
//#define FAHeightIncrPer_first_r_port       12.0
double area_param_t::aparam_FAHeightIncrPer_first_r_port;
//#define FAHeightIncrPer_later_r_port       12.0
double area_param_t::aparam_FAHeightIncrPer_later_r_port;
//#define FAWidthIncrPer_first_rw_or_w_port  16.0 
double area_param_t::aparam_FAWidthIncrPer_first_rw_or_w_port;
//#define FAWidthIncrPer_later_rw_or_w_port  9.6
double area_param_t::aparam_FAWidthIncrPer_later_rw_or_w_port;
//#define FAWidthIncrPer_first_r_port        12.0
double area_param_t::aparam_FAWidthIncrPer_first_r_port;
//#define FAWidthIncrPer_later_r_port        9.6
double area_param_t::aparam_FAWidthIncrPer_later_r_port;
int area_param_t::aparam_pure_sram_flag;

