/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* Design: 
 * -------
 *  (1) A single file declares both the compile-time and run-time 
 * 		configuration entries. This file is first included to be 
 * 		macro-extended for compile-time constants; then included 
 * 		so each config-entry can be added into a centralized list.
 * 	
 * 	(2) At runtime, the central parser is used to read runtime 
 * 		values for such configuration entries. 
 *
 * 	(3) Currently only support three config data types: 
 * 		integer (uint8, long, etc), string, and array.
 *
 * Files:
 * ------
 * 	(1) config_def.h:    all configuration entries declared here
 * 	(2) config.runtime:  placed in the simics home directory
 * 	(3) config_macro.h:  pre-defined macros (it includes config_def.h)
 * 	(4) config_extern.h: extern included in definitions.h
 *
 * 	(5) config.h: 		 definitions of config entries, for printing
 * 	(6) config.cc:		 implementation, (also includes config_def.h)
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

const uint32 CONF_BUF_SIZE = 1024;

/******* global constant *******/
enum conf_type_t {
	CT_BOOL,
	CT_INTEGER,
	CT_STRING,
	CT_ARRAY_INT,
	CT_NUM_TYPES
};

const string config_types [] = {
	"BOOL",
	"INTEGER",
	"STRING",
	"ARRAY_INT",
};
	
/******* basic types for configuration: number, string, array, etc *******/
class conf_entry_t {
protected:
	bool  fixed;      /* fixed at compile time ? */
	conf_type_t type; /* type */
	string name; 	  /* name  */
	string desc; 	  /* description */

public:
	/* list pointer */
	conf_entry_t * next;

public:
	/* constructor */
	conf_entry_t (const string &n, const string &d);

	/* destructor */
	virtual ~conf_entry_t ();
	
	/* set description string */
	void set_description (const string &new_desc);

	/* set type */
	void set_type (const conf_type_t new_type);
	conf_type_t get_type (void);

	/* match name */
	bool name_as (const string &n);

	/* is fixed? */
	bool is_fixed ();

	/* type matches */
	bool type_as (const conf_type_t new_type);

	/* generate print format string */
	void print_fmt (char *fmt, const string &appendix);

/* virtual functions */
	/* print */
	virtual void print ();
};

/******* bool config entry class *******/
class conf_bool_t : public conf_entry_t {
private:
	bool value;
	bool * value_ptr;
	
public:
	/* constructor */
	conf_bool_t (const string &n, const string &d, bool fixed, const bool val);
	conf_bool_t (const string &n, const string &d, bool fixed, bool * ptr);
	
	/* desctructor */
	~conf_bool_t ();

	/* set/get value */
	void set_value (bool val);
	bool get_value ();

	/* print */
	virtual void print ();
};

/******* integer config entry class *******/
class conf_integer_t : public conf_entry_t {
private:
	int64 value;
	int64 * value_ptr;
	
public:
	/* constructor */
	conf_integer_t (const string &n, const string &d, bool fixed, const int64 val);
	conf_integer_t (const string &n, const string &d, bool fixed, int64 * ptr);
	
	/* desctructor */
	~conf_integer_t ();

	/* set/get value */
	void set_value (int64 val);
	int64 get_value ();

	/* print */
	virtual void print ();
};

/******* string config entry class *******/
class conf_string_t : public conf_entry_t {
private:
	string value;
	string * value_ptr;

public:
	/* constructor */
	conf_string_t (const string &n, const string &d, bool fixed, const string val);
	conf_string_t (const string &n, const string &d, bool fixed, string * ptr);
	
	/* desctructor */
	~conf_string_t ();

	/* set value */
	void set_value (const string &val);
	string get_value ();

	/* print */
	virtual void print ();
};

/******* integer array config entry class *******/
class conf_int_array_t : public conf_entry_t {
private:
	int size;
	int64 * value;
	int64 * value_ptr;

public:
	/* constructor */
	conf_int_array_t (const string &n, const string &d, bool fixed, int size, ...);
	conf_int_array_t (const string &n, const string &d, bool fixed, int size, int64 * value_ptr);
	
	/* desctructor */
	~conf_int_array_t ();

	/* get size */
	int get_size ();

	/* set value */
	void set_value (bool fixed, int index, int64 val);
    
    void set_value (const char *val);

	/* print */
	virtual void print ();
};

/******* helper class for parsing runtime config *******/
class config_t;  // forward declaration

class conf_parser_t {
private:
	string filename;
	string base_location;


    FILE * f;
    int  pos;
	int  row;

    char buf [CONF_BUF_SIZE];
    char key [CONF_BUF_SIZE];
	char val [CONF_BUF_SIZE];

public:
	vector <string> includes;

    conf_parser_t (const string &fn);
    ~conf_parser_t ();

	/* helpers */
    bool match_symbol (char c, bool allow_space = false);
    char is_delimiter ();
    int  skip_spaces ();
    int  next_space (bool count_delimiter = false);

	/* get key or value */
    bool get_key ();
	bool get_bool_value ();
	bool get_int_value ();
	bool get_str_value ();

	/* parser a line of given type after the config name is obtained */
	void parse_runtime_version (config_t *conf);
	void parse_bool (config_t *conf);
	void parse_integer (config_t *conf);
	void parse_inc_integer (config_t *conf);
	void parse_string (config_t *conf);
	void parse_int_array (config_t *conf);
	void parse_include (config_t *conf);

	/* main entry of parsing */
    void parse (config_t * conf);

	void get_base_location ();
};


/******* centralized list of config entries *******/
class config_t {
protected:
	int num;
	conf_entry_t * list, * tail;
	string conf_runtime_version;

/* Singleton: class wide instance */
public:

	config_t ();
	~config_t ();

	/* enqueue */
	void enqueue (conf_entry_t * ent);
	
	/* create config_entry_t */
	conf_bool_t * addconf_bool (const string &n, const string &d, 
			bool fixed, bool val);
	conf_bool_t * addconf_bool (const string &n, const string &d, 
			bool fixed, bool * val_ptr);

	conf_integer_t * addconf_integer (const string &n, const string &d, 
			bool fixed, int64 val);
	conf_integer_t * addconf_integer (const string &n, const string &d, 
			bool fixed, int64 * val_ptr);

	conf_string_t * addconf_string (const string &n, const string &d, 
			bool fixed, const string & val);
	conf_string_t * addconf_string (const string &n, const string &d, 
			bool fixed, string * val_ptr);

	conf_int_array_t * addconf_int_array (const string &n, const string &d, 
			bool fixed, int size, ...);
	conf_int_array_t * addconf_int_array (const string &n, const string &d, 
			bool fixed, int size, int64 * val);

	/* whether the config is runtime changeable, returns NULL if not */
	conf_entry_t * runtime_changeable(const char *name, const conf_type_t t,
			char *reason);
	
	/* register all config entries */
	void register_entries ();

	/* print all config values */
	void print ();
	
	/* runtime value */
	void parse_runtime (const string& filename);

	/* set runtime value */
	bool set_runtime_value (const char *name, const char *val, char *reason);
	/* set runtime value */

	string get_value (const string & name);
	/* set runtime value */
	conf_entry_t * find (const string & name);
};


#endif
