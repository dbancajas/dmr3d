
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Implementation file for power estimation for processor cores
 * initial author: Koushik Chakraborty
 *
 */ 

 

//  #include "simics/first.h"
// RCSID("$Id: core_power.cc,v 1.10.2.17 2006/03/02 23:58:42 kchak Exp $");

#include "definitions.h"
#include "config_extern.h" 
 

#include <math.h>
#include "isa.h"
#include "dynamic.h"
#include "fu.h"
#include "mai_instr.h"
#include "counter.h"
#include "stats.h"
#include "profiles.h"
#include "power_profile.h"
#include "mem_xaction.h"
#include "basic_circuit.h"
#include "sequencer.h"
#include "chip.h"
#include "power_obj.h"
#include "sparc_regfile.h"
#include "circuit_param.h"
#include "power_util.h"
#include "core_power.h"
#include "cacti_def.h"
#include "cacti_interface.h"
#include "cacti_time.h"
#include "verbose_level.h" 
#include "hotspot_interface.h"


// FIXME  tech node to supplied from somewhere else
 
core_power_t::core_power_t(string name, power_object_t *_p, sequencer_t *_seq)
    : power_profile_t (name), power_obj(_p), seq(_seq)
{
    bct = power_obj->get_basic_circuit();
    // FIXME: get alu count from sequencer
    power_util = new power_util_t(4, 4, bct);
    cacti_time = power_obj->get_cacti_time();
    res_ialu = 4;
    res_fpalu = 4;
    res_memport = 2; // FIX ME
    
    unit_activity_power = new core_power_result_type();
    hotspot_obj = new hotspot_t(this, seq->get_chip());
    turnoff_factor = 0.1; // This will be obtained from hotspot
    compute_device_width();
    init_regfiles();
    initialize_stats();
    initialize_calibration();
    default_turnoff_factor();
    calculate_power();
    clear_access_stats();
    dump_power_stats();
    do_dynamic_power_scaling();
    seq->get_chip()->register_power_entity(this);
}

void core_power_t::compute_temperature()
{
    hotspot_obj->evaluate_temperature();
        
}

void core_power_t::do_dynamic_power_scaling()
{
    
    double vdd_scaling = (double) (g_conf_vdd_scaling_factor) / 1000;
    double power_scaling = vdd_scaling * vdd_scaling;
     
      
    unit_activity_power->btb *= power_scaling;
    unit_activity_power->local_predict *= power_scaling;
    unit_activity_power->global_predict *= power_scaling;
    unit_activity_power->chooser *= power_scaling;
    unit_activity_power->ras *= power_scaling;
    unit_activity_power->rat_driver *= power_scaling;
    unit_activity_power->rat_decoder *= power_scaling;
    unit_activity_power->rat_wordline *= power_scaling;
    unit_activity_power->rat_bitline *= power_scaling;
    unit_activity_power->rat_senseamp *= power_scaling;
    unit_activity_power->dcl_compare *= power_scaling;
    unit_activity_power->dcl_pencode *= power_scaling;
    unit_activity_power->inst_decoder_power *= power_scaling;
    unit_activity_power->wakeup_tagdrive *= power_scaling;
    unit_activity_power->wakeup_tagmatch *= power_scaling;
    unit_activity_power->wakeup_ormatch *= power_scaling;
    unit_activity_power->lsq_wakeup_tagdrive *= power_scaling;
    unit_activity_power->lsq_wakeup_tagmatch *= power_scaling;
    unit_activity_power->lsq_wakeup_ormatch *= power_scaling;
    unit_activity_power->selection *= power_scaling;
    unit_activity_power->reorder_driver *= power_scaling;
    unit_activity_power->reorder_decoder *= power_scaling;
    unit_activity_power->reorder_wordline *= power_scaling;
    unit_activity_power->reorder_bitline *= power_scaling;
    unit_activity_power->reorder_senseamp *= power_scaling;
    unit_activity_power->rs_driver *= power_scaling;
    unit_activity_power->rs_decoder *= power_scaling;
    unit_activity_power->rs_wordline *= power_scaling;
    unit_activity_power->rs_bitline *= power_scaling;
    unit_activity_power->rs_senseamp *= power_scaling;
    unit_activity_power->lsq_rs_driver *= power_scaling;
    unit_activity_power->lsq_rs_decoder *= power_scaling;
    unit_activity_power->lsq_rs_wordline *= power_scaling;
    unit_activity_power->lsq_rs_bitline *= power_scaling;
    unit_activity_power->lsq_rs_senseamp *= power_scaling;
    unit_activity_power->resultbus *= power_scaling;
    unit_activity_power->total_power *= power_scaling;
    unit_activity_power->ialu_power *= power_scaling;
    unit_activity_power->falu_power *= power_scaling;
    unit_activity_power->bpred_power *= power_scaling;
    unit_activity_power->rename_power *= power_scaling;
    unit_activity_power->rat_power *= power_scaling;
    unit_activity_power->dcl_power *= power_scaling;
    unit_activity_power->window_power *= power_scaling;
    unit_activity_power->lsq_power *= power_scaling;
    unit_activity_power->wakeup_power *= power_scaling;
    unit_activity_power->lsq_wakeup_power *= power_scaling;
    unit_activity_power->rs_power *= power_scaling;
    unit_activity_power->rs_power_nobit *= power_scaling;
    unit_activity_power->lsq_rs_power *= power_scaling;
    unit_activity_power->lsq_rs_power_nobit *= power_scaling;
    unit_activity_power->selection_power *= power_scaling;
    unit_activity_power->result_power *= power_scaling;
    unit_activity_power->itlb_power *= power_scaling;
    unit_activity_power->dtlb_power *= power_scaling;
    unit_activity_power->clock_power *= power_scaling;
    
    for (uint32 i = 0; i < NUM_REGFILES; i++)
    {
        regfile[i]->regfile_driver *= power_scaling;
        regfile[i]->regfile_decoder *= power_scaling;
        regfile[i]->regfile_wordline *= power_scaling;
        regfile[i]->regfile_bitline *= power_scaling;
        regfile[i]->regfile_senseamp *= power_scaling;
        regfile[i]->regfile_power *= power_scaling;
        regfile[i]->regfile_power_nobit *= power_scaling;
    }
    
}

void core_power_t::initialize_calibration()
{
    // rename_power_calibration = 3.5;
    // bpred_power_calibration = 2.8;
    // window_power_calibration = 6.8;
    // lsq_power_calibration = 8.9;
    // regfile_power_calibration = 1.2;
    // alu_power_calibration = 0.17;
    // itlb_power_calibration = 9.8;
    // dtlb_power_calibration = 10.5;
    // clock_power_calibration = 0.0;
    
    //dynamic_power_factor = 1;
    
    rename_power_calibration = 1.0 ;
    bpred_power_calibration = 1.0 ;
    window_power_calibration = 0.125 ;
    
    lsq_power_calibration = 1.0 ;
    regfile_power_calibration = 1.8 ;
    alu_power_calibration = 0.17 ;
    itlb_power_calibration = 1.0 ;
    dtlb_power_calibration = 1.0 ;
    clock_power_calibration = 0.05 ;
    resultbus_power_calibration = 0;
    // Hotspot doesn't have anything particular to clock
    // Accounting for clocking power seems to be different for
    // Alpha and Intel: Intel account clock power only for 
    // global nodes, which gives only 8%. Clocking network
    // power inside each individual unit, is accounted within
    // that unit. This works better for interfacing with HotSpot,
    // so we assume similar.
    
    
}

