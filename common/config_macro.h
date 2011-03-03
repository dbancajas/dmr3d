/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _CONFIG_MACRO_H_
#define _CONFIG_MACRO_H_

/* Macro definitions, to declare config variables */
#define CONF_COMPILE_V(long_string) \
	const string g_conf_compile_time_version = #long_string;
	 
#define CONF_C_BOOL(name, desc, value) \
	const bool g_conf_ ## name = value;

#define CONF_C_INTEGER(name, desc, value) \
	const int64 g_conf_ ## name = value;

#define CONF_C_STRING(name, desc, str) \
	const string g_conf_ ## name = str;
	
#define CONF_C_ARRAY_INT(name, desc, size, values...) \
	const int64 g_conf_ ## name [] = { values };

#define CONF_V_BOOL(name, desc, value) \
	bool g_conf_ ## name = value;

#define CONF_V_INTEGER(name, desc, value) \
	int64 g_conf_ ## name = value;

#define CONF_V_STRING(name, desc, str) \
	string g_conf_ ## name = str;

#define CONF_V_ARRAY_INT(name, desc, size, values...) \
	int64 g_conf_ ## name [] = { values };

/* Include config_def.h to defined g_compile_config_name and configs */
#include "config_params.h"

/* Undefine the macros, thus in config.cc we can re-define such macros 
 * for config entry registration */
#undef CONF_COMPILE_V
#undef CONF_C_BOOL
#undef CONF_C_INTEGER
#undef CONF_C_STRING
#undef CONF_C_ARRAY_INT
#undef CONF_V_BOOL
#undef CONF_V_INTEGER
#undef CONF_V_STRING
#undef CONF_V_ARRAY_INT


#endif /* _CONFIG_MACRO_H_ */
