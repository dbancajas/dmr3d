/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Implementation file for power estimation for caches
 * initial author: Koushik Chakraborty
 *
 */ 
 


//  #include "simics/first.h"
// RCSID("$Id: cache_power.cc,v 1.10.2.17 2006/03/02 23:58:42 kchak Exp $");

#include "definitions.h"
#include "config_extern.h" 
 

#include <math.h>
#include "isa.h"
#include "dynamic.h"
#include "counter.h"
#include "stats.h"
#include "profiles.h"
#include "power_profile.h"
#include "basic_circuit.h"
#include "power_obj.h"
#include "device.h"
#include "cache.h"
#include "cacti_interface.h"
#include "cache_power.h"
#include "startup.h"
#include "chip.h"
#include "mem_hier.h"
#include "stats.h"
#include "circuit_param.h"
#include "area_param.h"
#include "cacti_def.h"
#include "transaction.h"
#include "sequencer.h"

#include "cacti_time.h"
#include "verbose_level.h" 

 
cache_power_t::cache_power_t(string name, cache_config_t *config, 
        power_object_t *_power_obj, bool tagarray)
    :  power_profile_t(name) , banks(config->banks), power_obj(_power_obj)
{
    
    // do something with the config
    
    initialize_stats();
    
    unit_activity_power = new cache_power_result_type();
    param = new parameter_type();
    cacti_result = new result_type();
    cacti_area_result = new arearesult_type();
    cacti_ar_subbank = new area_type();
    cpu_obj = 0;
    
    bzero(cacti_result, sizeof(result_type));
    bzero(cacti_area_result, sizeof(arearesult_type));
    bzero(cacti_ar_subbank, sizeof(area_type));
    
    // For large caches, simply provide a single bank
    // Cacti does not like non-power of two banks
    
    double Nsubbanks = (config->size > 512) ? 1 : (double)config->banks;
    
    param->cache_size = config->size * 1024 / config->banks;
    param->block_size = config->lsize;
    // CACTI can't handle more than 32 associative
    param->tag_associativity =  (config->assoc > 32 ? 32: config->assoc);
    param->data_associativity = (config->assoc > 32 ? 32: config->assoc);
    param->number_of_sets = (config->size * 1024) / (config->lsize * config->assoc * config->banks);
    param->num_write_ports = 0;
    param->num_readwrite_ports = (config->size <= 64) ? 2 : 1;
    param->num_read_ports = 0;
    param->num_single_ended_read_ports = 0;
    param->fully_assoc = 0;
    param->fudgefactor = circuit_param_t::cparam_FUDGEFACTOR;
    param->tech_size = circuit_param_t::cparam_FEATURESIZE;
    param->VddPow = circuit_param_t::cparam_VddPow;
    param->sequential_access = (config->size > 64) ? 1 : 0;
    param->fast_access = (config->size <= 64) ? 1 : 0;
    //param->fast_access = 0;
    param->force_tag = 0;
   // param->tag_size = 40;
    param->nr_bits_out =   config->bits_out;
    param->pure_sram = 0;
    
    float scaling_correction = (float) g_conf_leakage_cycle_adjust / 1000;
    leak_energy = scaling_correction * ( (double) config->bank_leak_energy / 1000) * (1E9 / (g_conf_chip_frequency * 1E6)) * config->banks;
    idle_leak_factor = g_conf_cache_idle_leak / 1000;
    
    
    //if (param->sequential_access)
    //    circuit_param_t::cparam_dualVt = 1;
    //else
        circuit_param_t::cparam_dualVt = 0;
    
    
    power_obj->get_cacti_time()->calculate_time (cacti_result,
        cacti_area_result,cacti_ar_subbank,param,&Nsubbanks);
    
    
    
    unit_activity_power->cache_power = cacti_result->total_power.readOp.dynamic * 0.67 +
        cacti_result->total_power.writeOp.dynamic * 0.33;
        

    if (tagarray) {
        read_hit_energy = ( cacti_result->bitline_power_tag.readOp.dynamic +
            cacti_result->wordline_power_tag.readOp.dynamic + 
            cacti_result->sense_amp_power_tag.readOp.dynamic +
            cacti_result->total_out_driver_power_data.readOp.dynamic / 2)  * 1e9;
        write_hit_energy = ( cacti_result->bitline_power_tag.writeOp.dynamic +
            cacti_result->wordline_power_tag.writeOp.dynamic + 
            cacti_result->sense_amp_power_tag.writeOp.dynamic +
            cacti_result->total_out_driver_power_data.writeOp.dynamic / 2)  * 1e9;
        read_miss_energy = read_hit_energy;
        write_miss_energy = write_hit_energy;
        
    } else {
        read_hit_energy = cacti_result->total_power.readOp.dynamic * 1e9;
        write_hit_energy = cacti_result->total_power.writeOp.dynamic * 1e9;
        // Modelling read_miss energy is nontrivial
        read_miss_energy = read_hit_energy;
        write_miss_energy = write_hit_energy;
    }
   
    calibrate_cache_power();
    stat_single_read_energy->set(read_hit_energy);
    
    convert_area(Nsubbanks);
    
    VERBOSE_OUT(verb_t::power, "[%s] size %u power %g (W) Total Read energy %g (nJ)\n"
        "Total leakage Read/Write Power all Banks (mW): %g Total Area (subbanked) %g leak_energy %g\n",  name.c_str(),
        config->size, cacti_result->total_power_allbanks.readOp.dynamic/cacti_result->cycle_time, 
        read_hit_energy, cacti_result->total_power_allbanks.readOp.leakage*1e3,
        cacti_area_result->subbankarea, leak_energy);
    
    
    cache_access = 0;
    mem_hier_t::ptr()->get_module_obj()->chip->register_power_entity(this);
    
    
}


