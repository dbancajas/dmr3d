/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

//  #include "simics/first.h"
// RCSID("$Id: debugio.cc,v 1.2.10.1 2006/01/30 16:46:58 kchak Exp $");

#include "definitions.h"
#include "config_extern.h"
#include "debugio.h"

// system wide log file
FILE *debugio_t::logfp = NULL;

debugio_t::debugio_t (void) {
	logfp = NULL;
}

debugio_t::~debugio_t (void) {
}


void
debugio_t::verbose_info (const uint32 level, const char *fmt, ...) {

	if (!(level & g_conf_debug_level)) return;
	
	va_list args;
	va_start (args, fmt);
	vfprintf (stdout, fmt, args);
	if (logfp) 
		vfprintf (logfp, fmt, args);
	va_end (args);	
}

void
debugio_t::verbose_log (const uint32 level, const char *fmt, ...) {

	if (!(level & g_conf_debug_level)) return;
	
	va_list args;
	va_start (args, fmt);
	if (logfp) 
		vfprintf (logfp, fmt, args);
	va_end (args);	
}

void
debugio_t::out_info (const char *fmt, ...) {

#ifndef __OPTIMIZE__    
	va_list args;
	va_start (args, fmt);
	vfprintf (stdout, fmt, args);
	if (logfp) 
		vfprintf (logfp, fmt, args);
	va_end (args);	
#endif    
}

void
debugio_t::out_log (const char *fmt, ...) {
	va_list args;
	va_start (args, fmt);
	if (logfp) 
		vfprintf (logfp, fmt, args);
	else
		vfprintf (stdout, fmt, args);
	va_end (args);	
}

void
debugio_t::flush_out() {
    if (logfp)
        fsync(fileno(logfp));
    else
        fsync(fileno(stdout));
}

void
debugio_t::out_error (const char *fmt, ...) {
	va_list args;
	va_start (args, fmt);
	vfprintf (stderr, fmt, args);
	if (logfp) 
		vfprintf (logfp, fmt, args);
	va_end (args);	
}

void 
debugio_t::open_log (const char *filename, bool overwrite) {
	if (overwrite) 
		logfp = fopen (filename, "w");
	else
		logfp = fopen (filename, "a");

	if (logfp == NULL)
		ERROR_OUT ("error: unable to open log file %s\n", filename);
}

void 
debugio_t::close_log (void) {
	if (logfp != NULL && logfp != stdout) 
		fclose (logfp);
	logfp = NULL;	
}

FILE*
debugio_t::get_logfp (void) {
	return logfp;
}

