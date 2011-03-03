/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: stats.cc,v 1.3.10.4 2006/01/30 16:46:58 kchak Exp $");

#include "definitions.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"

void 
STAT_INC (st_entry_t *ctr, 
	uint32 index, uint32 value_y) {

	if (ctr) 
		STAT_ADD (ctr, 1, index, value_y);
}

void 
STAT_ADD (st_entry_t *ctr, uint32 count, 
	uint32 index, uint32 value_y) {
	
	if (ctr) 
		ctr->inc_total (count, index, value_y);
}

void
STAT_DEC (st_entry_t *ctr, 
	uint32 index, uint32 value_y) {

	if (ctr) 
		STAT_SET (ctr, STAT_GET (ctr) - 1, index, value_y);
}

void 
STAT_SET (st_entry_t *ctr, uint32 value, 
	uint32 index, uint32 value_y) {

	if (ctr)
		ctr->set (value);
}

uint64
STAT_GET (st_entry_t *ctr) {

	if (ctr) 
		return ((uint64) ctr->get_total ());
	else
		return 0;
}


/******* Implementation of stats_list_entry_t *******/
inline stats_list_entry_t::stats_list_entry_t () {
	next = NULL;
}

inline stats_list_entry_t::stats_list_entry_t (st_entry_t *ent) {
	entry = ent;
	next = NULL;
}
inline void
stats_list_entry_t::clear() {
	entry->clear();
}

inline void
stats_list_entry_t::from_file(FILE *fl) {
    entry->from_file(fl);
}

inline void
stats_list_entry_t::to_file(FILE *fl) {
    entry->to_file(fl);
}

inline void 
stats_list_entry_t::print () {
	entry->print ();	
}

/******* The interface to create different stats entries, histograms, etc *******/

/* constructor */
stats_t::stats_t () { 
    hidden = false;
	num = 0; list = NULL; tail = NULL;
}

/* constructor, with name */
stats_t::stats_t (const string &n) { 
    hidden = false;
	num = 0; list = NULL; tail = NULL;
	name = n;
}

/* destructor */
stats_t::~stats_t () { 
	for (int i = 0; i < num; i++) {
		stats_list_entry_t * n = list->next;
		delete list->entry;
		delete list;
		list = n;
	}
}

/* enqueue */
st_entry_t * 
stats_t::enqueue (st_entry_t * ent) {
	if (list == NULL) {

		list = new stats_list_entry_t (ent);
		tail = list;
	} else {
		stats_list_entry_t * t = new stats_list_entry_t (ent);
		tail->next = t;
		tail = t;
	}
	num ++;
	return ent;
}

/* check to see if entries with the same name exist */
bool 
stats_t::check_name (const string &name) {
	/* check if duplicated name exists */
	stats_list_entry_t * p = list;
	while (p != NULL) {
		if (p->entry->name_as (name)) {
			DEBUG_OUT("stats_t: name \"%s\" already exists!\n", 
					name.c_str());
			return true;
		}
		p= p->next;
	}
	return false;
}
	
/* create a counter */
st_entry_t * 
stats_t::create_counter (const string &name, 
	const string &desc, 
	counter_type_t type, ... ) 
{
	st_entry_t * ptr = NULL;

	if (check_name (name)) return NULL; /* check */

   	va_list ap;
	va_start (ap, type);

	int num_components;
	string *names, *descs;

	switch (type) {
		case CT_BASIC:
			ptr = new base_counter_t (name, desc);
			break;
        case CT_DOUBLE:
            ptr = new double_counter_t (name, desc);
            break;
		case CT_GROUP:
   			num_components = va_arg (ap, uint32);
			ptr = new group_counter_t (name, desc, (uint64)num_components);
			break;
		case CT_BREAKDOWN:
			num_components = va_arg (ap, uint32);
			names = va_arg (ap, string *);
			descs = va_arg (ap, string *);
			ptr = new breakdown_counter_t (name, desc, 
					(uint64)num_components, names, descs);
			break;
		case CT_NUM:
		default:
			break;
	}

   	va_end (ap);

	/* enqueue */
	if (ptr) enqueue (ptr);

	return ptr;
}

base_histo_t *
stats_t::create_histogram (const string &n, 
	const string &d, 
	base_histo_index_t * xind, 
	base_histo_index_t * yind)
{
	/* check */
	if (check_name (n)) {
		if (yind != NULL)
			delete yind;
		if (xind != NULL)
			delete xind;

		return NULL; 
	}

	if (yind == NULL) { /* create 1d histogram */
		histo_1d_t * h = new histo_1d_t (n, d, xind);
		enqueue (h);
		return h;
	} else {
		histo_2d_t * h = new histo_2d_t (n, d, xind, yind);
		enqueue (h);
		return h;
	}

	return NULL;
}

/* create printing formular */
st_print_t *
stats_t::create_print (const string &n, 
	const string &d, 
	int num, 
	st_entry_t ** c_array, 
	double (*fptr) (st_entry_t **), 
	bool pINT) 
{
	if (check_name (n)) return NULL; /* check */

	st_print_t * ret = new st_print_t (n, d, num, c_array, fptr, pINT);
	if (ret) enqueue (ret);
	return ret;
}
	
/* create ratio print */
ratio_print_t *
stats_t::create_ratio_print (const string &n, 
	const string &d,
	st_entry_t * top,
	st_entry_t * bottom) 
{
	if (check_name (n)) return NULL; /* check */

	ratio_print_t *ret = new ratio_print_t (n, d, top, bottom);
	if (ret) enqueue(ret);
	return ret;
}

