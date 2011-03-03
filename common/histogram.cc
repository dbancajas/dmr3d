/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: histogram.cc,v 1.2.12.3 2005/11/29 00:13:25 pwells Exp $");

#include "definitions.h"
#include "counter.h"
#include "histogram.h"


/******* histogram index classes *******/
base_histo_index_t::base_histo_index_t (uint32 num, uint32 start)  {
	num_bins = num;
	start_value = start;
}

uint32 
base_histo_index_t::get_num_bins () { 
	return num_bins; 
}

int 
base_histo_index_t::get_index (uint32 value) { 
	return 0; 
}    

uint32 
base_histo_index_t::lower_bound (uint32 bin) { 
	return 0; 
}

void 
base_histo_index_t::print (const string &n, const string &d) {
}
	
/******* uniform_histo_index_t implementation *******/
uniform_histo_index_t::uniform_histo_index_t (uint32 num, 
	uint32 wid, 
	uint32 start) 
	: base_histo_index_t (num, start) 
{ 
	width = wid; 
}

uint32 
uniform_histo_index_t::lower_bound (uint32 bin) { 
	return start_value + width * bin; 
}

int 
uniform_histo_index_t::get_index (uint32 value) {
	uint32 diff;
	if (value < start_value) 
		return -1;
	
    diff = (value - start_value) / width;
	if (diff >= num_bins) 
		diff = num_bins;
	return diff;
}

void 
uniform_histo_index_t::print (const string &n, const string &d) {
	char fmt [64];
	sprintf (fmt, "%%%ds # UNFRM  B %%d S %%d W %%d | %%%ds\n", 26, 20);
	DEBUG_LOG (fmt, n.c_str(), num_bins, start_value, width, d.c_str());
}

/******* exp (base 2^x) histo_index_t implementation *******/
exp2x_histo_index_t::exp2x_histo_index_t (uint32 num, 
	uint32 bits, 
	uint32 start) 
	: base_histo_index_t (num, start)  
{
   	/* ASSERT (start > 0); */
	bits_shift = bits;
}
	
uint32 
exp2x_histo_index_t::lower_bound (uint32 bin) { 
   	return start_value << (bits_shift * bin);
}

int 
exp2x_histo_index_t::get_index (uint32 value) {
	uint32 bin = 0;

	if (value < start_value)
		return -1;

	uint64 base = start_value << bits_shift;
	
	while (value >= base) {
		value = value >> bits_shift;
		bin ++;
		if (bin > num_bins) return num_bins;
	}
	return bin;
}

void 
exp2x_histo_index_t::print (const string &n, const string &d) {
	char fmt [64];
	sprintf (fmt, "%%%ds # EXP2X  B %%d S %%d F %%d | %%%ds\n", 26, 20);
	DEBUG_LOG (fmt, n.c_str(), num_bins, start_value, 1<<bits_shift, d.c_str());
}

/******* exp (base of any integer) histo_index_t implementation *******/
exp_histo_index_t::exp_histo_index_t (uint32 num, 
	uint32 b, 
	uint32 start) 
	: base_histo_index_t (num, start)  
{
	/* ASSERT (start > 0); */
	base = (b < 2) ? 2 : b;
}
	
uint32 
exp_histo_index_t::lower_bound (uint32 bin) { 
	uint32 num = start_value;
	for (uint32 i = 0; i < bin; i++) 
		num *= base;
	return num;
}

int 
exp_histo_index_t::get_index (uint32 value) {
	uint32 bin = 0;

	if (value < start_value)
		return -1;

	uint64 B = start_value * base;
	
	while (value >= B) {
		value = value / base;
		bin ++;
		if (bin > num_bins) return num_bins;
	}
	return bin;
}

void 
exp_histo_index_t::print (const string &n, const string &d) {
	char fmt [64];
	sprintf (fmt, "%%%ds # EXPNT  B %%d S %%d F %%d | %%%ds\n", 26, 20);
	DEBUG_LOG (fmt, n.c_str(), num_bins, start_value, base, d.c_str());
}

/******* bound histo_index_t implementation *******/
bound_histo_index_t::bound_histo_index_t (uint32 num, 
	uint32 * lb_arr) 
	: base_histo_index_t (num, lb_arr [0]) 
{
	/* ASSERT (num > 0); */
	num_bins = num;
	lb_array = new uint32 [num];
	lb_array [0] = lb_arr [0];
	uint32 old = lb_array [0];
	for (uint32 i = 1; i < num; i++) {
		if (old >= lb_arr [i]) {
			DEBUG_LOG ("\n****\n");
			DEBUG_LOG ("Bound histo init: lower bounds not mono-increasing!\n");
			DEBUG_LOG ("Bound histo init: bound [%d] = %u >= %u = bound [%d]!\n", 
					i - 1, old, lb_arr [i], i);
			DEBUG_LOG ("Bound histo init: lower bounds must be ``uint32''!\n");
			DEBUG_LOG ("****\n\n");
//			exit (1);
		}
		lb_array [i] = lb_arr [i];
	}
}
	
