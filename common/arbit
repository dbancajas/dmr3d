/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: config.cc,v 1.1.1.1.12.5 2006/12/12 18:36:56 pwells Exp $");

#include "definitions.h"
#include "config.h"
#include "config_macro.h"

#define PARSER_PRINT DEBUG_LOG("WARNING: config file \"%s\", row %d, col %d. ", filename.c_str(), row, pos+1); FAIL;

/******* conf_entry_t implementation *******/
conf_entry_t::conf_entry_t (const string &n, const string &d) {
	fixed = true;
	next = NULL;
	type = CT_INTEGER;
	name = string(n);
	desc = string(d);
}

void 
conf_entry_t::set_description (const string &new_desc) {
	desc = string(new_desc);
}

void conf_entry_t::set_type (const conf_type_t new_type) {
	type = new_type;
}

bool
conf_entry_t::is_fixed () {
	return fixed;
}

bool 
conf_entry_t::type_as (const conf_type_t new_type) {
	return (new_type == type);
}

conf_type_t
conf_entry_t::get_type (void) {
	return type;
}

bool 
conf_entry_t::name_as (const string &n) {
	return (n == name);
}

void 
conf_entry_t::print_fmt (char *fmt, const string &appendix) {
	char buf [128];
	
	int len = appendix.size();
	if (len) 
		 sprintf (buf, "%%%llds%s", g_conf_name_width - len, appendix.c_str());
	else sprintf (buf, "%%%llds", g_conf_name_width);

	switch (type) {
		case CT_BOOL:
			sprintf (fmt, "%s %%%llds %%%llds\n", buf, g_conf_value_width, g_conf_desc_width);
			break;
		case CT_INTEGER:
			sprintf (fmt, "%s %%%lldlld %%%llds\n", buf, g_conf_value_width, g_conf_desc_width);
			break;
		case CT_STRING:
			sprintf (fmt, "%s %%%llds %%%llds\n", buf, g_conf_value_width, g_conf_desc_width);
			break;
		case CT_ARRAY_INT:
			/* first line print array size, second line print the elements */
			sprintf (fmt, "%s %%%lldd %%%llds\n", buf, g_conf_value_width, g_conf_desc_width);
			break;
		default:
			DEBUG_LOG("Unknown configuration type: %d\n", type);
			break;
	}
}

conf_entry_t::~conf_entry_t () {
	/* nothing */
}

void 
conf_entry_t::print () {
	/* nothing */
}

/******* conf_bool_t implementation *******/
conf_bool_t::conf_bool_t (const string &n, const string &d, 
		bool compile_fixed, const bool val) : conf_entry_t (n, d) 
{
	type = CT_BOOL;
	ASSERT(compile_fixed);
   	fixed = compile_fixed;
   	value = val;
}

conf_bool_t::conf_bool_t (const string &n, const string &d, 
		bool compile_fixed, bool * ptr) : conf_entry_t (n, d) 
{
	type = CT_BOOL;
	ASSERT(!compile_fixed);
   	fixed = compile_fixed;
   	value_ptr = ptr;
}

conf_bool_t::~conf_bool_t () {
	/* nothing */
}

void 
conf_bool_t::print () {
	char fmt [256];
	print_fmt (fmt, "");
	if (fixed) {
		if (value)
		   	DEBUG_LOG(fmt, name.c_str(), "true", desc.c_str());
		else DEBUG_LOG(fmt, name.c_str(), "false", desc.c_str());
	} else {
		if (*value_ptr)
			DEBUG_LOG(fmt, name.c_str(), "true", desc.c_str());
		else DEBUG_LOG(fmt, name.c_str(), "false", desc.c_str());
	}	 
}

void 
conf_bool_t::set_value (bool val) {
	if (!fixed) 
		(*value_ptr) = val;
	else { 
		DEBUG_LOG("WARNING: trying to change compile-time fixed value.\n");
		print ();
	}
}

bool
conf_bool_t::get_value () {
	return *value_ptr;
}

/******* conf_integer_t implementation *******/
conf_integer_t::conf_integer_t (const string &n, const string &d, 
		bool compile_fixed, const int64 val) : conf_entry_t (n, d) 
{
	ASSERT(compile_fixed);
   	fixed = compile_fixed;
   	value = val;
}

