/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _DEBUGIO_H_
#define _DEBUGIO_H_

class debugio_t {

public:
	debugio_t ();
	~debugio_t ();

	static void verbose_info (const uint32 level, const char *fmt, ...);
	static void verbose_log (const uint32 level, const char *fmt, ...);

	static void out_info (const char *fmt, ...);
	static void out_log (const char *fmt, ...);
    static void flush_out();

	static void out_error (const char*fmt, ...);
	
	static FILE *get_logfp (void);
	static void open_log (const char *filename, bool overwrite);
	static void close_log (void);

private:
	static FILE *logfp;
};


	
#endif