bound_histo_index_t::~bound_histo_index_t () {
	delete [] lb_array;
}

uint32 
bound_histo_index_t::lower_bound (uint32 bin) { 
	return lb_array [bin]; 
}

int 
bound_histo_index_t::get_index (uint32 value) {
	if (value < lb_array [0])
		return -1;

	for (uint32 i=0; i<num_bins-1; i++) {
		if (value < lb_array [i+1]) {
			return i;
		}
	}
	return (num_bins - 1);
}

void 
bound_histo_index_t::print (const string &n, const string &d) {
	char fmt [64];
	sprintf (fmt, "%%%ds # BOUND  B %%d S %%d | %%%ds\n", 26, 20);
	DEBUG_LOG (fmt, n.c_str(), num_bins, start_value, d.c_str());
}

/******* base histograms implementation *******/
base_histo_t::base_histo_t (const string &n, const string &d): 
	st_entry_t (n, d) {
   	array = NULL; 
	sum = 0;
   	underflow = overflow = 0;
   	uf_sum = of_sum = 0;
	t_sum = t_count = 0;
	num_bins = 0;
}

base_histo_t::~base_histo_t () {
}

int 
base_histo_t::get_index (uint32 value_x, uint32 value_y) { 
	return 0; 
}

void
base_histo_t::to_file(FILE *fl)
{
    fprintf(fl, "%llu %llu %llu %llu %llu %llu %llu %llu\n",
        sum, underflow, overflow, uf_sum, of_sum, t_sum, t_count, num_bins);
    for (uint64 i = 0; i < num_bins; i++)
        fprintf(fl, "%llu ", array[i]);
    fprintf(fl, "\n");
}

void
base_histo_t::from_file(FILE *fl)
{
	if (new_stat) return;
    fscanf(fl, "%llu %llu %llu %llu %llu %llu %llu %llu\n",
        &sum, &underflow, &overflow, &uf_sum, &of_sum, &t_sum, &t_count, &num_bins);
    for (uint64 i = 0; i < num_bins; i++)
        fscanf(fl, "%llu ", &array[i]);
    fscanf(fl, "\n");
    
}

void
base_histo_t::clear() {
	for (uint64 i = 0; i < num_bins; i++)
		array[i] = 0;
    sum = 0;
   	underflow = overflow = 0;
   	uf_sum = of_sum = 0;
	t_sum = t_count = 0;
}

uint32 
base_histo_t::get_bins () { 
	return 0; 
}

/* get/set data */
uint64 
base_histo_t::get_points () { 
	return 0; 
}

void 
base_histo_t::inc_total (uint64 count, uint32 value_x, uint32 value_y) {
}

/* print stats */
void 
base_histo_t::print () {};

void 
base_histo_t::print_stats () {};

/******* 1D histograms implementation *******/
histo_1d_t::histo_1d_t (const string & n, 
	const string &d, 
	base_histo_index_t *ind )
	: base_histo_t (n, d)
{
	index = ind;

	num_bins = ind->num_bins;
	array = new uint64 [num_bins];
	for (uint64 i = 0; i < num_bins; i++) 
		array [i] = 0;
}

histo_1d_t::~histo_1d_t () {
	delete []array;
	delete index;
}

uint32 
histo_1d_t::get_bins ()	{ 
	return index->get_num_bins (); 
}

int 
histo_1d_t::get_index (uint32 value_x) {
	return index->get_index (value_x);
}

uint64 
histo_1d_t::get_points () {
	int i, bins = get_bins ();
	uint64 pt = 0;
	for (i = 0; i < bins; i++) 
		pt += array [i];

	return pt;
}

void
histo_1d_t::inc (uint32 val, uint64 count, uint32 value_y) {
	inc_total (count, val, value_y);
}

void 
histo_1d_t::inc_total (uint64 count, uint32 val, uint32 value_y) {
	int i = index->get_index (val);
	if (i == -1) {
		underflow += count;
		uf_sum += val * count;
	} else
	if ( i == (int) (index->num_bins)) {
		overflow += count;
		of_sum += val * count;
	} else {
		array [i] += count;
		sum += val * count;
	}

	t_sum   += val * count;
	t_count += count;
	
#ifdef DEBUG_HISTO 
	DEBUG_LOG ("histo_1d inc val %u count %u bin %d ", val, count, i);
	if (i == -1) 
		DEBUG_LOG ("underflow %lld\n", underflow);
	else if (i == (int) (index->num_bins)) 
		DEBUG_LOG ("overflow %lld\n", overflow);
	else DEBUG_LOG ("bin_pts %lld\n", array [i]);
#endif
}

