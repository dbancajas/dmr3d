
/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* $Id: line.h,v 1.2.4.3 2005/11/18 22:24:14 kchak Exp $
 *
 * description:    generic (non-protocol specific) cache line
 * initial author: Philip Wells 
 *
 */
 
#ifndef _LINE_H_
#define _LINE_H_

class line_t {

 public:
	// destructor
	virtual ~line_t();
	
	// Initialize line just after construction
	virtual void init(cache_t *cache, addr_t idx, uint32 lsize);
		
	// Initialize line on a miss
	virtual void reinit(addr_t _tag);
	
	// Write Back buffer 
	virtual void wb_init(addr_t _tag, addr_t _index, line_t *cache_line);
	
	virtual void from_file(FILE *file);
	virtual void to_file(FILE *file);
	
	// Accessor functions
	addr_t get_index();
	addr_t get_tag();
	addr_t get_address();
	cache_t *get_cache();

	tick_t get_last_use();
	void mark_use();

	const uint8* get_data();
	void store_data(const uint8* _data, const addr_t offset, const uint32 size);
	
	
	// Virtual functions
	virtual bool is_free() = 0;      // Is this line free (eg idle, invalid)
	virtual bool can_replace() = 0;  // Write back may still be necessary
	virtual bool is_stable() = 0;    // Is this line in a stable state

	void mark_prefetched(bool val);
	bool is_prefetched();
    void set_index(addr_t _idx);
    void set_tag(addr_t _tag);
    void set_last_use(tick_t _l_use);
    
    // Hack for warmup line states
    virtual void warmup_state() {}
	
	
 protected:
	cache_t *cache;        // pointer to cache this line is part of
	
	uint8   lsize;         // Number of bytes in this line

	uint64  tag;           // Tag of address currently stored in line
	uint64  index;         // Position of line in cache
	uint8   *data;         // The data

	tick_t  last_use;      // i.e. for replacement

	bool    prefetched;    // has this line been prefetched and not used yet? 

};


inline void
line_t::mark_prefetched(bool val)
{
	prefetched = val;
}

inline bool
line_t::is_prefetched() 
{
	return prefetched;
}	

#endif // _LINE_H_
