/* This file contains code which is part of the WATTCH power estimation
 * extenstions to SimpleScalar.  WATTCH is the work of David Brooks with
 * assistance from Vivek Tiwari and under the direction of Professor
 * Margaret Martonosi of Princeton University.
 *
 * WATTCH was adapted for use in dynamic SimpleScalar by James Dinan.
 */

/*------------------------------------------------------------
 *  Copyright 1994 Digital Equipment Corporation and Steve Wilton
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-
 * free right and license under any changes, enhancements or extensions
 * made to the core functions of the software, including but not limited to
 * those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to Digital any
 * such changes, enhancements or extensions that they make and inform Digital
 * of noteworthy uses of this software.  Correspondence should be provided
 * to Digital at:
 *
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/
 
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Header file for power estimation in ms2sim
 * initial author: Koushik Chakraborty
 *
 */ 

#ifndef _POWER_HEADER_H
#define _POWER_HEADER_H

class power_t : public profile_entry_t {
    protected:
    
    /* config */
    
        uint32 res_ialu;
        uint32 res_fpalu;
        uint32 res_memport;
        basic_circuit_t *bct;
        power_util_t *power_util;

    /* access */
    
        uint64 rename_access;
        uint64 bpred_access;
        uint64 window_access;
        uint64 lsq_access;
        uint64 icache_access;
        uint64 dcache_access;
        uint64 dcache2_access;
        uint64 alu_access;
        uint64 ialu_access;
        uint64 falu_access;
        uint64 resultbus_access;

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
        double_counter_t * icache_power ;
        double_counter_t * dcache_power ;
        double_counter_t * dcache2_power ;
        double_counter_t * alu_power ;
        double_counter_t * falu_power ;
        double_counter_t * resultbus_power ;
        double_counter_t * clock_power ;
        
        double_counter_t * rename_power_cc1 ;
        double_counter_t * bpred_power_cc1 ;
        double_counter_t * window_power_cc1 ;
        double_counter_t * lsq_power_cc1 ;
        double_counter_t * regfile_power_cc1 ;
        double_counter_t * icache_power_cc1 ;
        double_counter_t * dcache_power_cc1 ;
        double_counter_t * dcache2_power_cc1 ;
        double_counter_t * alu_power_cc1 ;
        double_counter_t * resultbus_power_cc1 ;
        double_counter_t * clock_power_cc1 ;
        
        double_counter_t * rename_power_cc2 ;
        double_counter_t * bpred_power_cc2 ;
        double_counter_t * window_power_cc2 ;
        double_counter_t * lsq_power_cc2 ;
        double_counter_t * regfile_power_cc2 ;
        double_counter_t * icache_power_cc2 ;
        double_counter_t * dcache_power_cc2 ;
        double_counter_t * dcache2_power_cc2 ;
        double_counter_t * alu_power_cc2 ;
        double_counter_t * resultbus_power_cc2 ;
        double_counter_t * clock_power_cc2 ;
        
        double_counter_t * rename_power_cc3 ;
        double_counter_t * bpred_power_cc3 ;
        double_counter_t * window_power_cc3 ;
        double_counter_t * lsq_power_cc3 ;
        double_counter_t * regfile_power_cc3 ;
        double_counter_t * icache_power_cc3 ;
        double_counter_t * dcache_power_cc3 ;
        double_counter_t * dcache2_power_cc3 ;
        double_counter_t * alu_power_cc3 ;
        double_counter_t * resultbus_power_cc3 ;
        double_counter_t * clock_power_cc3 ;
        
        double_counter_t * total_cycle_power;
        double_counter_t * total_cycle_power_cc1;
        double_counter_t * total_cycle_power_cc2;
        double_counter_t * total_cycle_power_cc3;
        
        double_counter_t * last_single_total_cycle_power_cc1  ;
        double_counter_t * last_single_total_cycle_power_cc2  ;
        double_counter_t * last_single_total_cycle_power_cc3  ;
        double_counter_t * current_total_cycle_power_cc1;
        double_counter_t * current_total_cycle_power_cc2;
        double_counter_t * current_total_cycle_power_cc3;
        
        double_counter_t * max_cycle_power_cc1  ;
        double_counter_t * max_cycle_power_cc2  ;
        double_counter_t * max_cycle_power_cc3  ;
        
        
    
    public:
    
        power_t(string name);
        void clear_access_stats();
        void update_power_stats();
        void pipe_stage_movement(pipe_stage_t prev_stage, pipe_stage_t curr_stage,
            dynamic_instr_t *d_inst);
        
        uint32 get_int_alu_count();
        uint32 get_fp_alu_count();
        void calculate_power(power_result_type *power);
        void memory_hierarchy_power();
        void branch_predictor_power();
    
};

#endif /* _POWER_H */
