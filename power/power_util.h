
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    utility functions invoked by the power class
 * initial author: Koushik Chakraborty
 *
 */ 

#ifndef _POWER_UTILITY_H_
#define _POWER_UTILITY_H_
 
class power_util_t {
    
     private:
        double global_clockcap;
        
        uint32 int_alu_count;
        uint32 fp_alu_count;
        basic_circuit_t *bct;
        double Powerfactor;
        
        
       
        //double Powerfactor;

        
     
     public:
     
        
         power_util_t(uint32 _ia, uint32 _fa, basic_circuit_t *);
         double simple_array_power(int rows,int cols,int rports,int wports,int cache);
         double array_bitline_power(int rows,int cols,double bitlinelength,
             int rports,int wports,int cache);
         double array_wordline_power(int rows,int cols,double wordlinelength,
             int rports,int wports,int cache);
         double array_decoder_power(int rows,int cols,double predeclength,
             int rports,int wports,int cache);
         double simple_array_decoder_power(int rows,int cols,int rports,int wports,int cache);
         double simple_array_bitline_power(int rows,int cols,int rports,int wports,int cache);
         double simple_array_wordline_power(int rows,int cols,int rports,int wports,int cache);
         double squarify_new(int rows,int cols);
         double total_clockpower(double die_length);
         double global_clockpower(double die_length);
         
         double driver_size(double driving_cap, double desiredrisetime);
         double compute_af(uint64 num_pop_count_cycle,uint64 total_pop_count_cycle,
             int pop_width) ;
         
         double cam_tagmatch(int rows,int cols,int rports,int wports);
         double cam_tagdrive(int rows,int cols,int rports,int wports);
         double cam_array(int rows,int cols,int rports,int wports);
         double selection_power(int win_entries);
         double compute_resultbus_power();
         double dcl_compare_power(int compare_bits);
         double compare_cap(int compare_bits);
         
         double senseamp_power(int cols);
         int squarify(int rows, int cols);
         
         /* register power stats */
         void power_reg_stats(struct stat_sdb_t *sdb);/* stats database */
         void calculate_time(time_result_type*, time_parameter_type*);
         void output_data(FILE*, time_result_type*, time_parameter_type*);
         int pop_count_slow(uint64 bits);
         uint64 pop_count32(unsigned int bits);
         uint64 pop_count64(uint64 bits);
         double get_powerfactor() { return Powerfactor;}
     


};



#endif

