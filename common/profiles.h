/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: profiles.h,v 1.1.2.1 2005/08/30 17:05:03 kchak Exp $
 *
 * description:    Main class for code profiling
 * initial author: Koushik Chakraborty 
 *
 */
 
 
#ifndef _PROFILES_H_
#define _PROFILES_H_

   
/******* The interface to create different stats entries, histograms, etc *******/

class profile_entry_t {
	protected:
	
	string name;
	string desc;
	stats_t *stats;
	
	public:
	profile_entry_t(string name);
	bool name_as(const string &n);
	string get_name();
    const char *get_cname();
	/* Print Function */
	virtual void print_stats();
	virtual ~profile_entry_t();
	/* Clear function */
	void clear_stats();
    
    virtual void to_file(FILE *f) {};
    virtual void from_file(FILE *f) {};
    //virtual void update_profile(void *ptr, profile_type p) {};
    
    virtual void cycle_end() {}
    
	
};

class profiles_list_entry_t {
public:
	profile_entry_t * entry;
	profiles_list_entry_t * next;

	profiles_list_entry_t ();
	profiles_list_entry_t (profile_entry_t * ent);

	void print_stats();
	void clear_stats();
};



class profiles_t {
protected:
	string name;
	uint32 num;
	profiles_list_entry_t * list, * tail;

public:
	profiles_t ();

	profiles_t (const string &n);

	/* destructor */
	~profiles_t ();

	/* check name */
	bool check_name (const string &name);

	/* enqueue */
	profile_entry_t * enqueue (profile_entry_t * ent);
    //void update_profile(void *ptr, profile_type type);
	
	

	/* print */
	void print ();

	/* clear */
	void clear();
    void to_file(FILE *f);
    void from_file(FILE *f);
    void cycle_end();

};


// List of individual profiles

#endif