void cache_power_t::calibrate_cache_power()
{
    read_hit_energy *= g_conf_cache_power_calibration;
    write_hit_energy *= g_conf_cache_power_calibration;
        // Modelling read_miss energy is nontrivial
    read_miss_energy *= g_conf_cache_power_calibration;
    write_miss_energy *= g_conf_cache_power_calibration;
    leak_energy *= g_conf_cache_power_calibration;
    
    double vdd_scaling = (double) (g_conf_vdd_scaling_factor) / 1000;
    double static_power_scaling = vdd_scaling;
    double dynamic_power_scaling = vdd_scaling * vdd_scaling;
    
    read_hit_energy *= dynamic_power_scaling;
    write_hit_energy *= dynamic_power_scaling;
    read_miss_energy *= dynamic_power_scaling;
    write_miss_energy *= dynamic_power_scaling;
    
    leak_energy *= static_power_scaling;
    
}
void cache_power_t::register_cpu_object(conf_object_t *cpu)
{
    cpu_obj = cpu;
    if (cpu_obj) cpu_seq = static_cast<sequencer_t*>((void *)cpu_obj);
}

void cache_power_t::convert_area(double NSubbanks)
{
    
    cacti_area_result->dataarray_area.scaled_area = cacti_area_result->dataarray_area.height * cacti_area_result->dataarray_area.width * CONVERT_TO_MMSQUARE;
    cacti_area_result->datapredecode_area.scaled_area = cacti_area_result->datapredecode_area.height * cacti_area_result->datapredecode_area.width * CONVERT_TO_MMSQUARE;
    cacti_area_result->datacolmuxpredecode_area.scaled_area = cacti_area_result->datacolmuxpredecode_area.height * cacti_area_result->datacolmuxpredecode_area.width * CONVERT_TO_MMSQUARE;
    cacti_area_result->datacolmuxpostdecode_area.scaled_area = cacti_area_result->datacolmuxpostdecode_area.height * cacti_area_result->datacolmuxpostdecode_area.width * CONVERT_TO_MMSQUARE;
    cacti_area_result->datawritesig_area.scaled_area = (param->num_readwrite_ports +
        param->num_read_ports+param->num_write_ports)* cacti_area_result->datawritesig_area.height * cacti_area_result->datawritesig_area.width * CONVERT_TO_MMSQUARE;
    
    cacti_area_result->tagarray_area.scaled_area = cacti_area_result->tagarray_area.height * cacti_area_result->tagarray_area.width * CONVERT_TO_MMSQUARE;
    cacti_area_result->tagpredecode_area.scaled_area = cacti_area_result->tagpredecode_area.height * cacti_area_result->tagpredecode_area.width * CONVERT_TO_MMSQUARE;
    cacti_area_result->tagcolmuxpredecode_area.scaled_area = cacti_area_result->tagcolmuxpredecode_area.height * cacti_area_result->tagcolmuxpredecode_area.width * CONVERT_TO_MMSQUARE;
    cacti_area_result->tagcolmuxpostdecode_area.scaled_area = cacti_area_result->tagcolmuxpostdecode_area.height* cacti_area_result->tagcolmuxpostdecode_area.width * CONVERT_TO_MMSQUARE;
    cacti_area_result->tagoutdrvdecode_area.scaled_area = cacti_area_result->tagoutdrvdecode_area.height * cacti_area_result->tagoutdrvdecode_area.width * CONVERT_TO_MMSQUARE;
    cacti_area_result->tagoutdrvsig_area.scaled_area = (param->num_readwrite_ports+param->num_read_ports+param->num_write_ports)*
    cacti_area_result->tagoutdrvsig_area.height * cacti_area_result->tagoutdrvsig_area.width * CONVERT_TO_MMSQUARE;
    
    cacti_area_result->perc_data = 100*area_param_t::aparam_area_all_dataramcells/(cacti_area_result->totalarea*CONVERT_TO_MMSQUARE);
    cacti_area_result->perc_tag  = 100*area_param_t::aparam_area_all_tagramcells/(cacti_area_result->totalarea*CONVERT_TO_MMSQUARE);
    cacti_area_result->perc_cont = 100*(cacti_area_result->totalarea*CONVERT_TO_MMSQUARE-
        area_param_t::aparam_area_all_dataramcells- area_param_t::aparam_area_all_tagramcells)/(cacti_area_result->totalarea*CONVERT_TO_MMSQUARE);
    cacti_area_result->sub_eff   = (area_param_t::aparam_area_all_dataramcells+
        area_param_t::aparam_area_all_tagramcells)*100/(cacti_area_result->totalarea/100000000.0);
    cacti_area_result->total_eff = (NSubbanks)*(area_param_t::aparam_area_all_dataramcells+
        area_param_t::aparam_area_all_tagramcells)*100/
    (cacti_ar_subbank->height * cacti_ar_subbank->width * CONVERT_TO_MMSQUARE);
    cacti_area_result->totalarea *= CONVERT_TO_MMSQUARE;
    cacti_area_result->subbankarea = cacti_ar_subbank->height * cacti_ar_subbank->width * CONVERT_TO_MMSQUARE; 
   
    
    
}