void core_power_t::compute_device_width()
{
    
    circuit_param_t::cparam_Wdec3to8n	= (14.4 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Wdec3to8p	= (14.4 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_WdecNORn	= (5.4 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_WdecNORp	= (30.5 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Wdecinvn	= (5.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Wdecinvp	= (10.0  * 1/circuit_param_t::cparam_FUDGEFACTOR);
    
    //circuit_param_t::cparam_Wmemcellbscale	2		/* means 2x bigger than Wmemcella */
    
    
    
    circuit_param_t::cparam_Wcomppreequ     = (40.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_WmuxdrvNANDn    = (20.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_WmuxdrvNANDp    = (80.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_WmuxdrvNORn	= (60.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_WmuxdrvNORp	= (80.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Wmuxdrv3n	= (200.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Wmuxdrv3p	= (480.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Woutdrvseln	= (12.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Woutdrvselp	= (20.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Woutdrvnandn	= (24.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Woutdrvnandp	= (10.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Woutdrvnorn	= (6.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Woutdrvnorp	= (40.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Woutdrivern	= (48.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    circuit_param_t::cparam_Woutdriverp	= (80.0 * 1/circuit_param_t::cparam_FUDGEFACTOR);
    
    

    
    
    
}

void core_power_t::init_regfiles()
{
    
    total_num_regs = 0;
    regfile = new sparc_regfile_t *[NUM_REGFILES];
    for (uint32 i = 0; i < NUM_REGFILES; i++)
    {
        // FIXXXXXXXXMEEEEEEEEEE
        regfile[i] = new sparc_regfile_t(32, 64, string("INT_FP"));
        total_num_regs += 32;
    }
        
}


void core_power_t::pipe_stage_movement(pipe_stage_t prev_pstage, pipe_stage_t curr_pstage,
    dynamic_instr_t *d_instr)
{
    switch(curr_pstage) {
        
        case PIPE_RENAME:
            rename_access++;
            if (d_instr->get_initiate_fetch())
                itlb_access++;
            if (d_instr->is_load() || d_instr->is_store())
                lsq_access++;
            else if (d_instr->get_type() == IT_CONTROL)
                bpred_access++;
            break;
        case PIPE_EXECUTE:
            window_selection_access++;
            break;
        case PIPE_EXECUTE_DONE:
            if (d_instr->get_type() == IT_EXECUTE)
                update_datapath_access(d_instr);
            break;
        case PIPE_MEM_ACCESS_REEXEC:
            window_preg_access += 5;
            window_wakeup_access += 2;
            lsq_access++;
            dtlb_access++;
            if (d_instr->is_load()) {
                lsq_wakeup_access++;
                snoop_status_t s_st = d_instr->get_mem_transaction()->get_snoop_status(); 
                if (s_st == SNOOP_BYPASS_COMPLETE ||
                    s_st == SNOOP_BYPASS_EXTND)
                    lsq_load_data_access++;
            } else {
                lsq_store_data_access++;
                lsq_preg_access++;
            }
            break;
        default:
            /* do nothing */
            ;
    }    
}


void core_power_t::initialize_stats()
{
    total_rename_access = stats->COUNTER_BASIC("total_rename_access", " Total Rename Access");
    total_bpred_access= stats->COUNTER_BASIC("total_bpred_access", " Total Branch Predictor Access");
    total_window_access= stats->COUNTER_BASIC("total_window_access", "Total Window Access");
    total_lsq_access= stats->COUNTER_BASIC("total_lsq_access", "Total LSQ access");
    total_regfile_access= stats->COUNTER_BASIC("total_regfile_access", "Total Register File Access");
    total_alu_access= stats->COUNTER_BASIC("total_alu_access", "Total ALU access");
    total_resultbus_access= stats->COUNTER_BASIC("total_resultbus_access", "Total Result Bus access");
    total_dtlb_access = stats->COUNTER_BASIC("total_dtlb_access", "Total Access into DTLB");
    total_itlb_access = stats->COUNTER_BASIC("total_itlb_access", "Total Access into ITLB");
    
    rename_power = stats->COUNTER_DOUBLE("rename_power", "Rename power");
    bpred_power = stats->COUNTER_DOUBLE("bpred_power", "Branch Predictor power");
    window_power = stats->COUNTER_DOUBLE("window_power", "Window power");
    lsq_power = stats->COUNTER_DOUBLE("lsq_power", "LSQ power");
    regfile_power = stats->COUNTER_DOUBLE("regfile_power", "Power consumed in the register file");
    alu_power = stats->COUNTER_DOUBLE("alu_power", "Power consumed in the ALU");
    falu_power = stats->COUNTER_DOUBLE("falu_power", "Power consumed in the Float ALU");
    itlb_power = stats->COUNTER_DOUBLE("itlb_power", "Power consumed in the ITLB");
    dtlb_power = stats->COUNTER_DOUBLE("dtlb_power", "Power consumed i the DTLB");
    resultbus_power = stats->COUNTER_DOUBLE("resultbus_power", "Power consumed in the resultbus");
    clock_power = stats->COUNTER_DOUBLE("clock_power", "Power consumed in the clocking network");
    
    rename_power_cc1  = stats->COUNTER_DOUBLE("rename_power_cc1 ", " Power in rename_power_cc1 ");
    bpred_power_cc1  = stats->COUNTER_DOUBLE("bpred_power_cc1 ", " Power in bpred_power_cc1 ");
    window_power_cc1  = stats->COUNTER_DOUBLE("window_power_cc1 ", " Power in window_power_cc1 ");
    lsq_power_cc1  = stats->COUNTER_DOUBLE("lsq_power_cc1 ", " Power in lsq_power_cc1 ");
    regfile_power_cc1  = stats->COUNTER_DOUBLE("regfile_power_cc1 ", " Power in regfile_power_cc1 ");
    alu_power_cc1  = stats->COUNTER_DOUBLE("alu_power_cc1 ", " Power in alu_power_cc1 ");
    falu_power_cc1 = stats->COUNTER_DOUBLE("falu_power_cc1", "Power consumed in the Float ALU");
    itlb_power_cc1 = stats->COUNTER_DOUBLE("itlb_power_cc1", "Power consumed in the ITLB");
    dtlb_power_cc1 = stats->COUNTER_DOUBLE("dtlb_power_cc1", "Power consumed in the DTLB");
    resultbus_power_cc1  = stats->COUNTER_DOUBLE("resultbus_power_cc1 ", " Power in resultbus_power_cc1 ");
    clock_power_cc1 = stats->COUNTER_DOUBLE("clock_power_cc1", " Power in clock_power_cc1");
    
    
    rename_power_cc2  = stats->COUNTER_DOUBLE("rename_power_cc2 ", " Power in rename_power_cc2 ");
    bpred_power_cc2  = stats->COUNTER_DOUBLE("bpred_power_cc2 ", " Power in bpred_power_cc2 ");
    window_power_cc2  = stats->COUNTER_DOUBLE("window_power_cc2 ", " Power in window_power_cc2 ");
    lsq_power_cc2  = stats->COUNTER_DOUBLE("lsq_power_cc2 ", " Power in lsq_power_cc2 ");
    regfile_power_cc2  = stats->COUNTER_DOUBLE("regfile_power_cc2 ", " Power in regfile_power_cc2 ");
    alu_power_cc2  = stats->COUNTER_DOUBLE("alu_power_cc2 ", " Power in alu_power_cc2 ");
    falu_power_cc2 = stats->COUNTER_DOUBLE("falu_power_cc2", "Power consumed in the Float ALU");
    itlb_power_cc2 = stats->COUNTER_DOUBLE("itlb_power_cc2", "Power consumed in the ITLB");
    dtlb_power_cc2 = stats->COUNTER_DOUBLE("dtlb_power_cc2", "Power consumed i the DTLB");
    resultbus_power_cc2  = stats->COUNTER_DOUBLE("resultbus_power_cc2 ", " Power in resultbus_power_cc2 ");
    clock_power_cc2  = stats->COUNTER_DOUBLE("clock_power_cc2 ", " Power in clock_power_cc2 ");
    
    
    rename_power_cc3  = stats->COUNTER_DOUBLE("rename_power_cc3 ", " Power in rename_power_cc3 ");
    bpred_power_cc3  = stats->COUNTER_DOUBLE("bpred_power_cc3 ", " Power in bpred_power_cc3 ");
    window_power_cc3  = stats->COUNTER_DOUBLE("window_power_cc3 ", " Power in window_power_cc3 ");
    lsq_power_cc3  = stats->COUNTER_DOUBLE("lsq_power_cc3 ", " Power in lsq_power_cc3 ");
    regfile_power_cc3  = stats->COUNTER_DOUBLE("regfile_power_cc3 ", " Power in regfile_power_cc3 ");
    alu_power_cc3  = stats->COUNTER_DOUBLE("alu_power_cc3 ", " Power in alu_power_cc3 ");
    falu_power_cc3 = stats->COUNTER_DOUBLE("falu_power_cc3", "Power consumed in the Float ALU");
    itlb_power_cc3 = stats->COUNTER_DOUBLE("itlb_power_cc3", "Power consumed in the ITLB");
    dtlb_power_cc3 = stats->COUNTER_DOUBLE("dtlb_power_cc3", "Power consumed i the DTLB");
    resultbus_power_cc3  = stats->COUNTER_DOUBLE("resultbus_power_cc3 ", " Power in resultbus_power_cc3 ");
    clock_power_cc3  = stats->COUNTER_DOUBLE("clock_power_cc3 ", " Power in clock_power_cc3 ");
    total_cycle_power = stats->COUNTER_DOUBLE("total_cycle_power", " Power in total_cycle_power");
    total_cycle_power_cc1 = stats->COUNTER_DOUBLE("total_cycle_power_cc1", " Power in total_cycle_power_cc1");
    total_cycle_power_cc2 = stats->COUNTER_DOUBLE("total_cycle_power_cc2", " Power in total_cycle_power_cc2");
    total_cycle_power_cc3 = stats->COUNTER_DOUBLE("total_cycle_power_cc3", " Power in total_cycle_power_cc3");
    
    rename_power_leakage  = stats->COUNTER_DOUBLE("rename_power_leakage ", " Power in rename_power_leakage ");
    bpred_power_leakage  = stats->COUNTER_DOUBLE("bpred_power_leakage ", " Power in bpred_power_leakage ");
    window_power_leakage  = stats->COUNTER_DOUBLE("window_power_leakage ", " Power in window_power_leakage ");
    lsq_power_leakage  = stats->COUNTER_DOUBLE("lsq_power_leakage ", " Power in lsq_power_leakage ");
    regfile_power_leakage  = stats->COUNTER_DOUBLE("regfile_power_leakage ", " Power in regfile_power_leakage ");
    alu_power_leakage  = stats->COUNTER_DOUBLE("alu_power_leakage ", " Power in alu_power_leakage ");
    falu_power_leakage = stats->COUNTER_DOUBLE("falu_power_leakage", "Power consumed in the Float ALU");
    itlb_power_leakage = stats->COUNTER_DOUBLE("itlb_power_leakage", "Power consumed in the ITLB");
    dtlb_power_leakage = stats->COUNTER_DOUBLE("dtlb_power_leakage", "Power consumed i the DTLB");
    resultbus_power_leakage  = stats->COUNTER_DOUBLE("resultbus_power_leakage ", " Power in resultbus_power_leakage ");
    clock_power_leakage  = stats->COUNTER_DOUBLE("clock_power_leakage ", " Power in clock_power_leakage ");
    total_cycle_power_leakage = stats->COUNTER_DOUBLE("total_cycle_power_leakage", " Power in total_cycle_power leakage");
    
    
    last_single_total_cycle_power_cc1   = stats->COUNTER_DOUBLE("last_single_total_cycle_power_cc1  ", " Power in last_single_total_cycle_power_cc1  ");
    last_single_total_cycle_power_cc2   = stats->COUNTER_DOUBLE("last_single_total_cycle_power_cc2  ", " Power in last_single_total_cycle_power_cc2  ");
    last_single_total_cycle_power_cc3   = stats->COUNTER_DOUBLE("last_single_total_cycle_power_cc3  ", " Power in last_single_total_cycle_power_cc3  ");
    
    
    current_total_cycle_power_cc1 = stats->COUNTER_DOUBLE("current_total_cycle_power_cc1", " Power in current_total_cycle_power_cc1");
    current_total_cycle_power_cc2 = stats->COUNTER_DOUBLE("current_total_cycle_power_cc2", " Power in current_total_cycle_power_cc2");
    current_total_cycle_power_cc3 = stats->COUNTER_DOUBLE("current_total_cycle_power_cc3", " Power in current_total_cycle_power_cc3");
    
    max_cycle_power_cc1   = stats->COUNTER_DOUBLE("max_cycle_power_cc1  ", " Power in max_cycle_power_cc1  ");
    max_cycle_power_cc2   = stats->COUNTER_DOUBLE("max_cycle_power_cc2  ", " Power in max_cycle_power_cc2  ");
    max_cycle_power_cc3   = stats->COUNTER_DOUBLE("max_cycle_power_cc3  ", " Power in max_cycle_power_cc3  ");
    
    
    power_cycle_count    = stats->COUNTER_BASIC("power_cycle_count", "Cycle count for power purpose");
    
    stat_avg_rename_power = stats->STAT_RATIO("stat_avg_rename_power", "Average power due to rename", 
        rename_power, power_cycle_count);
    
    stat_avg_bpred_power = stats->STAT_RATIO("stat_avg_bpred_power", "Average power due to branch pred", 
        bpred_power, power_cycle_count);
    
    stat_avg_window_power = stats->STAT_RATIO("stat_avg_window_power", "Average power due to window", 
        window_power, power_cycle_count);
    stat_avg_lsq_power = stats->STAT_RATIO("stat_avg_lsq_power", "Average LSQ power",
        lsq_power, power_cycle_count);
    
    stat_avg_regfile_power = stats->STAT_RATIO("stat_avg_regfile_power", "Average power due to RF", 
        regfile_power, power_cycle_count);
    stat_avg_alu_power = stats->STAT_RATIO("stat_avg_alu_power", "avergae ALU power ",
        alu_power, power_cycle_count);
    stat_avg_resultbus_power = stats->STAT_RATIO("stat_avg_resultbus_power", "Average resultbus power",
        resultbus_power, power_cycle_count);
    
    stat_avg_clock_power = stats->STAT_RATIO("stat_avg_clock_power", "Average power due to clocking",
        clock_power, power_cycle_count);
    
    
    stat_avg_rename_power_cc1 = stats->STAT_RATIO("stat_avg_rename_power_cc1", "Average power due to rename", 
        rename_power_cc1, power_cycle_count);
    
    stat_avg_bpred_power_cc1 = stats->STAT_RATIO("stat_avg_bpred_power_cc1", "Average power due to branch pred", 
        bpred_power_cc1, power_cycle_count);
    
    stat_avg_window_power_cc1 = stats->STAT_RATIO("stat_avg_window_power_cc1", "Average power due to window", 
        window_power_cc1, power_cycle_count);
    stat_avg_lsq_power_cc1 = stats->STAT_RATIO("stat_avg_lsq_power_cc1", "Average power in LSQ", 
        lsq_power_cc1, power_cycle_count);
    
    stat_avg_regfile_power_cc1 = stats->STAT_RATIO("stat_avg_regfile_power_cc1", "Average power due to RF", 
        regfile_power_cc1, power_cycle_count);
    
    stat_avg_alu_power_cc1 = stats->STAT_RATIO("stat_avg_alu_power_cc1", "avergae ALU power CC1",
        alu_power_cc1, power_cycle_count);
    stat_avg_resultbus_power_cc1 = stats->STAT_RATIO("stat_avg_resultbus_power_cc1", "Average resultbus power",
        resultbus_power_cc1, power_cycle_count);
    
    stat_avg_clock_power_cc1 = stats->STAT_RATIO("stat_avg_clock_power_cc1", "Average power due to clock in CC1",
        clock_power_cc1, power_cycle_count);
    
    stat_avg_rename_power_cc3 = stats->STAT_RATIO("stat_avg_rename_power_cc3", "Average power due to rename", 
        rename_power_cc3, power_cycle_count);
    
    stat_avg_bpred_power_cc3 = stats->STAT_RATIO("stat_avg_bpred_power_cc3", "Average power due to branch pred", 
        bpred_power_cc3, power_cycle_count);
    
    stat_avg_window_power_cc3 = stats->STAT_RATIO("stat_avg_window_power_cc3", "Average power due to window", 
        window_power_cc3, power_cycle_count);
    stat_avg_lsq_power_cc3 = stats->STAT_RATIO("stat_avg_lsq_power_cc3", "Average power in LSQ", 
        lsq_power_cc3, power_cycle_count);
    
    
    stat_avg_regfile_power_cc3 = stats->STAT_RATIO("stat_avg_regfile_power_cc3", "Average power due to RF", 
        regfile_power_cc3, power_cycle_count);
    
    stat_avg_alu_power_cc3 = stats->STAT_RATIO("stat_avg_alu_power_cc3", "avergae ALU power CC3",
        alu_power_cc3, power_cycle_count);
    
    stat_avg_resultbus_power_cc3 = stats->STAT_RATIO("stat_avg_resultbus_power_cc3", "Average resultbus power",
        resultbus_power_cc3, power_cycle_count);
    
    stat_avg_clock_power_cc3 = stats->STAT_RATIO("stat_avg_clock_power_cc3", "Average power due to clock in CC3",
        clock_power_cc3, power_cycle_count);
    
    stat_avg_total_cycle_power = stats->STAT_RATIO("avg_total_cycle_power", "Average total power for every cycle",
        total_cycle_power, power_cycle_count);
    stat_avg_total_cycle_power = stats->STAT_RATIO("avg_total_cycle_power_cc1", "Average total power for every cycle CC1",
        total_cycle_power_cc1, power_cycle_count);
    stat_avg_total_cycle_power_cc2 = stats->STAT_RATIO("avg_total_cycle_power_cc2", "Average total power for every cycle CC2",
        total_cycle_power_cc2, power_cycle_count);
    stat_avg_total_cycle_power_cc3 = stats->STAT_RATIO("avg_total_cycle_power_cc3", "Average total power for every cycle CC3",
        total_cycle_power_cc3, power_cycle_count);
    
    
}




void core_power_t::clear_access_stats()
{
    
    rename_access=0;
    bpred_access=0;
    window_access=0;
    lsq_access=0;
    alu_access=0;
    ialu_access=0;
    falu_access=0;
    resultbus_access=0;
    itlb_access = 0;
    dtlb_access = 0;
    
    window_preg_access=0;
    window_selection_access=0;
    window_wakeup_access=0;
    lsq_store_data_access=0;
    lsq_load_data_access=0;
    lsq_wakeup_access=0;
    lsq_preg_access=0;
    
    max_rename_access = 0;
    max_bpred_access = 0;
    max_window_access = 0;
    max_lsq_access = 0;
    max_regfile_access = 0;
    max_icache_access = 0;
    max_dcache_access = 0;
    max_dcache2_access = 0;
    max_alu_access = 0;
    max_resultbus_access = 0;
    
    window_total_pop_count_cycle=0;
    window_num_pop_count_cycle=0;
    lsq_total_pop_count_cycle=0;
    lsq_num_pop_count_cycle=0;
    resultbus_total_pop_count_cycle=0;
    resultbus_num_pop_count_cycle=0;
    
    for (uint32 i = 0; i < NUM_REGFILES; i++)
    {
        regfile[i]->regfile_access = 0;
        regfile[i]->total_pop_count_cycle = 0;
        regfile[i]->num_pop_count_cycle = 0;
    }
}


void core_power_t::update_power_stats()
{
    double resultbus_af_b;
    int i;
    
    STAT_INC(power_cycle_count);
    
#ifdef DYNAMIC_AF
    resultbus_af_b = power_util->compute_af(resultbus_num_pop_count_cycle,resultbus_total_pop_count_cycle,g_conf_data_width);
    
    for (i = 0; i < NUM_REGFILES; i++)
    {
        regfile[i]->af = power_util->compute_af(regfile[i]->num_pop_count_cycle,
            regfile[i]->total_pop_count_cycle, regfile[i]->data_width);
    }
#endif
    
    rename_power->inc_total(unit_activity_power->rename_power);
    bpred_power->inc_total(unit_activity_power->bpred_power);
    window_power->inc_total(unit_activity_power->window_power);
    lsq_power->inc_total(unit_activity_power->lsq_power);
    
    alu_power->inc_total(unit_activity_power->ialu_power + unit_activity_power->falu_power);
    falu_power->inc_total(unit_activity_power->falu_power);
    resultbus_power->inc_total(unit_activity_power->resultbus);
    clock_power->inc_total(unit_activity_power->clock_power);
    
    window_access = window_preg_access + window_selection_access + window_wakeup_access;
    
    total_rename_access->inc_total(rename_access);
    total_bpred_access->inc_total(bpred_access);
    total_window_access->inc_total(window_access);
    total_lsq_access->inc_total(lsq_access);
    total_resultbus_access->inc_total(resultbus_access);
    total_alu_access->inc_total(alu_access);
    
    max_rename_access=MAX(rename_access,max_rename_access);
    max_bpred_access=MAX(bpred_access,max_bpred_access);
    max_window_access=MAX(window_access,max_window_access);
    max_lsq_access=MAX(lsq_access,max_lsq_access);
    max_alu_access=MAX(alu_access,max_alu_access);
    max_resultbus_access=MAX(resultbus_access,max_resultbus_access);
    
    STAT_SET(total_regfile_access, 0);
    max_regfile_access = 0;
    
    for (i = 0; i < NUM_REGFILES; i++)
    {
        regfile[i]->regfile_total_power += regfile[i]->regfile_power;
        regfile[i]->total_regfile_access += regfile[i]->regfile_access;
        regfile[i]->max_regfile_access = MAX(regfile[i]->regfile_access, regfile[i]->max_regfile_access);
        
        total_regfile_access->inc_total(regfile[i]->total_regfile_access);
        max_regfile_access = MAX(max_regfile_access, regfile[i]->max_regfile_access);
    }
    
    // Rename Power
    if(rename_access) {
        rename_power_cc1->inc_total(unit_activity_power->rename_power);
        rename_power_cc2->inc_total(((double)rename_access/(double)g_conf_max_fetch)*unit_activity_power->rename_power);
        rename_power_cc3->inc_total(((double)rename_access/(double)g_conf_max_fetch)*unit_activity_power->rename_power);
    }
    else 
        rename_power_cc3->inc_total(turnoff_factor*unit_activity_power->rename_power);
    rename_power_leakage->inc_total(rename_turnoff_factor * unit_activity_power->rename_power);
    
    
    // BPRED power
    if(bpred_access) {
            if(bpred_access <= 2)
                bpred_power_cc1->inc_total(unit_activity_power->bpred_power);
            else
                bpred_power_cc1->inc_total(((double)bpred_access/2.0) * unit_activity_power->bpred_power);
            bpred_power_cc2->inc_total(((double)bpred_access/2.0) * unit_activity_power->bpred_power);
            bpred_power_cc3->inc_total(((double)bpred_access/2.0) * unit_activity_power->bpred_power);
    }
    else
        bpred_power_cc3->inc_total(turnoff_factor*unit_activity_power->bpred_power);
    bpred_power_leakage->inc_total(bpred_turnoff_factor * unit_activity_power->bpred_power);
    
        
    update_window_power();
    
    update_lsq_power();
    
    update_regfile_power();
    
    update_tlb_power();
    
    // ALU POWER    
                        
    if(alu_access) {
        if(ialu_access)
            alu_power_cc1->inc_total(unit_activity_power->ialu_power);
        else
            alu_power_cc3->inc_total(turnoff_factor*unit_activity_power->ialu_power);
        if(falu_access)
            falu_power_cc1->inc_total(unit_activity_power->falu_power);
        else
            falu_power_cc3->inc_total(turnoff_factor*unit_activity_power->falu_power);
        
            alu_power_cc2->inc_total(((double)ialu_access/(double)res_ialu)*unit_activity_power->ialu_power);
            falu_power_cc2->inc_total(((double)falu_access/(double)res_fpalu)*unit_activity_power->falu_power);
            alu_power_cc3->inc_total(((double)ialu_access/(double)res_ialu)*unit_activity_power->ialu_power);
            falu_power_cc3->inc_total(((double)falu_access/(double)res_fpalu)*unit_activity_power->falu_power);
    }
    else {
        
        alu_power_cc3->inc_total(turnoff_factor*unit_activity_power->ialu_power );
        falu_power_cc3->inc_total(turnoff_factor * unit_activity_power->falu_power);
    }
    
    
    alu_power_leakage->inc_total (alu_turnoff_factor * unit_activity_power->ialu_power);
    falu_power_leakage->inc_total (falu_turnoff_factor * unit_activity_power->falu_power);
    
    
    
#ifdef STATIC_AF
    if(resultbus_access) {
        ASSERT(g_conf_max_issue);
        if(resultbus_access <= (uint32) g_conf_max_issue) {
            resultbus_power_cc1->inc_total(unit_activity_power->resultbus);
        }
        else {
            resultbus_power_cc1->inc_total(((double)resultbus_access/(double)g_conf_max_issue)*unit_activity_power->resultbus);
        }
        resultbus_power_cc2->inc_total(((double)resultbus_access/(double)g_conf_max_issue)*unit_activity_power->resultbus);
        resultbus_power_cc3->inc_total(((double)resultbus_access/(double)g_conf_max_issue)*unit_activity_power->resultbus);
    }
    else
        resultbus_power_cc3->inc_total(turnoff_factor*unit_activity_power->resultbus);
#else
    if(resultbus_access) {
        ASSERT(g_conf_max_issue);
        if(resultbus_access <= (uint32) g_conf_max_issue) {
            resultbus_power_cc1->inc_total(resultbus_af_b*unit_activity_power->resultbus);
        }
        else {
            resultbus_power_cc1->inc_total(((double)resultbus_access/(double)g_conf_max_issue)*resultbus_af_b*unit_activity_power->resultbus);
        }
        resultbus_power_cc2->inc_total(((double)resultbus_access/(double)g_conf_max_issue)*resultbus_af_b*unit_activity_power->resultbus);
        resultbus_power_cc3->inc_total(((double)resultbus_access/(double)g_conf_max_issue)*resultbus_af_b*unit_activity_power->resultbus);
    }
    else
        resultbus_power_cc3->inc_total(turnoff_factor*unit_activity_power->resultbus);
#endif
           
    
    total_cycle_power->set(rename_power->get_total() + bpred_power->get_total() + 
        window_power->get_total() + lsq_power->get_total() + regfile_power->get_total() + 
        alu_power->get_total() + resultbus_power->get_total());
    
    total_cycle_power_cc1->set(rename_power_cc1->get_total() +  bpred_power_cc1->get_total()
    +  window_power_cc1->get_total() +  lsq_power_cc1->get_total() +  regfile_power_cc1->get_total() 
     +  alu_power_cc1->get_total() +  resultbus_power_cc1->get_total());
    
    total_cycle_power_cc2->set(rename_power_cc2->get_total() +  bpred_power_cc2->get_total()
        + window_power_cc2->get_total() +  lsq_power_cc2->get_total() + 
         regfile_power_cc2->get_total() +   alu_power_cc2->get_total() + 
         resultbus_power_cc2->get_total());
    
    
    total_cycle_power_cc3->set(rename_power_cc3->get_total() + bpred_power_cc3->get_total() + 
    window_power_cc3->get_total() + lsq_power_cc3->get_total() + regfile_power_cc3->get_total() + 
    alu_power_cc3->get_total() + resultbus_power_cc3->get_total());
    
    total_cycle_power_leakage->set(rename_power_leakage->get_total() + bpred_power_leakage->get_total() + 
        window_power_leakage->get_total() + lsq_power_leakage->get_total() + regfile_power_leakage->get_total() + 
        alu_power_leakage->get_total() + falu_power_leakage->get_total()  );
    
    
    if (total_cycle_power->get_total()) {
        
        clock_power_cc1->inc_total(unit_activity_power->clock_power*(
            total_cycle_power_cc1->get_total()/total_cycle_power->get_total()));
        clock_power_cc2->inc_total(unit_activity_power->clock_power*(
            total_cycle_power_cc2->get_total()/total_cycle_power->get_total()));
        
        clock_power_cc3->inc_total(unit_activity_power->clock_power*(
            total_cycle_power_cc3->get_total()/total_cycle_power->get_total()));
        
    }
    
    total_cycle_power_cc1->inc_total(clock_power_cc1->get_total());
    total_cycle_power_cc2->inc_total(clock_power_cc2->get_total());
    total_cycle_power_cc3->inc_total(clock_power_cc3->get_total());
    
    
    current_total_cycle_power_cc1->set( total_cycle_power_cc1->get_total()
        - last_single_total_cycle_power_cc1->get_total());
    current_total_cycle_power_cc2->set( total_cycle_power_cc2->get_total()
        - last_single_total_cycle_power_cc2->get_total());
    current_total_cycle_power_cc3->set(  total_cycle_power_cc3->get_total()
        - last_single_total_cycle_power_cc3->get_total());
    
    max_cycle_power_cc1->set(MAX(max_cycle_power_cc1->get_total(), 
        current_total_cycle_power_cc1->get_total()));
    max_cycle_power_cc2->set(MAX( max_cycle_power_cc2->get_total(), 
        current_total_cycle_power_cc2->get_total()));
    max_cycle_power_cc3->set(MAX(max_cycle_power_cc3->get_total(), 
        current_total_cycle_power_cc3->get_total()));
    
    last_single_total_cycle_power_cc1->set(total_cycle_power_cc1->get_total());
    last_single_total_cycle_power_cc2->set(total_cycle_power_cc2->get_total());
    last_single_total_cycle_power_cc3->set(total_cycle_power_cc3->get_total());
    
}




void core_power_t::calculate_power()
{
    //double clockpower;
    //double predeclength, wordlinelength, bitlinelength;
    //int ndwl, ndbl, nspd, ntwl, ntbl, ntspd, c,b,a,cache, rowsb, colsb;
    
    
    /* these variables are needed to use Cacti to auto-size cache arrays 
    (for optimal delay) */
    //time_result_type time_result;
    //time_parameter_type time_parameters;
    
    /* used to autosize other structures, like bpred tables */
    //int scale_factor;
    
    /* loop index variable */
    int i;
    
    calculate_frontend_power();
    calculate_window_power();
    calculate_lsq_power();
    calculate_regfile_power();
    
    calculate_datapath_power();
    calculate_tlb_power();
    
    perform_crossover_scaling(/* FIXXMEEEEEE */ 1.2); // taken from DSS
    
    // Clock power
    unit_activity_power->clock_power = power_util->total_clockpower(.018);
    
    unit_activity_power->total_power = unit_activity_power->local_predict + unit_activity_power->global_predict + 
    unit_activity_power->chooser + unit_activity_power->btb +
    unit_activity_power->rat_decoder + unit_activity_power->rat_wordline + 
    unit_activity_power->rat_bitline + unit_activity_power->rat_senseamp + 
    unit_activity_power->dcl_compare + unit_activity_power->dcl_pencode + 
    unit_activity_power->inst_decoder_power +
    unit_activity_power->wakeup_tagdrive + unit_activity_power->wakeup_tagmatch + 
    unit_activity_power->selection +
    /* unit_activity_power->regfile_decoder + unit_activity_power->regfile_wordline + 
    unit_activity_power->regfile_bitline + unit_activity_power->regfile_senseamp +  */
    unit_activity_power->rs_decoder + unit_activity_power->rs_wordline +
    unit_activity_power->rs_bitline + unit_activity_power->rs_senseamp + 
    unit_activity_power->lsq_wakeup_tagdrive + unit_activity_power->lsq_wakeup_tagmatch +
    unit_activity_power->lsq_rs_decoder + unit_activity_power->lsq_rs_wordline +
    unit_activity_power->lsq_rs_bitline + unit_activity_power->lsq_rs_senseamp +
    unit_activity_power->resultbus +
    unit_activity_power->clock_power +
    unit_activity_power->itlb_power + 
    unit_activity_power->dtlb_power ; 
    
    for (i = 0; i < NUM_REGFILES; i++)
    {
        unit_activity_power->total_power += regfile[i]->regfile_decoder + regfile[i]->regfile_wordline +
        regfile[i]->regfile_bitline + regfile[i]->regfile_senseamp;
    }
    
    
    
    unit_activity_power->bpred_power = unit_activity_power->btb + unit_activity_power->local_predict + 
        unit_activity_power->global_predict + unit_activity_power->chooser + unit_activity_power->ras;
    
    unit_activity_power->rat_power = unit_activity_power->rat_decoder + 
    unit_activity_power->rat_wordline + unit_activity_power->rat_bitline + unit_activity_power->rat_senseamp;
    
    unit_activity_power->dcl_power = unit_activity_power->dcl_compare + unit_activity_power->dcl_pencode;
    
    unit_activity_power->rename_power = unit_activity_power->rat_power + 
    unit_activity_power->dcl_power + unit_activity_power->inst_decoder_power;
    
    unit_activity_power->wakeup_power = unit_activity_power->wakeup_tagdrive + unit_activity_power->wakeup_tagmatch + 
    unit_activity_power->wakeup_ormatch;
    
    unit_activity_power->rs_power = unit_activity_power->rs_decoder + 
    unit_activity_power->rs_wordline + unit_activity_power->rs_bitline + unit_activity_power->rs_senseamp;
    
    unit_activity_power->rs_power_nobit = unit_activity_power->rs_decoder + 
    unit_activity_power->rs_wordline + unit_activity_power->rs_senseamp;
    
    unit_activity_power->window_power = unit_activity_power->wakeup_power + unit_activity_power->rs_power + 
    unit_activity_power->selection;
    
    unit_activity_power->lsq_rs_power = unit_activity_power->lsq_rs_decoder + 
    unit_activity_power->lsq_rs_wordline + unit_activity_power->lsq_rs_bitline + 
    unit_activity_power->lsq_rs_senseamp;
    
    unit_activity_power->lsq_rs_power_nobit = unit_activity_power->lsq_rs_decoder + 
    unit_activity_power->lsq_rs_wordline + unit_activity_power->lsq_rs_senseamp;
    
    unit_activity_power->lsq_wakeup_power = unit_activity_power->lsq_wakeup_tagdrive + unit_activity_power->lsq_wakeup_tagmatch;
    
    unit_activity_power->lsq_power = unit_activity_power->lsq_wakeup_power + unit_activity_power->lsq_rs_power;
    
    for (i = 0; i < NUM_REGFILES; i++)
    {
        regfile[i]->regfile_power = regfile[i]->regfile_decoder + 
        regfile[i]->regfile_wordline + regfile[i]->regfile_bitline + 
        regfile[i]->regfile_senseamp;
        
        regfile[i]->regfile_power_nobit = regfile[i]->regfile_decoder + 
        regfile[i]->regfile_wordline + regfile[i]->regfile_senseamp;
    }
    
    calibrate_power_components();
    accumulate_all_power();
    //dump_power_stats(power);
    
}

void core_power_t::calibrate_power_components()
{
    unit_activity_power->rename_power *= rename_power_calibration;
    unit_activity_power->bpred_power *= bpred_power_calibration;
    unit_activity_power->window_power *= window_power_calibration;
    unit_activity_power->lsq_power *= lsq_power_calibration;
    unit_activity_power->ialu_power *= alu_power_calibration;
    unit_activity_power->itlb_power *= itlb_power_calibration;
    unit_activity_power->dtlb_power *= dtlb_power_calibration;
    unit_activity_power->falu_power *= falu_power_calibration;
    unit_activity_power->resultbus *= resultbus_power_calibration;
    unit_activity_power->clock_power *= clock_power_calibration;
    for (uint32 i = 0; i < NUM_REGFILES; i++)
    {
        regfile[i]->regfile_power *= regfile_power_calibration;
        regfile[i]->regfile_power_nobit *= regfile_power_calibration;
    }
    
    
}


void core_power_t::accumulate_all_power()
{
    unit_activity_power->total_power = unit_activity_power->rename_power +
        unit_activity_power->bpred_power + unit_activity_power->window_power + 
        unit_activity_power->lsq_power +  
        unit_activity_power->ialu_power + unit_activity_power->itlb_power +
        unit_activity_power->dtlb_power + unit_activity_power->falu_power + 
        unit_activity_power->resultbus + unit_activity_power->clock_power;

}


void core_power_t::calculate_frontend_power()
{
        
    double predeclength, wordlinelength, bitlinelength;
    int cache = 0;
    int ndwl, ndbl, nspd, ntwl, ntbl, ntspd, c,b,a, rowsb, colsb;
    
    int nvreg_width = (int)ceil(bct->logtwo((double)total_num_regs));
    
    int npreg_width = (int)ceil(bct->logtwo((double)g_conf_window_size));
    
    predeclength = total_num_regs * 
    (RatCellHeight + 3 * g_conf_max_fetch * WordlineSpacing);
    
    wordlinelength = npreg_width * 
    (RatCellWidth + 
        6 * g_conf_max_fetch * BitlineSpacing + 
        RatShiftRegWidth*RatNumShift);
    
    bitlinelength = total_num_regs * (RatCellHeight + 3 * g_conf_max_fetch * WordlineSpacing);
    
    
    VERBOSE_OUT(verb_t::power, "rat power stats\n");
    
    unit_activity_power->rat_decoder = power_util->array_decoder_power(total_num_regs,npreg_width,predeclength,2*g_conf_max_fetch,g_conf_max_fetch,cache);
    unit_activity_power->rat_wordline = power_util->array_wordline_power(total_num_regs,npreg_width,wordlinelength,2*g_conf_max_fetch,g_conf_max_fetch,cache);
    unit_activity_power->rat_bitline = power_util->array_bitline_power(total_num_regs,npreg_width,bitlinelength,2*g_conf_max_fetch,g_conf_max_fetch,cache);
    unit_activity_power->rat_senseamp = 0;
    
    unit_activity_power->dcl_compare = power_util->dcl_compare_power(nvreg_width);
    unit_activity_power->dcl_pencode = 0;
    unit_activity_power->inst_decoder_power = g_conf_max_fetch * power_util->simple_array_decoder_power(g_conf_opcode_length,1,1,1,cache);

    parameter_type *param = new parameter_type();
    result_type *cacti_result = new result_type();
    arearesult_type *cacti_area_result = new arearesult_type();
    area_type *cacti_ar_subbank = new area_type();
    
 
    // /* Load cache values into what cacti is expecting */
    // time_parameters.cache_size = btb_config[0] * (g_conf_addr_width/8) * btb_config[1]; /* C */
    // time_parameters.block_size = (g_conf_addr_width/8); /* B */
    // time_parameters.associativity = btb_config[1]; /* A */
    // time_parameters.number_of_sets = btb_config[0]; /* C/(B*A) */
    
    double Nsubbanks = 1;
    
    param->cache_size = (int) bct->pow2(g_conf_yags_choice_bits) * 
        (g_conf_addr_width/8) ; /* C */
    param->block_size = (g_conf_addr_width/8); /* B */
    param->data_associativity = 1; /* A */
    param->tag_associativity  = 1;
    param->number_of_sets = (int) bct->pow2(g_conf_yags_choice_bits); /* C/(B*A) */
    param->num_readwrite_ports = 1; // Number of cache-line fetch
    param->nr_bits_out = 64; // 64 bit jump address?
    
    power_obj->get_cacti_time()->calculate_time (cacti_result,
        cacti_area_result,cacti_ar_subbank,param,&Nsubbanks);
    
    ndwl=cacti_result->best_Ndwl;
    ndbl=cacti_result->best_Ndbl;
    nspd=cacti_result->best_Nspd;
    ntwl=cacti_result->best_Ntwl;
    ntbl=cacti_result->best_Ntbl;
    ntspd=cacti_result->best_Ntspd;
    c = param->cache_size;
    b = param->block_size;
    a = param->data_associativity; 
    
    cache=1;
    
    
    /* Figure out how many rows/cols there are now */
    rowsb = c/(b*a*ndbl*nspd);
    colsb = 8*b*a*nspd/ndwl;
    
     {
        VERBOSE_OUT(verb_t::power, "%d KB %d-way btb (%d-byte block size):\n",c,a,b);
        VERBOSE_OUT(verb_t::power, "ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
        VERBOSE_OUT(verb_t::power, "%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
    }
    
    predeclength = rowsb * (RegCellHeight + WordlineSpacing);
    wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
    bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);
    
    
        VERBOSE_OUT(verb_t::power, "btb power stats\n");
    unit_activity_power->btb = ndwl*ndbl*(power_util->array_decoder_power(rowsb,colsb,predeclength,1,1,cache)
        + power_util->array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache)
        + power_util->array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache)
        + power_util->senseamp_power(colsb));
    
    cache=1;
    
    
    //double scale_factor = power_util->squarify(twolev_config[0],twolev_config[2]);
    // predeclength = (twolev_config[0] / scale_factor)* (RegCellHeight + WordlineSpacing);
    // wordlinelength = twolev_config[2] * scale_factor *  (RegCellWidth + BitlineSpacing);
    // bitlinelength = (twolev_config[0] / scale_factor) * (RegCellHeight + WordlineSpacing);
    
    
    //     VERBOSE_OUT(verb_t::power, "local predict power stats\n");
    
    // power->local_predict = power_util->array_decoder_power(twolev_config[0]/scale_factor,twolev_config[2]*scale_factor,predeclength,1,1,cache) + power_util->array_wordline_power(twolev_config[0]/scale_factor,twolev_config[2]*scale_factor,wordlinelength,1,1,cache) + power_util->array_bitline_power(twolev_config[0]/scale_factor,twolev_config[2]*scale_factor,bitlinelength,1,1,cache) + power_util->senseamp_power(twolev_config[2]*scale_factor);
    
    // scale_factor = power_util->squarify(twolev_config[1],3);
    
    // predeclength = (twolev_config[1] / scale_factor)* (RegCellHeight + WordlineSpacing);
    // wordlinelength = 3 * scale_factor *  (RegCellWidth + BitlineSpacing);
    // bitlinelength = (twolev_config[1] / scale_factor) * (RegCellHeight + WordlineSpacing);
    
    
    
    //     VERBOSE_OUT(verb_t::power, "local predict power stats\n");
    // power->local_predict += power_util->array_decoder_power(twolev_config[1]/scale_factor,3*scale_factor,predeclength,1,1,cache) + power_util->array_wordline_power(twolev_config[1]/scale_factor,3*scale_factor,wordlinelength,1,1,cache) + power_util->array_bitline_power(twolev_config[1]/scale_factor,3*scale_factor,bitlinelength,1,1,cache) + power_util->senseamp_power(3*scale_factor);
    
    
    //     VERBOSE_OUT(verb_t::power, "bimod_config[0] == %d\n",bimod_config[0]);
    
    // scale_factor = power_util->squarify(bimod_config[0],2);
    
    // predeclength = bimod_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);
    // wordlinelength = 2*scale_factor *  (RegCellWidth + BitlineSpacing);
    // bitlinelength = bimod_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);
    
    
    
    //     VERBOSE_OUT(verb_t::power, "global predict power stats\n");
    // power->global_predict = power_util->array_decoder_power(bimod_config[0]/scale_factor,2*scale_factor,predeclength,1,1,cache) + power_util->array_wordline_power(bimod_config[0]/scale_factor,2*scale_factor,wordlinelength,1,1,cache) + power_util->array_bitline_power(bimod_config[0]/scale_factor,2*scale_factor,bitlinelength,1,1,cache) + power_util->senseamp_power(2*scale_factor);
    
    // scale_factor = power_util->squarify(comb_config[0],2);
    
    // predeclength = comb_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);
    // wordlinelength = 2*scale_factor *  (RegCellWidth + BitlineSpacing);
    // bitlinelength = comb_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);
    
    
    //     VERBOSE_OUT(verb_t::power, "chooser predict power stats\n");
    // power->chooser = power_util->array_decoder_power(comb_config[0]/scale_factor,2*scale_factor,predeclength,1,1,cache) + power_util->array_wordline_power(comb_config[0]/scale_factor,2*scale_factor,wordlinelength,1,1,cache) + power_util->array_bitline_power(comb_config[0]/scale_factor,2*scale_factor,bitlinelength,1,1,cache) + power_util->senseamp_power(2*scale_factor);
    
    
    VERBOSE_OUT(verb_t::power, "RAS predict power stats\n");
    unit_activity_power->ras = power_util->simple_array_power(g_conf_ras_table_bits,g_conf_addr_width,1,1,0);
    
       
    
}

void core_power_t::calculate_window_power()
{
     
    //int nvreg_width = (int)ceil(bct->logtwo((double)total_num_regs));
    int npreg_width = (int)ceil(bct->logtwo((double)g_conf_window_size));
    
    
    /* RAT has shadow bits stored in each cell, this makes the
    cell size larger than normal array structures, so we must
    compute it here */
    unit_activity_power->wakeup_tagdrive = power_util->cam_tagdrive(g_conf_window_size,npreg_width,g_conf_max_issue,g_conf_max_issue);
    unit_activity_power->wakeup_tagmatch = power_util->cam_tagmatch(g_conf_window_size,npreg_width,g_conf_max_issue,g_conf_max_issue);
    unit_activity_power->wakeup_ormatch =0; 
    
    unit_activity_power->selection = power_util->selection_power(g_conf_window_size);
    
        
    double predeclength, wordlinelength, bitlinelength;
    int cache = 0;
    
    predeclength = g_conf_window_size * (RegCellHeight + 3 * g_conf_max_issue * WordlineSpacing);
    
    wordlinelength = g_conf_data_width *  (RegCellWidth +6 * g_conf_max_issue * BitlineSpacing);
    
    bitlinelength = g_conf_window_size * (RegCellHeight + 3 * g_conf_max_issue * WordlineSpacing);
    
    
    VERBOSE_OUT(verb_t::power, "res station power stats\n");
    unit_activity_power->rs_decoder = power_util->array_decoder_power(g_conf_window_size,g_conf_data_width,predeclength,2*g_conf_max_issue,g_conf_max_issue,cache);
    unit_activity_power->rs_wordline = power_util->array_wordline_power(g_conf_window_size,g_conf_data_width,wordlinelength,2*g_conf_max_issue,g_conf_max_issue,cache);
    unit_activity_power->rs_bitline = power_util->array_bitline_power(g_conf_window_size,g_conf_data_width,bitlinelength,2*g_conf_max_issue,g_conf_max_issue,cache);
    /* no senseamps in reg file structures (only caches) */
    unit_activity_power->rs_senseamp =0;
    unit_activity_power->rs_power_nobit = unit_activity_power->rs_decoder + 
        unit_activity_power->rs_wordline  + unit_activity_power->rs_senseamp;
    
   unit_activity_power->rs_power = unit_activity_power->rs_decoder + 
    unit_activity_power->rs_wordline + unit_activity_power->rs_bitline + 
    unit_activity_power->rs_senseamp;
    
}

void core_power_t::perform_crossover_scaling(double crossover_scaling)
{
    
    unit_activity_power->rat_decoder *= crossover_scaling;
    unit_activity_power->rat_wordline *= crossover_scaling;
    unit_activity_power->rat_bitline *= crossover_scaling;
    
    unit_activity_power->dcl_compare *= crossover_scaling;
    unit_activity_power->dcl_pencode *= crossover_scaling;
    unit_activity_power->inst_decoder_power *= crossover_scaling;
    unit_activity_power->wakeup_tagdrive *= crossover_scaling;
    unit_activity_power->wakeup_tagmatch *= crossover_scaling;
    unit_activity_power->wakeup_ormatch *= crossover_scaling;
    
    unit_activity_power->selection *= crossover_scaling;
    
    for (uint32 i = 0; i < NUM_REGFILES; i++)
    {
        regfile[i]->regfile_decoder *= crossover_scaling;
        regfile[i]->regfile_wordline *= crossover_scaling;
        regfile[i]->regfile_bitline *= crossover_scaling;
        regfile[i]->regfile_senseamp *= crossover_scaling;
    }
    
    unit_activity_power->rs_decoder *= crossover_scaling;
    unit_activity_power->rs_wordline *= crossover_scaling;
    unit_activity_power->rs_bitline *= crossover_scaling;
    unit_activity_power->rs_senseamp *= crossover_scaling;
    
    unit_activity_power->lsq_wakeup_tagdrive *= crossover_scaling;
    unit_activity_power->lsq_wakeup_tagmatch *= crossover_scaling;
    
    unit_activity_power->lsq_rs_decoder *= crossover_scaling;
    unit_activity_power->lsq_rs_wordline *= crossover_scaling;
    unit_activity_power->lsq_rs_bitline *= crossover_scaling;
    unit_activity_power->lsq_rs_senseamp *= crossover_scaling;
    
    unit_activity_power->resultbus *= crossover_scaling;
    
    unit_activity_power->btb *= crossover_scaling;
    unit_activity_power->local_predict *= crossover_scaling;
    unit_activity_power->global_predict *= crossover_scaling;
    unit_activity_power->chooser *= crossover_scaling;
    
    unit_activity_power->clock_power *= crossover_scaling;
    
    
}

void core_power_t::calculate_lsq_power()
{
    
        
    double predeclength, wordlinelength, bitlinelength;
    int cache = 0;
    
    predeclength = g_conf_window_size * (RegCellHeight + 3 * g_conf_max_issue * WordlineSpacing);
    
    wordlinelength = g_conf_data_width * 
    (RegCellWidth + 
        6 * g_conf_max_issue * BitlineSpacing);
    
    bitlinelength = g_conf_window_size * (RegCellHeight + 3 * g_conf_max_issue * WordlineSpacing);
    VERBOSE_OUT(verb_t::power, "lsq station power stats\n");
    
    /* addresses go into lsq tag's */
    unit_activity_power->lsq_wakeup_tagdrive = power_util->cam_tagdrive(g_conf_lsq_load_size,g_conf_addr_width,res_memport,res_memport);
    unit_activity_power->lsq_wakeup_tagmatch = power_util->cam_tagmatch(g_conf_lsq_load_size,g_conf_addr_width,res_memport,res_memport);
    unit_activity_power->lsq_wakeup_ormatch =0; 
    
    predeclength = g_conf_window_size * (RegCellHeight + 3 * g_conf_max_issue * WordlineSpacing);
    wordlinelength = g_conf_data_width * 
    (RegCellWidth + 
        4 * res_memport * BitlineSpacing);
    
    bitlinelength = g_conf_window_size * (RegCellHeight + 4 * res_memport * WordlineSpacing);
    
    
    
    unit_activity_power->lsq_rs_decoder = power_util->array_decoder_power(g_conf_lsq_load_size,g_conf_data_width,predeclength,res_memport,res_memport,cache);
    unit_activity_power->lsq_rs_wordline = power_util->array_wordline_power(g_conf_lsq_load_size,g_conf_data_width,wordlinelength,res_memport,res_memport,cache);
    unit_activity_power->lsq_rs_bitline = power_util->array_bitline_power(g_conf_lsq_load_size,g_conf_data_width,bitlinelength,res_memport,res_memport,cache);
    unit_activity_power->lsq_rs_senseamp =0;

    
    
    
    /* addresses go into lsq tag's */
    unit_activity_power->lsq_wakeup_tagdrive = power_util->cam_tagdrive(g_conf_lsq_load_size,g_conf_addr_width,res_memport,res_memport);
    unit_activity_power->lsq_wakeup_tagmatch = power_util->cam_tagmatch(g_conf_lsq_load_size,g_conf_addr_width,res_memport,res_memport);
    unit_activity_power->lsq_wakeup_ormatch =0; 
    /* rs's hold data */
        
    unit_activity_power->lsq_rs_power = unit_activity_power->lsq_rs_decoder + 
    unit_activity_power->lsq_rs_wordline + unit_activity_power->lsq_rs_bitline + 
    unit_activity_power->lsq_rs_senseamp;
    
    unit_activity_power->lsq_rs_power_nobit = unit_activity_power->lsq_rs_decoder + 
    unit_activity_power->lsq_rs_wordline + unit_activity_power->lsq_rs_senseamp;
    
    unit_activity_power->lsq_wakeup_power = unit_activity_power->lsq_wakeup_tagdrive + unit_activity_power->lsq_wakeup_tagmatch;
    
    unit_activity_power->lsq_power = unit_activity_power->lsq_wakeup_power + unit_activity_power->lsq_rs_power;
    
}


void core_power_t::calculate_regfile_power()
{
    double predeclength, wordlinelength, bitlinelength;
    int cache = 0;
    
    for (uint32 i = 0; i < NUM_REGFILES; i++)
    {
        predeclength = regfile[i]->num_regs * (RegCellHeight + 3 * g_conf_max_issue * WordlineSpacing);
        wordlinelength = regfile[i]->data_width * (RegCellWidth + 6 * g_conf_max_issue * BitlineSpacing);
        bitlinelength = regfile[i]->num_regs * (RegCellHeight + 3 * g_conf_max_issue * WordlineSpacing);
        
        parameter_type *param = (parameter_type *) malloc(sizeof(parameter_type));
        
        double Nsubbanks = 1.00;
        
        param->cache_size = regfile[i]->num_regs * 8;
        param->block_size = 8;
        param->tag_associativity = 1;
        param->data_associativity = 1;
        param->number_of_sets = regfile[i]->num_regs ;
        param->num_write_ports = g_conf_max_issue ;
        param->num_readwrite_ports = 0;
        param->num_read_ports = g_conf_max_issue * 2;
        param->num_single_ended_read_ports = 0;
        param->fully_assoc = 0;
        param->fudgefactor = circuit_param_t::cparam_FUDGEFACTOR;
        param->tech_size = circuit_param_t::cparam_FEATURESIZE;
        param->VddPow = circuit_param_t::cparam_VddPow;
        param->sequential_access = 0;
        param->fast_access = 1;
        param->force_tag = 1;
        param->tag_size = 0;
        param->nr_bits_out = 64 ; // ???
        param->pure_sram = 1;
        
        cacti_time->calculate_time(regfile[i]->result, regfile[i]->area_result,
            regfile[i]->ar_subbank, param, &Nsubbanks);
        VERBOSE_OUT(verb_t::power, "Total dynamic Read Power at max. freq. (W):"
            "%g\n",regfile[i]->result->total_power_allbanks.readOp.dynamic/regfile[i]->result->cycle_time);    
        
        VERBOSE_OUT(verb_t::power, "regfile power stats for regfile %s\n", regfile[i]->shortname.c_str());
        
        regfile[i]->regfile_decoder = power_util->array_decoder_power(regfile[i]->num_regs,
            regfile[i]->data_width, predeclength, 2*g_conf_max_issue, g_conf_max_issue, cache);
        regfile[i]->regfile_wordline = power_util->array_wordline_power(regfile[i]->num_regs,
            regfile[i]->data_width, wordlinelength, 2*g_conf_max_issue, g_conf_max_issue, cache);
        regfile[i]->regfile_bitline = power_util->array_bitline_power(regfile[i]->num_regs,
            regfile[i]->data_width, bitlinelength, 2*g_conf_max_issue, g_conf_max_issue, cache);
        regfile[i]->regfile_senseamp = 0;
    }
    
    
  for (uint32 i = 0; i < NUM_REGFILES; i++)
  {
    regfile[i]->regfile_power = regfile[i]->regfile_decoder + 
      regfile[i]->regfile_wordline + regfile[i]->regfile_bitline + 
      regfile[i]->regfile_senseamp;

    regfile[i]->regfile_power_nobit = regfile[i]->regfile_decoder + 
      regfile[i]->regfile_wordline + regfile[i]->regfile_senseamp;
      
    regfile[i]->regfile_total_power = 0;
	regfile[i]->regfile_total_power_cc1 = 0;
	regfile[i]->regfile_total_power_cc2 = 0;
	regfile[i]->regfile_total_power_cc3 = 0;
  }
    
}


void core_power_t::calculate_tlb_power()
{
    
    parameter_type *param = new parameter_type();
    result_type *cacti_result = new result_type();
    arearesult_type *cacti_area_result = new arearesult_type();
    area_type *cacti_ar_subbank = new area_type();
    
    
    double Nsubbanks = 1;
    
    param->cache_size = (int) 128 * (g_conf_addr_width/8) ; /* C */
    param->block_size = (g_conf_addr_width/8); /* B */
    param->data_associativity = 2; /* A */
    param->tag_associativity  = 2;
    param->num_readwrite_ports = 2; // Number of cache-line fetch
    param->nr_bits_out = 48; // 64 bit jump address?
    
    power_obj->get_cacti_time()->calculate_time (cacti_result,
        cacti_area_result,cacti_ar_subbank,param,&Nsubbanks);
    
    
    // unit_activity_power->dtlb_power = ( cacti_result->bitline_power_tag.readOp.dynamic +
    //         cacti_result->wordline_power_tag.readOp.dynamic + 
    //         cacti_result->sense_amp_power_tag.readOp.dynamic +
    //         cacti_result->total_out_driver_power_data.readOp.dynamic / 2)  * 1e9;
    // unit_activity_power->itlb_power = unit_activity_power->dtlb_power;
    
    unit_activity_power->dtlb_power = cacti_result->total_power.readOp.dynamic;
    unit_activity_power->itlb_power = cacti_result->total_power.readOp.dynamic;
    
}

void core_power_t::calculate_datapath_power()
{
    
    /* FIXME: ALU power is a simple constant, it would be better
    to include bit AFs and have different numbers for different
    types of operations */
    if (g_conf_data_width == 32)
        unit_activity_power->ialu_power = res_ialu * I_ADD32 * power_util->get_powerfactor();
    else
        unit_activity_power->ialu_power = res_ialu * I_ADD * power_util->get_powerfactor();
    unit_activity_power->falu_power = res_fpalu * F_ADD * power_util->get_powerfactor();
    unit_activity_power->resultbus = power_util->compute_resultbus_power();
}

void core_power_t::update_tlb_power()
{
    if (itlb_access) {
        itlb_power_cc1->inc_total(unit_activity_power->itlb_power);
        itlb_power_cc2->inc_total((double) itlb_access * unit_activity_power->itlb_power);
        itlb_power_cc3->inc_total((double) itlb_access * unit_activity_power->itlb_power);        
    } else 
        itlb_power_cc3->inc_total(turnoff_factor * unit_activity_power->itlb_power);
    itlb_power_leakage->inc_total(itlb_turnoff_factor * unit_activity_power->itlb_power);
    
    if (dtlb_access) {
        if (dtlb_access <= res_memport)
            dtlb_power_cc1->inc_total(unit_activity_power->dtlb_power);
        else 
            dtlb_power_cc1->inc_total(((double) dtlb_access / (double) res_memport) * unit_activity_power->dtlb_power);
        dtlb_power_cc2->inc_total(((double) dtlb_access / (double) res_memport) * unit_activity_power->dtlb_power);
        dtlb_power_cc3->inc_total(((double) dtlb_access / (double) res_memport) * unit_activity_power->dtlb_power);        
    } else 
        dtlb_power_cc3->inc_total(turnoff_factor * unit_activity_power->dtlb_power);
    dtlb_power_leakage->inc_total(dtlb_turnoff_factor * unit_activity_power->dtlb_power);
    
    
}

void core_power_t::update_lsq_power()
{
       
    double lsq_af_b = power_util->compute_af(lsq_num_pop_count_cycle,lsq_total_pop_count_cycle,g_conf_data_width);
    
    if(lsq_wakeup_access) {
        if(lsq_wakeup_access <= res_memport)
            lsq_power_cc1->inc_total(unit_activity_power->lsq_wakeup_power);
        else
            lsq_power_cc1->inc_total(((double)lsq_wakeup_access/((double)res_memport))*unit_activity_power->lsq_wakeup_power);
        lsq_power_cc2->inc_total(((double)lsq_wakeup_access/((double)res_memport))*unit_activity_power->lsq_wakeup_power);
        lsq_power_cc3->inc_total(((double)lsq_wakeup_access/((double)res_memport))*unit_activity_power->lsq_wakeup_power);
    }
    else
        lsq_power_cc3->inc_total(turnoff_factor*unit_activity_power->lsq_wakeup_power);
    
#ifdef STATIC_AF
    if(lsq_preg_access) {
        if(lsq_preg_access <= res_memport)
            lsq_power_cc1+=unit_activity_power->lsq_rs_power;
        else
            lsq_power_cc1+=((double)lsq_preg_access/((double)res_memport))*unit_activity_power->lsq_rs_power;
        lsq_power_cc2+=((double)lsq_preg_access/((double)res_memport))*unit_activity_power->lsq_rs_power;
        lsq_power_cc3+=((double)lsq_preg_access/((double)res_memport))*unit_activity_power->lsq_rs_power;
    }
    else
        lsq_power_cc3+=turnoff_factor*unit_activity_power->lsq_rs_power;
#else
    if(lsq_preg_access) {
        if(lsq_preg_access <= res_memport)
            lsq_power_cc1->inc_total(unit_activity_power->lsq_rs_power_nobit + lsq_af_b*unit_activity_power->lsq_rs_bitline);
        else
            lsq_power_cc1->inc_total(((double)lsq_preg_access/((double)res_memport))*(unit_activity_power->lsq_rs_power_nobit + lsq_af_b*unit_activity_power->lsq_rs_bitline));
        lsq_power_cc2->inc_total(((double)lsq_preg_access/((double)res_memport))*(unit_activity_power->lsq_rs_power_nobit + lsq_af_b*unit_activity_power->lsq_rs_bitline));
        lsq_power_cc3->inc_total(((double)lsq_preg_access/((double)res_memport))*(unit_activity_power->lsq_rs_power_nobit + lsq_af_b*unit_activity_power->lsq_rs_bitline));
    }
    else
        lsq_power_cc3->inc_total(turnoff_factor*unit_activity_power->lsq_rs_power);
#endif
 
    // Leakage component
    lsq_power_leakage->inc_total(lsq_turnoff_factor * (unit_activity_power->lsq_rs_power +
        unit_activity_power->lsq_wakeup_power));
    
}

void core_power_t::update_window_power()
{
    double window_af_b = power_util->compute_af(window_num_pop_count_cycle,window_total_pop_count_cycle,g_conf_data_width);
    
    
#ifdef STATIC_AF
    if(window_preg_access) {
        if(window_preg_access <= 3*g_conf_max_issue)
            window_power_cc1+=unit_activity_power->rs_power;
        else
            window_power_cc1+=((double)window_preg_access/(3.0*(double)g_conf_max_issue))*unit_activity_power->rs_power;
        window_power_cc2+=((double)window_preg_access/(3.0*(double)g_conf_max_issue))*unit_activity_power->rs_power;
        window_power_cc3+=((double)window_preg_access/(3.0*(double)g_conf_max_issue))*unit_activity_power->rs_power;
    }
    else
        window_power_cc3+=turnoff_factor*unit_activity_power->rs_power;
#elif defined(DYNAMIC_AF)
    if(window_preg_access) {
        if(window_preg_access <= 3*(uint32)g_conf_max_issue)
            window_power_cc1->inc_total(unit_activity_power->rs_power_nobit + window_af_b*unit_activity_power->rs_bitline);
        else
            window_power_cc1->inc_total(((double)window_preg_access/(3.0*(double)g_conf_max_issue))*(unit_activity_power->rs_power_nobit + window_af_b*unit_activity_power->rs_bitline));
        window_power_cc2->inc_total(((double)window_preg_access/(3.0*(double)g_conf_max_issue))*(unit_activity_power->rs_power_nobit + window_af_b*unit_activity_power->rs_bitline));
        window_power_cc3->inc_total(((double)window_preg_access/(3.0*(double)g_conf_max_issue))*(unit_activity_power->rs_power_nobit + window_af_b*unit_activity_power->rs_bitline));
    }
    else
        window_power_cc3->inc_total(turnoff_factor*unit_activity_power->rs_power);
#else
    FAIL_MSG("no AF-style defined\n");
#endif
        
    if(window_selection_access) {
        if(window_selection_access <= (uint32) g_conf_max_issue)
            window_power_cc1->inc_total(unit_activity_power->selection);
        else
            window_power_cc1->inc_total(((double)window_selection_access/((double)g_conf_max_issue))*unit_activity_power->selection);
        window_power_cc2->inc_total(((double)window_selection_access/((double)g_conf_max_issue))*unit_activity_power->selection);
        window_power_cc3->inc_total(((double)window_selection_access/((double)g_conf_max_issue))*unit_activity_power->selection);
    }
    else
        window_power_cc3->inc_total(turnoff_factor*unit_activity_power->selection);
        
    if(window_wakeup_access) {
        if(window_wakeup_access <= (uint32)g_conf_max_issue)
            window_power_cc1->inc_total(unit_activity_power->wakeup_power);
        else
            window_power_cc1->inc_total(((double)window_wakeup_access/((double)g_conf_max_issue))*unit_activity_power->wakeup_power);
        window_power_cc2->inc_total(((double)window_wakeup_access/((double)g_conf_max_issue))*unit_activity_power->wakeup_power);
        window_power_cc3->inc_total(((double)window_wakeup_access/((double)g_conf_max_issue))*unit_activity_power->wakeup_power);
    }
    else
        window_power_cc3->inc_total(turnoff_factor*unit_activity_power->wakeup_power);
 
    // Account leakage
    window_power_leakage->inc_total(window_turnoff_factor * (unit_activity_power->wakeup_power +
        unit_activity_power->selection + unit_activity_power->rs_power));
    
}

void core_power_t::dump_power_stats()
{
    
  VERBOSE_OUT(verb_t::power," branch target buffer power (W): %g\n",unit_activity_power->btb);
  VERBOSE_OUT(verb_t::power," local predict power (W): %g\n",unit_activity_power->local_predict);
  VERBOSE_OUT(verb_t::power," global predict power (W): %g\n",unit_activity_power->global_predict);
  VERBOSE_OUT(verb_t::power," chooser power (W): %g\n",unit_activity_power->chooser);
  VERBOSE_OUT(verb_t::power," RAS power (W): %g\n",unit_activity_power->ras);
  //VERBOSE_OUT(verb_t::power,"Rename Logic Power Consumption: %g  (%.3g%%)\n",rename_power,100*rename_power/total_power);
  VERBOSE_OUT(verb_t::power," Instruction Decode Power (W): %g\n",unit_activity_power->inst_decoder_power);
  VERBOSE_OUT(verb_t::power," RAT decode_power (W): %g\n",unit_activity_power->rat_decoder);
  VERBOSE_OUT(verb_t::power," RAT wordline_power (W): %g\n",unit_activity_power->rat_wordline);
  VERBOSE_OUT(verb_t::power," RAT bitline_power (W): %g\n",unit_activity_power->rat_bitline);
  VERBOSE_OUT(verb_t::power," DCL Comparators (W): %g\n",unit_activity_power->dcl_compare);
  //VERBOSE_OUT(verb_t::power,"Instruction Window Power Consumption: %g  (%.3g%%)\n",window_power,100*window_power/total_power);
  VERBOSE_OUT(verb_t::power," tagdrive (W): %g\n",unit_activity_power->wakeup_tagdrive);
  VERBOSE_OUT(verb_t::power," tagmatch (W): %g\n",unit_activity_power->wakeup_tagmatch);
  VERBOSE_OUT(verb_t::power," Selection Logic (W): %g\n",unit_activity_power->selection);
  VERBOSE_OUT(verb_t::power," decode_power (W): %g\n",unit_activity_power->rs_decoder);
  VERBOSE_OUT(verb_t::power," wordline_power (W): %g\n",unit_activity_power->rs_wordline);
  VERBOSE_OUT(verb_t::power," bitline_power (W): %g\n",unit_activity_power->rs_bitline);
  //VERBOSE_OUT(verb_t::power,"Load/Store Queue Power Consumption: %g  (%.3g%%)\n",lsq_power,100*lsq_power/total_power);
  VERBOSE_OUT(verb_t::power," tagdrive (W): %g\n",unit_activity_power->lsq_wakeup_tagdrive);
  VERBOSE_OUT(verb_t::power," tagmatch (W): %g\n",unit_activity_power->lsq_wakeup_tagmatch);
  VERBOSE_OUT(verb_t::power," decode_power (W): %g\n",unit_activity_power->lsq_rs_decoder);
  VERBOSE_OUT(verb_t::power," wordline_power (W): %g\n",unit_activity_power->lsq_rs_wordline);
  VERBOSE_OUT(verb_t::power," bitline_power (W): %g\n",unit_activity_power->lsq_rs_bitline);

}


uint32 core_power_t::get_index_from_regnum(uint32 reg_num)
{
    ///// FIXMEEEEE
    return reg_num % NUM_REGFILES;
}

void core_power_t::access_regfile(uint32 reg_num, uint64 value)
{
	/* This function generates an access to a register file as well as a
	   population count. */
	
	/* Customize this to handle each register file in regfile[] */

    uint32 regfile_index = get_index_from_regnum(reg_num);
    regfile[regfile_index]->increment_access();
#ifdef DYNAMIC_AF    
    regfile[regfile_index]->total_pop_count_cycle += power_util->pop_count64(value);
    regfile[regfile_index]->num_pop_count_cycle++;
#endif    
    
    
}

void core_power_t::update_datapath_access(dynamic_instr_t *d_instr)
{
    mai_instruction_t *mai_instr = d_instr->get_mai_instruction();    
    int32 regs[96];
    bzero(regs, sizeof(int32) * 96);
    int32 num_srcs = d_instr->get_num_srcs();
    int32 num_dests = d_instr->get_num_dests();
    mai_instr->get_reg(regs, num_srcs, num_dests);
    
    // arbit ???
    window_preg_access += 4;
    
    
    for (int32 i = 0; i < num_srcs; i++)
    {
        int64 i_value = mai_instr->read_input_reg (regs[i]);
        access_regfile(regs[i], i_value);
        window_preg_access++;
    }
    for (int32 i = 0; i < num_dests; i++)
    {
        int64 o_value = mai_instr->read_output_reg (regs[RD + i]);
        access_regfile(regs[i], o_value);
        window_preg_access++;
        resultbus_access++;
        window_wakeup_access++;
        
    }
    
    // Make sure the access is marked ....
    fu_type_t fu_type = d_instr->get_fu()->get_id();
    if (fu_type >= FU_INTALU && fu_type <= FU_INTDIV)
    {
        ialu_access++;
    } else if (fu_type >= FU_FLOATADD && fu_type <= FU_FLOATSQRT) {
        falu_access++;
    } else if (fu_type != FU_NONE)
        FAIL;
        
    alu_access = ialu_access + falu_access;
    
}

void core_power_t::update_regfile_power()
{

#ifdef STATIC_AF
    for (uint32 i = 0; i < NUM_REGFILES; i++)
    {
        if(regfile[i]->regfile_access) {
            if(regfile[i]->regfile_access <= (3* (uint32) g_conf_max_commit))
                regfile[i]->regfile_total_power_cc1 +=  regfile[i]->regfile_power;
            else
                regfile[i]->regfile_total_power_cc1 += ((double)regfile[i]->regfile_access/(3.0*(double)g_conf_max_commit))*regfile[i]->regfile_power);
            
            regfile[i]->regfile_total_power_cc2 ->inc_total( ((double)regfile[i]->regfile_access/(3.0*(double)g_conf_max_commit))*regfile[i]->regfile_power);
            regfile[i]->regfile_total_power_cc3 ->inc_total( ((double)regfile[i]->regfile_access/(3.0*(double)g_conf_max_commit))*regfile[i]->regfile_power);
        }
        else
            regfile[i]->regfile_total_power_cc3 ->inc_total( turnoff_factor*regfile[i]->regfile_power);
    }
#else
    for (uint32 i = 0; i < NUM_REGFILES; i++)
    {
        if(regfile[i]->regfile_access) {
            if(regfile[i]->regfile_access <= (3* (uint32) g_conf_max_commit))
                regfile[i]->regfile_total_power_cc1 += regfile[i]->regfile_power_nobit + regfile[i]->af*regfile[i]->regfile_bitline;
            else
                regfile[i]->regfile_total_power_cc1 += ((double)regfile[i]->regfile_access/(3.0*(double)g_conf_max_commit))*(regfile[i]->regfile_power_nobit + regfile[i]->af*regfile[i]->regfile_bitline);
            
            regfile[i]->regfile_total_power_cc2 += ((double)regfile[i]->regfile_access/(3.0*(double)g_conf_max_commit))*(regfile[i]->regfile_power_nobit + regfile[i]->af*regfile[i]->regfile_bitline);
            regfile[i]->regfile_total_power_cc3 += ((double)regfile[i]->regfile_access/(3.0*(double)g_conf_max_commit))*(regfile[i]->regfile_power_nobit + regfile[i]->af*regfile[i]->regfile_bitline);
        }
        else
            regfile[i]->regfile_total_power_cc3 += turnoff_factor*regfile[i]->regfile_power;
    }
#endif

                 
/* These will be generated each cycle now.. */

    regfile_power->set(0);
    regfile_power_cc1->set(0);
    regfile_power_cc2->set(0);
    regfile_power_cc3->set(0);
    
    for (uint32 i = 0; i < NUM_REGFILES; i++)
    {
        /* add the power from each register file together to get the total regfile unit_activity_power-> */
        regfile_power->inc_total( regfile[i]->regfile_total_power);
        regfile_power_cc1->inc_total( regfile[i]->regfile_total_power_cc1);
        regfile_power_cc2->inc_total( regfile[i]->regfile_total_power_cc2);
        regfile_power_cc3->inc_total( regfile[i]->regfile_total_power_cc3);
        if (i == 0)
            regfile_power_leakage->inc_total (regfile[i]->regfile_power * intreg_turnoff_factor);
        else
            regfile_power_leakage->inc_total (regfile[i]->regfile_power * fpreg_turnoff_factor);
    }
    


}


void core_power_t::to_file(FILE *file)
{
    hotspot_obj->to_file(file);
    stats->to_file(file);
}

void core_power_t::from_file(FILE *file)
{
    hotspot_obj->from_file(file);
    stats->from_file(file);
}

void core_power_t::print_stats()
{
    hotspot_obj->print_stats();
    stats->print();
}

        

double core_power_t:: get_rename_power() {
    return rename_power_cc3->get_total();
}
 
double core_power_t:: get_bpred_power() {
    return bpred_power_cc3->get_total();
}

double core_power_t:: get_dcache_power(){
    return dcache_power_profile->get_current_power();
}
double core_power_t:: get_icache_power() {
    return icache_power_profile->get_current_power() ;
}

double core_power_t:: get_window_power(){
    //return window_power_cc3->get_total();
    return window_power_cc2->get_total() + window_power_leakage->get_total();
}

double core_power_t:: get_lsq_power() {
    //return lsq_power_cc3->get_total();
    return lsq_power_cc2->get_total() + lsq_power_leakage->get_total();
    
}

double core_power_t:: get_intreg_power() {
    //return regfile_power_cc3->get_total();
    return regfile_power_cc2->get_total() + regfile_power_leakage->get_total();
    
}

double core_power_t:: get_intalu_power(){
    //return alu_power_cc3->get_total();
    return alu_power_cc2->get_total() + alu_power_leakage->get_total() ;
    
}
 
double core_power_t:: get_fpalu_power() {
    // FIXME GET THE FPALU POWER
    return falu_power_cc3->get_total();
}


double core_power_t:: get_itlb_power() {
    return itlb_power_cc3->get_total();
    
}
 
double core_power_t:: get_dtlb_power() {
    return dtlb_power_cc3->get_total();
}

double core_power_t::get_fpmap_power() {
    // FIXME
    return 0.0; 
}

double core_power_t::get_fpreg_power() {
    // FIXME
    return 0.0;
}

double core_power_t::get_intmap_power() {
    //return rename_power_cc3->get_total();
    return rename_power_cc2->get_total() + rename_power_leakage->get_total();
}

double core_power_t::get_current_power() {
    return total_cycle_power_cc3->get_total();
}

double core_power_t::get_current_switching_power() {
    return total_cycle_power_cc2->get_total();
}

double core_power_t::get_current_leakage_power() {
    return total_cycle_power_leakage->get_total();
}

void core_power_t::set_icache_power_profile(power_profile_t *p)
{
    icache_power_profile = p;
}

void core_power_t::set_dcache_power_profile(power_profile_t *p)
{
    dcache_power_profile = p;
}

void core_power_t:: set_rename_turnoff_factor(double val) {
    rename_turnoff_factor = val;
}

void core_power_t:: set_bpred_turnoff_factor(double val) {
    bpred_turnoff_factor = val;
}
    
void core_power_t:: set_window_turnoff_factor(double val) {
    window_turnoff_factor = val;
    
}

void core_power_t:: set_lsq_turnoff_factor(double val) {
    lsq_turnoff_factor = val;
}

void core_power_t:: set_ialu_turnoff_factor(double val) {
    alu_turnoff_factor = val;
}

void core_power_t:: set_falu_turnoff_factor(double val) {
    falu_turnoff_factor = val;
}

void core_power_t:: set_itlb_turnoff_factor(double val) {
    itlb_turnoff_factor = val;
    
}

void core_power_t:: set_intreg_turnoff_factor(double val) {
    intreg_turnoff_factor = val;
    
}

void core_power_t:: set_fpreg_turnoff_factor(double val) { 
    fpreg_turnoff_factor= val; 

}

void core_power_t::default_turnoff_factor()
{
    float scaling_correction = (float) g_conf_leakage_cycle_adjust / 1000;
    rename_turnoff_factor= 0.05 * scaling_correction;
    bpred_turnoff_factor= 0.05 * scaling_correction;
    window_turnoff_factor= 0.05 * scaling_correction;
    lsq_turnoff_factor= 0.05 * scaling_correction;
    alu_turnoff_factor= 0.05 * scaling_correction;
    falu_turnoff_factor= 0.05 * scaling_correction;
    itlb_turnoff_factor= 0.05 * scaling_correction;
    dtlb_turnoff_factor = 0.05 * scaling_correction;
    intreg_turnoff_factor= 0.05 * scaling_correction;
    fpreg_turnoff_factor= 0.05 * scaling_correction;
    turnoff_factor *= scaling_correction;
    
    
}

void core_power_t::reset_turnoff_factor()
{
    rename_turnoff_factor= 0;
    bpred_turnoff_factor= 0;
    window_turnoff_factor= 0;
    lsq_turnoff_factor= 0;
    alu_turnoff_factor= 0;
    falu_turnoff_factor= 0;
    itlb_turnoff_factor= 0;
    dtlb_turnoff_factor = 0;
    intreg_turnoff_factor= 0;
    fpreg_turnoff_factor= 0;
}

void core_power_t::set_turnoff_factor(string unit, double factor)
{
    
    
    //turnoff_factor *= scaling_correction;
    // Some entries leakage factor are accumulative .. 
    
    
    if (unit == "Bpred") {
        bpred_turnoff_factor = factor;
    } else if (unit == "DTB") {
        dtlb_turnoff_factor = factor;
    } else if (unit == "FPAdd") {
        falu_turnoff_factor += factor;
    } else if (unit == "FPReg") {
        fpreg_turnoff_factor += factor;
    } else if (unit == "FPMul") {
        falu_turnoff_factor += factor;
    } else if (unit == "FPMap") {
        rename_turnoff_factor += factor;
    } else if (unit == "IntMap") {
        rename_turnoff_factor += factor;
    } else if (unit == "IntQ") {
        window_turnoff_factor += factor;
    } else if (unit == "IntReg") {
        intreg_turnoff_factor += factor;
    } else if (unit == "IntExec") {
        alu_turnoff_factor = factor;
    } else if (unit == "FPQ") {
        window_turnoff_factor += factor;
    } else if (unit == "LdStQ") {
        lsq_turnoff_factor = factor;
    } else if (unit == "ITB") {
        itlb_turnoff_factor = factor;
    }
    
}

uint32 core_power_t::get_sequencer_id()
{
    return seq->get_id();
}