conf_integer_t::conf_integer_t (const string &n, const string &d, 
		bool compile_fixed, int64 * ptr) : conf_entry_t (n, d) 
{
	ASSERT(!compile_fixed);
   	fixed = compile_fixed;
   	value_ptr = ptr;
}

conf_integer_t::~conf_integer_t () {
	/* nothing */
}

void 
conf_integer_t::print () {
	char fmt [256];
	print_fmt (fmt, "");
	if (fixed)
		 DEBUG_LOG(fmt, name.c_str(), value, desc.c_str());
	else DEBUG_LOG(fmt, name.c_str(), (*value_ptr), desc.c_str());
}

void 
conf_integer_t::set_value (int64 val) {
	if (!fixed) 
		(*value_ptr) = val;
	else { 
		DEBUG_LOG("WARNING: trying to change compile-time fixed value.\n");
		print ();
	}
}

int64
conf_integer_t::get_value () {
	return *value_ptr;
}

/******* conf_string_t implementation *******/
conf_string_t::conf_string_t (const string &n, const string &d, 
		bool compile_fixed, const string val) : conf_entry_t (n, d) 
{
	type = CT_STRING;
	ASSERT(compile_fixed);
   	fixed = compile_fixed;
   	value = val;
}

conf_string_t::conf_string_t (const string &n, const string &d, 
		bool compile_fixed, string * ptr) : conf_entry_t (n, d) 
{
	type = CT_STRING;
	ASSERT(!compile_fixed);
   	fixed = compile_fixed;
   	value_ptr = ptr;
}

conf_string_t::~conf_string_t () {
	/* nothing */
}

void 
conf_string_t::print () {
	char fmt [256];
	print_fmt (fmt, "");
	if (fixed)
		 DEBUG_LOG(fmt, name.c_str(), value.c_str(), desc.c_str());
	else DEBUG_LOG(fmt, name.c_str(), value_ptr->c_str(), desc.c_str());
}

void 
conf_string_t::set_value (const string &val) {
	if (!fixed) 
		(*value_ptr) = val;
	else { 
		DEBUG_LOG("WARNING: trying to change compile-time fixed value.\n");
		print ();
	}
}

string 
conf_string_t::get_value () {
	return (*value_ptr);
}

/******* conf_int_array_t implementation *******/
conf_int_array_t::conf_int_array_t (const string &n, const string &d, 
	bool compile_fixed, int p_size, ...)
	: conf_entry_t (n, d) 
{
	size = p_size;
	ASSERT (size > 0);
	type = CT_ARRAY_INT;
	ASSERT(compile_fixed);
   	fixed = compile_fixed;
   	value = new int64 [p_size];
	if (value == NULL) {
		DEBUG_LOG("ERROR: can't allocate memory for int_array \"%s\"", 
				n.c_str());
	} 
}

conf_int_array_t::conf_int_array_t (const string &n, const string &d, 
	bool compile_fixed, int size, int64 * ptr) 
	: conf_entry_t (n, d) 
{
	this->size = size;
	ASSERT (size > 0);
	type = CT_ARRAY_INT;
	ASSERT(!compile_fixed);
   	fixed = compile_fixed;
   	value_ptr = ptr;
}

conf_int_array_t::~conf_int_array_t () {
	/* nothing */
}

void 
conf_int_array_t::print () {
	char fmt [256];
	print_fmt (fmt, "");
	DEBUG_LOG(fmt, name.c_str(), size, desc.c_str());
	
	DEBUG_LOG("  [ ");
	for (int i = 0; i < size; i++) {
		if (fixed)
			 DEBUG_LOG("%lld ", value[i]);
		else DEBUG_LOG("%lld ", value_ptr[i]);
	}
	DEBUG_LOG("]  \n");
}

int
conf_int_array_t::get_size () {
	return size;
}

void 
conf_int_array_t::set_value (bool fixed, int index, int64 val) {
	if (index >= 0 && index < size) {
		if (fixed)
		   	 value [index] = val;
		else value_ptr [index] = val;
	}
	else DEBUG_LOG("WARNING: runtime config value out of array bound.\n");
}


