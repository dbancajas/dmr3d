/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _COUNTER_H_
#define _COUNTER_H_


/******* basic types for stats purpose: counter, etc *******/

class st_entry_t {
protected:
	string name; 	/* name  */
	string desc; 	/* description */

	// attributes
	bool hidden;
	bool void_zero_entries;
	bool hex_index;
	bool terse_format;
	bool new_stat; // hack to not read from warm checkpoints if new stats

public:
	/* constructor */
	st_entry_t (const string &n, const string &d);

	/* compare if of the same name */
	bool name_as (const string &n);

	/* generate print format string */
	void print_fmt (char *fmt, const string &appendix, bool pFP = false);

/* virtual functions */
	/* set and get */
	virtual void inc_total (uint64 count = 1, uint32 index = 0, 
		uint32 value_y = 0);
	virtual double get_total ();
	virtual void set (uint64 value, uint32 index = 0);
	virtual void clear();	
	/* destructor */
	virtual ~st_entry_t ();
	
	/* print */
	virtual void print () {};

	void set_hidden (bool _hid = true);
	void set_new_stat (bool _new);
	void set_void_zero_entries ();
	void set_hex_index ();
	void set_terse_format ();
    virtual void to_file(FILE *) {};
    virtual void from_file(FILE *) {};

};

/* stats entry for printing ONLY (similar to STATS_FORMULA) */
class st_print_t : public st_entry_t {
private:
	bool print_int; /* whether print uint or float */
	
	int num_params;			/* number of parameters */
	st_entry_t ** array;	/* pointer array to each of the counters */
	double (*ptr) (st_entry_t **);	/* func. ptr to the evaluation routine */

public:
	st_print_t (const string & n, const string & d, 
			int num_param, st_entry_t ** counters, 
			double (*funcPtr) (st_entry_t **), bool pINT = true);

	virtual double get_total ();
	virtual void print ();
};

/******* special case of st_print_t: ratio; only two counters *******/
class ratio_print_t : public st_entry_t {
public:
	st_entry_t * top;
	st_entry_t * bottom;

	ratio_print_t (const string &n, const string &d, 
			st_entry_t * top, st_entry_t * bottom);

	virtual void print();
};

/******* basic counter: e.g., num_instr_committed *******/
class base_counter_t : public st_entry_t {
protected:
	uint64 total;

public:
	/* constructor */
	base_counter_t (const string &n, const string &d);

	/* set and get */
	virtual void inc_total (uint64 count = 1, uint32 index = 0, 
		uint32 value_y = 0);
	virtual double get_total ();
	virtual void set (uint64 value, uint32 index = 0);

	/* print */
	virtual void print ();
	/* clear */
	void clear();
    virtual void to_file(FILE *fl);
    virtual void from_file(FILE *fl);
};

class double_counter_t : public st_entry_t {
protected:
	double total;

public:
	/* constructor */
	double_counter_t (const string &n, const string &d);

	/* set and get */
	virtual void inc_total (double count = 1, uint32 index = 0, 
		uint32 value_y = 0);
	virtual double get_total ();
	virtual void set (double value, uint32 index = 0);

	/* print */
	virtual void print ();
	/* clear */
	void clear();
    virtual void to_file(FILE *fl);
    virtual void from_file(FILE *fl);
};


/******* group of counters: e.g. misses per CPU *******/
class group_counter_t : public st_entry_t {
protected:
	uint64 * array; /* ptr array to base_counter_t entries */
	uint64 num_entries;       /* how many entries in this group */
	
public:
	/* constructor */
	group_counter_t (const string &n, const string &d, uint64 num);

	/* get number of entries */
	uint64 get_num () { return num_entries; };

	/* set and get */
	virtual void inc_total (uint64 count = 1, uint32 index = 0, 
		uint32 value_y = 0);
    virtual void set (uint64 val, uint32 index);
	virtual double get_total();

	/* print */
	virtual void print ();
	/* clear */
	void clear();
    virtual void to_file(FILE *);
    virtual void from_file(FILE *);
};

/******* counters for breakdown: e.g. 4C breakdown *******/
class breakdown_counter_t : public group_counter_t {
private:
	vector<string> name_array;
	vector<string> desc_array;

public:
	/* constructor */
	breakdown_counter_t (const string &n, const string &d, int num, 
			string * names, string * descs);

	/* print */
	virtual void print ();
};

#endif
