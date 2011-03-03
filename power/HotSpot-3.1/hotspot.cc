/* 
 * This is a trace-level thermal simulator. It reads power values 
 * from an input trace file and outputs the corresponding instantaneous 
 * temperature values to an output trace file. It also outputs the steady 
 * state temperature values to stdout.
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "flp.h"
#include "temperature.h"
#include "temperature_block.h"
#include "temperature_grid.h"
#include "util.h"
#include "hotspot.h"

/* HotSpot thermal model is offered in two flavours - the block
 * version and the grid version. The block model models temperature
 * per functional block of the floorplan while the grid model
 * chops the chip up into a matrix of grid cells and models the 
 * temperature of each cell. It is also capable of modeling a
 * 3-d chip with multiple floorplans stacked on top of each
 * other. The choice of which model to choose is done through
 * a command line or configuration file parameter model_type. 
 * "-model_type block" chooses the block model while "-model_type grid"
 * chooses the grid model. 
 */

/* Guidelines for choosing the block or the grid model	*/

/**************************************************************************/
/* This version of HotSpot contains two methods for solving temperatures: */
/* 	1) Block Model -- the same as HotSpot 2.0							  */
/*	2) Grid Model -- the die is divided into regular grid cells (NEW!) 	  */
/**************************************************************************/
/* How the grid model works: 											  */
/* 	The grid model first reads in floorplan and maps block-based power	  */
/* to each grid cell, then solves the temperatures for all the grid cells, */
/* finally, converts the resulting grid temperatures back to block-based  */
/* temperatures.														  */
/**************************************************************************/
/* The grid model is useful when 										  */
/* 	1) More detailed temperature distribution inside a functional unit    */
/*     is desired.														  */
/*  2) Too many functional units are included in the floorplan, resulting */
/*		 in extremely long computation time if using the Block Model      */
/*	3) If temperature information is desired for many tiny units,		  */ 
/* 		 such as individual register file entry.						  */
/**************************************************************************/
/*	Comparisons between Grid Model and Block Model:						  */
/*		In general, the grid model is more accurate, because it can deal  */
/*	with various floorplans and it provides temperature gradient across	  */
/*	each functional unit. The block model models essentially the center	  */
/*	temperature of each functional unit. But the block model is typically */
/*	faster because there are less nodes to solve.						  */
/*		Therefore, unless it is the case where the grid model is 		  */
/*	definitely	needed, we suggest using the block model for computation  */
/*  efficiency.															  */
/**************************************************************************/
/* Other features of the grid model:									  */
/*	1) Multi-layer -- the grid model supports multilayer structures, such */
/* 		 as 3D integration where multiple silicon layers with 			  */
/*		 different floorplans and dissipating power,					  */
/* 		 or multilayer of on-chip interconnects dissipating self-heating  */
/*		 power, etc. The user needs to provide a layer config file (.lcf).*/
/*		 An example layer.lcf file is provided with this release.		  */
/**************************************************************************/

void usage(int argc, char **argv)
{
	fprintf(stdout, "Usage: %s -f <file> -p <file> [-o <file>] [-c <file>] [-d <file>] [options]\n", argv[0]);
	fprintf(stdout, "A thermal simulator that reads power trace from a file and outputs temperatures.\n");
	fprintf(stdout, "Options:(may be specified in any order, within \"[]\" means optional)\n");
	fprintf(stdout, "   -f <file>\tfloorplan input file for the block model (e.g. ev6.flp) (or)\n");
	fprintf(stdout, "            \tdefault floorplan file for the grid model, which is the same as the\n");
	fprintf(stdout, "            \tfloorplan file specified in layer 0 of the layer configuration file\n");
	fprintf(stdout, "            \t(e.g. layer.lcf)\n");
	fprintf(stdout, "   -p <file>\tpower trace input file (e.g. gcc.ptrace)\n");
	fprintf(stdout, "  [-o <file>]\ttransient temperature trace output file - if not provided, only\n");
	fprintf(stdout, "            \tsteady state temperatures are output to stdout\n");
	fprintf(stdout, "  [-c <file>]\tinput configuration parameters from file (e.g. hotspot.config)\n");
	fprintf(stdout, "  [-d <file>]\toutput configuration parameters to file\n");
	fprintf(stdout, "  [options]\tzero or more options of the form \"-<name> <value>\",\n");
	fprintf(stdout, "           \toverride the options from config file. e.g. \"-model_type block\" selects\n");
	fprintf(stdout, "           \tthe block model while \"-model_type grid\" selects the grid model\n");
}

