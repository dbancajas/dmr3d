/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    Implementation file of the hotspot interface 
 * initial author: Koushik Chakraborty
 *
 */ 


//  #include "simics/first.h"
// RCSID("$Id: hotspot_interface.cc,v 1.1.2.30 2006/08/18 14:30:18 kchak Exp $");
#include "definitions.h"
#include "config_extern.h"
#include "chip.h"

#include <string.h>

#include "temperature.h"
#include "flp.h"
#include "util.h"
#include "profiles.h"
#include "power_profile.h"
#include "core_power.h"
#include "stats.h"
#include "counter.h"
#include "hotspot_interface.h"
#include "sequencer.h"
#include "verbose_level.h"


 
hotspot_t::hotspot_t(core_power_t *_core_power, chip_t *_p)
: core_power(_core_power), p(_p)
{
    
    if (!g_conf_temperature_update_interval) return;
    
    p_file = 0;
    
    string init_file = construct_init_file();
    flp = read_flp((char *)g_conf_floorplan_file.c_str(), FALSE);

	/* 
	 * configure thermal model parameters. default_thermal_config 
	 * returns a set of default parameters. only those configuration
	 * parameters (config.*) that need to be changed are set explicitly. 
	 */
	thermal_config_t config = default_thermal_config();
	strcpy(config.init_file, init_file.c_str());
	strcpy(config.steady_file, g_conf_hotspot_steady_file.c_str());

	/* allocate and initialize the RC model	*/
	model = alloc_RC_model(&config, flp);
	populate_R_model(model, flp);
	populate_C_model(model, flp);

    
    // Set the sampling interval based on frequency and temperature interval
    config.sampling_intvl = (1 << g_conf_temperature_update_interval) * Period;
	/* allocate the temp and power arrays	*/
	/* using hotspot_vector to internally allocate any extra nodes needed	*/
	temp = hotspot_vector(model);
	power = hotspot_vector(model);
	steady_temp = hotspot_vector(model);
	overall_power = hotspot_vector(model);
	
	/* set up initial instantaneous temperatures */
	if (strcmp(model->config->init_file, NULLFILE)) {
		if (!model->config->dtm_used)	/* initial T = steady T for no DTM	*/
			read_temp(model, temp, model->config->init_file, FALSE);
		else	/* initial T = clipped steady T with DTM	*/
			read_temp(model, temp, model->config->init_file, TRUE);
	}
	else	/* no input file - use init_temp as the common temperature	*/
		set_temp(model, temp, model->config->init_temp);
    
    initialize_stats();
    if (g_conf_power_trace) open_power_trace_file();
    initialize_leakage_factor();
}

string hotspot_t::construct_init_file()
{
    uint32 core_id = core_power->get_sequencer_id();
    char core_string[4];
    stringstream c_string;
    c_string << core_id;
    //sprintf(core_string, "%d", core_id);
    string ret = g_conf_hotspot_init_root + "/" + 
        string(getenv("BENCHMARK")) + "." + g_conf_hotspot_init_suffix + "." + 
        //string((const char *) core_string, strlen(core_string));
        c_string.str() ;
    return ret;
}

void hotspot_t::open_power_trace_file()
{
    string stat_out = string(getenv("OUTPUT_FILE"));
    string::size_type pos = stat_out.find("stats", 0);
    string ptrace_file = stat_out.substr(0, pos) + "ptrace";
    char ptrace_filename[100];
    sprintf(ptrace_filename, "%s%d", ptrace_file.c_str(), core_power->get_seq()->get_id());
    printf("Opening Power Trace File %s\n", ptrace_filename);
    p_file = fopen(ptrace_filename, "a");
    
    for (int32 i = 0; i < flp->n_units; i++) {
        fprintf(p_file, "%10s", flp->units[i].name);
    }
    fprintf(p_file, "\n");
    
}


