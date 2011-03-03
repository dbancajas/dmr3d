/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _HISTOGRAM_H_
#define _HISTOGRAM_H_

#undef DEBUG_HISTO

/******* histogram: e.g. miss latency or degree of use *******/

//#define PRINT_CDF   /* will only print PDF, not CDF */

/******* base index *******/
class base_histo_index_t {
public:
	uint32 start_value;  /* default is 0 */
	uint32 num_bins;     /* number of bins */

	uint32 get_num_bins ();
	virtual int get_index (uint32 value);
	virtual uint32 lower_bound (uint32 bin);

	/* constructor */
	base_histo_index_t (uint32 num, uint32 start);

	/* print (using the histogram's name and description) */
	virtual void print (const string &n, const string &d);
	
	/* destructor */
	virtual ~base_histo_index_t() {}
};

/******* uniformly bin'ed (of equal width) index *******/
class uniform_histo_index_t : public base_histo_index_t {
public: 
	uint32 width;        /* bin width */

	uniform_histo_index_t (uint32 num, uint32 wid, uint32 start = 0);

	/* get index */
	virtual int get_index (uint32 value);
	virtual uint32 lower_bound (uint32 bin);

	/* print (using the histogram's name and description) */
	virtual void print (const string &n, const string &d) ;
};

/******* (base 2^x) exponential histogram index *******/
class exp2x_histo_index_t : public base_histo_index_t {
public:
	uint32 bits_shift;	/* bits to right-shift */
	
	/* constructor: assume that starts from value 1: [1, 2), [2, 4), [4, 8), etc. */
	exp2x_histo_index_t (uint32 num, uint32 bits = 1, uint32 start = 1);
	virtual int get_index (uint32 value);
	virtual uint32 lower_bound (uint32 bin);

	/* print (using the histogram's name and description) */
	virtual void print (const string &n, const string &d) ;
};

/******* (base of any int) exponential histogram index *******/
class exp_histo_index_t : public base_histo_index_t { 
public:
	uint32 base;
	
	/* constructor */
	exp_histo_index_t (uint32 num, uint32 b, uint32 start = 1);
	virtual int get_index (uint32 value);
	virtual uint32 lower_bound (uint32 bin);

	/* print (using the histogram's name and description) */
	virtual void print (const string &n, const string &d) ;
};

/******* index defined by lower bound array *******/
class bound_histo_index_t : public base_histo_index_t { 
private:
	uint32 * lb_array;  /* lower_bounds */
	
public:
	/* constructor */
	bound_histo_index_t (uint32 num, uint32 * lb_arr);

	/* destructor */
	virtual ~bound_histo_index_t ();

	/* get index */
	virtual int get_index (uint32 value);
	virtual uint32 lower_bound (uint32 bin);

	/* print (using the histogram's name and description) */
	virtual void print (const string &n, const string &d);
};

/******* base histogram class *******/
class base_histo_t : public st_entry_t {
protected:
	/* st_entry_t * array; */
	uint64 * array;
	uint64 sum;
	uint64 underflow, overflow;
	uint64 uf_sum, of_sum;
	uint64 t_sum, t_count;
	uint64 num_bins;

public:
	/* constuctor */
	base_histo_t (const string &n, const string &d);

	/* destructor */
	virtual ~base_histo_t ();

	/* get attributes */
	virtual int get_index (uint32 index, uint32 value_y = 0);
	virtual uint32 get_bins ();

	/* get/set data */
	virtual uint64 get_points ();
	virtual void inc_total (uint64 count = 1, uint32 index = 0, 
		uint32 value_y = 0);

	/* print stats */
	virtual void print ();
	virtual void print_stats ();
	/* clear */
	virtual void clear();
    virtual void to_file(FILE *fl);
    virtual void from_file(FILE *fl);
};

/******* 1D histograms, with various kinds of indexes *******/
class histo_1d_t : public base_histo_t {
private:
	base_histo_index_t * index;

public:
	/* constructor */
	histo_1d_t (const string &n, const string &d, 
		base_histo_index_t *ind);

	/* destructor: will delete the index pointer */
	~histo_1d_t ();

	/* get attributes */
	virtual int get_index (uint32 index);
	virtual uint32 get_bins ();

    /* get/set data */
    virtual uint64 get_points ();
	virtual void inc_total (uint64 count = 1, uint32 index = 0, 
		uint32 value_y = 0);

	void inc (uint32 index, uint64 count, uint32 value_y = 0);

	/* print stats */
   	virtual void print ();

	/* stats: mean, median, etc */
   	virtual void print_stats ();
};

/******* 2D histograms, with various kinds of indexes *******/
class histo_2d_t : public base_histo_t {
private:
	uint64 * array;
	base_histo_index_t * xindex, * yindex;
	
public:
	/* constructor */
	histo_2d_t (const string &n, const string &d, 
			base_histo_index_t *xind, base_histo_index_t *yind);

	/* destructor */
	~histo_2d_t ();
	
	/* get bins */
	virtual uint32 get_x_bins ();
	virtual uint32 get_y_bins ();
	virtual uint32 get_bins ();

	/* compose index from x, y indices: 
	   x11, x12, .. x1n; x21, x22, .. x2n; ...; xk1, xk2, .. xkn */
	uint32 compose_index (uint32 x, uint64 y);
	
	/* get attributes */
   	virtual int get_index (uint32 index, uint32 value_y = 0);

	/* get/set data */
   	virtual uint64 get_points ();
   	virtual void inc_total (uint64 count = 1, uint32 index = 0, 
		uint32 value_y = 0);

	/* print stats */
   	virtual void print ();
 
	/* stats: mean, median, V, stdev */
   	void print_stats ();
};

#endif