/* 
 * parse a table of name-value string pairs and add the configuration
 * parameters to 'config'
 */
void global_config_from_strs(global_config_t *config, str_pair *table, int size)
{
	int idx;
	if ((idx = get_str_index(table, size, "f")) >= 0) {
		if(sscanf(table[idx].value, "%s", config->flp_file) != 1)
			fatal("invalid format for configuration  parameter flp_file");
	} else {
		fatal("required parameter flp_file missing. check usage\n");
	}
	if ((idx = get_str_index(table, size, "p")) >= 0) {
		if(sscanf(table[idx].value, "%s", config->p_infile) != 1)
			fatal("invalid format for configuration  parameter p_infile");
	} else {
		fatal("required parameter p_infile missing. check usage\n");
	}
	if ((idx = get_str_index(table, size, "o")) >= 0) {
		if(sscanf(table[idx].value, "%s", config->t_outfile) != 1)
			fatal("invalid format for configuration  parameter t_outfile");
	} else {
		strcpy(config->t_outfile, NULLFILE);
	}
	if ((idx = get_str_index(table, size, "c")) >= 0) {
		if(sscanf(table[idx].value, "%s", config->config) != 1)
			fatal("invalid format for configuration  parameter config");
	} else {
		strcpy(config->config, NULLFILE);
	}
	if ((idx = get_str_index(table, size, "d")) >= 0) {
		if(sscanf(table[idx].value, "%s", config->dump_config) != 1)
			fatal("invalid format for configuration  parameter dump_config");
	} else {
		strcpy(config->dump_config, NULLFILE);
	}
}

/* 
 * convert config into a table of name-value pairs. returns the no.
 * of parameters converted
 */
int global_config_to_strs(global_config_t *config, str_pair *table, int max_entries)
{
	if (max_entries < 5)
		fatal("not enough entries in table\n");

	sprintf(table[0].name, "f");
	sprintf(table[1].name, "p");
	sprintf(table[2].name, "o");
	sprintf(table[3].name, "c");
	sprintf(table[4].name, "d");

	sprintf(table[0].value, "%s", config->flp_file);
	sprintf(table[1].value, "%s", config->p_infile);
	sprintf(table[2].value, "%s", config->t_outfile);
	sprintf(table[3].value, "%s", config->config);
	sprintf(table[4].value, "%s", config->dump_config);

	return 5;
}

/* 
 * read a single line of trace file containing names
 * of functional blocks
 */
int read_names(FILE *fp, char **names)
{
	char line[STR_SIZE], *src;
	int i;

	/* read the entire line	*/
	fgets(line, STR_SIZE, fp);
	if (feof(fp))
		fatal("not enough names in trace file\n");

	/* new line not read yet	*/	
	if(line[strlen(line)-1] != '\n')
		fatal("line too long\n");

	/* chop the names from the line read	*/
	for(i=0,src=line; *src && i < MAX_UNITS; i++) {
		if(!sscanf(src, "%s", names[i]))
			fatal("invalid format of names");
		src += strlen(names[i]);
		while (isspace((int)*src))
			src++;
	}
	if(*src && i == MAX_UNITS)
		fatal("no. of units exceeded limit");

	return i;
}

/* read a single line of power trace numbers	*/
int read_vals(FILE *fp, double *vals)
{
	char line[STR_SIZE], temp[STR_SIZE], *src;
	int i;

	/* read the entire line	*/
	fgets(line, STR_SIZE, fp);
	if (feof(fp))
		return 0;

	/* new line not read yet	*/	
	if(line[strlen(line)-1] != '\n')
		fatal("line too long\n");

	/* chop the power values from the line read	*/
	for(i=0,src=line; *src && i < MAX_UNITS; i++) {
		if(!sscanf(src, "%s", temp) || !sscanf(src, "%lf", &vals[i]))
			fatal("invalid format of values");
		src += strlen(temp);
		while (isspace((int)*src))
			src++;
	}
	if(*src && i == MAX_UNITS)
		fatal("no. of entries exceeded limit");

	return i;
}

/* write a single line of functional unit names	*/
void write_names(FILE *fp, char **names, int size)
{
	int i;
	for(i=0; i < size-1; i++)
		fprintf(fp, "%s\t", names[i]);
	fprintf(fp, "%s\n", names[i]);
}

