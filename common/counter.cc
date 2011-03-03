/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: counter.cc,v 1.2.12.6 2006/03/02 23:58:32 pwells Exp $");

#include "definitions.h"
#include "counter.h"
#include "config_extern.h"

/******* st_entry_t implementation *******/
st_entry_t::st_entry_t (const string &n, 
	const string &d) {
	name = string(n);
	desc = string(d);
	hidden = false;
	void_zero_entries = false;
	hex_index = false;
	terse_format = false;
	new_stat = false;
}

st_entry_t::~st_entry_t () {
}

void
st_entry_t::set_hidden (bool _hid) {
	hidden = _hid;
}

void
st_entry_t::set_new_stat (bool _new) {
	new_stat = _new;
}

void
st_entry_t::set_void_zero_entries () {
	void_zero_entries = true;
}


void
st_entry_t::set_hex_index () {
	hex_index = true;
}

void
st_entry_t::set_terse_format () {
	terse_format = true;
}


/* compare if of the same name */
bool 
st_entry_t::name_as (const string &n) {
	return name == n;
}

inline void 
st_entry_t::inc_total (uint64 count, uint32 index, uint32 value_y) {
}

inline double 
st_entry_t::get_total () { 
	return 0; 
}

void
st_entry_t::set (uint64 value, uint32 index) {
}

void
st_entry_t::clear () {
	set(0);
}


void 
st_entry_t::print_fmt (char *fmt, 
	const string &appendix, 
	bool pFP) 
{
	int len = appendix.size();
	if (pFP) { 	
		/* print in floating point format */
		if (len) 
			sprintf (fmt, "  %%%llds%s %%%lld.3f %%%llds\n", 
				g_conf_name_width - len, appendix.c_str(), g_conf_value_width, g_conf_desc_width);
		else sprintf (fmt, "  %%%llds %%%lld.3f %%%llds\n", 
				g_conf_name_width, g_conf_value_width, g_conf_desc_width);
	} else {					
		/* print in uint64 format */
		if (len) 
			sprintf (fmt, "  %%%llds%s %%%lldlld %%%llds\n", 
				g_conf_name_width - len, appendix.c_str(), g_conf_value_width, g_conf_desc_width);
		else sprintf (fmt, "  %%%llds %%%lldlld %%%llds\n", 
				g_conf_name_width, g_conf_value_width, g_conf_desc_width);
	}
}

/******* st_print_t implementation (e.g. (A+B-C)/ (D+E) ) *******/
st_print_t::st_print_t (const string & n, 
	const string & d, 
	int num, 
	st_entry_t ** c_array, 
	double (*fptr) (st_entry_t **), 
	bool pINT)
	: st_entry_t (n, d) {
	ptr = fptr;
	print_int = pINT;
	num_params = num;

	array = new st_entry_t* [num];
	for (int i = 0; i < num; i++) 
		array [i] = c_array [i];
}

double
st_print_t::get_total () {
	return ptr (array);
}

void 
st_print_t::print () {
	if (hidden) return; 
	char fmt [64];
	
	print_fmt (fmt, "", (!print_int));
	double result = get_total ();
	if (print_int) 
		DEBUG_LOG (fmt, name.c_str(), static_cast <uint64> (result), desc.c_str());
	else DEBUG_LOG (fmt, name.c_str(), result, desc.c_str());
}

/******* basic counter implementation (e.g. num_instr_committed) *******/
base_counter_t::base_counter_t (const string &n, 
	const string &d)
	: st_entry_t (n, d) {
   	total = 0;
}

void base_counter_t::to_file(FILE *fl)
{
   fprintf(fl, "%llu\n" , total);
}

void base_counter_t::from_file(FILE *fl)
{
	if (new_stat) return;
    fscanf(fl, "%llu\n", &total);
}

inline void 
base_counter_t::inc_total (uint64 count, uint32 index, uint32 value_y) {
	total += count;
}

inline double 
base_counter_t::get_total () {
	return total;
}

void
base_counter_t::set (uint64 value, uint32 index) {
	total = value;
}

void
base_counter_t::clear() {
	total = 0;
}

void 
base_counter_t::print () {
	char fmt [64];

	if (hidden) return;

	print_fmt (fmt, "");
	DEBUG_LOG (fmt, name.c_str(), total, desc.c_str());
}

/******* double counter implementation (e.g. num_instr_committed) *******/
double_counter_t::double_counter_t (const string &n, 
	const string &d)
	: st_entry_t (n, d) {
   	total = 0.0;
}

void double_counter_t::to_file(FILE *fl)
{
    if (new_stat) return;
   fprintf(fl, "%lf\n" , total);
}

void double_counter_t::from_file(FILE *fl)
{
	if (new_stat) return;
    int32 ret_val = fscanf(fl, "%lf\n", &total);
    ASSERT(ret_val == 1);
}

