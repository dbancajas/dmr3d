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

 
#ifndef _CACTI_TIME_H
#define _CACTI_TIME_H_


class cacti_time_t {
    
    
    private:
    
        leakage_t *leakage_obj;
        basic_circuit_t *bct;
        double Vt_bit_nmos_low;
        double Vthn;
        double Vt_bit_pmos_low;
        double Vthp; // params from leakage obj
        double Tkelvin;
        double Tox;
        double tech_length0;
        double Vt_cell_nmos_high, Vt_cell_pmos_high;
        
        
        int sequential_access_flag;
        int fast_cache_access_flag;
        
        
        double Coutdrvtreesegments[20], Routdrvtreesegments[20];
        double WoutdrvtreeN[20];
        int    nr_outdrvtreesegments;
        
        double Cmuxdrvtreesegments[20], Rmuxdrvtreesegments[20];
        double WmuxdrvtreeN[20];
        int    nr_muxdrvtreesegments;
        
        
        
        
       
        
       
        double VbitprePow;
        
    public:
    
    cacti_time_t(double tech, leakage_t *_leakage, basic_circuit_t *);
    
    void reset_tag_device_widths() ;
    void reset_data_device_widths();
    void compute_device_widths(int C,int B,int A,int fullyassoc, int Ndwl,int Ndbl,double Nspd);
    void compute_tag_device_widths(int C,int B,int A,int Ntspd,int Ntwl,int Ntbl,double NSubbanks);
    double cmos_ileakage(double nWidth, double pWidth,
        double nVthresh_dual, double nVthreshold, double pVthresh_dual, double pVthreshold) ;
    void reset_powerDef(powerDef *power) ;
    void mult_powerDef(powerDef *power, int val) ;
    void mac_powerDef(powerDef *sum,powerDef *mult, int val) ;
    void copy_powerDef(powerDef *dest, powerDef source) ;
    void copy_and_div_powerDef(powerDef *dest, powerDef source, double val) ;
    void add_powerDef(powerDef *sum, powerDef a, powerDef b) ;
    double objective_function(double delay_weight, double area_weight, double power_weight,
        double delay,double area,double power);
    double decoder_delay(int C, int B,int A,int Ndwl,int Ndbl,double Nspd,double NSubbanks,
        double *Tdecdrive,double *Tdecoder1,double *Tdecoder2,double inrisetime,
        double *outrisetime, int *nor_inputs,powerDef *power);
    double decoder_tag_delay(int C, int B,int A,int Ntwl,int Ntbl, int Ntspd,double NSubbanks,
        double *Tdecdrive, double *Tdecoder1, double *Tdecoder2,double inrisetime,
        double *outrisetime, int *nor_inputs,powerDef *power);
    void precalc_muxdrv_widths(int C,int B,int A,int Ndwl,int Ndbl,double Nspd,
        double * wirelength_v,double * wirelength_h);
    double senseext_driver_delay(int A,char fullyassoc,double inputtime,
        double *outputtime, double wirelength_sense_v,double wirelength_sense_h, powerDef *power);
    double half_compare_delay(int C,int B,int A,int Ntwl,int Ntbl,int Ntspd,
        double NSubbanks,double inputtime,double *outputtime,powerDef *power);
    void calc_wire_parameters(parameter_type *parameters) ;
    
    void subbanks_routing_power(char fullyassoc,int A,double NSubbanks,
						double *subbank_h,double *subbank_v,powerDef *power);
    
    void subbank_routing_length (int C,int B,int A,char fullyassoc,int Ndbl,
        double Nspd,int Ndwl,int Ntbl,int Ntwl,int Ntspd,double NSubbanks,
        double *subbank_v,double *subbank_h);
    
    double address_routing_delay (int C,int B,int A, char fullyassoc,int Ndwl,int Ndbl,
        double Nspd,int Ntwl,int Ntbl,int Ntspd, double *NSubbanks,
        double *outrisetime,powerDef *power);
    
    double fa_tag_delay (int C,int B,int Ntwl,int Ntbl,int Ntspd, double *Tagdrive,
        double *Tag1, double *Tag2, double *Tag3, double *Tag4, double *Tag5, 
        double *outrisetime,int *nor_inputs, powerDef *power);
    
    double wordline_delay (int C, int B,int A,int Ndwl, int Ndbl, double Nspd,
				double inrisetime,double *outrisetime,powerDef *power);
    double wordline_tag_delay (int C,int B,int A,int Ntspd,int Ntwl,int Ntbl,double NSubbanks,
        double inrisetime,double *outrisetime,powerDef *power);
    
    double bitline_delay (int C,int A,int B,int Ndwl,int Ndbl,double Nspd,
        double inrisetime, double *outrisetime,powerDef *power,double Tpre);
    
    double bitline_tag_delay (int C,int A,int B,int Ntwl,int Ntbl,int Ntspd,
        double NSubbanks,double inrisetime,double *outrisetime,powerDef *power,
        double Tpre);
    double sense_amp_delay (int C,int B,int A,int Ndwl,int Ndbl,
        double Nspd, double inrisetime,double *outrisetime, powerDef *power);
    
    double sense_amp_tag_delay (int C,int B,int A,int Ntwl,int Ntbl,int Ntspd,
        double NSubbanks,double inrisetime, double *outrisetime, powerDef *power);
    
    double mux_driver_delay (int Ntbl,int Ntspd,double inputtime, 
        double *outputtime,double wirelength);
    
    double compare_time (int C,int A,int Ntbl,int Ntspd,double NSubbanks,
        double inputtime,double *outputtime,powerDef *power);

    void calculate_time (result_type *result,arearesult_type *arearesult,
        area_type *arearesult_subbanked,parameter_type *parameters,double *NSubbanks);
    
    double mux_driver_delay_dualin (int C,int B,int A,int Ntbl,int Ntspd,double inputtime1,
        double *outputtime,double wirelength_v,double wirelength_h,powerDef *power);
    double total_out_driver_delay (int C,int B,int A,char fullyassoc,int Ndbl,double Nspd,int Ndwl,int Ntbl,int Ntwl,
        int Ntspd,double NSubbanks,double inputtime,double *outputtime,powerDef *power);
    double valid_driver_delay (int C,int B,int A,char fullyassoc,int Ndbl,int Ndwl,
        double Nspd,int Ntbl,int Ntwl,int Ntspd,double *NSubbanks,double inputtime,powerDef *power);
    double dataoutput_delay (int C,int B,int A,char fullyassoc,int Ndbl,double Nspd,int Ndwl,
        double inrisetime,double *outrisetime,powerDef *power);
    double selb_delay_tag_path (double inrisetime,double *outrisetime,powerDef *power);
    double precharge_delay (double worddata);
    
    void subbank_dim (int C,int B,int A,char fullyassoc,
			 int Ndbl,int Ndwl,double Nspd,int Ntbl,int Ntwl,int Ntspd,
			 double NSubbanks, double *subbank_h,double *subbank_v);
    
    
};



#endif