void hotspot_t::initialize_stats()
{
    stats = new stats_t("hotspot_stats");
    
    stat_num_compute = stats->COUNTER_BASIC("stat_num_compute", "Number of HotSpot compute");
    total_regfile_temp = stats->COUNTER_DOUBLE("total_regfile_temp", "total regfile temp");
    total_intexe_temp = stats->COUNTER_DOUBLE("total_intexe_temp", "total intexe temp");
    total_window_temp = stats->COUNTER_DOUBLE("total_window_temp", "total window temp");
    
    total_regfile_temp->set_hidden();
    total_intexe_temp->set_hidden();
    total_window_temp->set_hidden();
    
    
    avg_regfile_temp = stats->STAT_RATIO("average_regfile_temp", "Average Temperature of Register File",
        total_regfile_temp, stat_num_compute);
    avg_intexe_temp = stats->STAT_RATIO("average_intexe_temp", "Average Temperature of ALU ",
        total_intexe_temp, stat_num_compute);
    avg_window_temp = stats->STAT_RATIO("average_window_temp", "Average Temperature of Instruction Window",
        total_window_temp, stat_num_compute);
    
    max_regfile_temp = stats->COUNTER_DOUBLE("max_regfile_temp", "Maximum Temperature of Register File");
    max_intexe_temp = stats->COUNTER_DOUBLE("max_intexe_temp", "Maximum Temperature of ALU ");
    max_window_temp = stats->COUNTER_DOUBLE("max_window_temp", "Maximum Temperature of Instruction Window");
    
    last_regfile_temp = stats->COUNTER_DOUBLE("last_regfile_temp", "Register File temp in the last compute cycle");
    last_intexe_temp = stats->COUNTER_DOUBLE("last_intexe_temp", "Integer ALU temperature in the last compute");
    last_window_temp = stats->COUNTER_DOUBLE("last_window_temp", "Window temperature in the last compue");
    max_core_power = stats->COUNTER_DOUBLE("max_core_power", "Maximum Core Power in a sampling interval");
    min_core_power = stats->COUNTER_DOUBLE("min_core_power", "Minimum Core Power in a sampling interval");
    

}

void hotspot_t::evaluate_temperature()
{
    
	int i;
    

    tick_t elapsed_time = (1 << g_conf_temperature_update_interval) ;
    
    /* set the per cycle power values as returned by Wattch/power simulator	*/
    power[get_blk_index(flp, "Icache")] =  core_power->get_icache_power();	
    power[get_blk_index(flp, "Dcache")] =  core_power->get_dcache_power();	
    power[get_blk_index(flp, "Bpred")] =  core_power->get_bpred_power();
    power[get_blk_index(flp, "LdStQ")] = core_power->get_lsq_power();
    power[get_blk_index(flp, "IntExec")] = core_power->get_intalu_power();
    power[get_blk_index(flp, "IntQ")] = core_power->get_window_power();
    // FIXME: combine FPAdd and FPMul ??
    power[get_blk_index(flp, "FPAdd")] =  2 * core_power->get_fpalu_power() / 3;
    power[get_blk_index(flp, "FPMul")] =  core_power->get_fpalu_power() / 3;
    power[get_blk_index(flp, "FPReg")] = core_power->get_fpreg_power();
    power[get_blk_index(flp, "IntMap")] = core_power->get_intmap_power();
    power[get_blk_index(flp, "IntReg")] = core_power->get_intreg_power();
    power[get_blk_index(flp, "FPMap")] = core_power->get_fpmap_power();
    power[get_blk_index(flp, "FPQ")] = 0;
    power[get_blk_index(flp, "ITB")] = core_power->get_itlb_power();
    power[get_blk_index(flp, "DTB")] = core_power->get_dtlb_power(); 
	
    /* ... more functional units ...	*/
    
    
    /* call compute_temp at regular intervals */
	// core_power_profile returns the total power for each call
    // To get the interval power, we need to deduct the overall_power
    // till this interval
    
    /* find the average power dissipated in the elapsed time */
    
    for (i = 0; i < flp->n_units; i++) {
        power[i] -= overall_power[i];
        overall_power[i] += power[i];
        //power[i] = 0;
        /* 
        * 'power' array is an aggregate of per cycle numbers over 
        * the sampling_intvl. so, compute the average power 
        */
        // power[i] *= (elapsed_time * model->config->base_proc_freq);
        power[i] *= 1.0E-9 / (model->config->sampling_intvl);
    }
    
    /* calculate the current temp given the previous temp,
    * time elapsed since then, and the average power dissipated during 
    * that interval */
    compute_temp(model, power, temp, elapsed_time / model->config->base_proc_freq);
    if (g_conf_power_trace) emit_power_trace();
    
    double total_power = 0;
    for (i = 0; i < flp->n_units; i++) {
    //    VERBOSE_OUT(verb_t::power, "%s T: %2.4f P: %2.5f\n", flp->units[i].name, temp[i], power[i]);
        total_power += power[i];
    }
    
    
    
    //VERBOSE_OUT(verb_t::power, "Total Power %2.5f\n", total_power);
    
    if (total_power > max_core_power->get_total())
        max_core_power->set(total_power);
    if (total_power < min_core_power->get_total())
        min_core_power->set(total_power);
    
    // At this point i will have the temperature, and therefore
    // using the area and empeircal formula for leakage power density
    // we can provide the leakage power consumed by each functional
    // block in the circuit.
    
    update_leakage_factors();
    
    update_stats();
    
}