void cache_power_t::initialize_stats()
{
    stat_total_cache_access = stats->COUNTER_BASIC("cache_total_access", "Lookup function access");
    stat_total_cache_read_hits = stats->COUNTER_BASIC("cache_total_read_hits", "Lookup function access causing read hits");
    stat_total_cache_write_hits = stats->COUNTER_BASIC("cache_total_write_hits", "Lookup function access causing write hits");
    // stat_cache_power = stats->COUNTER_DOUBLE("cache_power", "No Clocking Cache Power");
    // stat_cache_power_cc1 = stats->COUNTER_DOUBLE("cache_power_cc1", " Cache Power with Clock Gating 1");
    // stat_cache_power_cc2 = stats->COUNTER_DOUBLE("cache_power_cc2", " Cache Power with Clock Gating 2");
    // stat_cache_power_cc3 = stats->COUNTER_DOUBLE("cache_power_cc3", " Cache Power with Clock Gating 3");
    
    // base_counter_t *st_cycle = mem_hier_t::ptr()->get_cycle_counter();
    
    // stat_avg_cache_power = stats->STAT_RATIO("cache_avg_power", "No Clocking Cache Power (avg)",
    //     stat_cache_power, st_cycle);
    // stat_avg_cache_power_cc1 = stats->STAT_RATIO("avg_cache_power_cc1", "Avg Cache Power with Clock Gating 1",
    //     stat_cache_power_cc1, st_cycle);
    // stat_avg_cache_power_cc2 = stats->STAT_RATIO("avg_cache_power_cc2", "Avg Cache Power with Clock Gating 2",
    //     stat_cache_power_cc2, st_cycle);
    // stat_avg_cache_power_cc3 = stats->STAT_RATIO("avg_cache_power_cc3", "Avg Cache Power with Clock Gating 3",
    //     stat_cache_power_cc3, st_cycle);
    
    stat_read_energy = stats->COUNTER_DOUBLE("cache_read_energy", "Read Energy ");
    stat_write_energy = stats->COUNTER_DOUBLE("cache_write_energy", "Write Energy");
    stat_single_read_energy = stats->COUNTER_DOUBLE("cache_single_read_energy", "Read Energy of a single operation");
    stat_leak_energy  = stats->COUNTER_DOUBLE("cache_leak_energy", "Leakage Energy");
}

void cache_power_t::lookup_access(mem_trans_t *trans, bool hit)
{
    STAT_INC(stat_total_cache_access);
    cache_access++;
    if (trans->read) {
        if (hit) {
            stat_read_energy->inc_total(read_hit_energy );
            STAT_INC(stat_total_cache_read_hits);
        }
        else stat_read_energy->inc_total(read_miss_energy );
    } else {
        if (hit) { 
            stat_write_energy->inc_total(write_hit_energy );
            STAT_INC(stat_total_cache_write_hits);
        }
        else stat_write_energy->inc_total(write_miss_energy );
    }
}

void cache_power_t::cycle_end()
{
    
    bool idle = false;
    if (cpu_obj) idle = cpu_seq->get_mai_object(0) ? false : true;
    cache_access = 0;
    stat_leak_energy->inc_total(idle ? leak_energy * idle_leak_factor : leak_energy);
    
    
}

void cache_power_t::to_file(FILE *file)
{
    fprintf(file, "%s\n", typeid(this).name());
    stats->to_file(file);
}

void cache_power_t::from_file(FILE *file)
{
    char classname[g_conf_max_classname_len];
	fscanf(file, "%s\n", classname);

    if (strcmp(classname, typeid(this).name()) != 0)
        DEBUG_OUT("Read %s   Require %s file_pos %d\n",classname, 
            typeid(this).name(), ftell(file));
	ASSERT(strcmp(classname, typeid(this).name()) == 0);
    stats->from_file(file);
    
}

double cache_power_t::get_current_power()
{
    return stat_read_energy->get_total() + stat_write_energy->get_total() +
    stat_leak_energy->get_total();
}

double cache_power_t::get_current_switching_power()
{
    return stat_read_energy->get_total() + stat_write_energy->get_total();
}

double cache_power_t::get_current_leakage_power()
{
    return stat_leak_energy->get_total();
}
