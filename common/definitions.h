/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _DEFINITIONS_H_
#define _DEFINITIONS_H_
// simics include files
//#include "simics/global.h"
#include <simics/api.h>
#include <simics/arch/sparc.h>
extern "C" {
#include "simics/alloc.h"
#include <simics/utils.h>
}


// C/C++ include files
#include <ctype.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

// STL include files
#include <map>
#include <string>
#include <list>
#include <queue>
#include <set>
#include <vector>
#include <ext/hash_map>
using namespace __gnu_cxx;
using namespace std;

// processor include files
#include "debugio.h"

// Common classes
class base_counter_t;
class double_counter_t;
class breakdown_counter_t;
class ratio_print_t;
class histo_1d_t;
class st_entry_t;
class st_print_t;
class group_counter_t;
class base_histo_index_t;
class base_histo_t;
class stats_t;
class profiles_t;
class profile_entry_t;
class config_t; 

// Power definitions
class basic_circuit_t;
class core_power_t;
class cache_power_t;
class power_util_t;

// Common typedefs
typedef uint64 tick_t;
typedef uint64 addr_t;

// output macros
#define ERROR_OUT debugio_t::out_error
#define DEBUG_OUT debugio_t::out_info
#define DEBUG_LOG debugio_t::out_log
#define DEBUG_FLUSH debugio_t::flush_out
#ifndef __OPTIMIZE__
#define VERBOSE_OUT(...) debugio_t::verbose_info(__VA_ARGS__)
#define VERBOSE_LOG(...) debugio_t::verbose_log(__VA_ARGS__)

#else
#define VERBOSE_OUT(...) {}
#define VERBOSE_LOG(...) {}
#endif
// assertion macros
#define FAIL ASSERT (0);
#define ASSERT_WARN(c) if ( !(c) ) WARNING;
#define WARNING ERROR_OUT ("WARNING: line %d\n",  __LINE__); 
#define FE_EXCEPTION_CHECK if (SIM_get_pending_exception ()) FAIL;
#define FAIL_MSG(fmt, ...)  ERROR_OUT ("ERROR: line %d:\n    " fmt "\n", \
                                        __LINE__, ##__VA_ARGS__); \
                            ASSERT(0);
#define UNIMPLEMENTED ERROR_OUT ("UNIMPLEMENTED: line %d\n",  __LINE__); 

// output macros
#define PRINT_MARK DEBUG_LOG ("-------------------------------------------------------------------------------\n")
#define PRINT_LINE DEBUG_LOG ("\n")

// stats macros. XXX should be removed
//#define STAT_INC(CTR) (CTR)->inc_total()

struct addr_t_hash_fn : public unary_function <uint64, size_t> {
	size_t operator() (uint64 addr) const { return (addr & 0xffffffff); }
};

// Include other defs
#include "proc_defs.h"
#include "mem_hier_defs.h"

#ifdef POWER_COMPILE
#include "power_defs.h"
#endif

#endif