void 
histo_1d_t::print () {

	if (hidden) return;
	index->print (name, desc);

	if (!terse_format) {
		char fmt [64];
		uint64 accu = 0;
		uint32 bins = index->num_bins;

		uint64 tot_points = get_points ();
		if (tot_points == 0) 
			tot_points = 1;

		sprintf (fmt, "%%12d %%13d : %%10lld %%7.3f %%7.3f \n");
		for (uint32 i = 0; i < bins; i++) {
			if (!array [i] && void_zero_entries) continue;
			accu += array [i];
			DEBUG_LOG (fmt, i, index->lower_bound (i), array [i], 
				array [i] * 100.0 / tot_points, accu * 100.0 / tot_points);
		}
	}

	print_stats ();
}

void 
histo_1d_t::print_stats () {
	/* from Adam Butts' code */
	
	double breakfracs [] = { 0.1, 0.2, 0.25, 0.33333, 0.4, 0.5, 
		0.6, 0.66667, 0.75, 0.8, 0.9, 1.0 };
	uint32 breaks [11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	uint32 bins = index->num_bins;
	uint32 max = 0, min = bins, ind = 0;
   	uint64 thresh, median = 0, mode = 0, mpoints = 0, cpoints = 0, points = 0;
    
    
	for (uint32 i = 0; i < bins; i++) {
		if (array [i] && i < min) min = i;
		if (array [i] && i > max) max = i;
		if (array [i] > mpoints) {
			mode = i;
			mpoints = array [i];
		}
		points += array [i];
	}

    
    
	mpoints = points / 2;
	thresh = static_cast <uint64> (breakfracs [ind] * points);

    for (uint32 i = 0; i < bins; i++) {
		cpoints += array [i];
		if (cpoints > mpoints) {
			median = i;
			mpoints = points;
		}

		while (cpoints > thresh) {
			breaks [ind] = i;
			thresh = static_cast <uint64> (points * breakfracs [++ind]);
		}
	}

	DEBUG_LOG (" min    %u   max %u\n", min, max);
	DEBUG_LOG (" median %lld lower_bound %u\n", median, index->lower_bound (median));
	DEBUG_LOG (" mode   %lld lower_bound %u  percentage %.2f\n", mode, 
			index->lower_bound (mode), array [mode] * 100.0 / points);
	DEBUG_LOG (" sum    %lld points %lld mean %.2f\n", sum, points, sum * 1.0 / points);
	DEBUG_LOG (" uf_sum %lld underflow %lld\n", uf_sum, underflow);
	DEBUG_LOG (" of_sum %lld overflow  %lld\n", of_sum, overflow);
	DEBUG_LOG ("  t_sum %lld points    %lld mean %.2f\n", t_sum, t_count, t_sum * 1.0 / t_count);
	DEBUG_LOG (" fraction_percet ");
	for (uint32 i = 0; i < 11; i++) 
		DEBUG_LOG ("%4u ", (uint32) (100 * breakfracs [i] + 0.5));
	DEBUG_LOG ("\n");
	DEBUG_LOG (" fraction_points ");
	for (uint32 i = 0; i < 11; i++) 
		DEBUG_LOG ("%4u ", breaks [i]);
	DEBUG_LOG ("\n");
}

/******* 2D histograms, with various kinds of indexes *******/
/* XXX: not finished yet */
histo_2d_t::histo_2d_t (const string &n, 
	const string &d, 
	base_histo_index_t *xind, 
	base_histo_index_t *yind )
	: base_histo_t (n, d)
{
	xindex = xind;
	xindex = yind;

	num_bins = xind->num_bins * yind->num_bins;
	array = new uint64 [num_bins];
	for (uint64 i = 0; i < num_bins; i++) 
		array [i] = 0;
}

histo_2d_t::~histo_2d_t () {
	delete [] array;
	delete xindex;
	delete yindex;
}

uint32 
histo_2d_t::get_x_bins () { 
	return xindex->get_num_bins (); 
}

uint32 
histo_2d_t::get_y_bins () { 
	return yindex->get_num_bins (); 
}

uint32 
histo_2d_t::get_bins () { 
	return get_x_bins () * get_y_bins (); 
}   

uint32 
histo_2d_t::compose_index (uint32 x, uint64 y) { 
	return get_x_bins () * y + x; 
}

int 
histo_2d_t::get_index (uint32 value_x, uint32 value_y) {
	int x = xindex->get_index (value_x);
	int y = yindex->get_index (value_y);
	int xb = (int) (xindex->num_bins);
	int yb = (int) (yindex->num_bins);
	
	if (x >= 0 && y >= 0 && x < xb && y < yb)
	    return compose_index (xindex->get_index (value_x), 
					yindex->get_index (value_y));
	
	if (x < 0 || y < 0)
		return -1;
	
	return xb * yb;
}

uint64 
histo_2d_t::get_points () {
   	int i, bins = get_bins ();
   	uint64 pt = 0;
   	for (i = 0; i < bins; i++)
	   	pt += array [i];

	return pt;
}

void 
histo_2d_t::inc_total (uint64 count, uint32 value_x, uint32 value_y) {
   	uint32 i = get_index (value_x, value_y);
   	array [i] += count;

	/* update sum */
}

void 
histo_2d_t::print () {
}

void 
histo_2d_t::print_stats () {
}
