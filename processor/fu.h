/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _FU_H_
#define _FU_H_

fu_type_t operator++ (fu_type_t &unit);

class fu_t {
private:
	fu_type_t id;
	uint32    latency;
	uint32    count;
	uint32    avail_count;
	uint32    pipe_latency;

public:
	fu_t (fu_type_t _id, uint32 _latency, uint32 _count);
	fu_t ();
	~fu_t ();
	
	fu_type_t get_id ();
	uint32 get_latency ();
	uint32 get_count ();
	uint32 get_pipe_latency ();
	uint32 get_avail_count ();

	bool get_fu ();
	void return_fu ();
	
};

#endif