void conf_int_array_t::set_value (const char *val)
{
    string a = string(val);
    int c;
    // Remove space
    string space = " ";
	while ((c = a.find(space, 0)) != -1)
	{
		a.replace(c, 1, "");
	}

	string comma = ",";
    uint32 index = 0;
    bool first = true;
	int passed_val;
	string num_str;
	while ((c = a.find(comma, 0)) != -1)
	{
		num_str = a.substr(0, c);
		passed_val = atoi(num_str.c_str());
        if (passed_val < 0) {
            DEBUG_LOG("Wrong Value passed as integer");
            FAIL;
        }
        
		if (first) first = false;
        else set_value(false, index++, passed_val) ;
		a.replace(0, c + 1, "");
	}
    
	passed_val = atoi(a.c_str());
	if (passed_val < 0) {
        DEBUG_LOG("Wrong Value passed as integer");
        FAIL;
    }
    set_value(false, index, passed_val);    
    
    
}
/* constructor */
config_t::config_t () {
	num = 0;
	list = tail = NULL;
	conf_runtime_version = "N/A";
}

/* desctructor */
config_t::~config_t () {
	for (int i = 0; i < num; i++) {
		conf_entry_t *n = list->next;
		delete list;
		list = n;
	}
}

/* enquene one config entry */
void
config_t::enqueue (conf_entry_t * ent) {
	if (list == NULL) {
		list = ent;
		tail = list;
	} else {
		tail->next = ent;
		tail = ent;
	}

	num ++;
}

void 
config_t::print () {
	char buf [64];
	conf_entry_t * t = list;
	
	PRINT_LINE;
	PRINT_MARK;
	sprintf(buf, "%%%llds %%%llds %%s\n", g_conf_name_width, g_conf_value_width-2);
	DEBUG_LOG(buf, "COMPILE_TIME_VERSION", "", g_conf_compile_time_version.c_str());
	DEBUG_LOG(buf, "RUNTIME_VERSION", "", conf_runtime_version.c_str());
	PRINT_LINE;

	if (t == NULL)
		return;

	while (t != tail->next) {
		t->print ();
		t = t->next;
	}
    
    DEBUG_FLUSH();
}

/* test to see if the given conf variable is runtime changeable */
conf_entry_t * 
config_t::runtime_changeable (const char *name, const conf_type_t tp, 
	char *reason) {
	
	conf_entry_t * t = list;
	
   	if (t == NULL) {
		strcpy (reason, "Config not found");
	   	return t;
	}

	while (t != tail->next) {
		if (t->name_as (name)) {
			if (t->is_fixed ()) {
				strcpy(reason, "Fixed at compile time");
				return NULL;
			}
			/* allow for type placeholder CT_NUM_TYPES, return the first match */
			if ((tp != CT_NUM_TYPES) && !(t->type_as (tp))) {
				strcpy (reason, "Config data type mismatch");
				return NULL;
			}
			return t;
		}
	   	t = t->next;
   	}

	strcpy (reason, "Config not found");
	return NULL;
}

/* search and return given conf variable */
conf_entry_t * 
config_t::find (const string & name) {
	
	conf_entry_t * t = list;
	
   	if (t == NULL)
	   	return NULL;

	while (t != tail->next) {
		if (t->name_as (name.c_str())) {
			return t;
		}
	   	t = t->next;
   	}

	return NULL;
}