inline void 
double_counter_t::inc_total (double count, uint32 index, uint32 value_y) {
	total = total +  count;
}

inline double 
double_counter_t::get_total () {
	return total;
}

void
double_counter_t::set (double value, uint32 index) {
	total = value;
}

void
double_counter_t::clear() {
	total = 0;
}

void 
double_counter_t::print () {
	char fmt [64];

	if (hidden) return;

	print_fmt (fmt, "", true);
	DEBUG_LOG (fmt, name.c_str(), total, desc.c_str());
}




/******* ratio between two counters (i.e. IPC) *******/
/* special case: ratio */
ratio_print_t::ratio_print_t (const string &n, 
	const string &d, 
	st_entry_t * top, 
	st_entry_t * bottom) 
	: st_entry_t (n, d) {
	this->top = top;
	this->bottom = bottom;
}

void 
ratio_print_t::print () {
	if (hidden) return; 
	uint64 tot = (uint64) bottom->get_total();

    char fmt [64];
    print_fmt (fmt, "", true);
    double result = 0;
	
	if (tot != 0) 
		result = top->get_total() * 1.0 / tot;

   	DEBUG_LOG (fmt, name.c_str(), result, desc.c_str());
}

/******* counter group implementation: e.g. misses per CPU *******/
group_counter_t::group_counter_t (const string &n, 
	const string &d, 
	uint64 num) 
	: st_entry_t (n, d) {
   	num_entries = num;
   	array = new uint64 [num];
	for (uint64 i = 0; i < num; i++)
		array [i] = 0;
			
}

void
group_counter_t::from_file(FILE *fl)
{
	if (new_stat) return;
    fscanf(fl, "%llu\n", &num_entries);
    for (uint64 i = 0; i < num_entries; i++)
    {
        fscanf(fl, "%llu", &array[i]);
    }
    fscanf(fl, "\n");
}

void 
group_counter_t::to_file(FILE *fl)
{
    fprintf(fl, "%llu\n", num_entries);
    for (uint32 i = 0; i < num_entries; i++)
        fprintf(fl, "%llu ", array[i]);
    fprintf(fl, "\n");
}

inline void
group_counter_t::inc_total (uint64 count, uint32 index, uint32 value_y) {
	ASSERT (index < num_entries && index >= 0);
	array [index] += count;
}

inline void
group_counter_t::set (uint64 val, uint32 index) {
    ASSERT (index < num_entries && index >= 0);
    array [index] = val;
}


inline double 
group_counter_t::get_total () {
   	uint64 ret = 0;
   	for (uint64 i = 0; i < num_entries; i++)
	   	ret += array [i];
   	return ret;
}

void
group_counter_t::clear() {
	for (uint64 i = 0; i < num_entries ; i++)
		array[i] = 0;
}

void 
group_counter_t::print () {
	if (hidden) return; 
	char fmt [64];
	uint64 t = (uint64) get_total ();
	print_fmt (fmt, "_totl");
	DEBUG_LOG (fmt, name.c_str(), t, desc.c_str());
	print_fmt (fmt, "_numb");
	DEBUG_LOG (fmt, name.c_str(), num_entries, desc.c_str());
	if (!hex_index)
		sprintf (fmt, "%12s  %%12lld : %%10lld %%7.3f\n", name.c_str());
	else 	
		sprintf (fmt, "%12s  0x%%12llx : %%10lld %%7.3f\n", name.c_str());

	for (uint64 i = 0; i < num_entries; i++) {
		if (!array [i] && void_zero_entries) continue;
		DEBUG_LOG (fmt, i, array [i], array [i] * 100.0 / t);
	}
}

/******* counters for breakdown: e.g. 4C breakdown *******/
breakdown_counter_t::breakdown_counter_t (const string &n, 
	const string &d, 
	int num, 
	string * names, 
	string * descs)
	: group_counter_t (n, d, num) {
	for (int i = 0; i < num; i++) {
	   	name_array.push_back(names [i]);
	   	desc_array.push_back(descs [i]);
   	}
}

void 
breakdown_counter_t::print () {
	if (hidden) return; 
	char fmt [64];
	uint64 t = (uint64) get_total ();
	print_fmt (fmt, "_totl");
	DEBUG_LOG (fmt, name.c_str(), t, desc.c_str());
	print_fmt (fmt, "_numb");
	DEBUG_LOG (fmt, name.c_str(), num_entries, desc.c_str());
	sprintf (fmt, "%12s  %%12s : %%10lld %%7.3f %%%llds\n", 
			name.c_str(), g_conf_desc_width - 8);
	for (uint64 i = 0; i < num_entries; i++) {
		if (void_zero_entries && !array [i]) continue;
		DEBUG_LOG (fmt, name_array[i].c_str(), array [i], 
				array [i]*100.0/t, desc_array [i].c_str());
	}
}

