/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

 
 

//  #include "simics/first.h"
// RCSID("$Id: disable_timer.cc,v 1.1.2.2 2006/07/28 01:29:59 pwells Exp $");

#include "definitions.h"
#include "config_extern.h"
#include "config.h"
#include "disable_timer.h"
#include "checkp_util.h"
#define PI 3.14152965

float surand()
{
  return( (float) rand()/RAND_MAX );
}

float urand(float low, float high)
{
  return(low+(high-low)*surand());
}

float genexp(float lambda)
{
  float u,x;
  u=surand();
  x=(-1/lambda)*log(u);
  return(x);
}

float gennor()
{
  float theta,rsq,x;
  theta=urand(0,2*PI);
  rsq=genexp(0.5);
  x=sqrt(rsq)*cos(theta);
  return(x);
}


disable_timer_t::disable_timer_t(uint32 _num_cores) :
    num_cores(_num_cores)
{
    disable_timer = new list<tick_t>[num_cores];
    last_disable_time = new tick_t[num_cores];
    disabled      = new bool[num_cores];
	
	if (g_conf_enable_disable_core) {
		for (uint32 i = 0; i < num_cores; i++)
		{
			last_disable_time[i] = 0;
			for (uint32 j = 0; j < g_conf_disable_event_count; j++)
			{
				tick_t val = get_random();
				if (val) disable_timer[i].push_back(val);
			}
			disable_timer[i].push_front(get_random() + i * g_conf_disable_period);
			disabled[i] = false;
		}
	}
	else if (g_conf_disable_faulty_cores) {
		// Disable all affected cores after 1000 cycles
		ASSERT(g_conf_disable_faulty_cores < num_cores);
		for (uint32 i= 0; i < num_cores; i++)
		{
			if (i < g_conf_disable_faulty_cores)
				disable_timer[i].push_back(10000*i + 1000);
			else
				disable_timer[i].push_back(~0ULL);
			
			last_disable_time[i] = 0;
			disabled[i] = false;
		}
	}		
}

tick_t disable_timer_t::get_random()
{
  float x;
  x = 0;	
  for (uint32 j = 0; j < 12; j++)	
      x=gennor() + x;
  x = g_conf_disable_interval_stddev * (x - 6) + g_conf_disable_interval_mean;
  return (tick_t) x;
}


bool disable_timer_t::disable_core_now(tick_t g_cycles, uint32 core)
{
    if (disabled[core]) return false;
    tick_t d_timer = disable_timer[core].front();
    if (g_cycles >= last_disable_time[core] + d_timer)
    {
        last_disable_time[core] = g_cycles;
        disable_timer[core].pop_front();
        disabled[core] = true;
        return true;
    }
    return false;
}

bool disable_timer_t::enable_core_now(tick_t g_cycles, uint32 core)
{
    if (!disabled[core]) return false;
    if (g_cycles >=last_disable_time[core] + g_conf_disable_period)
    {
        last_disable_time[core] = g_cycles;
        disabled[core] = false;
        return true;
    }
    return false;
}


void disable_timer_t::write_checkpoint(FILE *file)
{
    checkpoint_util_t *cp = new checkpoint_util_t(); 
    for (uint32 i = 0; i < num_cores; i++)
    {
        uint32 j = disabled[i] ? 1 : 0;
        fprintf(file, "%u %llu\n", j, last_disable_time[i]);
        cp->list_tick_t_to_file(disable_timer[i], file);
    }
}

void disable_timer_t::read_checkpoint(FILE *file)
{
    checkpoint_util_t *cp = new checkpoint_util_t(); 
    for (uint32 i = 0; i < num_cores; i++)
    {
        uint32 j;
        fscanf(file, "%u %llu\n", &j, &last_disable_time[i]);
        disabled[i] = (j == 1);
        disable_timer[i].clear();
        cp->list_tick_t_from_file(disable_timer[i], file);
    }
}
