/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 
 * Description: Cacti routines (old version)
 * Initial Author: Koushik Chakraborty
 */

 
#ifndef _CACTI_TIME_H
#define _CACTI_TIME_H


class cacti_time_t {
    private:
    
    basic_circuit_t *bct;
    
    
    
    double bitline_delay(int C, int A, int B, int Ndwl, int Ndbl,
        int Nspd,double inrisetime,double *outrisetime);
    double bitline_tag_delay(int C,int A,int B,int Ntwl,int Ntbl,int Ntspd,
        double inrisetime,double *outrisetime);
    double compare_time(int C,int A,int Ntbl,int Ntspd, 
        double inputtime,double *outputtime);
    double dataoutput_delay(int C, int B, int A, int Ndbl, int Nspd,
        int Ndwl, double inrisetime,double *outrisetime);
    double decoder_delay(int C, int B, int A, int Ndwl, int Ndbl, int Nspd, 
        int Ntwl, int Ntbl, int Ntspd, double *Tdecdrive,double *Tdecoder1,
        double *Tdecoder2,double *outrisetime);
    double decoder_tag_delay(int C, int B, int A, int Ndwl, int Ndbl,
        int Nspd, int Ntwl, int Ntbl, int Ntspd, double *Tdecdrive, 
        double *Tdecoder1, double *Tdecoder2,double *outrisetime);
    double mux_driver_delay(int C, int B,int A,int Ndbl,int Nspd,
        int Ndwl,int Ntbl,int Ntspd,double inputtime,double *outputtime);
    int organizational_parameters_valid(int rows,int cols,int Ndwl,
        int Ndbl,int Nspd,int Ntwl,int Ntbl,int Ntspd);
    double precharge_delay(double worddata);
    double selb_delay_tag_path(double inrisetime,double *outrisetime);
    double sense_amp_delay(double inrisetime,double *outrisetime);
    double sense_amp_tag_delay(double inrisetime,double *outrisetime);
    double valid_driver_delay(int C,int A,int Ntbl,int Ntspd,
        double inputtime);
    double wordline_delay(int B,int A,int Ndwl,int Nspd,
        double inrisetime, double *outrisetime);
    double wordline_tag_delay(int C,int A,int Ntspd,int Ntwl,
        double inrisetime,double *outrisetime);
        
        
        
        
        
        
    public:
    
        cacti_time_t();
        void calculate_time(time_result_type *result, 
            time_parameter_type *parameters);
        
        
};

#endif
