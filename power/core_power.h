
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Header file for power estimation for processor cores
 * initial author: Koushik Chakraborty
 *
 */ 

 
 
#ifndef _CORE_POWER_H_
#define _CORE_POWER_H_

#ifdef POWER_COMPILE

#define NUM_REGFILES 2

class core_power_t : public power_profile_t {
    
    
    private:
    
        power_object_t *power_obj;    
        power_util_t *power_util;
        basic_circuit_t *bct;
        sequencer_t *seq;
        
        power_profile_t *icache_power_profile;
        power_profile_t *dcache_power_profile;

        core_power_result_type *unit_activity_power;
        sparc_regfile_t **regfile;
        cacti_time_t *cacti_time;
        hotspot_t  *hotspot_obj;
        // Clock Gating ... do we need it??
        double turnoff_factor; 
        
        // temperature dependent leakage factor for all units
        double rename_turnoff_factor;
        double bpred_turnoff_factor;
        double window_turnoff_factor;
        double lsq_turnoff_factor;
        double alu_turnoff_factor;
        double falu_turnoff_factor;
        double itlb_turnoff_factor;
        double dtlb_turnoff_factor;
        double intreg_turnoff_factor;
        double fpreg_turnoff_factor;
        
        
        
        /* config */
    
        uint32 total_num_regs;
        uint32 res_ialu;
        uint32 res_fpalu;
        uint32 res_memport;
        
    /* access */
    
        uint64 rename_access;
        uint64 bpred_access;
        uint64 window_access;
        uint64 lsq_access;
        uint64 alu_access;
        uint64 ialu_access;
        uint64 falu_access;
        uint64 resultbus_access;
        uint64 itlb_access;
        uint64 dtlb_access;

        uint64 window_selection_access;
        uint64 window_wakeup_access;
        uint64 window_preg_access;
        uint64 lsq_preg_access;
        uint64 lsq_wakeup_access;
        uint64 lsq_store_data_access;
        uint64 lsq_load_data_access;
        
        // Total Access
        
        base_counter_t *total_rename_access;
        base_counter_t * total_bpred_access;
        base_counter_t * total_window_access;
        base_counter_t * total_lsq_access;
        base_counter_t * total_regfile_access;
        base_counter_t * total_icache_access;
        base_counter_t * total_dcache_access;
        base_counter_t * total_dcache2_access;
        base_counter_t * total_alu_access;
        base_counter_t * total_resultbus_access;
        base_counter_t * total_dtlb_access;
        base_counter_t * total_itlb_access;
        
        uint64 max_rename_access;
        uint64 max_bpred_access;
        uint64 max_window_access;
        uint64 max_lsq_access;
        uint64 max_regfile_access;
        uint64 max_icache_access;
        uint64 max_dcache_access;
        uint64 max_dcache2_access;
        uint64 max_alu_access;
        uint64 max_resultbus_access;
        
        
        uint64 window_total_pop_count_cycle;
        uint64 window_num_pop_count_cycle;
        uint64 lsq_total_pop_count_cycle;
        uint64 lsq_num_pop_count_cycle;
        uint64 resultbus_total_pop_count_cycle;
        uint64 resultbus_num_pop_count_cycle;



    /* power stats */    
        double_counter_t * rename_power ;
        double_counter_t * bpred_power ;
        double_counter_t * window_power ;
        double_counter_t * lsq_power ;
        double_counter_t * regfile_power ;
        double_counter_t * alu_power ;
        double_counter_t * itlb_power;
        double_counter_t * dtlb_power;
        double_counter_t * falu_power ;
        double_counter_t * resultbus_power ;
        double_counter_t * clock_power ;
        
        double_counter_t * rename_power_cc1 ;
        double_counter_t * bpred_power_cc1 ;
        double_counter_t * window_power_cc1 ;
        double_counter_t * lsq_power_cc1 ;
        double_counter_t * regfile_power_cc1 ;
        double_counter_t * alu_power_cc1 ;
        double_counter_t * itlb_power_cc1;
        double_counter_t * dtlb_power_cc1;
        double_counter_t * falu_power_cc1;
        double_counter_t * resultbus_power_cc1 ;
        double_counter_t * clock_power_cc1 ;
        
        double_counter_t * rename_power_cc2 ;
        double_counter_t * bpred_power_cc2 ;
        double_counter_t * window_power_cc2 ;
        double_counter_t * lsq_power_cc2 ;
        double_counter_t * regfile_power_cc2 ;
        double_counter_t * alu_power_cc2 ;
        double_counter_t * itlb_power_cc2;
        double_counter_t * dtlb_power_cc2;
        double_counter_t * falu_power_cc2;
        double_counter_t * resultbus_power_cc2 ;
        double_counter_t * clock_power_cc2 ;
        
        double_counter_t * rename_power_cc3 ;
        double_counter_t * bpred_power_cc3 ;
        double_counter_t * window_power_cc3 ;
        double_counter_t * lsq_power_cc3 ;
        double_counter_t * regfile_power_cc3 ;
        double_counter_t * alu_power_cc3 ;
        double_counter_t * itlb_power_cc3;
        double_counter_t * dtlb_power_cc3;
        double_counter_t * falu_power_cc3;
        double_counter_t * resultbus_power_cc3 ;
        double_counter_t * clock_power_cc3 ;
        
        double_counter_t * rename_power_leakage ;
        double_counter_t * bpred_power_leakage ;
        double_counter_t * window_power_leakage ;
        double_counter_t * lsq_power_leakage ;
        double_counter_t * regfile_power_leakage ;
        double_counter_t * alu_power_leakage ;
        double_counter_t * itlb_power_leakage;
        double_counter_t * dtlb_power_leakage;
        double_counter_t * falu_power_leakage;
        double_counter_t * resultbus_power_leakage ;
        double_counter_t * clock_power_leakage ;
        