/* write a single line of temperature trace(in degree C)	*/
void write_vals(FILE *fp, double *vals, int size)
{
	int i;
	for(i=0; i < size-1; i++)
		fprintf(fp, "%.2f\t", vals[i]-273.15);
	fprintf(fp, "%.2f\n", vals[i]-273.15);
}

char **alloc_names(int nr, int nc)
{
	int i;
	char **m;

	m = (char **) calloc (nr, sizeof(char *));
	assert(m != NULL);
	m[0] = (char *) calloc (nr * nc, sizeof(char));
	assert(m[0] != NULL);

	for (i = 1; i < nr; i++)
    	m[i] =  m[0] + nc * i;

	return m;
}

void free_names(char **m)
{
	free(m[0]);
	free(m);
}

/* 
 * main function - reads instantaneous power values (in W) from a trace
 * file (e.g. "gcc.ptrace") and outputs instantaneous temperature values (in C) to
 * a trace file("gcc.ttrace"). also outputs steady state temperature values
 * (including those of the internal nodes of the model) onto stdout. the
 * trace files are 2-d matrices with each column representing a functional
 * functional block and each row representing a time unit(sampling_intvl).
 * columns are tab-separated and each row is a separate line. the first
 * line contains the names of the functional blocks. the order in which
 * the columns are specified doesn't have to match that of the floorplan 
 * file.
 */