/* create histogram index */
base_histo_index_t * 
stats_t::create_histo_index (histo_index_type_t type, 
	uint32 num_bins, ... ) 
{
	base_histo_index_t * ptr = NULL;
	
	uint32 start, width, bits, base;
	uint32 * lb_array;
	
   	va_list ap;
	va_start (ap, num_bins);

	switch (type) { 
		case HIT_UNIFORM:
			width = va_arg (ap, uint32);
   			start = va_arg (ap, uint32);
			ptr = new uniform_histo_index_t (num_bins, width, start);
			break;
		case HIT_EXP_2X:
			bits  = va_arg (ap, uint32);
   			start = va_arg (ap, uint32);
			ptr = new exp2x_histo_index_t (num_bins, bits, start);
			break;
		case HIT_EXP_OTHER:
			base  = va_arg (ap, uint32);
   			start = va_arg (ap, uint32);
			ptr = new exp_histo_index_t (num_bins, base, start);
			break;
		case HIT_BOUND:
			lb_array = va_arg (ap, uint32*);
			ptr = new bound_histo_index_t (num_bins, lb_array);
			break;
		case HIT_NUM:
		default:
			break;
	}

	return ptr;
}

void
stats_t::clear() {
	if (list == NULL) return;
	stats_list_entry_t *t=list;
	while (t != tail->next) {
		t->clear();
		t = t->next;
	}
}

void stats_t::set_hidden(bool flag) {
    hidden = flag;
}

/* print */
void stats_t::print () {
	if (list == NULL || hidden) return;
	stats_list_entry_t * t = list;
	PRINT_LINE;
	PRINT_MARK;
	DEBUG_LOG ("stats_t %s\n", name.c_str ());
	while (t != tail->next) {
		t->print ();
		/* PRINT_MARK; */
		t = t->next;
	}
    DEBUG_FLUSH();
}

void stats_t::to_file(FILE *fl) {
    if (list == NULL) return;
    stats_list_entry_t *t = list;
    while (t != tail->next) {
        t->to_file(fl);
        t = t->next;
    }
}

void stats_t::from_file(FILE *fl) {
    if (list == NULL) return;
    stats_list_entry_t *t = list;
    while (t != tail->next) {
        t->from_file(fl);
        t = t->next;
    }
}

/******* global functions to create different counters/histograms */
base_counter_t *
stats_t::COUNTER_BASIC (const string &NAME, const string &DESC)
{
	return dynamic_cast<base_counter_t*>
		(create_counter (NAME, DESC, CT_BASIC));
}

double_counter_t *
stats_t::COUNTER_DOUBLE (const string &NAME, const string &DESC)
{
	return dynamic_cast<double_counter_t*>
		(create_counter (NAME, DESC, CT_DOUBLE));
}

st_print_t * 
stats_t::STAT_PRINT (const string &NAME, const string &DESC,    
		int NUM, st_entry_t ** ST_ARRAY, 
		double (*FUNC_PTR)(st_entry_t **), bool INT_FORMAT) 
{
	return create_print (NAME, DESC, 
		NUM, ST_ARRAY, FUNC_PTR, INT_FORMAT);
}

ratio_print_t *
stats_t::STAT_RATIO (const string &NAME, const string &DESC, 
		st_entry_t *TOP, st_entry_t *BOTTOM) 
{
	return create_ratio_print (NAME, DESC, TOP, BOTTOM);
}

group_counter_t *
stats_t::COUNTER_GROUP (const string &NAME, const string &DESC, int NUM) 
{
	return dynamic_cast<group_counter_t*>
		(create_counter (NAME, DESC, CT_GROUP, NUM));
}

breakdown_counter_t *
stats_t::COUNTER_BREAKDOWN (const string &NAME, const string &DESC,
		int NUM, string * C_NAME_ARR, string * C_DESC_ARR) 
{
	return dynamic_cast<breakdown_counter_t*>
		(create_counter (NAME, DESC, CT_BREAKDOWN,
		        NUM, C_NAME_ARR, C_DESC_ARR));
}

histo_1d_t * 
stats_t::HISTO_UNIFORM (const string & NAME, const string & DESC, 
		uint32 BINS, uint32 WID, uint32 START)
{
	uniform_histo_index_t * IND_PTR = dynamic_cast<uniform_histo_index_t*> 
		(create_histo_index (HIT_UNIFORM, BINS, WID, START));
	
	return dynamic_cast<histo_1d_t*>
		(create_histogram (NAME, DESC, IND_PTR, NULL));
}
		
histo_1d_t * 
stats_t::HISTO_EXP2 (const string & NAME, const string & DESC, 
		uint32 BINS, uint32 BITS, uint32 START)
{
	exp2x_histo_index_t * IND_PTR = dynamic_cast<exp2x_histo_index_t*>
		(create_histo_index (HIT_EXP_2X, BINS, BITS, START));
	
	return dynamic_cast<histo_1d_t*>
		(create_histogram (NAME, DESC, IND_PTR, NULL));
}
	    
histo_1d_t * 
stats_t::HISTO_EXP (const string & NAME, const string & DESC, 
		uint32 BINS, uint32 FAC, uint32 ST) 
{
	exp_histo_index_t * IND_PTR = dynamic_cast<exp_histo_index_t*>
		(create_histo_index (HIT_EXP_OTHER, BINS, FAC, ST));
	
	return dynamic_cast<histo_1d_t*>
		(create_histogram (NAME, DESC, IND_PTR, NULL));
	    
}

histo_1d_t * 
stats_t::HISTO_BOUND (const string &N, const string &D, uint32 BINS, uint32 * LB_ARRAY) 
{
	bound_histo_index_t * IND_PTR = dynamic_cast<bound_histo_index_t*>
		(create_histo_index (HIT_BOUND, BINS, LB_ARRAY));

	return dynamic_cast<histo_1d_t*>
		(create_histogram (N, D, IND_PTR, NULL));
}