void hotspot_t::emit_power_trace()
{
    for (int32 i = 0; i < flp->n_units; i++) 
        fprintf(p_file, "    %2.4f", power[i]);
    fprintf(p_file, "\n");
    
}

void hotspot_t::update_leakage_factors()
{
    double leak_f;
    float scaling_correction = (float) g_conf_leakage_cycle_adjust / 1000;
    double vdd_scaling = (double) (g_conf_vdd_scaling_factor) / 1000;
    double static_power_scaling = 1/vdd_scaling; // To avoid duplicating the effect from dynamic power
    // Here making the assumption that the savings from leakage current is minimal
    double current_reduction = 1;
    if (vdd_scaling != 1) {
        double vdd_reduction = 0.25 * 0.9 * (vdd_scaling - 1);
        current_reduction = exp(vdd_reduction);
    }
    
    scaling_correction = current_reduction * scaling_correction * static_power_scaling;
    
    core_power->reset_turnoff_factor();
    for (int i = 0; i < flp->n_units; i++) {
        leak_f = calculate_leakage_factor(temp[i]) * scaling_correction; 
        core_power->set_turnoff_factor(string(flp->units[i].name), leak_f);
    }
    
    // Icache
    // leak_f = calculate_leakage_factor(temp[get_blk_index(flp, "Icache")]);
    // core_power->set_icache_turnoff_factor(leak_f);
    // Dcache
    // leak_f = calculate_leakage_factor(temp[get_blk_index(flp, "Dcache")]);
    // core_power->set_dcache_turnoff_factor(leak_f);
    // Bpred
    // leak_f = calculate_leakage_factor(temp[get_blk_index(flp, "Bpred")]);
    // core_power->set_bpred_turnoff_factor(leak_f);
    // DTB
    // leak_f = calculate_leakage_factor(temp[get_blk_index(flp, "DTB")]);
    // core_power->set_dtb_turnoff_factor(leak_f);
    
    // FPAdd and FPMul
    // leak_f = calculate_leakage_factor(temp[get_blk_index(flp, "FPAdd")]);
    // double leak_f2 = calculate_leakage_factor(temp[get_blk_index(flp, "FPMul")]);
    // core_power->set_falu_turnoff_factor(leak_f + leak_f2);
    
    // FPReg
    // leak_f = calculate_leakage_factor(temp[get_blk_index(flp, "FPReg")]);
    // core_power->set_fpreg_turnoff_factor(leak_f);
    
    // FPMap and IntMap
    // leak_f = calculate_leakage_factor(temp[get_blk_index(flp, "FPMap")]);
    // leak_f2 = calculate_leakage_factor(temp[get_blk_index(flp, "IntMap")]);
    // core_power->set_rename_turnoff_factor(leak_f + leak_f2);
    
    // Iwindow
    // leak_f = calculate_leakage_factor(temp[get_blk_index(flp, "FPQ")]);
    // leak_f2 = calculate_leakage_factor(temp[get_blk_index(flp, "IntQ")]);
    // core_power->set_window_turnoff_factor(leak_f + leak_f2);
    // Int Register File
    // leak_f = calculate_leakage_factor(temp[get_blk_index(flp, "IntReg")]);
    // core_power->set_intreg_turnoff_factor(leak_f);
    // Load Store Queue
    // leak_f = calculate_leakage_factor(temp[get_blk_index(flp, "LdStQ")]);
    // core_power->set_lsq_turnoff_factor(leak_f);
    // Instruction ITB
    // leak_f = calculate_leakage_factor(temp[get_blk_index(flp, "ITB")]);
    // core_power->set_itb_turnoff_factor(leak_f);
    
    
    
    
}   
void hotspot_t::update_stats()
{
    STAT_INC(stat_num_compute);
    total_regfile_temp->inc_total(temp[get_blk_index(flp, "IntReg")]);
    total_intexe_temp->inc_total(temp[get_blk_index(flp, "IntExec")]);
    total_window_temp->inc_total(temp[get_blk_index(flp, "IntQ")]);
        
    last_regfile_temp->set(temp[get_blk_index(flp, "IntReg")]);
    last_intexe_temp->set(temp[get_blk_index(flp, "IntExec")]);
    last_window_temp->set(temp[get_blk_index(flp, "IntQ")]);
    
    
    // Update the max
    double t_item = max_regfile_temp->get_total();
    if (temp[get_blk_index(flp, "IntReg")] > t_item)
        max_regfile_temp->set(temp[get_blk_index(flp, "IntReg")]);
    t_item = max_intexe_temp->get_total();
    if (temp[get_blk_index(flp, "IntExec")] > t_item)
        max_intexe_temp->set(temp[get_blk_index(flp, "IntExec")]);
    t_item = max_window_temp->get_total();
    if (temp[get_blk_index(flp, "IntQ")] > t_item)
        max_window_temp->set(temp[get_blk_index(flp, "IntQ")]);
    
}

