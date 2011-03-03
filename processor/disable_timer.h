/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

#ifndef _DISABLE_TIMER_H_
#define _DISABLE_TIMER_H_


class disable_timer_t {
    uint32 num_cores;
    
    tick_t *last_disable_time;
    list<tick_t> *disable_timer;
    bool   *disabled;
    tick_t get_random();
    
    
    public:
        disable_timer_t(uint32 _num_cores);
        
        bool disable_core_now(tick_t g_cycles, uint32 core);
        bool enable_core_now(tick_t g_cycles, uint32 core);
        
        void read_checkpoint(FILE *file);
        void write_checkpoint(FILE *file);
};

#endif
