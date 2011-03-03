
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

 /* description:    Header file: SPARC regfile for power estimation in ms2sim
 * initial author: Koushik Chakraborty
 *
 */ 
 

class sparc_regfile_t {
    
    public:
    
	/* Configurable Parameters */
	string shortname;
	uint32 data_width;
	uint32 num_regs;
	/* Per-Cycle Counters */
	counter_t regfile_access;
	counter_t total_pop_count_cycle;
	counter_t num_pop_count_cycle;
	/* Per-Cycle parameters */
	double af; /* Dynamic Activity Factor */
	/* Power parameters */
	double regfile_driver;
	double regfile_decoder;
	double regfile_wordline;
	double regfile_bitline;
	double regfile_senseamp;
	double regfile_power;
	double regfile_power_nobit;
	/* Overall Statistics */
	counter_t total_regfile_access;
	counter_t max_regfile_access;
	double regfile_total_power;
	double regfile_total_power_cc1;
	double regfile_total_power_cc2;
	double regfile_total_power_cc3;
    
    result_type *result;
    arearesult_type *area_result;
    area_type *ar_subbank;
    
    sparc_regfile_t(uint32, uint32, string );
    
    uint32 get_num_regs();
    void increment_access();
    
};
