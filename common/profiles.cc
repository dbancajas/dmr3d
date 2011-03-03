/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: profiles.cc,v 1.1.2.3 2006/01/30 16:46:58 kchak Exp $");

#include "definitions.h"
#include "counter.h"
#include "histogram.h"
#include "stats.h"
#include "profiles.h"


/******* Implementation of profile_entry_t **********/
profile_entry_t::profile_entry_t(string _name) :
	name(_name)
{
	stats = new stats_t(name);
}

void
profile_entry_t::print_stats() {
	if (stats) stats->print();
}

void
profile_entry_t::clear_stats() {
	if (stats) stats->clear();
}

profile_entry_t::~profile_entry_t() {
}

/* compare if of the same name */
bool 
profile_entry_t::name_as(const string &n) {
	return name == n;
}

const char *
profile_entry_t::get_cname() {
    return name.c_str();
}

string
profile_entry_t::get_name() {
	return name;
}

/******* Implementation of profiles_list_entry_t *******/
inline profiles_list_entry_t::profiles_list_entry_t () {
	next = NULL;
}

inline profiles_list_entry_t::profiles_list_entry_t (profile_entry_t *ent) {
	entry = ent;
	next = NULL;
}
inline void
profiles_list_entry_t::clear_stats() {
	entry->clear_stats();
}

inline void 
profiles_list_entry_t::print_stats() {
	entry->print_stats();	
}


/******* The interface to create different profiles entries, histograms, etc *******/

/* constructor */
profiles_t::profiles_t () { 
	num = 0; list = NULL; tail = NULL;
}

/* constructor, with name */
profiles_t::profiles_t (const string &n) { 
	num = 0; list = NULL; tail = NULL;
	name = n;
}

/* destructor */
profiles_t::~profiles_t () { 
	for (uint32 i = 0; i < num; i++) {
		profiles_list_entry_t * n = list->next;
		delete list->entry;
		delete list;
		list = n;
	}
}

/* enqueue */
profile_entry_t * 
profiles_t::enqueue (profile_entry_t * ent) {
	if (list == NULL) {

		list = new profiles_list_entry_t (ent);
		tail = list;
	} else {
		profiles_list_entry_t * t = new profiles_list_entry_t (ent);
		tail->next = t;
		tail = t;
	}
	num++;
	return ent;
}


/* check to see if entries with the same name exist */
bool 
profiles_t::check_name (const string &name) {
	/* check if duplicated name exists */
	profiles_list_entry_t * p = list;
	while (p != NULL) {
		if (p->entry->name_as (name)) {
			DEBUG_OUT("profiles_t: name \"%s\" already exists!\n", 
					name.c_str());
			return true;
		}
		p= p->next;
	}
	return false;
}

void
profiles_t::clear() {
	if (list == NULL) return;
	profiles_list_entry_t *t=list;
	while (t != tail->next) {
		t->clear_stats();
		t = t->next;
	}
}


/* print */
void profiles_t::print () {
	if (list == NULL) return;
	profiles_list_entry_t * t = list;
	PRINT_LINE;
	PRINT_MARK;
	DEBUG_LOG ("profiles_t %s\n", name.c_str ());
	while (t != tail->next) {
		t->print_stats();
		/* PRINT_MARK; */
		t = t->next;
	}
}


void profiles_t::to_file(FILE *f) {
    if (list == NULL) return;
    profiles_list_entry_t *t = list;
    while (t != tail->next) {
        t->entry->to_file(f);
        t = t->next;
    }
    
}


void profiles_t::from_file(FILE *f) {
    if (list == NULL) return;
    profiles_list_entry_t *t = list;
    while (t != tail->next) {
        t->entry->from_file(f);
        t = t->next;
    }
    
}

void profiles_t::cycle_end () {
	if (list == NULL) return;
    profiles_list_entry_t *t = list;
    while (t != tail->next) {
        t->entry->cycle_end();
        t = t->next;
    }
    
}