double hotspot_t::calculate_leakage_factor(double temp_val)
{
    // Calculate the index
    int temp_floor = (int) temp_val;
    double interp_temp;
    int f_index;
    if (temp_val > (temp_floor + 0.5)) {
        f_index = (temp_floor - 303) * 2 + 1;
        interp_temp = (double) temp_floor + 0.5;
    } else {
        f_index = (temp_floor - 303) * 2;
        interp_temp = (double) temp_floor;
    }

    double val_lower = leakage_factor[f_index];
    double val_upper = leakage_factor[f_index + 1];    
    double factor = val_lower + (val_upper - val_lower) * (interp_temp - temp_val);
    
    // Leakage factor is ratio of dynamic power and leakage power
    // We assume leakage factor is 0.03 at 30 degree C
    // Assuming 10% activity factor, this implies per cycle this factor is 0.03
    return factor * 0.03;
    
}

void hotspot_t::to_file(FILE *f)
{
    stats->to_file(f);
    for (int32 i = 0; i < flp->n_units; i++) {
        fprintf(f, "%lf %lf\n", overall_power[i], temp[i]);
    }
    
    
}

void hotspot_t::from_file(FILE *f)
{
    stats->from_file(f);
    for (int32 i = 0; i < flp->n_units; i++) {
        fscanf(f, "%lf %lf\n", &overall_power[i], &temp[i]);
    }
}

void hotspot_t::print_stats()
{
    stats->print();
    if (g_conf_power_trace) fflush(p_file);
}

