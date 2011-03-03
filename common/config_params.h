// Compile time configuration Version 
CONF_COMPILE_V (default_config_testing)

CONF_V_INTEGER (debug_level, "bitmask of debug output", 0)

CONF_C_INTEGER (name_width, "stats name width", 25)
CONF_C_INTEGER (value_width, "stats value width", 20)
CONF_C_INTEGER (desc_width, "stats desc width", 50)

#include "proc_params.h"
#include "mem_hier_params.h"

