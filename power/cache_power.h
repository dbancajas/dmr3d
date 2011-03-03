
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Header file for power estimation for caches
 * initial author: Koushik Chakraborty
 *
 */ 

 
#ifndef _CACHE_POWER_H_
#define _CACHE_POWER_H_

#ifdef POWER_COMPILE


class cache_power_t : public power_profile_t {
    
    private:
    
        cache_power_result_type *unit_activity_power;
        
        // Cacti results
        
        result_type *cacti_result;
        arearesult_type *cacti_area_result;
        area_type *cacti_ar_subbank;
        parameter_type *param;
        
        
        
        double read_hit_energy;
        double write_hit_energy;
        double read_miss_energy;
        double leak_energy;
        double write_miss_energy;
        
        
        uint32 banks;
        uint32 cache_access;
        power_object_t *power_obj;
        conf_object_t *cpu_obj;
        sequencer_t *cpu_seq;
        double idle_leak_factor;
        // Stats
        
        base_counter_t * stat_total_cache_access;
        base_counter_t * stat_total_cache_write_hits;
        base_counter_t * stat_total_cache_read_hits;
        double_counter_t *stat_cache_power;
        double_counter_t *stat_cache_power_cc1;
        double_counter_t *stat_cache_power_cc2;
        double_counter_t *stat_cache_power_cc3;
    
    
        double_counter_t *stat_single_read_energy;
        double_counter_t *stat_read_energy;
        double_counter_t *stat_write_energy;
        double_counter_t *stat_leak_energy;
        double_counter_t *stat_overall_energy;
        
        double_counter_t *stat_leakage_energy;
        
        ratio_print_t *stat_avg_cache_power;
        ratio_print_t *stat_avg_cache_power_cc1;
        ratio_print_t *stat_avg_cache_power_cc2;
        ratio_print_t *stat_avg_cache_power_cc3;
        
        
    
        void initialize_stats();
        void cycle_end();
        void convert_area(double);
        void calibrate_cache_power();
    
    public:
        cache_power_t(string name, cache_config_t *config, power_object_t *, 
            bool tagarray);
        void lookup_access(mem_trans_t *trans, bool);
        void to_file(FILE *file);
        void from_file(FILE *file);
        double get_current_power();
        double get_current_switching_power();
        double get_current_leakage_power();
        
        void register_cpu_object(conf_object_t *cpu);
    
    
    
};

#endif


#endif