        double_counter_t * total_cycle_power;
        double_counter_t * total_cycle_power_cc1;
        double_counter_t * total_cycle_power_cc2;
        double_counter_t * total_cycle_power_cc3;
        double_counter_t * total_cycle_power_leakage;
        
        double_counter_t * last_single_total_cycle_power_cc1  ;
        double_counter_t * last_single_total_cycle_power_cc2  ;
        double_counter_t * last_single_total_cycle_power_cc3  ;
        double_counter_t * current_total_cycle_power_cc1;
        double_counter_t * current_total_cycle_power_cc2;
        double_counter_t * current_total_cycle_power_cc3;
        
        double_counter_t * max_cycle_power_cc1  ;
        double_counter_t * max_cycle_power_cc2  ;
        double_counter_t * max_cycle_power_cc3  ;
        
        
        base_counter_t *power_cycle_count;
        
        ratio_print_t  *stat_avg_rename_power;
        ratio_print_t  *stat_avg_bpred_power;
        ratio_print_t  *stat_avg_window_power;
        ratio_print_t  *stat_avg_lsq_power;
        ratio_print_t  *stat_avg_regfile_power;
        ratio_print_t  *stat_avg_alu_power;
        ratio_print_t  *stat_avg_resultbus_power;
        ratio_print_t  *stat_avg_clock_power;
        
        
        
        ratio_print_t  *stat_avg_rename_power_cc1;
        ratio_print_t  *stat_avg_bpred_power_cc1;
        ratio_print_t  *stat_avg_window_power_cc1;
        ratio_print_t  *stat_avg_lsq_power_cc1;
        ratio_print_t  *stat_avg_regfile_power_cc1;
        ratio_print_t  *stat_avg_alu_power_cc1;
        ratio_print_t  *stat_avg_resultbus_power_cc1;
        ratio_print_t  *stat_avg_clock_power_cc1;
        
        
        
        ratio_print_t  *stat_avg_rename_power_cc3;
        ratio_print_t  *stat_avg_bpred_power_cc3;
        ratio_print_t  *stat_avg_window_power_cc3;
        ratio_print_t  *stat_avg_lsq_power_cc3;
        ratio_print_t  *stat_avg_regfile_power_cc3;
        ratio_print_t  *stat_avg_alu_power_cc3;
        ratio_print_t  *stat_avg_resultbus_power_cc3;
        ratio_print_t  *stat_avg_clock_power_cc3;
        
        ratio_print_t *stat_avg_total_cycle_power;
        ratio_print_t *stat_avg_total_cycle_power_cc1;
        ratio_print_t *stat_avg_total_cycle_power_cc3;
        ratio_print_t *stat_avg_total_cycle_power_cc2;
        
        
        double rename_power_calibration ;
        double bpred_power_calibration ;
        double window_power_calibration ;
        double lsq_power_calibration ;
        double regfile_power_calibration ;
        double alu_power_calibration ;
        double itlb_power_calibration;
        double dtlb_power_calibration;
        double falu_power_calibration;
        double resultbus_power_calibration ;
        double clock_power_calibration ;
        
        void compute_device_width();
        void calculate_frontend_power();
        void calculate_window_power();
        void calculate_lsq_power();
        void calculate_regfile_power();
        void calculate_datapath_power();
        void calculate_tlb_power();
        void perform_crossover_scaling(double _scf);
        
        void update_lsq_power();
        void update_window_power();
        void update_regfile_power();
        void update_tlb_power();
        
        void init_regfiles();
        uint32 get_index_from_regnum(uint32 reg_num);
        void update_datapath_access(dynamic_instr_t *d_instr);
        void accumulate_all_power();
        void calibrate_power_components();
        void initialize_calibration();
        void default_turnoff_factor();
        void do_dynamic_power_scaling();
        
    public:
    
        core_power_t(string name, power_object_t *, sequencer_t *seq);
        void initialize_stats();
        void pipe_stage_movement(pipe_stage_t prev_pstage, pipe_stage_t curr_pstage,
            dynamic_instr_t *d_instr);
        
        void update_power_stats();
        void clear_access_stats();
        void calculate_power();
        void access_regfile(uint32 reg_num, uint64 value);
        void compute_temperature();
        
        void dump_power_stats();
        
        void to_file(FILE *file);
        void from_file(FILE *file);
        
        
        double get_rename_power();
        double get_bpred_power();
        double get_dcache_power();
        double get_icache_power();
        double get_window_power();
        double get_lsq_power();
        double get_intreg_power();
        double get_intalu_power();
        double get_fpalu_power();
        double get_itlb_power();
        double get_dtlb_power();
        double get_fpmap_power();
        double get_intmap_power();
        double get_fpreg_power();
        
        void set_turnoff_factor(string, double);
        void reset_turnoff_factor();
        
        void set_rename_turnoff_factor(double);
        void set_bpred_turnoff_factor(double);
        void set_window_turnoff_factor(double);
        void set_lsq_turnoff_factor(double);
        void set_ialu_turnoff_factor(double);
        void set_falu_turnoff_factor(double);
        void set_itlb_turnoff_factor(double);
        void set_intreg_turnoff_factor(double);
        void set_fpreg_turnoff_factor(double);
        
        double get_current_power();
        double get_current_switching_power();
        double get_current_leakage_power();
        
        
        void set_icache_power_profile(power_profile_t *);
        void set_dcache_power_profile(power_profile_t *);
        sequencer_t *get_seq() { return seq;}
        uint32 get_sequencer_id();
        void print_stats();
        
        
};

#endif


#endif