void hotspot_t::initialize_leakage_factor()
{
    
    // Leakage factors starts from 30C at each 0.5C upto 110C
    leakage_factor = new double[200];
    
    leakage_factor[0] = 1.0000;
    leakage_factor[1] = 1.00729607936095;
    leakage_factor[2] = 1.01462401933047;
    leakage_factor[3] = 1.02198127105986;
    leakage_factor[4] = 1.02936910897348;
    leakage_factor[5] = 1.03678880749565;
    leakage_factor[6] = 1.04423781777771;
    leakage_factor[7] = 1.05171868866833;
    leakage_factor[8] = 1.05923014574317;
    leakage_factor[9] = 1.06677091457789;
    leakage_factor[10] = 1.07434354402117;
    leakage_factor[11] = 1.08194675964867;
    leakage_factor[12] = 1.08958056146039;
    leakage_factor[13] = 1.09724494945633;
    leakage_factor[14] = 1.10493992363649;
    leakage_factor[15] = 1.11266548400088;
    leakage_factor[16] = 1.12042163054948;
    leakage_factor[17] = 1.12820836328231;
    leakage_factor[18] = 1.13602568219935;
    leakage_factor[19] = 1.14387358730062;
    leakage_factor[20] = 1.1517520785861;
    leakage_factor[21] = 1.15966115605581;
    leakage_factor[22] = 1.16760081970974;
    leakage_factor[23] = 1.17557106954789;
    leakage_factor[24] = 1.1835731799946;
    leakage_factor[25] = 1.19160460220119;
    leakage_factor[26] = 1.199666610592;
    leakage_factor[27] = 1.20775920516703;
    leakage_factor[28] = 1.21588238592628;
    leakage_factor[29] = 1.22403615286975;
    leakage_factor[30] = 1.23222050599744;
    leakage_factor[31] = 1.24043544530935;
    leakage_factor[32] = 1.24868097080549;
    leakage_factor[33] = 1.2569558080615;
    leakage_factor[34] = 1.26526250592607;
    leakage_factor[35] = 1.27359978997487;
    leakage_factor[36] = 1.28196638578354;
    leakage_factor[37] = 1.29036484220078;
    leakage_factor[38] = 1.29879261037789;
    leakage_factor[39] = 1.30725096473923;
    leakage_factor[40] = 1.31573990528478;
    leakage_factor[41] = 1.32425943201456;
    leakage_factor[42] = 1.33280954492856;
    leakage_factor[43] = 1.34139024402677;
    leakage_factor[44] = 1.35000025488487;
    leakage_factor[45] = 1.35864085192718;
    leakage_factor[46] = 1.36731203515372;
    leakage_factor[47] = 1.37601380456448;
    leakage_factor[48] = 1.38474616015946;
    leakage_factor[49] = 1.39350910193865;
    leakage_factor[50] = 1.40230135547773;
    leakage_factor[51] = 1.41112419520103;
    leakage_factor[52] = 1.41997762110855;
    leakage_factor[53] = 1.42886035877594;
    leakage_factor[54] = 1.4377749570519;
    leakage_factor[55] = 1.44671886708774;
    leakage_factor[56] = 1.45569208888345;
    leakage_factor[57] = 1.46469717128773;
    leakage_factor[58] = 1.47373156545189;
    leakage_factor[59] = 1.48279654580026;
    leakage_factor[60] = 1.49189083790852;
    leakage_factor[61] = 1.50101571620099;
    leakage_factor[62] = 1.51017118067769;
    leakage_factor[63] = 1.51935595691426;
    leakage_factor[64] = 1.52857131933506;
    leakage_factor[65] = 1.53781726794007;
    leakage_factor[66] = 1.54709252830496;
    leakage_factor[67] = 1.55639837485408;
    leakage_factor[68] = 1.56573480758741;
    leakage_factor[69] = 1.57509927765628;
    leakage_factor[70] = 1.58449560833372;
    leakage_factor[71] = 1.59392125077103;
    leakage_factor[72] = 1.60337620496822;
    leakage_factor[73] = 1.61286301977397;
    leakage_factor[74] = 1.62237787191526;
    leakage_factor[75] = 1.63192331024076;
    leakage_factor[76] = 1.64149933475049;
    leakage_factor[77] = 1.65110339659576;
    leakage_factor[78] = 1.66073931904959;
    leakage_factor[79] = 1.67040455326329;
    leakage_factor[80] = 1.68009909923687;
    leakage_factor[81] = 1.68982295697034;
    leakage_factor[82] = 1.69957740088802;
    leakage_factor[83] = 1.70936243098992;
    leakage_factor[84] = 1.71917549842736;
    leakage_factor[85] = 1.72902042647336;
    leakage_factor[86] = 1.7388933918549;
    leakage_factor[87] = 1.74879694342066;
    leakage_factor[88] = 1.75872980674629;
    leakage_factor[89] = 1.76869198183181;
    leakage_factor[90] = 1.7786834686772;
    leakage_factor[91] = 1.78870554170681;
    leakage_factor[92] = 1.7987569264963;
    leakage_factor[93] = 1.80883889747001;
    leakage_factor[94] = 1.81894890577926;
    leakage_factor[95] = 1.82908950027273;
    leakage_factor[96] = 1.83925940652607;
    leakage_factor[97] = 1.8494586245393;
    leakage_factor[98] = 1.8596871543124;
    leakage_factor[99] = 1.86994499584538;
    leakage_factor[100] = 1.88023342356258;
    leakage_factor[101] = 1.89054988861531;
    leakage_factor[102] = 1.90089693985227;
    leakage_factor[103] = 1.9112733028491;
    leakage_factor[104] = 1.92167897760582;
    leakage_factor[105] = 1.93211396412241;
    leakage_factor[106] = 1.94257698797453;
    leakage_factor[107] = 1.95307059801088;
    leakage_factor[108] = 1.9635935198071;
    leakage_factor[109] = 1.97414575336321;
    leakage_factor[110] = 1.98472729867919;
    leakage_factor[111] = 1.99533815575505;
    leakage_factor[112] = 2.00597832459078;
    leakage_factor[113] = 2.01664653076205;
    leakage_factor[114] = 2.02734532311755;
    leakage_factor[115] = 2.03807342723292;
    leakage_factor[116] = 2.04882956868383;
    leakage_factor[117] = 2.05961502189461;
    leakage_factor[118] = 2.07042978686527;
    leakage_factor[119] = 2.08127386359581;
    leakage_factor[120] = 2.09214725208623;
    leakage_factor[121] = 2.10304995233653;
    leakage_factor[122] = 2.11398068992236;
    leakage_factor[123] = 2.12494073926807;
    leakage_factor[124] = 2.13593010037366;
    leakage_factor[125] = 2.14694877323913;
    leakage_factor[126] = 2.15799548344013;
    leakage_factor[127] = 2.16907150540101;
    leakage_factor[128] = 2.18017683912177;
    leakage_factor[129] = 2.19131021017806;
    leakage_factor[130] = 2.20247289299423;
    leakage_factor[131] = 2.21366488757028;
    leakage_factor[132] = 2.22488491948187;
    leakage_factor[133] = 2.23613426315333;
    leakage_factor[134] = 2.24741291858468;
    leakage_factor[135] = 2.25871961135155;
    leakage_factor[136] = 2.27005561587831;
    leakage_factor[137] = 2.2814196577406;
    leakage_factor[138] = 2.29281301136277;
    leakage_factor[139] = 2.30423440232047;
    leakage_factor[140] = 2.31568510503805;
    leakage_factor[141] = 2.32716384509117;
    leakage_factor[142] = 2.33867189690417;
    leakage_factor[143] = 2.3502079860527;
    leakage_factor[144] = 2.36177338696111;
    leakage_factor[145] = 2.37336682520505;
    leakage_factor[146] = 2.38498957520888;
    leakage_factor[147] = 2.39664036254824;
    leakage_factor[148] = 2.40831918722313;
    leakage_factor[149] = 2.4200273236579;
    leakage_factor[150] = 2.43176349742821;
    leakage_factor[151] = 2.44352770853405;
    leakage_factor[152] = 2.45532123139978;
    leakage_factor[153] = 2.46714279160103;
    leakage_factor[154] = 2.47899238913783;
    leakage_factor[155] = 2.49087002401015;
    leakage_factor[156] = 2.50277697064236;
    leakage_factor[157] = 2.5147119546101;
    leakage_factor[158] = 2.52667497591338;
    leakage_factor[159] = 2.53866730897654;
    leakage_factor[160] = 2.55068767937523;   
    leakage_factor[161] = 2.57068767937523;   
    
}
