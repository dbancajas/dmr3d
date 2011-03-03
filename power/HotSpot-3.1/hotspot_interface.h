



#ifndef _HOTSPOT_H_
#define _HOTSPOT_H_


class hotspot_t {
    
    
    private:
    
    
        
    
    
        flp_t *flp;
        RC_model_t *model;
        double *temp, *power;
        double *overall_power, *steady_temp;
        core_power_t *core_power;
        chip_t *p;
        stats_t *stats;
        FILE *p_file;
        
        
        base_counter_t   *stat_num_compute;
        double_counter_t *total_regfile_temp;
        double_counter_t *total_intexe_temp;
        double_counter_t *total_window_temp;
        ratio_print_t *avg_regfile_temp;
        ratio_print_t *avg_intexe_temp;
        ratio_print_t *avg_window_temp;
        double_counter_t *max_regfile_temp;
        double_counter_t *max_intexe_temp;
        double_counter_t *max_window_temp;
        double_counter_t *max_core_power;
        double_counter_t *min_core_power;
        
        double_counter_t *last_regfile_temp;
        double_counter_t *last_intexe_temp;
        double_counter_t *last_window_temp;
        
        double *leakage_factor;
        
        void initialize_stats();
        void update_stats();
        void emit_power_trace();
        void open_power_trace_file();
        void initialize_leakage_factor();
        void update_leakage_factors();
        double calculate_leakage_factor(double);
        
        string construct_floorplan_file();
        string construct_init_file();
        string construct_steady_file();
        
        
    public:
    
    
        hotspot_t(core_power_t *, chip_t *);
        void evaluate_temperature();
        void print_stats();
        void to_file(FILE *);
        void from_file(FILE *);
    
    
};




#endif