/***** compile-time registration *****/
void 
config_t::register_entries () {

#define CONF_C_BOOL(name, desc, value) \
	addconf_bool((#name), (desc), true, (bool)(value));
#define CONF_V_BOOL(name, desc, value) \
	addconf_bool((#name), (desc), false, (bool*)(&g_conf_ ## name));
	
#define CONF_C_INTEGER(name, desc, value) \
	addconf_integer((#name), (desc), true, (int64)(value));
#define CONF_V_INTEGER(name, desc, value) \
	addconf_integer((#name), (desc), false, (int64*)(&g_conf_ ## name));

#define CONF_C_STRING(name, desc, value) \
	addconf_string((#name), (desc), true, (value));
#define CONF_V_STRING(name, desc, value) \
	addconf_string((#name), (desc), false, (string*)(&g_conf_ ## name));
	
#define CONF_C_ARRAY_INT(name, desc, size, values...) \
	addconf_int_array((#name), (desc), true, size, values);
#define CONF_V_ARRAY_INT(name, desc, size, values...) \
	addconf_int_array((#name), (desc), false, size, (int64*)(&g_conf_ ## name));

#define CONF_COMPILE_V(long_string) ;

#include "config_params.h"

#undef CONF_C_BOOL
#undef CONF_V_BOOL
#undef CONF_C_INTEGER
#undef CONF_V_INTEGER
#undef CONF_C_STRING
#undef CONF_V_STRING
#undef CONF_C_ARRAY_INT
#undef CONF_V_ARRAY_INT
#undef CONF_COMPILE_V
}

/* read the runtime config file, and set runtime config values */
void
config_t::parse_runtime (const string & fn) {
	conf_parser_t * parser = new conf_parser_t (fn);
	parser->parse (this);

	for (uint32 i = 0; i < parser->includes.size (); i++) 
		parse_runtime (parser->includes [i]);
		
	delete parser;
}

/* set runtime value for given param name, return reason if failed */
bool 
config_t::set_runtime_value (const char *param, 
		const char *val, char *reason) {
	/* assume no name conflict, using generic type to match */
	conf_entry_t * ent;
   	ent = runtime_changeable (param, CT_NUM_TYPES, reason); 
   	if (ent == NULL) {
		strcpy (reason, "Config entry not found");
		return false;
	}
	
	/* parse value according to config_type */
   	conf_type_t type = ent->get_type ();
	bool ret = true;
	conf_bool_t * bool_ptr;
	bool vbool;
	conf_integer_t * int_ptr;
	int64 vint;
	conf_string_t * str_ptr;
    conf_int_array_t *array_int_ptr;
	
   	switch (type) {
	   	case CT_BOOL:
			bool_ptr = dynamic_cast<conf_bool_t*>(ent);
			vbool = (val [0] == 't');
			if ( strcmp (val, "true") && strcmp (val, "false") ) {
			   	strcpy (reason, 
						"Boolean parameter must be \'true\' or \'false\'.\n");
			   	ret = false;
				break;
		   	}
			bool_ptr->set_value (vbool);
		   	break;
	   	case CT_INTEGER: 
			/* TODO: add sanity checking for integer */
			int_ptr = dynamic_cast<conf_integer_t*>(ent);
		   	vint = atoll (val);
		   	int_ptr->set_value (vint);
		   	break;
	   	case CT_STRING:
			str_ptr = dynamic_cast<conf_string_t*>(ent);
			str_ptr->set_value (val);
		   	break;
        case CT_ARRAY_INT:
            array_int_ptr = dynamic_cast<conf_int_array_t *>(ent);
            array_int_ptr->set_value(val);
            break;
		default:
			sprintf (reason, "Config type (%d) not supported yet ", type);
			ret = false;
	}

	return ret;
}

/* get runtime value for given param name as string */
string 
config_t::get_value (const string & param) {
	/* assume no name conflict, using generic type to match */
	conf_entry_t * ent;
   	ent = find (param); 
   	if (ent == NULL) {
		return "";
	}
	
	/* parse value according to config_type */
   	conf_type_t type = ent->get_type ();
	conf_bool_t * bool_ptr;
	conf_integer_t * int_ptr;
	conf_string_t * str_ptr;
    conf_int_array_t *array_int_ptr;
	
	char char_arr[256];
	
   	switch (type) {
	   	case CT_BOOL:
			bool_ptr = dynamic_cast<conf_bool_t*>(ent);
			return (bool_ptr->get_value() ? "true" : "false");
	   	case CT_INTEGER: 
			int_ptr = dynamic_cast<conf_integer_t*>(ent);
			snprintf(char_arr, 255, "%lld", int_ptr->get_value()); 
		   	return string(char_arr);
	   	case CT_STRING:
			str_ptr = dynamic_cast<conf_string_t*>(ent);
			return str_ptr->get_value();
        case CT_ARRAY_INT:
            array_int_ptr = dynamic_cast<conf_int_array_t *>(ent);
			UNIMPLEMENTED;
			return "";
		default:
			UNIMPLEMENTED;
			return "";
	}
}

/* create bool config entry, enqueue */
conf_bool_t * 
config_t::addconf_bool (const string &n, const string &d, 
	bool fixed, bool val) {
	conf_bool_t * ptr = new conf_bool_t (n, d, fixed, val);
	enqueue (ptr);
	return ptr;
}

conf_bool_t * 
config_t::addconf_bool (const string &n, const string &d, 
	bool fixed, bool * val_ptr) {
	conf_bool_t * ptr = new conf_bool_t (n, d, fixed, val_ptr);
	enqueue (ptr);
	return ptr;
}

/* create integer config entry, enqueue */
conf_integer_t * 
config_t::addconf_integer (const string &n, const string &d, 
	bool fixed, int64 val) {
	conf_integer_t * ptr = new conf_integer_t (n, d, fixed, val);
	enqueue (ptr);
	return ptr;
}

conf_integer_t * 
config_t::addconf_integer (const string &n, const string &d, 
	bool fixed, int64 * val_ptr) {
	conf_integer_t * ptr = new conf_integer_t (n, d, fixed, val_ptr);
	enqueue (ptr);
	return ptr;
}

/* create string config entry, enqueue */
conf_string_t *
config_t::addconf_string (const string &n, const string &d,
		bool fixed, const string & val) {
	conf_string_t * ptr = new conf_string_t (n, d, fixed, val);
	enqueue (ptr);
	return ptr;
}

conf_string_t * 
config_t::addconf_string (const string &n, const string &d, 
	bool fixed, string * val_ptr) {
	conf_string_t * ptr = new conf_string_t (n, d, fixed, val_ptr);
	enqueue (ptr);
	return ptr;
}

/* create int array config entry, enqueue */
conf_int_array_t *
config_t::addconf_int_array (const string &n, const string &d,
		bool fixed, int size, ...) {
	conf_int_array_t * ptr = new conf_int_array_t (n, d, fixed, size);

	/* read values */
	va_list ap;
   	va_start (ap, size);
   	for (int i = 0; i < size; i++) {
		int64 v = (int64)(va_arg (ap, int32));
	   	ptr->set_value(true, i, v);
	}
	va_end (ap);
	
	enqueue (ptr);
	return ptr;
}

conf_int_array_t * 
config_t::addconf_int_array (const string &n, const string &d, 
	bool fixed, int size, int64 * val) {
	conf_int_array_t * ptr = new conf_int_array_t (n, d, fixed, size, val);
	enqueue (ptr);
	return ptr;
}

/******* conf_parser_t implementation *******/
/* constructor */
conf_parser_t::conf_parser_t (const string &fn) {
	filename = fn;
	f = fopen (fn.c_str(), "r");
	if (f == NULL) {
		DEBUG_LOG("WARNING: can't open runtime config \"%s\"\n", fn.c_str());
		FAIL;
	}
	row = 0;

	get_base_location ();
}

/* destructor */
conf_parser_t::~conf_parser_t () {
	if (f)
		fclose (f);
}

/* start from pos+1, match a symbol, skip to the next non-space,
 * returns false if no match */
bool 
conf_parser_t::match_symbol (char c, bool allow_space) {

	if (buf [pos] == c) {
		pos ++;
		skip_spaces ();
		return true;
	}
	
	if (allow_space) {
		while (isspace (buf [pos])) {
			pos ++;
			if (buf [pos] == c) {
				pos ++;
				skip_spaces ();
				return true;
			}
		}
	}

	/* report error */
    DEBUG_LOG("Symbol not matching, expecting \'%c\'.\n", c);
	PARSER_PRINT;
	FAIL;

	return false;
}

/* see if the current character is a conf-specific delimiter */
char 
conf_parser_t::is_delimiter () {
	const char delimiter [] = "/(),\"";

	if (strchr (delimiter, buf[pos]) != NULL)
		return buf[pos];

	return '\0';
}

/* advance pos, return the index of the first non-space character */
int 
conf_parser_t::skip_spaces () {
	char c = buf [pos];
	
	while ( (c != '\n') && (c != '\0') ) {
		if (isspace(c)) 
			c = buf [++ pos];
		else return pos;
	}

	return -1;  /* end */
}

/* advance pos, return the index of next space character */
int 
conf_parser_t::next_space (bool count_delimiter) {
	if (pos < 0 || pos >= (int)strlen (buf))
		return -1;

	char c = buf [pos];
	if (count_delimiter) {
		while (!isspace(c) && !is_delimiter()) 
			c = buf [++ pos];
	} else {
		while (!isspace(c))
			c = buf [++ pos];
	}
	return pos;
}

/* copy keyword into key, return false if no keyword  */
bool
conf_parser_t::get_key () {
	char c;
	int end;
	int start = skip_spaces ();
	
	if (start == -1) {
		key[0] = '\0';
		return false;
	}

	if (is_delimiter()) {
		c = buf [pos];
		if (c == '/') {
			pos ++;
			match_symbol (c);
		} else {
			PARSER_PRINT;
		   	DEBUG_LOG("Unexpected character \'%c\'.\n", c);
			FAIL;
		}
		return false;
	}

	end = next_space (true);
   	c = buf [end];
   	buf [end] = '\0';
   	strcpy (key, &(buf [start]));
   	buf [end] = c;

	if (strlen (key))
		return true;

	return false;   
}

/* parse boolean value */
bool 
conf_parser_t::get_bool_value () {
	int start, end;
	char c = buf [pos];

	start = pos;
   	if (c != 't' && c!= 'f') {
		PARSER_PRINT;
	   	DEBUG_LOG("Boolean doesn't start with \'t\' or \'f\'.\n");
		FAIL;
	   	return false;
   	} else {
		if (c == 't')
			end = pos + 4;
		else end = pos + 5;
	}
	
	/* copy the string out */
	c = buf [end];
	buf [end] = '\0';
	strcpy (val, &(buf [start]));
   	buf [end] = c;

	if ( strcmp (val, "true") && strcmp (val, "false") ) {
	   	DEBUG_LOG("Boolean parameter must be \'true\' or \'false\'.\n", val);
		return false;
	}
	
	/* move beyond the ending quote */
	pos = end;
	return true;
	
}

/* parse string value */

void
conf_parser_t::get_base_location () {
	int end = filename.rfind ("/");
	base_location = filename.substr (0, end + 1);
}

bool 
conf_parser_t::get_str_value () {
	int start, end;

	skip_spaces ();

	char c = buf [pos];

	
    if (c != '"') {
        PARSER_PRINT;
        DEBUG_LOG("String doesn't start with \".\n");
		FAIL;
        return false;
    }

    c = buf [++ pos];
    start = end = pos; 
    while (c != '"') {
        if (c == '\n' || c == '\0') {
            pos = end;
            return false;
        }
        c = buf [++ end];
    }
    pos = end;

    /* copy the string out */
    buf [end] = '\0';
    strcpy (val, &(buf [start]));
    buf [end] = c;

    /* move beyond the ending quote */
    pos ++;
    return true;
}

/* parse an integer value */
bool 
conf_parser_t::get_int_value () {
	int start = pos;
    int end = pos;
	bool has_digit = false;
	char c = buf [end];

	if (c == '+' || c == '-') 
		c = buf [++ end];

	while (isdigit(c)) {
		has_digit = true;
		c = buf [++ end];
	}
	pos = end;

	if (has_digit) { /* copy out the int value string */
		buf [end] = '\0';
		strcpy (val, &(buf [start]));
   		buf [end] = c;

		return true;
	} 
	return false;
}

/* parser runtime version */
void 
conf_parser_t::parse_runtime_version (config_t *conf) {
}

/* parser string */
void 
conf_parser_t::parse_string (config_t *conf) {
	/* match ( */
   	match_symbol ('(', true);

	/* get config name */
   	if (!get_key ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting config variable name.\n");
		FAIL;
		return;
   	} else { /* skip a comma */
	   	if (!match_symbol (',', true))
			return;
   	}

	/* see if this config variable is runtime changeable */
	char reason[64];
	conf_string_t * str_ptr = dynamic_cast<conf_string_t*>
		(conf->runtime_changeable (key, CT_STRING, reason));

	if (str_ptr == NULL) {
		PARSER_PRINT;
        DEBUG_LOG("Config \"%s\" not changeable at runtime [%s].\n", 
				key, reason);
		FAIL;
		return;
	}

	/* skip description */
   	if (!get_str_value ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting config description (a string).\n");
		FAIL;
		return;
   	} else { /* skip a comma */
	   	if (!match_symbol (',', true))
		   	return;
   	}

	/* get value */
   	if (!get_str_value ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting a string value.\n");
		FAIL;
	    return;
   	} else { /* skip ) */
	   	if (!match_symbol (')', true))
			return;
   	}

	/* update the value */
	str_ptr->set_value (val);
}

/* parser int array */
void
conf_parser_t::parse_int_array (config_t *conf) {
	/* match ( */
   	match_symbol ('(', true);

	/* get config name */
   	if (!get_key ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting config variable name.\n");
		FAIL;
		return;
   	} else { /* skip a comma */
	   	if (!match_symbol (',', true))
			return;
   	}

	/* see if this config variable is runtime changeable */
	char reason[64];
	conf_int_array_t * int_ptr = dynamic_cast<conf_int_array_t*>
		(conf->runtime_changeable (key, CT_ARRAY_INT, reason));

	if (int_ptr == NULL) {
		PARSER_PRINT;
        DEBUG_LOG("Config \"%s\" not changeable at runtime [%s].\n", 
				key, reason);
		FAIL;
		return;
	}

	/* skip description */
   	if (!get_str_value ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting config description (a string).\n");
		FAIL;
		return;
   	} else { /* skip a comma */
	   	if (!match_symbol (',', true))
		   	return;
   	}

	/* get value of array size */
   	if (!get_int_value ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting an integer value.\n");
		FAIL;
	   	return;
   	} else { /* skip a comma */
	   	if (!match_symbol (',', true))
		   	return;
   	}
	
	int size_read = atoll (val);
	int size = int_ptr->get_size ();
	if (size > size_read) {
        DEBUG_LOG("Array size should be \"%d\", but I read a \"%d\".\n",
				size, size_read);
        DEBUG_FLUSH();   
	   	PARSER_PRINT;
		FAIL;
	   	return;
	}

	for (int i = 0; i < size; i++) {
		/* get value */
   		if (!get_int_value ()) {
	   		PARSER_PRINT;
		   	DEBUG_LOG("Expecting an integer value.\n");
			FAIL;
		    return;
	   	} else { /* skip a comma */

		   	if ( (i != size - 1) && (!match_symbol (',', true)) ) 
			   	return;
	   	}

		/* update the value */
	   	int64 v = atoll (val);
	   	int_ptr->set_value (false, i, v);
	}

   	match_symbol (')', true);
}

/* parse boolean */
void
conf_parser_t::parse_bool (config_t *conf) {
	/* match ( */
	match_symbol ('(', true);

	/* get config name */
   	if (!get_key ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting config variable name.\n");
		FAIL;
		return;
   	} else { /* skip a comma */
	   	if (!match_symbol (',', true))
			return;
   	}

	/* see if this config variable is runtime changeable */
	char reason[64];
	conf_bool_t * bool_ptr = dynamic_cast<conf_bool_t*>
		(conf->runtime_changeable (key, CT_BOOL, reason));

	if (bool_ptr == NULL) {
		PARSER_PRINT;
        DEBUG_LOG("Config \"%s\" not changeable at runtime [%s].\n", 
				key, reason);
		FAIL;
		return;
	}

	/* skip description */
   	if (!get_str_value ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting config description (a string).\n");
		FAIL;
		return;
   	} else { /* skip a comma */
	   	if (!match_symbol (',', true))
		   	return;
   	}

	/* get value */
   	if (!get_bool_value ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting a boolean value.\n");
		FAIL;
	    return;
   	} else { /* skip */
	   	if (!match_symbol (')', true))
			return;
   	}

	/* update the value */
	bool v = (val [0] == 't');
	bool_ptr->set_value (v);
}

/* parse integer */
void 
conf_parser_t::parse_integer (config_t * conf) {
	/* match ( */
   	match_symbol ('(', true);

	/* get config name */
   	if (!get_key ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting config variable name.\n");
		FAIL;
		return;
   	} else { /* skip a comma */
	   	if (!match_symbol (',', true))
			return;
   	}

	/* see if this config variable is runtime changeable */
	char reason[64];
	conf_integer_t * int_ptr = dynamic_cast<conf_integer_t*>
		(conf->runtime_changeable (key, CT_INTEGER, reason));

	if (int_ptr == NULL) {
        DEBUG_LOG("Config \"%s\" not changeable at runtime [%s].\n", 
				key, reason);
		PARSER_PRINT;
		FAIL;
		return;
	}

	/* skip description */
   	if (!get_str_value ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting config description (a string).\n");
		FAIL;
		return;
   	} else { /* skip a comma */
	   	if (!match_symbol (',', true))
		   	return;
   	}

	/* get value */
   	if (!get_int_value ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting an integer value.\n");
		FAIL;
	    return;
   	} else { /* skip */
	   	if (!match_symbol (')', true))
			return;
   	}

	/* update the value */
	int64 v = atoll (val);
	int_ptr->set_value (v);
}

/* parse increment integer */
void 
conf_parser_t::parse_inc_integer (config_t * conf) {
	/* match ( */
   	match_symbol ('(', true);

	/* get config name */
   	if (!get_key ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting config variable name.\n");
		FAIL;
		return;
   	} else { /* skip a comma */
	   	if (!match_symbol (',', true))
			return;
   	}

	/* see if this config variable is runtime changeable */
	char reason[64];
	conf_integer_t * int_ptr = dynamic_cast<conf_integer_t*>
		(conf->runtime_changeable (key, CT_INTEGER, reason));

	if (int_ptr == NULL) {
		PARSER_PRINT;
        DEBUG_LOG("Config \"%s\" not changeable at runtime [%s].\n", 
				key, reason);
		FAIL;
		return;
	}

	/* skip description */
   	if (!get_str_value ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting config description (a string).\n");
		FAIL;
		return;
   	} else { /* skip a comma */
	   	if (!match_symbol (',', true))
		   	return;
   	}

	/* get value */
   	if (!get_int_value ()) {
	   	PARSER_PRINT;
	   	DEBUG_LOG("Expecting an integer value.\n");
		FAIL;
	    return;
   	} else { /* skip */
	   	if (!match_symbol (')', true))
			return;
   	}

	/* increment the value */
	int64 v = atoll (val);
	int_ptr->set_value (int_ptr->get_value() + v);
}

void
conf_parser_t::parse_include (config_t *conf) {
	if (!get_str_value ()) {
		PARSER_PRINT;
		DEBUG_LOG("Expecting include file.\n");
		FAIL;
		return;
	}
	
	string inc (base_location);
	inc.append (val);

	includes.push_back (inc);
}

/* parse */
void 
conf_parser_t::parse (config_t *conf) {
	if (f == NULL) 
		return;

	while ( fgets (buf, CONF_BUF_SIZE, f) != NULL ) {
		row ++;
		pos = 0;

		/* if key invalid or empty line, skip */
		if (!get_key ())
			continue;

        /* ignore some keywords */
        if (   !strcmp(key, "CONF_COMPILE_V")
            || !strcmp(key, "CONF_C_BOOL")
            || !strcmp(key, "CONF_C_INTEGER")
            || !strcmp(key, "CONF_C_STRING")
            || !strcmp(key, "CONF_C_ARRAY_INT") )
        {
            DEBUG_LOG("keyword = %s ignored\n", key);
            continue;
        }

        /* if it's runtime config */
        else if (!strcmp (key, "CONF_RUNTIME_V"))
			parse_runtime_version (conf);
        else if (!strcmp (key, "CONF_V_BOOL")) 
			parse_bool (conf);
        else if (!strcmp (key, "CONF_V_INTEGER")) 
			parse_integer (conf);
        else if (!strcmp (key, "CONF_V_INC_INTEGER")) 
			parse_inc_integer (conf);
		else if (!strcmp (key, "CONF_V_STRING"))
			parse_string (conf);
		else if (!strcmp (key, "CONF_V_ARRAY_INT"))
			parse_int_array (conf);
		else if (!strcmp (key, "#include"))
			parse_include (conf);

        /* invalid keyword, ignore it */
        else {
            DEBUG_LOG("keyword = %s invalid\n", key);
            continue;
        }
	}
}
