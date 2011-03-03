/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _STATS_H_
#define _STATS_H_

/******* The interface to create different stats entries, histograms, etc *******/

class stats_list_entry_t {
public:
	st_entry_t * entry;
	stats_list_entry_t * next;

	stats_list_entry_t ();
	stats_list_entry_t (st_entry_t * ent);

	void print ();
	void clear();
    void to_file(FILE *);
    void from_file(FILE *);
};

/* types of histo indexing schemes */
enum histo_index_type_t {
	HIT_UNIFORM,    /* bin sizes are the same */
	HIT_EXP_2X,     /* bin sizes increase exponentially, 
						by a factor of power of 2 (e.g. 2, 4, 16) */
	HIT_EXP_OTHER,	/* bin sizes increase exponentially */
	HIT_BOUND,		/* bins specified by their upper/lower bounds */
	HIT_NUM, 		/* invalid type */
};

enum counter_type_t {
	CT_BASIC,		/* basic counter, only increment */
    CT_DOUBLE,      /* double counter, increment */
	// CT_RATIO,		/* ratio counter, i.e. IPC; predictors: mis-pred/total */
	CT_GROUP,		/* counter group, i.e. per CPU counter */
	CT_BREAKDOWN,	/* breakdown counter group, i.e. 4C misses */
	CT_NUM,			/* invalid counter type */
};

class stats_t {
protected:
	string name;
	int num;
	stats_list_entry_t * list, * tail;
    bool hidden;

public:
	stats_t ();

	stats_t (const string &n);
    void set_hidden(bool);

	/* destructor */
	~stats_t ();

	/* check name */
	bool check_name (const string &name);

	/* enqueue */
	st_entry_t * enqueue (st_entry_t * ent);
	
	/* create counter */
	st_entry_t * create_counter (const string &name, const string &desc, 
			counter_type_t t,  ... );

	/* create printing formular */
	st_print_t * create_print (const string &n, const string &d, int num, 
		st_entry_t ** c_array, double (*fptr) (st_entry_t **), bool pINT);

	ratio_print_t * create_ratio_print (const string &n, const string &d, 
		st_entry_t * top, st_entry_t *bottom);
		
	/* create histogram */
	base_histo_index_t * create_histo_index (histo_index_type_t type, 
		uint32 num_bins, ... );

	base_histo_t * create_histogram (const string &n, const string &d, 
		base_histo_index_t *xind, base_histo_index_t *yind);
	

	/* print */
	void print ();

	/* clear */
	void clear();
    void to_file(FILE *fl);
    void from_file(FILE *fl);

/******* global functions: used to create different counters/histograms */
base_counter_t * COUNTER_BASIC (const string &NAME, 
	const string &DESC);

double_counter_t * COUNTER_DOUBLE (const string &NAME, 
	const string &DESC);

st_print_t * STAT_PRINT (const string &NAME, const string &DESC, 
				int num, st_entry_t ** ST_ARRAY, 
				double (*FUNC_PTR)(st_entry_t **), bool INT_FORMAT);

ratio_print_t * STAT_RATIO(const string &NAME, const string &DESC, 
					st_entry_t *TOP, st_entry_t *BOTTOM);

group_counter_t * COUNTER_GROUP (const string &NAME, const string &DESC, 
					int NUM);

breakdown_counter_t * COUNTER_BREAKDOWN (const string &N, const string &D,
				        int NUM, string * C_NAME_ARR, string * C_DESC_ARR);

histo_1d_t * HISTO_UNIFORM (const string &NAME, const string &DESC,
		        uint32 BINS, uint32 WIDTH, uint32 START);

histo_1d_t * HISTO_EXP2 (const string &NAME, const string &DESC,
		        uint32 BINS, uint32 BITS, uint32 START);

histo_1d_t * HISTO_EXP (const string &NAME, const string &DESC,
		        uint32 BINS, uint32 FACTOR, uint32 START);

histo_1d_t * HISTO_BOUND(const string &N, const string &D, 
				uint32 BINS, uint32 * LB_ARRAY);
};


void 
STAT_INC (st_entry_t *ctr, uint32 index = 0, 
	uint32 value_y = 0);

void 
STAT_ADD (st_entry_t *ctr, uint32 count, uint32 index = 0, 
	uint32 value_y = 0);

void 
STAT_SET (st_entry_t *ctr, uint32 value, uint32 index = 0, 
	uint32 value_y = 0);

void 
STAT_DEC (st_entry_t *ctr, uint32 index = 0, 
	uint32 value_y = 0);


uint64 
STAT_GET (st_entry_t *ctr);

#endif