int main(int argc, char **argv)
{
	int i, n = 0, num, size, lines = 0, do_transient = TRUE;
	int total = 0;
	char **names;
	double *vals;
	/* trace file pointers	*/
	FILE *pin, *tout;
	/* floorplan	*/
	flp_t *flp;
	/* hotspot temperature model	*/
	RC_model_t *model;
	/* instantaneous temperature and power values	*/
	double *temp, *power;
	/* steady state temperature and power values	*/
	double *overall_power, *steady_temp;
	/* thermal model configuration parameters	*/
	thermal_config_t thermal_config;
	/* global configuration parameters	*/
	global_config_t global_config;
	/* table to hold options and configuration */
	str_pair table[MAX_ENTRIES];

	if (!(argc >= 5 && argc % 2)) {
		usage(argc, argv);
		return 1;
	}
	
	size = parse_cmdline(table, MAX_ENTRIES, argc, argv);
	global_config_from_strs(&global_config, table, size);

	/* no transient simulation, only steady state	*/
	if(!strcmp(global_config.t_outfile, NULLFILE))
		do_transient = FALSE;

	/* read configuration file	*/
	if (strcmp(global_config.config, NULLFILE))
		size += read_str_pairs(&table[size], MAX_ENTRIES, global_config.config);

	/* 
	 * earlier entries override later ones. so, command line options 
	 * have priority over config file 
	 */
	size = str_pairs_remove_duplicates(table, size);

	/* get defaults */
	thermal_config = default_thermal_config();
	/* modify according to command line / config file	*/
	thermal_config_add_from_strs(&thermal_config, table, size);

	/* dump configuration if specified	*/
	if (strcmp(global_config.dump_config, NULLFILE)) {
		size = global_config_to_strs(&global_config, table, MAX_ENTRIES);
		size += thermal_config_to_strs(&thermal_config, &table[size], MAX_ENTRIES-size);
		/* prefix the name of the variable with a '-'	*/
		dump_str_pairs(table, size, global_config.dump_config, "-");
	}

	/* initialization: the flp_file global configuration 
	 * parameter means different things for the block and grid 
	 * models. it is the input floorplan file for the block model
	 * and the default floorplan file for the grid model, which
	 * reads the actual floorplans of all the 3-d layers of the
	 * chip from the layer configuration file. Also note that
	 * for the grid model, this default floorplan file and the 
	 * floorplan file specified in layer 0 of the layer configuration 
	 * file have to be the same. 
	 */
	flp = read_flp(global_config.flp_file, FALSE);

	/* allocate and initialize the RC model	*/
	model = alloc_RC_model(&thermal_config, flp);
	populate_R_model(model, flp);
	if (do_transient)
		populate_C_model(model, flp);

	/* allocate the temp and power arrays	*/
	/* using hotspot_vector to internally allocate any extra nodes needed	*/
	if (do_transient)
		temp = hotspot_vector(model);
	power = hotspot_vector(model);
	steady_temp = hotspot_vector(model);
	overall_power = hotspot_vector(model);
	
	/* set up initial instantaneous temperatures */
	if (do_transient && strcmp(model->config->init_file, NULLFILE)) {
		if (!model->config->dtm_used)	/* initial T = steady T for no DTM	*/
			read_temp(model, temp, model->config->init_file, FALSE);
		else	/* initial T = clipped steady T with DTM	*/
			read_temp(model, temp, model->config->init_file, TRUE);
	} else if (do_transient)	/* no input file - use init_temp as the common temperature	*/
		set_temp(model, temp, model->config->init_temp);

	/* n is the number of functional blocks in the block model
	 * while it is the sum total of the number of functional blocks
	 * of all the floorplans in the different layers of the grid
	 * model. 'total' is the no. of internal nodes except spreader
	 * and sink
	 */
	if (model->type == BLOCK_MODEL) {
		n = model->block->flp->n_units;
		total = model->block->n_nodes - EXTRA;
	} else if (model->type == GRID_MODEL) {
		n = model->grid->sum_n_units;
		total =  model->grid->total_n_blocks;
	} else 
		fatal("unknown model type\n");

	if(!(pin = fopen(global_config.p_infile, "r")))
		fatal("unable to open power trace input file\n");
	if(do_transient && !(tout = fopen(global_config.t_outfile, "w")))
		fatal("unable to open temperature trace file for output\n");

	/* names of functional units	*/
	names = alloc_names(MAX_UNITS, STR_SIZE);
	if(read_names(pin, names) != n)
		fatal("no. of units in floorplan and trace file differ\n");

	/* header line of temperature trace	*/
	if (do_transient)
		write_names(tout, names, n);

	/* read the instantaneous power trace	*/
	vals = dvector(MAX_UNITS);
	while ((num=read_vals(pin, vals)) != 0) {
		if(num != n)
			fatal("invalid trace file format\n");
		/* note: grid model has potentially multiple floorplans
		 * because of its 3-D chip capability. so, we require
		 * the functional unit ordering in the power trace file 
		 * to be a concatenation of all the floorplan files in the
		 * order they appear in the layer configuration file.
		 * this means the flexibility of specification in the block 
		 * model input files is not there in the grid model, which is
		 * the price we pay for its 3-D ability. Also, there is no 
		 * permutation and re-permutation for the grid model as in 
		 * the block model.
		 */
		for(i=0; i < n; i++) {
			if (model->type == BLOCK_MODEL)
				/* permute the power numbers according to the floorplan order	*/
				power[get_blk_index(flp, names[i])] = vals[i];
			else
				power[i] = vals[i];
		}

		/* compute temperature	*/
		if (do_transient) {
			compute_temp(model, power, temp, model->config->sampling_intvl);
	
			/* permute back to the trace file order	- only for block model */
			if (model->type == BLOCK_MODEL)
				for(i=0; i < n; i++)
					vals[i] = temp[get_blk_index(flp, names[i])];
		
			/* output instantaneous temperature trace	*/
			if (model->type == BLOCK_MODEL)
				write_vals(tout, vals, n);
			else
				/* no re-permutation for grid model	*/
				write_vals(tout, temp, n);
		}		
	
		/* for computing average	*/
		for(i=0; i < n; i++)
			overall_power[i] += power[i];
		lines++;
	}

	if(!lines)
		fatal("no power numbers in trace file\n");
		
	/* for computing average	*/
	for(i=0; i < n; i++)
		overall_power[i] /= lines;

	/* steady state temperature	*/
	steady_state_temp(model, overall_power, steady_temp);

	/* print steady state results	*/
	fprintf(stdout, "Unit\tSteady(Kelvin)\n");
	dump_temp(model, steady_temp, "stdout");

	/* dump steady state temperatures on to file if needed	*/
	if (strcmp(model->config->steady_file, NULLFILE))
		dump_temp(model, steady_temp, model->config->steady_file);

	/* cleanup	*/
	fclose(pin);
	if (do_transient)
		fclose(tout);
	delete_RC_model(model);
	free_flp(flp, 0);
	if (do_transient)
		free_dvector(temp);
	free_dvector(power);
	free_dvector(steady_temp);
	free_dvector(overall_power);
	free_names(names);
	free_dvector(vals);

	return 0;
}
