/* Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
 * Multiscalar Project.  ALL RIGHTS RESERVED.
 *
 * This software is furnished under the Multiscalar license.
 * For details see the LICENSE.mscalar file in the top-level source
 * directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
 *
 */

/* description:    config functions for different mem-hier topologies
 * initial author: Philip Wells 
 *
 */
 
//  #include "simics/first.h"
// RCSID("$Id: networks.cc,v 1.10.2.18 2006/07/28 01:29:56 pwells Exp $");

#include "definitions.h"
#include "config_extern.h"
#include "protocols.h"
#include "mem_hier.h"

// Must include all the devices we create here
#include "device.h"
#include "link.h"
#include "mainmem.h"
#include "dram.h"
#include "proc.h"
#include "split_msi_bus.h"
#include "split_unip_bus.h"
#include "mp_bus.h"
#include "cache.h"
#include "tcache.h"
#include "iodev.h"
#include "interconnect.h"
#include "sequencer.h"
#include "mesh_simple.h"
#include "mesh_sandwich.h"


// Protocol definitions
#include "protocols.h"

#ifdef COMPILE_CMP_INCL 
#include "cmp_incl_l1_cache.h"
#include "cmp_incl_l2_cache.h"
#endif 

#ifdef COMPILE_CMP_INCL_WT
#include "cmp_incl_wt_l1_cache.h"
#include "cmp_incl_wt_l2_cache.h"
#endif

#ifdef COMPILE_CMP_EXCL 
#include "cmp_excl_l1_cache.h"
#include "cmp_excl_l2_cache.h"
#include "cmp_excl_l1dir_array.h"
#endif 

#ifdef COMPILE_CMP_EX_3L 
#include "cmp_excl_3l_l1_cache.h"
#include "cmp_excl_3l_l2_cache.h"
#include "cmp_excl_3l_l2dir_array.h"
#include "cmp_excl_3l_l3_cache.h"
#endif 

#ifdef COMPILE_CMP_INCL_3L 
#include "cmp_incl_3l_l1_cache.h"
#include "cmp_incl_3l_l2_cache.h"
#include "cmp_incl_3l_l3_cache.h"
#endif 

#ifdef COMPILE_UNIP_TWO_DRAM
#include "unip_two_l1d_cache.h"
#include "unip_two_l1i_cache.h"
#include "unip_two_l2_cache.h"
#endif 

#ifdef COMPILE_UNIP_TWO
#include "unip_two_l1d_cache.h"
#include "unip_two_l1i_cache.h"
#include "unip_two_l2_cache.h"
#endif 

#ifdef POWER_COMPILE
#include "power_profile.h"
#include "core_power.h"
#endif

// What a horrible place for these, but need to be defined in a non-template .cc
template <class prot_sm_t, class msg_t>
const uint8 proc_t<prot_sm_t, msg_t>::icache_link;  
template <class prot_sm_t, class msg_t>
const uint8 proc_t<prot_sm_t, msg_t>::dcache_link;  
template <class prot_sm_t, class msg_t>
const uint8 proc_t<prot_sm_t, msg_t>::k_icache_link;  
template <class prot_sm_t, class msg_t>
const uint8 proc_t<prot_sm_t, msg_t>::k_dcache_link;  

uint32 mem_hier_t::
round_up_pow2(uint32 val) {
	uint32 ret = 1;
	while ((ret <<= 1) < val);
	return ret;
}

void mem_hier_t::reallocate_links()
{
	link_t ** tmp_links = new link_t *[links_allocated];
	for (uint32 i = 0; i < links_allocated; i++)
		tmp_links[i] = links[i];
	delete [] links;
	
	links = new link_t *[links_allocated * 2];
	
	for (uint32 i = 0; i < links_allocated ; i ++)
		links[i] = tmp_links[i];
	
	delete [] tmp_links;
	links_allocated = 2 * links_allocated;
	
	
}

void mem_hier_t::reallocate_devices()
{
	device_t **tmp_dvc = new device_t *[devices_allocated];
	for (uint32 i = 0; i < devices_allocated; i++)
		tmp_dvc[i] = devices[i];
	delete [] devices;
	devices = new device_t *[devices_allocated * 2];
	
	for (uint32 i = 0; i < devices_allocated; i++)
		devices[i] = tmp_dvc[i];
	
	delete [] tmp_dvc;
	devices_allocated = 2 * devices_allocated;
	
	
}


void mem_hier_t::init_network()
{
	// Make conservative assumptions on number of possible devices
	// TODO: Make a reasonable device count without stupid assumptions
	devices_allocated = num_processors * 6;
	
	// Possible links allocated :- assume that each device is connected to
	// every other device in 2 possible ways - 4 * n(C)2
	links_allocated = 2 * devices_allocated * devices_allocated;
	
	devices = new device_t *[devices_allocated];
	links = new link_t * [links_allocated];
	
	deviceindex = linkindex = 0;
	
}


void
mem_hier_t::join_devices(uint32 dev_indxA,uint32 dev_indxB, uint32 numlinksAtoB, link_desc_t *ldA,
                         uint32 numlinksBtoA, link_desc_t *ldB)
{
	// Set up links between A to B;
	for (uint32 i = 0; i < numlinksAtoB; i++) {
		if (ldA[i].buffered) {
			links[linkindex] = new buffered_link_t(devices[dev_indxA], devices[dev_indxB],
			                                         ldA[i].link_latency, ldA[i].msgs_per_link);
		} else {
			links[linkindex] = new link_t(devices[dev_indxA], devices[dev_indxB]);
		}
		devices[dev_indxA]->set_link(ldA[i].link_id, links[linkindex]);
		linkindex++;
		if (linkindex == links_allocated)
			reallocate_links();
	}
	

	// Set up links between B to A
	for (uint32 i = 0; i < numlinksBtoA; i++) {
		if (ldB[i].buffered) {
			links[linkindex] = new buffered_link_t(devices[dev_indxB], devices[dev_indxA],
			                                         ldB[i].link_latency, ldB[i].msgs_per_link);
		} else {
			links[linkindex] = new link_t(devices[dev_indxB], devices[dev_indxA]);
		}
		devices[dev_indxB]->set_link(ldB[i].link_id, links[linkindex]);
		linkindex++;
		if (linkindex == links_allocated)
			reallocate_links();
	}


}

int 
mem_hier_t::add_device(device_t *dev)
{
	devices[deviceindex++] = dev;
	if (deviceindex == devices_allocated)
		reallocate_devices();
	return (deviceindex - 1);
}


void
mem_hier_t::create_unip_one_dram()
{
#ifndef COMPILE_UNIP_ONE
	ASSERT_MSG(0, "COMPILE_UNIP_ONE not defined\n");
#else
        typedef unip_one_prot_t prot_t;

        // Hook up single level uniprocessor cache and dramsim main memory
        num_links = 4;
        num_devices = 3;
        links = new link_t * [num_links];
        devices = new device_t * [num_devices];

        // Create cache conf for this topology
        cache_config_t cache_conf;
        cache_conf.assoc = g_conf_l1d_assoc;
        cache_conf.lsize = g_conf_l1d_lsize;
        cache_conf.size = g_conf_l1d_size;
        cache_conf.banks = g_conf_l1d_banks;
        cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
        cache_conf.req_q_size = g_conf_l1d_req_q_size;
        cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
        cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
        cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
        cache_conf.writeback_buffersize = 8;

        devices[0] = new proc_t<prot_t, simple_proc_msg_t>("proc", get_cpu_object(0));
        devices[1] = new unip_one_cache_t("unipcache", 2, &cache_conf, 0, 0);
        devices[2] = new dram_t<prot_t, simple_mainmem_msg_t>("dram", 1);
	printf("dram constructed!!!!!!!!!!!!!!!!!!\n");

        // Proc-cache link
        links[0] = new link_t(devices[0], devices[1]);

        // Cache-proc link
        if (g_conf_delay_zero_latency || g_conf_l1d_latency > 0) {
                links[1] = new buffered_link_t(devices[1], devices[0],
                                                                           g_conf_l1d_latency, 0);
        } else {
                links[1] = new link_t(devices[1], devices[0]);
        }

        // cache memory link
        links[2] = new buffered_link_t(devices[1], devices[2],
                                                                   g_conf_default_link_latency,
                                                                   g_conf_default_messages_per_link);
        links[3] = new buffered_link_t(devices[2], devices[1],
                                                                   g_conf_default_link_latency,
                                                                   g_conf_default_messages_per_link);

        // Set proc link to cache
        devices[0]->set_link(0, links[0]);  // IFetch and Data down same link
        devices[0]->set_link(1, links[0]);

        // Set cache links to proc and mem
        devices[1]->set_link(0, links[1]);
        devices[1]->set_link(1, links[2]);

        // Set mem link to cache
        devices[2]->set_link(0, links[3]);

#endif
}
						 
void
mem_hier_t::create_unip_one_default()
{
#ifndef COMPILE_UNIP_ONE
	ASSERT_MSG(0, "COMPILE_UNIP_ONE not defined\n");
#else

	typedef unip_one_prot_t prot_t;

	// Hook up single level uniprocessor cache and main memory
	num_links = 4;
	num_devices = 3;
	links = new link_t * [num_links];
	devices = new device_t * [num_devices];

	// Create cache conf for this topology
	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = 8;
	
	devices[0] = new proc_t<prot_t, simple_proc_msg_t>("proc", get_cpu_object(0));
	devices[1] = new unip_one_cache_t("unipcache", 2, &cache_conf, 0, 0);
	devices[2] = new main_mem_t<prot_t, simple_mainmem_msg_t>("mainmem", 1);

	// Proc-cache link
	links[0] = new link_t(devices[0], devices[1]);

	// Cache-proc link
	if (g_conf_delay_zero_latency || g_conf_l1d_latency > 0) {
		links[1] = new buffered_link_t(devices[1], devices[0],
									   g_conf_l1d_latency, 0);
	} else {
		links[1] = new link_t(devices[1], devices[0]);
	}

	// cache memory link
	links[2] = new buffered_link_t(devices[1], devices[2], 
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);
	links[3] = new buffered_link_t(devices[2], devices[1],
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);
	
	// Set proc link to cache
	devices[0]->set_link(0, links[0]);  // IFetch and Data down same link
	devices[0]->set_link(1, links[0]);
	
	// Set cache links to proc and mem
	devices[1]->set_link(0, links[1]);
	devices[1]->set_link(1, links[2]);

	// Set mem link to cache
	devices[2]->set_link(0, links[3]);
#endif // COMPILE_UNIP_ONE
}

void
mem_hier_t::create_unip_one_mp_msi()
{
#ifndef COMPILE_MP_ONE_MSI
	ASSERT_MSG(0, "COMPILE_MP_ONE_MSI not defined\n");
#else

	typedef mp_one_msi_prot_sm_t prot_t;

	// Hook up single level uniprocessor cache and main memory
	num_links = 4;
	num_devices = 3;
	links = new link_t * [num_links];
	devices = new device_t * [num_devices];

	// Create cache conf for this topology
	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = 8;
	
	devices[0] = new proc_t<prot_t, simple_proc_msg_t>("proc", get_cpu_object(0));
	devices[1] = new mp_one_msi_cache_t("msicache", &cache_conf, get_cpu_object(0), 0, 0);
	devices[2] = new mp_one_msi_main_mem_t("mainmem", 1);

	// Proc-cache link
	links[0] = new link_t(devices[0], devices[1]);

	// Cache-proc link
	if (g_conf_delay_zero_latency || g_conf_l1d_latency > 0) {
		links[1] = new buffered_link_t(devices[1], devices[0],
									   g_conf_l1d_latency, 0);
	} else {
		links[1] = new link_t(devices[1], devices[0]);
	}

	// cache memory link
	links[2] = new buffered_link_t(devices[1], devices[2], 
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);
	links[3] = new buffered_link_t(devices[2], devices[1],
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);
	
	// Set proc link to cache
	devices[0]->set_link(0, links[0]);
	
	// Set cache links to proc and mem
	devices[1]->set_link(0, links[1]);
	devices[1]->set_link(1, links[2]);
	devices[1]->set_link(1, links[2]);   // Same
	
	// Set mem link to cache
	devices[2]->set_link(0, links[3]);
#endif // COMPILE_MP_ONE_MSI
}

void
mem_hier_t::create_minimal_default()
{
	ASSERT(0);
#if 0
	// TODO: Use a different protocol?  Doesn't really matter...
	typedef unip_one_prot_t prot_t;

	// Hook procs directly to main mem
	num_links = 2;
	num_devices = 2;
	links = new link_t * [num_links];
	devices = new device_t * [num_devices];

	devices[0] = new proc_t<prot_t, default_msg_t>("proc", get_cpu_object(0));
	devices[1] = new main_mem_t<prot_t, default_msg_t>("mainmem", 1);

	// Proc-mem link
	links[0] = new buffered_link_t(devices[0], devices[1], 
									g_conf_default_link_latency,
									g_conf_default_messages_per_link);
	links[1] = new buffered_link_t(devices[1], devices[0],
									g_conf_default_link_latency,
									g_conf_default_messages_per_link);

	// Set proc link to cache
	devices[0]->set_link(0, links[0]);
	
	// Set mem link to cache
	devices[1]->set_link(0, links[1]);
#endif // 0
}

void
mem_hier_t::create_mp_minimal_default()
{
	/*
	// TODO: Use a different protocol?  Doesn't really matter since no caches...
	typedef unip_one_prot_t prot_t;

	// Hook procs directly to main mem
	num_links = (1 + num_processors) * 2;
	num_devices = 2 + num_processors;
	links = new link_t * [num_links];
	devices = new device_t * [num_devices];

	for(int i = 0; i < num_processors; i++) { 
		devices[i] = new proc_t<prot_t, default_msg_t>(1);
	}
	devices[num_processors] = new main_mem_t<prot_t, default_msg_t>(1);
	devices[num_processors+1] = new main_mem_t<prot_t, default_msg_t>(1);

	// Proc-mem link
	links[0] = new multicast_link_t(&devices[0], num_processors,
									&devices[num_devices-1], 1, 
									g_conf_default_link_latency,
									g_conf_default_messages_per_link);
	links[1] = new multicast_link_t(&devices[num_devices-1], 1,
	                                &devices[0], num_processors,
									g_conf_default_link_latency,
									g_conf_default_messages_per_link);

	// Set proc link to cache
	devices[0]->set_link(0, links[0]);
	
	// Set mem link to cache
	devices[1]->set_link(0, links[1]);
	*/
}

void
mem_hier_t::create_unip_one_bus()
{
#ifndef COMPILE_UNIP_ONE
	ASSERT_MSG(0, "COMPILE_UNIP_ONE not defined\n");
#else

	typedef unip_one_prot_t prot_t;

	// Hook up single level uniprocessor cache and main memory
	num_links = 6;
	num_devices = 4;
	links = new link_t * [num_links];
	devices = new device_t * [num_devices];

	// Create cache conf for this topology
	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = 8;
	
	devices[0] = new proc_t<prot_t, simple_proc_msg_t>("proc", get_cpu_object(0));
	devices[1] = new unip_one_cache_t("unipcache", 2, &cache_conf, 0, 0);
	devices[2] = new main_mem_t<prot_t, simple_mainmem_msg_t>("mainmem", num_processors);
	devices[3] = new split_unip_bus_t<prot_t, simple_mainmem_msg_t>("bus", 1, 1);
	
	// Proc-cache link
	links[0] = new link_t(devices[0], devices[1]);

	// Cache-proc link
	if (g_conf_delay_zero_latency || g_conf_l1d_latency > 0) {
		links[1] = new buffered_link_t(devices[1], devices[0],
									   g_conf_l1d_latency, 0);
	} else {
		links[1] = new link_t(devices[1], devices[0]);
	}

	// cache bus link
	links[2] = new buffered_link_t(devices[1], devices[3], 
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);
	links[3] = new buffered_link_t(devices[3], devices[1],
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);
	// bus memory links
	links[4] = new buffered_link_t(devices[2], devices[3], 
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);
	links[5] = new buffered_link_t(devices[3], devices[2],
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);
	
	// Set proc link to cache
	devices[0]->set_link(0, links[0]);
	
	// Set cache links to bus and procs
	devices[1]->set_link(0, links[1]);
	devices[1]->set_link(1, links[2]);

	// Set bus links to cache and memory
	devices[3]->set_link(1,links[5]);
	devices[3]->set_link(0, links[3]);
	
	// Set mem link to bus
	devices[2]->set_link(0, links[4]);

#endif // COMPILE_UNIP_ONE
}



void
mem_hier_t::create_mp_bus()
{

#ifndef COMPILE_MP_ONE_MSI
	ASSERT_MSG(0, "COMPILE_MP_ONE_MSI not defined\n");
#else

	typedef mp_one_msi_prot_sm_t prot_t;
	//typedef unip_one_prot_t prot_t;


	// Hook up single level uniprocessor cache and main memory

	num_links = 5 * num_processors + 2;
	num_devices = 2 * num_processors + 2;
	links = new link_t * [num_links];
	devices = new device_t * [num_devices];

	// Create cache conf for this topology
	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = 8;
	
	char *num = new char[2];
	string proc = "proc";
	string cache = "cache";
	for (uint32 i = 0; i < num_processors; i++) {
		sprintf(num, "%u", i);
		devices[i] = new proc_t<prot_t, simple_proc_msg_t>(proc+num,
														   get_cpu_object(i));
		devices[i + num_processors] = new mp_one_msi_cache_t(cache+num,
															 &cache_conf, get_cpu_object(i),i, 0);
		links[i] = new link_t(devices[i], devices[i + num_processors]);

		// Cache-proc link
		if (g_conf_delay_zero_latency || g_conf_l1d_latency > 0) {
			links[i + num_processors] =
				new buffered_link_t(devices[i + num_processors],
									devices[i], g_conf_l1d_latency, 0);
		} else {
			links[i + num_processors] =
				new link_t(devices[i + num_processors], devices[i]);
		}
	}
	delete[] num;
	
	devices[2 * num_processors + 1] =
		new mp_one_msi_main_mem_t("mainmem", num_processors);
	devices[2 * num_processors] =
		 new mp_bus_t<prot_t, mp_one_msi_msg_t>("bus", num_processors, 1);


	// cache bus link
	for (uint32 i = 0; i < num_processors; i++) {

		// Address links from cache 
		links[i + 2 * num_processors] = new link_t(devices[num_processors + i],
												   devices[2*num_processors]);
		// DataResp links from cache
		links[i + 4 * num_processors] = 
			new buffered_link_t(devices[num_processors + i],
								devices[2*num_processors],
								g_conf_default_link_latency,
								g_conf_default_messages_per_link);
		// Link back to cache
		links[i + 3 * num_processors] = new link_t(devices[2 * num_processors],
												   devices[num_processors + i]);
	}
	
	// bus memory links
	links[5 * num_processors] =
		new buffered_link_t(devices[2 * num_processors], 
							devices[2 *  num_processors + 1], 
							g_conf_default_link_latency,
							g_conf_default_messages_per_link);
	links[5 * num_processors + 1] = 
		new buffered_link_t(devices[2 *  num_processors + 1], 
							devices[2 *  num_processors],
							g_conf_default_link_latency,
							g_conf_default_messages_per_link);
	
	// Set proc link to cache
	for (uint32 i = 0; i < num_processors; i++) {
		devices[i]->set_link(proc_t<prot_t, simple_proc_msg_t>::dcache_link,
							 links[i]);  // proc to cache links
		devices[i]->set_link(proc_t<prot_t, simple_proc_msg_t>::icache_link,
							 links[i]);  // ifetch to same link
		// cache to proc links
		devices[i + num_processors]->set_link(0, links[i + num_processors]);
		// cache to bus request links
		devices[i + num_processors]->set_link(1, links[i + 2 * num_processors]);
		// cache to bus dataresp links
		devices[i + num_processors]->set_link(2, links[i + 4 * num_processors]);
		// bus to cache links
		devices[2 * num_processors]->set_link(i, links[i + 3 * num_processors]);
	}

	
	// Bus to memory link and reverse
	devices[2 * num_processors]->set_link(num_processors,
										  links[5 * num_processors]);
	devices[2 * num_processors + 1]->set_link(0, links[5 * num_processors + 1]);
#endif // COMPILE_MP_ONE_MSI
}

void
mem_hier_t::create_unip_two_default_dram()
{

#ifndef COMPILE_UNIP_TWO_DRAM
	ASSERT_MSG(0, "COMPILE_UNIP_TWO_DRAM not defined\n");
#else

	typedef unip_two_l1d_prot_sm_t prot_t;
	
	// Device Indicies
	uint32 proc = 0; 
	uint32 l1d = 1; 
	uint32 l1i = 2; 
	uint32 l2 = 3; 
	uint32 dram = 4; 

	// Hook up 2-level, split uniprocessor caches and main memory
	num_links = 10;
	num_devices = 5;
	links = new link_t * [num_links];
	devices = new device_t * [num_devices];

	// Create Devices
	devices[proc] = new proc_t<prot_t, simple_proc_msg_t>("proc", get_cpu_object(0));

	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = 0;
	devices[l1d] = new unip_two_l1d_cache_t("L1-DCache", &cache_conf, l1d, 0);

	cache_conf.assoc = g_conf_l1i_assoc;
	cache_conf.lsize = g_conf_l1i_lsize;
	cache_conf.size = g_conf_l1i_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1i_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1i_req_q_size;
	cache_conf.wait_q_size = g_conf_l1i_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1i_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1i_requests_per_mshr;
	devices[l1i] = new unip_two_l1i_cache_t("L1-ICache", &cache_conf, l1i, ICACHE_STREAM_PREFETCH);

	cache_conf.assoc = g_conf_l2_assoc;
	cache_conf.lsize = g_conf_l2_lsize;
	cache_conf.size = g_conf_l2_size;
	cache_conf.banks = g_conf_l2_banks;
	cache_conf.bank_bw = g_conf_l2_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l2_req_q_size;
	cache_conf.wait_q_size = g_conf_l2_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l2_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l2_requests_per_mshr;
	devices[l2] = new unip_two_l2_cache_t("L2-Cache", &cache_conf, l2, 0);
	//dramptr = new dram_t<prot_t, simple_mainmem_msg_t>("dram", 1);
	//devices[dram] = new main_mem_t<prot_t, simple_mainmem_msg_t>("dram", 1);
	//devices[dram] = dramptr;

	// Proc-Dcache link
	links[0] = new link_t(devices[proc], devices[l1d]);
	// DCache-proc link
	if (g_conf_delay_zero_latency || g_conf_l1d_latency > 0) {
		links[1] = new buffered_link_t(devices[l1d], devices[proc],
									   g_conf_l1d_latency, 0);
	} else {
		links[1] = new link_t(devices[l1d], devices[proc]);
	}
	links[2] = new link_t(devices[proc], devices[l1i]);
	// ICache-proc link
	if (g_conf_delay_zero_latency || g_conf_l1i_latency > 0) {
		links[3] = new buffered_link_t(devices[l1i], devices[proc],
									   g_conf_l1i_latency, 0);
	} else {
		links[3] = new link_t(devices[l1i], devices[proc]);
	}
	// L1 to L2
	links[4] = new link_t(devices[l1d], devices[l2]);
	links[5] = new buffered_link_t(devices[l2], devices[l1d], g_conf_l2_latency, 0);
	links[6] = new link_t(devices[l1i], devices[l2]);
	links[7] = new buffered_link_t(devices[l2], devices[l1i], g_conf_l2_latency, 0);
	// cache memory link
	links[8] = new buffered_link_t(devices[l2], devices[dram], 
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);
	links[9] = new buffered_link_t(devices[dram], devices[l2],
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);
	
	// Set proc link to cache
	devices[proc]->set_link(proc_t<prot_t, simple_proc_msg_t>::dcache_link, links[0]);
	devices[proc]->set_link(proc_t<prot_t, simple_proc_msg_t>::icache_link, links[2]);
	// Set cache links for L1s
	devices[l1d]->set_link(unip_two_l1d_cache_t::proc_link, links[1]);
	devices[l1d]->set_link(unip_two_l1d_cache_t::l2_link, links[4]);
	devices[l1i]->set_link(unip_two_l1i_cache_t::proc_link, links[3]);
	devices[l1i]->set_link(unip_two_l1i_cache_t::l2_link, links[6]);
	// Set l2 links
	devices[l2]->set_link(unip_two_l2_cache_t::dcache_link, links[5]);
	devices[l2]->set_link(unip_two_l2_cache_t::icache_link, links[7]);
	devices[l2]->set_link(unip_two_l2_cache_t::mainmem_link, links[8]);
	
	// Set mem link to cache
	devices[dram]->set_link(0, links[9]);
#endif // COMPILE_UNIP_TWO_DRAM
}

void
mem_hier_t::create_unip_two_default()
{

#ifndef COMPILE_UNIP_TWO
	ASSERT_MSG(0, "COMPILE_UNIP_TWO not defined\n");
#else

	typedef unip_two_l1d_prot_sm_t prot_t;
	
	// Device Indicies
	uint32 proc = 0; 
	uint32 l1d = 1; 
	uint32 l1i = 2; 
	uint32 l2 = 3; 
	uint32 mainmem = 4; 

	// Hook up 2-level, split uniprocessor caches and main memory
	num_links = 10;
	num_devices = 5;
	links = new link_t * [num_links];
	devices = new device_t * [num_devices];

	// Create Devices
	devices[proc] = new proc_t<prot_t, simple_proc_msg_t>("proc", get_cpu_object(0));

	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = 0;
	devices[l1d] = new unip_two_l1d_cache_t("L1-DCache", &cache_conf, l1d, 0);

	cache_conf.assoc = g_conf_l1i_assoc;
	cache_conf.lsize = g_conf_l1i_lsize;
	cache_conf.size = g_conf_l1i_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1i_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1i_req_q_size;
	cache_conf.wait_q_size = g_conf_l1i_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1i_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1i_requests_per_mshr;
	devices[l1i] = new unip_two_l1i_cache_t("L1-ICache", &cache_conf, l1i, ICACHE_STREAM_PREFETCH);

	cache_conf.assoc = g_conf_l2_assoc;
	cache_conf.lsize = g_conf_l2_lsize;
	cache_conf.size = g_conf_l2_size;
	cache_conf.banks = g_conf_l2_banks;
	cache_conf.bank_bw = g_conf_l2_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l2_req_q_size;
	cache_conf.wait_q_size = g_conf_l2_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l2_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l2_requests_per_mshr;
	devices[l2] = new unip_two_l2_cache_t("L2-Cache", &cache_conf, l2, 0);
		
	//devices[mainmem] = new main_mem_t<prot_t, simple_mainmem_msg_t>("mainmem", 1);
	devices[mainmem] = dramptr; 

	// Proc-Dcache link
	links[0] = new link_t(devices[proc], devices[l1d]);
	// DCache-proc link
	if (g_conf_delay_zero_latency || g_conf_l1d_latency > 0) {
		links[1] = new buffered_link_t(devices[l1d], devices[proc],
									   g_conf_l1d_latency, 0);
	} else {
		links[1] = new link_t(devices[l1d], devices[proc]);
	}
	links[2] = new link_t(devices[proc], devices[l1i]);
	// ICache-proc link
	if (g_conf_delay_zero_latency || g_conf_l1i_latency > 0) {
		links[3] = new buffered_link_t(devices[l1i], devices[proc],
									   g_conf_l1i_latency, 0);
	} else {
		links[3] = new link_t(devices[l1i], devices[proc]);
	}
	// L1 to L2
	links[4] = new link_t(devices[l1d], devices[l2]);
	links[5] = new buffered_link_t(devices[l2], devices[l1d], g_conf_l2_latency, 0);
	links[6] = new link_t(devices[l1i], devices[l2]);
	links[7] = new buffered_link_t(devices[l2], devices[l1i], g_conf_l2_latency, 0);
	// cache memory link
	links[8] = new buffered_link_t(devices[l2], devices[mainmem], 
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);
	links[9] = new buffered_link_t(devices[mainmem], devices[l2],
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);
	
	// Set proc link to cache
	devices[proc]->set_link(proc_t<prot_t, simple_proc_msg_t>::dcache_link, links[0]);
	devices[proc]->set_link(proc_t<prot_t, simple_proc_msg_t>::icache_link, links[2]);
	// Set cache links for L1s
	devices[l1d]->set_link(unip_two_l1d_cache_t::proc_link, links[1]);
	devices[l1d]->set_link(unip_two_l1d_cache_t::l2_link, links[4]);
	devices[l1i]->set_link(unip_two_l1i_cache_t::proc_link, links[3]);
	devices[l1i]->set_link(unip_two_l1i_cache_t::l2_link, links[6]);
	// Set l2 links
	devices[l2]->set_link(unip_two_l2_cache_t::dcache_link, links[5]);
	devices[l2]->set_link(unip_two_l2_cache_t::icache_link, links[7]);
	devices[l2]->set_link(unip_two_l2_cache_t::mainmem_link, links[8]);
	
	// Set mem link to cache
	devices[mainmem]->set_link(0, links[9]);
#endif // COMPILE_UNIP_TWO
}

void mem_hier_t::create_cmp_excl()
{
#ifndef COMPILE_CMP_EXCL
    ASSERT_MSG(0, "COMPILE_CMP_EXCL not defined\n");
#else
    typedef cmp_excl_l1_protocol_t L1prot_t;
	typedef cmp_excl_l2_protocol_t L2prot_t;
	typedef cmp_excl_l1dir_protocol_t L1dir_prot_t;
	
	link_desc_t normal_link;
	link_desc_t buffered_link;
	normal_link.buffered = false;
	normal_link.link_id = 0;
    normal_link.link_latency = 0;
	buffered_link.buffered = true;
	buffered_link.link_latency = g_conf_default_link_latency;
	buffered_link.msgs_per_link = g_conf_default_messages_per_link;
	
	uint32 proc_index[num_processors];
	
    uint32 num_l1_cache = 2 * num_processors;

	char num[2];
	string proc = "proc";
	for (uint32 i = 0; i < num_processors; i++) {
		ASSERT(get_cpu_object(i));
		sprintf(num, "%u", i);
		proc_index[i] = add_device(new proc_t<L1prot_t, simple_proc_msg_t>(proc+string(num), get_cpu_object(i)));
	}
	
	uint32 dcache_index[num_processors];
	
	// Add up all the D-Cache
	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l1d_writeback_buffers;
	
	// Set up link descriptor between proc and dcache
	link_desc_t proc_dcache_link[1];
	proc_dcache_link[0] = normal_link;
	proc_dcache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::dcache_link;
	
	link_desc_t dcache_proc_link[1];
	dcache_proc_link[0] = buffered_link;
	dcache_proc_link[0].link_latency = g_conf_l1d_latency;
	dcache_proc_link[0].link_id =  L1prot_t::proc_link;
	
	string cache = "Dcache";
	// Create the Interconnect First
    interconnect_t *icn = new interconnect_t(2 * num_processors + g_conf_l2_banks);
    add_device(icn);
    
	// create the d-caches and set up the links
	for (uint32 i = 0; i < num_processors; i++)
	{
	    sprintf(num, "%u", i);	
		dcache_index[i] = add_device(new cmp_excl_l1_cache_t(cache+num,&cache_conf, i, get_cpu_object(i), 0));
		join_devices(proc_index[i], dcache_index[i], 1, proc_dcache_link, 1, dcache_proc_link);
        devices[dcache_index[i]]->create_direct_link(L1prot_t::interconnect_link, 
            static_cast<device_t *> (icn));
	}
	
    // Set up the IDs
    for (uint32 i = 0; i < num_processors; i++)
    {
        devices[dcache_index[i]]->set_interconnect_id(icn->get_interconnect_id(devices[dcache_index[i]]));
    }
	// I-CACHE set up
	
	uint32 icache_index[num_processors];
	
	cache_conf.assoc = g_conf_l1i_assoc;
	cache_conf.lsize = g_conf_l1i_lsize;
	cache_conf.size = g_conf_l1i_size;
	cache_conf.banks = g_conf_l1i_banks;
	cache_conf.bank_bw = g_conf_l1i_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1i_req_q_size;
	cache_conf.wait_q_size = g_conf_l1i_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1i_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1i_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l1d_writeback_buffers;
	
	link_desc_t proc_icache_link[1];
	proc_icache_link[0] = normal_link;
	proc_icache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::icache_link;
	
	link_desc_t icache_proc_link[1];
	icache_proc_link[0] = buffered_link;
	icache_proc_link[0].link_latency = g_conf_l1i_latency;
	icache_proc_link[0].link_id =  L1prot_t::proc_link;
	
	cache = "Icache";
	for (uint32 i = 0; i < num_processors; i++) 
	{
		sprintf(num, "%u", i);	
		icache_index[i] = add_device(new cmp_excl_l1_cache_t(cache+num,&cache_conf, i + num_processors, get_cpu_object(i), ICACHE_STREAM_PREFETCH));
		//icache_dev[i] = icache_index[i];
		join_devices(proc_index[i], icache_index[i], 1, proc_icache_link, 1, icache_proc_link);
        devices[icache_index[i]]->create_direct_link(L1prot_t::interconnect_link, 
            static_cast<device_t *> (icn));
	}
	
    // Set the interconnect id of Icaches
    for (uint32 i = 0; i < num_processors; i++)
    {
        devices[icache_index[i]]->set_interconnect_id(icn->get_interconnect_id(devices[icache_index[i]]));
    }
	
    // L1 directory Setup
	ASSERT(g_conf_l1i_assoc == g_conf_l1d_assoc);
	ASSERT(g_conf_l1i_size  == g_conf_l1d_size);
	ASSERT(g_conf_l1i_lsize == g_conf_l1d_lsize);
	cache_conf.assoc = g_conf_l1d_assoc * num_l1_cache;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size * num_l1_cache;
	cache_conf.banks = g_conf_l2_banks;
	cache_conf.bank_bw = g_conf_l2_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l2_req_q_size;
	cache_conf.wait_q_size = g_conf_l2_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l2_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l2_requests_per_mshr;
	cache_conf.writeback_buffersize = 0;

	uint32 l1dir_index = add_device(new cmp_excl_l1dir_array_t("L1Dir", &cache_conf, num_l1_cache + 1, 0, 10));
	devices[l1dir_index]->set_interconnect_id(icn->get_clustered_id(devices[l1dir_index], g_conf_l2_banks));
    devices[l1dir_index]->create_fifo_link(L1dir_prot_t::interconnect_link, 
        static_cast<device_t *> (icn), g_conf_l1dir_latency, 0);

	// L2 Setup
	cache_conf.assoc = g_conf_l2_assoc;
	cache_conf.lsize = g_conf_l2_lsize;
	cache_conf.size = g_conf_l2_size;
	cache_conf.banks = 1;
	cache_conf.bank_bw = 0;
	cache_conf.req_q_size = 0;
	cache_conf.wait_q_size = 0;
	cache_conf.num_mshrs = 0;
	cache_conf.requests_per_mshr = 0;
	cache_conf.writeback_buffersize = 0;

	uint32 sharedL2_index = add_device(new cmp_excl_l2_cache_t("sharedL2", &cache_conf, num_l1_cache + 2, 0));
    devices[sharedL2_index]->create_fifo_link(L2prot_t::l1dir_link, 
        static_cast<device_t *> (devices[l1dir_index]), 
		g_conf_l2_latency - g_conf_l1dir_latency, 0);

	// Connect L1Dir to L2
	devices[l1dir_index]->create_fifo_link(L1dir_prot_t::l2_addr_link, 
        static_cast<device_t *> (devices[sharedL2_index]), 0, 0);
    devices[l1dir_index]->create_fifo_link(L1dir_prot_t::l2_data_link, 
        static_cast<device_t *> (devices[sharedL2_index]), 0, 0);
    
    icn->setup_interconnect();
    uint32 mainmem_index = add_device(new main_mem_t<L2prot_t, simple_mainmem_msg_t>("mainmem", num_l1_cache + 3));
    
    link_desc_t L2_mem[2];
	L2_mem[0] = buffered_link;
	L2_mem[0].link_latency = g_conf_default_link_latency;
	L2_mem[0].link_id = cmp_excl_l2_cache_t::mainmem_addr_link; 
     
	L2_mem[1] = buffered_link;
	L2_mem[1].link_latency = g_conf_default_link_latency;
	L2_mem[1].link_id = cmp_excl_l2_cache_t::mainmem_data_link; 
    
    link_desc_t mem_L2[1];
    mem_L2[0] = buffered_link;
    mem_L2[0].link_latency = g_conf_default_link_latency;
    mem_L2[0].link_id = 0;

    join_devices(sharedL2_index, mainmem_index, 2, L2_mem, 1, mem_L2);  
    // Set the L2 cache ptr
    cmp_excl_l1dir_array_t *l1dir = static_cast<cmp_excl_l1dir_array_t *>
        (devices[l1dir_index]);
    for (uint32 i = 0; i < num_processors; i++)
    {
        cmp_excl_l1_cache_t *l1cache = static_cast<cmp_excl_l1_cache_t *>
            (devices[dcache_index[i]]);
        l1cache->set_l1dir_array_ptr(l1dir);
        l1cache = static_cast<cmp_excl_l1_cache_t *>(devices[icache_index[i]]);
        l1cache->set_l1dir_array_ptr(l1dir);
    }
        
  	num_devices = deviceindex;
	num_links = linkindex;
    
#endif
    
    
}

void mem_hier_t::create_cmp_excl_3l()
{
#ifndef COMPILE_CMP_EX_3L
    ASSERT_MSG(0, "COMPILE_CMP_EX_3L not defined\n");
#else
    typedef cmp_excl_3l_l1_protocol_t L1prot_t;
	typedef cmp_excl_3l_l2_protocol_t L2prot_t;
	typedef cmp_excl_3l_l2dir_protocol_t L2dir_prot_t;
	typedef cmp_excl_3l_l3_protocol_t L3prot_t;
	
	link_desc_t normal_link;
	link_desc_t buffered_link;
	normal_link.buffered = false;
	normal_link.link_id = 0;
	normal_link.link_latency = 0;
	normal_link.msgs_per_link = g_conf_default_messages_per_link;
	buffered_link.buffered = true;
	buffered_link.link_latency = g_conf_default_link_latency;
	buffered_link.msgs_per_link = g_conf_default_messages_per_link;
	buffered_link.link_id = 0;
	
	uint32 proc_index[num_processors];
	
	uint32 num_l1_cache = 2 * num_processors;
	uint32 num_l2_cache = num_processors;

	if (g_conf_sep_kern_cache) {
		num_l1_cache += 2 * num_processors;
		num_l2_cache += num_processors;
	}
	
	char num[3];
	string proc = "proc";
	for (uint32 i = 0; i < num_processors; i++) {
		ASSERT(get_cpu_object(i));
		sprintf(num, "%u", i);
		proc_index[i] = add_device(new proc_t<L1prot_t, simple_proc_msg_t>(proc+string(num), get_cpu_object(i)));
	}
	
	uint32 dcache_index[num_processors];
	uint32 kdcache_index[num_processors];
	
	// Add up all the D-Cache
	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = 0;
	
	// Set up link descriptor between proc and dcache
	link_desc_t proc_dcache_link[1];
	proc_dcache_link[0] = normal_link;
	proc_dcache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::dcache_link;
	
	link_desc_t proc_kdcache_link[1];
	proc_kdcache_link[0] = normal_link;
	proc_kdcache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::k_dcache_link;
	
	link_desc_t dcache_proc_link[1];
	dcache_proc_link[0] = buffered_link;
	dcache_proc_link[0].link_latency = g_conf_l1d_latency;
	dcache_proc_link[0].link_id =  L1prot_t::proc_link;
	
	string cache = "Dcache";
	string kcache = "K-Dcache";

	// Create the Interconnect First
    interconnect_t *icn;
	if (g_conf_cmp_interconnect_topo == 0) 
		icn = new interconnect_t(num_l2_cache + g_conf_l3_banks);
	else if (g_conf_cmp_interconnect_topo == 1) {
		ASSERT(num_l2_cache == g_conf_l3_banks);
		icn = new mesh_simple_t(num_l2_cache + g_conf_l3_banks);
	} else 
		FAIL;
	
    add_device(icn);
    
	// create the d-caches and set up the links
	for (uint32 i = 0; i < num_processors; i++)
	{
	    sprintf(num, "%u", i);	
		dcache_index[i] = add_device(new cmp_excl_3l_l1_cache_t(cache+num,&cache_conf, i, false, get_cpu_object(i), 0));
		join_devices(proc_index[i], dcache_index[i], 1, proc_dcache_link, 1, dcache_proc_link);
		devices[dcache_index[i]]->set_interconnect_id(L2prot_t::dcache_id);

		if (g_conf_sep_kern_cache) {
			kdcache_index[i] = add_device(new cmp_excl_3l_l1_cache_t(kcache+num,&cache_conf, i, false, get_cpu_object(i), 0));
			join_devices(proc_index[i], kdcache_index[i], 1, proc_kdcache_link, 1, dcache_proc_link);
			devices[kdcache_index[i]]->set_interconnect_id(L2prot_t::dcache_id);
		}
	}
	
	// I-CACHE set up
	
	uint32 icache_index[num_processors];
	uint32 kicache_index[num_processors];
	
	cache_conf.assoc = g_conf_l1i_assoc;
	cache_conf.lsize = g_conf_l1i_lsize;
	cache_conf.size = g_conf_l1i_size;
	cache_conf.banks = g_conf_l1i_banks;
	cache_conf.bank_bw = g_conf_l1i_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1i_req_q_size;
	cache_conf.wait_q_size = g_conf_l1i_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1i_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1i_requests_per_mshr;
	cache_conf.writeback_buffersize = 0;
	
	link_desc_t proc_icache_link[1];
	proc_icache_link[0] = normal_link;
	proc_icache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::icache_link;
	
	link_desc_t proc_kicache_link[1];
	proc_kicache_link[0] = normal_link;
	proc_kicache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::k_icache_link;
	
	link_desc_t icache_proc_link[1];
	icache_proc_link[0] = buffered_link;
	icache_proc_link[0].link_latency = g_conf_l1i_latency;
	icache_proc_link[0].link_id =  L1prot_t::proc_link;
	
	cache = "Icache";
	kcache = "K-Icache";
	for (uint32 i = 0; i < num_processors; i++) 
	{
		sprintf(num, "%u", i);	
		icache_index[i] = add_device(new cmp_excl_3l_l1_cache_t(cache+num,&cache_conf, i, true, get_cpu_object(i), ICACHE_STREAM_PREFETCH));
		join_devices(proc_index[i], icache_index[i], 1, proc_icache_link, 1, icache_proc_link);
		devices[icache_index[i]]->set_interconnect_id(L2prot_t::icache_id);
		
		if (g_conf_sep_kern_cache) {
			kicache_index[i] = add_device(new cmp_excl_3l_l1_cache_t(kcache+num,&cache_conf, i, true, get_cpu_object(i), ICACHE_STREAM_PREFETCH));
			join_devices(proc_index[i], kicache_index[i], 1, proc_kicache_link, 1, icache_proc_link);
			devices[kicache_index[i]]->set_interconnect_id(L2prot_t::icache_id);
		}			
	}
	

	cache_conf.assoc = g_conf_l2_assoc;
	cache_conf.lsize = g_conf_l2_lsize;
	cache_conf.size = g_conf_l2_size;
	cache_conf.banks = g_conf_l2_banks;
	cache_conf.bank_bw = g_conf_l2_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l2_req_q_size;
	cache_conf.wait_q_size = g_conf_l2_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l2_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l2_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l2_writeback_buffers;
	
	link_desc_t l1l2_link[1];
	l1l2_link[0] = buffered_link;
	l1l2_link[0].link_id = L1prot_t::l2_link;
	l1l2_link[0].link_latency = 1;
	
	link_desc_t l2l1d_link[1];
	l2l1d_link[0] = buffered_link;
	l2l1d_link[0].link_latency = g_conf_l2_latency;
	l2l1d_link[0].link_id =  L2prot_t::dcache_link;
	
	link_desc_t l2l1i_link[1];
	l2l1i_link[0] = buffered_link;
	l2l1i_link[0].link_latency = g_conf_l2_latency;
	l2l1i_link[0].link_id =  L2prot_t::icache_link;
	
	uint32 l2cache_index[num_processors];
	uint32 kl2cache_index[num_processors];

	// create the L2 caches and set up the links
	cache = "L2cache";
	kcache = "K-L2cache";
	for (uint32 i = 0; i < num_processors; i++)
	{
	    sprintf(num, "%u", i);	
		l2cache_index[i] = add_device(new cmp_excl_3l_l2_cache_t(cache+num,&cache_conf, i, get_cpu_object(i), 0));
		join_devices(l2cache_index[i], dcache_index[i], 1, l2l1d_link, 1, l1l2_link);
		join_devices(l2cache_index[i], icache_index[i], 1, l2l1i_link, 1, l1l2_link);
        devices[l2cache_index[i]]->create_direct_link(L2prot_t::interconnect_link, 
            static_cast<device_t *> (icn));
		devices[l2cache_index[i]]->set_interconnect_id(icn->get_interconnect_id(devices[l2cache_index[i]]));

		if (g_conf_sep_kern_cache) {
			kl2cache_index[i] = add_device(new cmp_excl_3l_l2_cache_t(kcache+num,&cache_conf, i, get_cpu_object(i), 0));
			join_devices(kl2cache_index[i], kdcache_index[i], 1, l2l1d_link, 1, l1l2_link);
			join_devices(kl2cache_index[i], kicache_index[i], 1, l2l1i_link, 1, l1l2_link);
			devices[kl2cache_index[i]]->create_direct_link(L2prot_t::interconnect_link, 
            	static_cast<device_t *> (icn));
			devices[kl2cache_index[i]]->set_interconnect_id(icn->get_interconnect_id(devices[kl2cache_index[i]]));
		}
	}

	// L2 Directory Setup
    if (g_conf_l2dir_assoc)
        cache_conf.assoc = g_conf_l2dir_assoc;
    else
        cache_conf.assoc = (g_conf_l2_assoc * round_up_pow2(num_l2_cache)) / g_conf_l2dir_assoc_factor;
	cache_conf.lsize = g_conf_l2_lsize;
    if (g_conf_l2dir_size)
        cache_conf.size = g_conf_l2dir_size;
    else
        cache_conf.size = g_conf_l2dir_assoc_factor * g_conf_l2_size * round_up_pow2(num_l2_cache);
	cache_conf.banks = g_conf_l3_banks;
	cache_conf.bank_bw = g_conf_l3_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l3_req_q_size;
	cache_conf.wait_q_size = g_conf_l3_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l3_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l3_requests_per_mshr;
	cache_conf.writeback_buffersize = 8 * g_conf_l2_writeback_buffers;
    uint64 prefetch_type = g_conf_l3_stride_prefetch ? DCACHE_STRIDE_PREFETCH : 0;
	
    
	uint32 l2dir_index = add_device(new cmp_excl_3l_l2dir_array_t("L2Dir", &cache_conf, 0, prefetch_type, 10, num_l2_cache));
	devices[l2dir_index]->set_interconnect_id(icn->get_clustered_id(devices[l2dir_index], g_conf_l3_banks));
    devices[l2dir_index]->create_fifo_link(L2dir_prot_t::interconnect_link, 
        static_cast<device_t *> (icn), g_conf_l1dir_latency, 0);

	// L3 Setup
	cache_conf.assoc = g_conf_l3_assoc;
	cache_conf.lsize = g_conf_l3_lsize;
	cache_conf.size = g_conf_l3_size;
	cache_conf.banks = g_conf_l3_banks;
	cache_conf.bank_bw = g_conf_l3_bank_bw_inv;
	cache_conf.req_q_size = 0;
	cache_conf.wait_q_size = 0;
	cache_conf.num_mshrs = 0;
	cache_conf.requests_per_mshr = 0;
	cache_conf.writeback_buffersize = 0;

    uint32 sharedL3_index = add_device(new cmp_excl_3l_l3_cache_t("sharedL3", &cache_conf, num_l1_cache + 2, 0));
    devices[sharedL3_index]->create_fifo_link(L3prot_t::l2dir_link, 
        static_cast<device_t *> (devices[l2dir_index]), 
		g_conf_l3_latency - g_conf_l1dir_latency, 0);

	// Connect L2Dir to L3 -- use one FIFO
	devices[l2dir_index]->create_fifo_link(L2dir_prot_t::l3_addr_link, 
        static_cast<device_t *> (devices[sharedL3_index]), 0, 0);
    devices[l2dir_index]->set_link(L2dir_prot_t::l3_data_link,
		devices[l2dir_index]->get_link(L2dir_prot_t::l3_addr_link));
    
    icn->setup_interconnect();
    uint32 mainmem_index = add_device(new main_mem_t<L2prot_t, simple_mainmem_msg_t>("mainmem", num_l1_cache + 3));
    
	// One link for both for now
    // link_desc_t L3_mem[2];
	// L3_mem[0] = bandwidth_fifo_link;
	// L3_mem[0].link_latency = g_conf_default_link_latency;
	// L3_mem[0].link_id = cmp_excl_3l_l3_cache_t::mainmem_addr_link; 
     
	// L3_mem[1] = buffered_link;
	// L3_mem[1].link_latency = g_conf_default_link_latency;
	// L3_mem[1].link_id = cmp_excl_3l_l3_cache_t::mainmem_data_link; 
    
    link_desc_t mem_L3[1];
    mem_L3[0] = buffered_link;
    mem_L3[0].link_latency = g_conf_default_link_latency;
    mem_L3[0].link_id = 0;

    join_devices(sharedL3_index, mainmem_index, 0, NULL, 1, mem_L3);  

	// g_conf_main_mem_bandwidth = bytes / 1000 cycles
	uint64 cycles_per_message = 0;
	if (g_conf_main_mem_bandwidth)
		cycles_per_message = 1000 * g_conf_l3_lsize / g_conf_main_mem_bandwidth;
	
	// connect same link for addr and data
	link_t *bw_link = new bandwidth_fifo_link_t(devices[sharedL3_index],
		devices[mainmem_index], g_conf_default_link_latency, 0, 
		cycles_per_message);
	links[linkindex++] =  bw_link;
	
	devices[sharedL3_index]->set_link(cmp_excl_3l_l3_cache_t::mainmem_addr_link,
		bw_link);
	devices[sharedL3_index]->set_link(cmp_excl_3l_l3_cache_t::mainmem_data_link,
		bw_link);

    // Set the L2 cache ptr
    cmp_excl_3l_l2dir_array_t *l2dir = static_cast<cmp_excl_3l_l2dir_array_t *>
        (devices[l2dir_index]);
    for (uint32 i = 0; i < num_processors; i++)
    {
        cmp_excl_3l_l2_cache_t *l2cache = static_cast<cmp_excl_3l_l2_cache_t *>
            (devices[l2cache_index[i]]);
        l2cache->set_l2dir_array_ptr(l2dir);
		
		if (g_conf_sep_kern_cache) {
			l2cache = static_cast<cmp_excl_3l_l2_cache_t *>
				(devices[kl2cache_index[i]]);
			l2cache->set_l2dir_array_ptr(l2dir);
			l2cache->set_kernel_cache(true);
			
			cmp_excl_3l_l1_cache_t *l1cache;
			l1cache = static_cast<cmp_excl_3l_l1_cache_t *>
				(devices[kicache_index[i]]);
			l1cache->set_kernel_cache(true);
			l1cache = static_cast<cmp_excl_3l_l1_cache_t *>
				(devices[kdcache_index[i]]);
			l1cache->set_kernel_cache(true);
			
		}
    }
        
  	num_devices = deviceindex;
	num_links = linkindex;
    
#endif
    
    
}



void mem_hier_t::create_cmp_incl_3l()
{
#ifndef COMPILE_CMP_INCL_3L
    ASSERT_MSG(0, "COMPILE_CMP_INCL_3L not defined\n");
#else
    	typedef cmp_incl_3l_l1_protocol_t L1prot_t;
	typedef cmp_incl_3l_l2_protocol_t L2prot_t;
	typedef cmp_incl_3l_l3_protocol_t L3prot_t;
	
	link_desc_t normal_link;
	link_desc_t buffered_link;
	normal_link.buffered = false;
	normal_link.link_id = 0;
	normal_link.link_latency = 0;
	normal_link.msgs_per_link = g_conf_default_messages_per_link;
	buffered_link.buffered = true;
	buffered_link.link_latency = g_conf_default_link_latency;
	buffered_link.msgs_per_link = g_conf_default_messages_per_link;
	buffered_link.link_id = 0;
	
	uint32 proc_index[num_processors];
	
    	uint32 num_l1_cache = 2 * num_processors;
    	uint32 num_l2_cache = num_processors;

	if (g_conf_sep_kern_cache) {
		num_l1_cache += 2 * num_processors;
		num_l2_cache += num_processors;
	}
	
	char num[3];
	string proc = "proc";
	for (uint32 i = 0; i < num_processors; i++) {
		ASSERT(get_cpu_object(i));
		sprintf(num, "%u", i);
		proc_index[i] = add_device(new proc_t<L1prot_t, simple_proc_msg_t>(proc+string(num), get_cpu_object(i)));
	}
	
	uint32 *dcache_index = new uint32[num_processors];
	uint32 kdcache_index[num_processors];
	
	// Add up all the D-Cache
	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = 0;
	
	// Set up link descriptor between proc and dcache
	link_desc_t proc_dcache_link[1];
	proc_dcache_link[0] = normal_link;
	proc_dcache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::dcache_link;
	
	link_desc_t proc_kdcache_link[1];
	proc_kdcache_link[0] = normal_link;
	proc_kdcache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::k_dcache_link;
	
	link_desc_t dcache_proc_link[1];
	dcache_proc_link[0] = buffered_link;
	dcache_proc_link[0].link_latency = g_conf_l1d_latency;
	dcache_proc_link[0].link_id =  L1prot_t::proc_link;
	
	string cache = "Dcache";
	string kcache = "K-Dcache";

	// Create the Interconnect First
    interconnect_t *icn;
	if (g_conf_cmp_interconnect_topo == 0) 
		icn = new interconnect_t(num_l2_cache + g_conf_l3_banks);
	else if (g_conf_cmp_interconnect_topo == 1) {
		ASSERT(num_l2_cache == g_conf_l3_banks);
		icn = new mesh_simple_t(num_l2_cache + g_conf_l3_banks);
	} else 
		FAIL;
	
    add_device(icn);
    
	// create the d-caches and set up the links
	for (uint32 i = 0; i < num_processors; i++)
	{
	    sprintf(num, "%u", i);	
		dcache_index[i] = add_device(new cmp_incl_3l_l1_cache_t(cache+num,&cache_conf, i, false, get_cpu_object(i), 0));
		join_devices(proc_index[i], dcache_index[i], 1, proc_dcache_link, 1, dcache_proc_link);
		devices[dcache_index[i]]->set_interconnect_id(L2prot_t::dcache_id);

		if (g_conf_sep_kern_cache) {
			kdcache_index[i] = add_device(new cmp_incl_3l_l1_cache_t(kcache+num,&cache_conf, i, false, get_cpu_object(i), 0));
			join_devices(proc_index[i], kdcache_index[i], 1, proc_kdcache_link, 1, dcache_proc_link);
			devices[kdcache_index[i]]->set_interconnect_id(L2prot_t::dcache_id);
		}
	}
	
	// I-CACHE set up
	
	uint32 *icache_index = new uint32[num_processors];
	uint32 kicache_index[num_processors];
	
	cache_conf.assoc = g_conf_l1i_assoc;
	cache_conf.lsize = g_conf_l1i_lsize;
	cache_conf.size = g_conf_l1i_size;
	cache_conf.banks = g_conf_l1i_banks;
	cache_conf.bank_bw = g_conf_l1i_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1i_req_q_size;
	cache_conf.wait_q_size = g_conf_l1i_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1i_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1i_requests_per_mshr;
	cache_conf.writeback_buffersize = 0;
	
	link_desc_t proc_icache_link[1];
	proc_icache_link[0] = normal_link;
	proc_icache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::icache_link;
	
	link_desc_t proc_kicache_link[1];
	proc_kicache_link[0] = normal_link;
	proc_kicache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::k_icache_link;
	
	link_desc_t icache_proc_link[1];
	icache_proc_link[0] = buffered_link;
	icache_proc_link[0].link_latency = g_conf_l1i_latency;
	icache_proc_link[0].link_id =  L1prot_t::proc_link;
	
	cache = "Icache";
	kcache = "K-Icache";
	for (uint32 i = 0; i < num_processors; i++) 
	{
		sprintf(num, "%u", i);	
		icache_index[i] = add_device(new cmp_incl_3l_l1_cache_t(cache+num,&cache_conf, i, true, get_cpu_object(i), ICACHE_STREAM_PREFETCH));
		join_devices(proc_index[i], icache_index[i], 1, proc_icache_link, 1, icache_proc_link);
		devices[icache_index[i]]->set_interconnect_id(L2prot_t::icache_id);
		
		if (g_conf_sep_kern_cache) {
			kicache_index[i] = add_device(new cmp_incl_3l_l1_cache_t(kcache+num,&cache_conf, i, true, get_cpu_object(i), ICACHE_STREAM_PREFETCH));
			join_devices(proc_index[i], kicache_index[i], 1, proc_kicache_link, 1, icache_proc_link);
			devices[kicache_index[i]]->set_interconnect_id(L2prot_t::icache_id);
		}			
	}
	

	cache_conf.assoc = g_conf_l2_assoc;
	cache_conf.lsize = g_conf_l2_lsize;
	cache_conf.size = g_conf_l2_size;
	cache_conf.banks = g_conf_l2_banks;
	cache_conf.bank_bw = g_conf_l2_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l2_req_q_size;
	cache_conf.wait_q_size = g_conf_l2_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l2_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l2_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l2_writeback_buffers;
	
	link_desc_t l1l2_link[1];
	l1l2_link[0] = buffered_link;
	l1l2_link[0].link_id = L1prot_t::l2_link;
	l1l2_link[0].link_latency = 1;
	
	link_desc_t l2l1d_link[1];
	l2l1d_link[0] = buffered_link;
	l2l1d_link[0].link_latency = g_conf_l2_latency;
	l2l1d_link[0].link_id =  L2prot_t::dcache_link;
	
	link_desc_t l2l1i_link[1];
	l2l1i_link[0] = buffered_link;
	l2l1i_link[0].link_latency = g_conf_l2_latency;
	l2l1i_link[0].link_id =  L2prot_t::icache_link;
	
	uint32 l2cache_index[num_processors];
	uint32 kl2cache_index[num_processors];

	// create the L2 caches and set up the links
	cache = "L2cache";
	kcache = "K-L2cache";
	for (uint32 i = 0; i < num_processors; i++)
	{
	    sprintf(num, "%u", i);	
		l2cache_index[i] = add_device(new cmp_incl_3l_l2_cache_t(cache+num,&cache_conf, i, get_cpu_object(i), 0));
		join_devices(l2cache_index[i], dcache_index[i], 1, l2l1d_link, 1, l1l2_link);
		join_devices(l2cache_index[i], icache_index[i], 1, l2l1i_link, 1, l1l2_link);
        devices[l2cache_index[i]]->create_direct_link(L2prot_t::interconnect_link, 
            static_cast<device_t *> (icn));
		devices[l2cache_index[i]]->set_interconnect_id(icn->get_interconnect_id(devices[l2cache_index[i]]));

		if (g_conf_sep_kern_cache) {
			kl2cache_index[i] = add_device(new cmp_incl_3l_l2_cache_t(kcache+num,&cache_conf, i, get_cpu_object(i), 0));
			join_devices(kl2cache_index[i], kdcache_index[i], 1, l2l1d_link, 1, l1l2_link);
			join_devices(kl2cache_index[i], kicache_index[i], 1, l2l1i_link, 1, l1l2_link);
			devices[kl2cache_index[i]]->create_direct_link(L2prot_t::interconnect_link, 
            	static_cast<device_t *> (icn));
			devices[kl2cache_index[i]]->set_interconnect_id(icn->get_interconnect_id(devices[kl2cache_index[i]]));
		}
	}

	// L2 Directory Setup
	
    // L3 Setup
	cache_conf.assoc = g_conf_l3_assoc;
	cache_conf.lsize = g_conf_l3_lsize;
	cache_conf.size = g_conf_l3_size;
	cache_conf.banks = g_conf_l3_banks;
	cache_conf.bank_bw = g_conf_l3_bank_bw_inv;
	cache_conf.req_q_size = 0;
	cache_conf.wait_q_size = 0;
	cache_conf.num_mshrs = g_conf_l2_num_mshrs * num_l2_cache;
	cache_conf.requests_per_mshr = g_conf_l2_requests_per_mshr * num_l2_cache;
	cache_conf.writeback_buffersize = g_conf_l2_writeback_buffers;
    uint64 prefetch_type = g_conf_l3_stride_prefetch ? DCACHE_STRIDE_PREFETCH : 0;
	
    
    uint32 sharedL3_index = add_device(new cmp_incl_3l_l3_cache_t("sharedL3", &cache_conf,
        0, prefetch_type, 10, num_l2_cache));
	devices[sharedL3_index]->set_interconnect_id(icn->get_clustered_id(devices[sharedL3_index], g_conf_l3_banks));
    devices[sharedL3_index]->create_fifo_link(L3prot_t::interconnect_link, 
        static_cast<device_t *> (icn), g_conf_l3_latency, 0);

	

    
    
    icn->setup_interconnect();
    uint32 mainmem_index = add_device(new main_mem_t<L2prot_t, simple_mainmem_msg_t>("mainmem", num_l1_cache + 3));
    
	// One link for both for now
    // link_desc_t L3_mem[2];
	// L3_mem[0] = bandwidth_fifo_link;
	// L3_mem[0].link_latency = g_conf_default_link_latency;
	// L3_mem[0].link_id = cmp_excl_3l_l3_cache_t::mainmem_addr_link; 
     
	// L3_mem[1] = buffered_link;
	// L3_mem[1].link_latency = g_conf_default_link_latency;
	// L3_mem[1].link_id = cmp_excl_3l_l3_cache_t::mainmem_data_link; 
    
    link_desc_t mem_L3[1];
    mem_L3[0] = buffered_link;
    mem_L3[0].link_latency = g_conf_default_link_latency;
    mem_L3[0].link_id = 0;

    join_devices(sharedL3_index, mainmem_index, 0, NULL, 1, mem_L3);  

	// g_conf_main_mem_bandwidth = bytes / 1000 cycles
	uint64 cycles_per_message = 1000 * g_conf_l3_lsize / g_conf_main_mem_bandwidth;
	
	// connect same link for addr and data
	link_t *bw_link = new bandwidth_fifo_link_t(devices[sharedL3_index],
		devices[mainmem_index], g_conf_default_link_latency, 0, 
		cycles_per_message);
	links[linkindex++] =  bw_link;
	
	devices[sharedL3_index]->set_link(cmp_incl_3l_l3_cache_t::mainmem_addr_link,
		bw_link);
	devices[sharedL3_index]->set_link(cmp_incl_3l_l3_cache_t::mainmem_data_link,
		bw_link);

    // Set the L2 cache ptr
    cmp_incl_3l_l3_cache_t *l3_cache = static_cast<cmp_incl_3l_l3_cache_t *>
        (devices[sharedL3_index]);
    for (uint32 i = 0; i < num_processors; i++)
    {
        cmp_incl_3l_l2_cache_t *l2cache = static_cast<cmp_incl_3l_l2_cache_t *>
            (devices[l2cache_index[i]]);
        l2cache->set_l3_cache_ptr(l3_cache);
		
		if (g_conf_sep_kern_cache) {
			l2cache = static_cast<cmp_incl_3l_l2_cache_t *>
				(devices[kl2cache_index[i]]);
			l2cache->set_l3_cache_ptr(l3_cache);
			l2cache->set_kernel_cache(true);
			
			cmp_incl_3l_l1_cache_t *l1cache;
			l1cache = static_cast<cmp_incl_3l_l1_cache_t *>
				(devices[kicache_index[i]]);
			l1cache->set_kernel_cache(true);
			l1cache = static_cast<cmp_incl_3l_l1_cache_t *>
				(devices[kdcache_index[i]]);
			l1cache->set_kernel_cache(true);
			
		}
    }
        
  	num_devices = deviceindex;
	num_links = linkindex;
    
    
#ifdef POWER_COMPILE
    if (g_conf_power_profile)
        register_icache_dcache_power(dcache_index, icache_index);
#endif

    
#endif
    
    
}

void mem_hier_t::create_cmp_incl(bool write_through)
{
#ifndef COMPILE_CMP_INCL
    	ASSERT_MSG(0, "COMPILE_CMP_INCL not defined\n");
#else
    	typedef cmp_incl_l1_protocol_t L1prot_t;
	typedef cmp_incl_l2_protocol_t L2prot_t;
	
	link_desc_t normal_link;
	link_desc_t buffered_link;
	normal_link.buffered = false;
	normal_link.link_id = 0;
	normal_link.link_latency = 0;
	normal_link.msgs_per_link = g_conf_default_messages_per_link;
	buffered_link.buffered = true;
	buffered_link.link_latency = g_conf_default_link_latency;
	buffered_link.msgs_per_link = g_conf_default_messages_per_link;
	buffered_link.link_id = 0;
	
	uint32 proc_index[num_processors];
	
	char num[2];
	string proc = "proc";

	for (uint32 i = 0; i < num_processors; i++) {
		ASSERT(get_cpu_object(i));
		sprintf(num, "%u", i);
		proc_index[i] = add_device(new proc_t<L1prot_t, simple_proc_msg_t>(proc+string(num), get_cpu_object(i)));
	}
	
	uint32 *dcache_index = new uint32[num_processors];
    	uint32 kdcache_index[num_processors];
	
	// Add up all the D-Cache
	cache_config_t cache_conf;
    	cache_conf.bank_leak_energy = 0;
    
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l1d_writeback_buffers;
	cache_conf.bank_leak_energy = g_conf_l1d_bank_leak;
    	cache_conf.bits_out = 64;
    
	// Set up link descriptor between proc and dcache
	link_desc_t proc_dcache_link[1];
	proc_dcache_link[0] = normal_link;
	proc_dcache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::dcache_link;
    
    	link_desc_t proc_kdcache_link[1];
	proc_kdcache_link[0] = normal_link;
	proc_kdcache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::k_dcache_link;
	
	link_desc_t dcache_proc_link[1];
	dcache_proc_link[0] = buffered_link;
	dcache_proc_link[0].link_latency = g_conf_l1d_latency;
	dcache_proc_link[0].link_id =  L1prot_t::proc_link;
	
	string cache = "Dcache";
    string kcache = "K-Dcache";
	// Create the Interconnect First
    
    interconnect_t *icn;
    if (g_conf_sep_kern_cache)
        icn = new interconnect_t(4 * num_processors + g_conf_l2_banks);
    else
        icn = new interconnect_t(2 * num_processors + g_conf_l2_banks);
    add_device(icn);
    
	// create the d-caches and set up the links
	for (uint32 i = 0; i < num_processors; i++)
	{
	    sprintf(num, "%u", i);	
        if (write_through)
            dcache_index[i] = add_device(new cmp_incl_wt_l1_cache_t(cache+num,&cache_conf, i, get_cpu_object(i), 0));
	else 
            dcache_index[i] = add_device(new cmp_incl_l1_cache_t(cache+num,&cache_conf, i, get_cpu_object(i), 0));
		
	join_devices(proc_index[i], dcache_index[i], 1, proc_dcache_link, 1, dcache_proc_link);
        devices[dcache_index[i]]->create_direct_link(L1prot_t::interconnect_link, 
            static_cast<device_t *> (icn));
        
       if (g_conf_sep_kern_cache) {
			kdcache_index[i] = add_device(new cmp_incl_l1_cache_t(kcache+num,&cache_conf, i, get_cpu_object(i), 0));
			join_devices(proc_index[i], kdcache_index[i], 1, proc_kdcache_link, 1, dcache_proc_link);
		    devices[kdcache_index[i]]->create_direct_link(L1prot_t::interconnect_link, 
                static_cast<device_t *> (icn));
	}
	}
	
    // Set up the IDs
    for (uint32 i = 0; i < num_processors; i++)
    {
        devices[dcache_index[i]]->set_interconnect_id(icn->get_interconnect_id(devices[dcache_index[i]]));
        if (g_conf_sep_kern_cache) {
            devices[kdcache_index[i]]->set_interconnect_id(icn->get_interconnect_id(devices[kdcache_index[i]]));
        }
    }
	// I-CACHE set up
	
	uint32 *icache_index = new uint32[num_processors];
    uint32 *kicache_index = new uint32[num_processors];
	
	cache_conf.assoc = g_conf_l1i_assoc;
	cache_conf.lsize = g_conf_l1i_lsize;
	cache_conf.size = g_conf_l1i_size;
	cache_conf.banks = g_conf_l1i_banks;
	cache_conf.bank_bw = g_conf_l1i_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1i_req_q_size;
	cache_conf.wait_q_size = g_conf_l1i_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1i_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1i_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l1d_writeback_buffers;
	cache_conf.bank_leak_energy = g_conf_l1i_bank_leak;
    cache_conf.bits_out = 64;
	
    link_desc_t proc_icache_link[1];
	proc_icache_link[0] = normal_link;
	proc_icache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::icache_link;
    
    link_desc_t proc_kicache_link[1];
	proc_kicache_link[0] = normal_link;
	proc_kicache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::k_icache_link;
	
	link_desc_t icache_proc_link[1];
	icache_proc_link[0] = buffered_link;
	icache_proc_link[0].link_latency = g_conf_l1i_latency;
	icache_proc_link[0].link_id =  L1prot_t::proc_link;
	
	cache = "Icache";
    kcache = "K-Icache";
    
	for (uint32 i = 0; i < num_processors; i++) 
	{
		sprintf(num, "%u", i);
        if (write_through)
            icache_index[i] = add_device(new cmp_incl_wt_l1_cache_t(cache+num,&cache_conf, i + num_processors, get_cpu_object(i), ICACHE_STREAM_PREFETCH));
        else        
            icache_index[i] = add_device(new cmp_incl_l1_cache_t(cache+num,&cache_conf, i + num_processors, get_cpu_object(i), ICACHE_STREAM_PREFETCH));
		//icache_dev[i] = icache_index[i];
		join_devices(proc_index[i], icache_index[i], 1, proc_icache_link, 1, icache_proc_link);
        devices[icache_index[i]]->create_direct_link(L1prot_t::interconnect_link, 
            static_cast<device_t *> (icn));
        
        if (g_conf_sep_kern_cache) {
			kicache_index[i] = add_device(new cmp_incl_l1_cache_t(kcache+num,&cache_conf, i + num_processors, get_cpu_object(i), ICACHE_STREAM_PREFETCH));
			join_devices(proc_index[i], kicache_index[i], 1, proc_kicache_link, 1, icache_proc_link);
		    devices[kicache_index[i]]->create_direct_link(L1prot_t::interconnect_link, 
                static_cast<device_t *> (icn));
        	
		}		
	}
	
    // Set the interconnect id of Icaches
    for (uint32 i = 0; i < num_processors; i++)
    {
        devices[icache_index[i]]->set_interconnect_id(icn->get_interconnect_id(devices[icache_index[i]]));
        if (g_conf_sep_kern_cache) {
            devices[kicache_index[i]]->set_interconnect_id(icn->get_interconnect_id(devices[kicache_index[i]]));
        }
    }
	
    
    // L2 Setup
	cache_conf.assoc = g_conf_l2_assoc;
	cache_conf.lsize = g_conf_l2_lsize;
	cache_conf.size = g_conf_l2_size;
	cache_conf.banks = g_conf_l2_banks;
	cache_conf.bank_bw = g_conf_l2_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l2_req_q_size;
	cache_conf.wait_q_size = g_conf_l2_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l2_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l2_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l2_writeback_buffers;
    cache_conf.bank_leak_energy = g_conf_l2_bank_leak;
    cache_conf.bits_out = 512;
    
    uint32 sharedL2_index;
    uint32 sharedL2_ID = (g_conf_sep_kern_cache) ? (4 * num_processors + 1) : (2 + num_processors + 1);
	if (write_through)
        sharedL2_index = add_device(new cmp_incl_wt_l2_cache_t("sharedL2", &cache_conf, sharedL2_ID, 0, 10));
    else         
        sharedL2_index = add_device(new cmp_incl_l2_cache_t("sharedL2", &cache_conf, sharedL2_ID , 0, 10));
	devices[sharedL2_index]->set_interconnect_id(icn->get_clustered_id(devices[sharedL2_index], g_conf_l2_banks));
    devices[sharedL2_index]->create_fifo_link(L2prot_t::interconnect_link, 
        static_cast<device_t *> (icn), g_conf_l2_latency, 0);
        
    
    icn->setup_interconnect();
    //uint32 mainmem_index = add_device(new main_mem_t<L2prot_t, simple_mainmem_msg_t>("mainmem", num_processors + 1));
    uint32 mainmem_index = add_device(dramptr);
    
    link_desc_t L2_mem[2];
	L2_mem[0] = buffered_link;
	L2_mem[0].link_latency = g_conf_default_link_latency;
	L2_mem[0].link_id = cmp_incl_l2_cache_t::mainmem_addr_link; 
     
	L2_mem[1] = buffered_link;
	L2_mem[1].link_latency = g_conf_default_link_latency;
	L2_mem[1].link_id = cmp_incl_l2_cache_t::mainmem_data_link; 
    
    link_desc_t mem_L2[1];
    mem_L2[0] = buffered_link;
    mem_L2[0].link_latency = g_conf_default_link_latency;
    mem_L2[0].link_id = 0;

    join_devices(sharedL2_index, mainmem_index, 2, L2_mem, 1, mem_L2);  
    // Set the L2 cache ptr
    cmp_incl_l2_cache_t *l2cache = static_cast<cmp_incl_l2_cache_t *>
        (devices[sharedL2_index]);
    for (uint32 i = 0; i < num_processors; i++)
    {
        cmp_incl_l1_cache_t *l1cache = static_cast<cmp_incl_l1_cache_t *>
            (devices[dcache_index[i]]);
        l1cache->set_L2_cache_ptr(l2cache);
        l1cache = static_cast<cmp_incl_l1_cache_t *>(devices[icache_index[i]]);
        l1cache->set_L2_cache_ptr(l2cache);
        if (g_conf_sep_kern_cache) {
            cmp_incl_l1_cache_t *l1cache = static_cast<cmp_incl_l1_cache_t *>
            (devices[kdcache_index[i]]);
            l1cache->set_L2_cache_ptr(l2cache);
            l1cache = static_cast<cmp_incl_l1_cache_t *>(devices[kicache_index[i]]);
            l1cache->set_L2_cache_ptr(l2cache);
                        
        }
    }
  	num_devices = deviceindex;
	num_links = linkindex;

#ifdef POWER_COMPILE
    if (g_conf_power_profile)
        register_icache_dcache_power(dcache_index, icache_index);
#endif


#endif
    
    
}

#ifdef POWER_COMPILE
void mem_hier_t::register_icache_dcache_power(uint32 *dcache_index, uint32 *icache_index)
{
    for (uint32 i = 0; i < num_processors; i++)
    {
        cache_t *icache = (cache_t *) devices[icache_index[i]];
        cache_t *dcache = (cache_t *) devices[dcache_index[i]];
        core_power_t *c_power = ((sequencer_t *) cpus[i])->get_core_power_profile();
        c_power->set_icache_power_profile(icache->get_cache_power());
        icache->get_cache_power()->register_cpu_object(cpus[i]);
        c_power->set_dcache_power_profile(dcache->get_cache_power());
        dcache->get_cache_power()->register_cpu_object(cpus[i]);
    }
    delete icache_index;
    delete dcache_index;
    
}
#endif

void mem_hier_t::create_cmp_incl_l2banks()
{
#ifndef COMPILE_CMP_INCL
    ASSERT_MSG(0, "COMPILE_CMP_INCL not defined\n");
#else
    typedef cmp_incl_l1_protocol_t L1prot_t;
	typedef cmp_incl_l2_protocol_t L2prot_t;
	
	link_desc_t normal_link;
	link_desc_t buffered_link;
	normal_link.buffered = false;
	normal_link.link_id = 0;
	normal_link.link_latency = 0;
	normal_link.msgs_per_link = g_conf_default_messages_per_link;
	buffered_link.buffered = true;
	buffered_link.link_latency = g_conf_default_link_latency;
	buffered_link.msgs_per_link = g_conf_default_messages_per_link;
	buffered_link.link_id = 0;
	uint32 proc_index[num_processors];
	
	char num[2];
	string proc = "proc";
	for (uint32 i = 0; i < num_processors; i++) {
		ASSERT(get_cpu_object(i));
		sprintf(num, "%u", i);
		proc_index[i] = add_device(new proc_t<L1prot_t, simple_proc_msg_t>(proc+string(num), get_cpu_object(i)));
	}
	
	uint32 *dcache_index = new uint32[num_processors];
	
	// Add up all the D-Cache
	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l1d_writeback_buffers;
	cache_conf.bank_stripe_bits = 0;
	
	// Set up link descriptor between proc and dcache
	link_desc_t proc_dcache_link[1];
	proc_dcache_link[0] = normal_link;
	proc_dcache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::dcache_link;
	
	link_desc_t dcache_proc_link[1];
	dcache_proc_link[0] = buffered_link;
	dcache_proc_link[0].link_latency = g_conf_l1d_latency;
	dcache_proc_link[0].link_id =  L1prot_t::proc_link;
	
	uint32 mesh_x_size = num_processors/2;
	// e.g., 4 banks are two rows of 2
	if (g_conf_l2_banks <= mesh_x_size)
		mesh_x_size /= 2;
	
	string cache = "Dcache";
	// Create the Interconnect First
    interconnect_t *icn = new mesh_sandwich_t(2 * num_processors + g_conf_l2_banks,
		mesh_x_size);
    add_device(icn);
    
	// create the d-caches and set up the links
	for (uint32 i = 0; i < num_processors; i++)
	{
	    sprintf(num, "%u", i);	
		dcache_index[i] = add_device(new cmp_incl_l1_cache_t(cache+num,&cache_conf, i, get_cpu_object(i), 0));
		join_devices(proc_index[i], dcache_index[i], 1, proc_dcache_link, 1, dcache_proc_link);
        devices[dcache_index[i]]->create_direct_link(L1prot_t::interconnect_link, 
            static_cast<device_t *> (icn));
	}
	
	// I-CACHE set up
	
	uint32 *icache_index = new uint32[num_processors];
	
	cache_conf.assoc = g_conf_l1i_assoc;
	cache_conf.lsize = g_conf_l1i_lsize;
	cache_conf.size = g_conf_l1i_size;
	cache_conf.banks = g_conf_l1i_banks;
	cache_conf.bank_bw = g_conf_l1i_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1i_req_q_size;
	cache_conf.wait_q_size = g_conf_l1i_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1i_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1i_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l1d_writeback_buffers;
	cache_conf.bank_stripe_bits = 0;
	
	link_desc_t proc_icache_link[1];
	proc_icache_link[0] = normal_link;
	proc_icache_link[0].link_id = proc_t<L1prot_t, simple_proc_msg_t>::icache_link;
	
	link_desc_t icache_proc_link[1];
	icache_proc_link[0] = buffered_link;
	icache_proc_link[0].link_latency = g_conf_l1i_latency;
	icache_proc_link[0].link_id =  L1prot_t::proc_link;
	
	cache = "Icache";
	for (uint32 i = 0; i < num_processors; i++) 
	{
		sprintf(num, "%u", i);	
		icache_index[i] = add_device(new cmp_incl_l1_cache_t(cache+num,&cache_conf, i + num_processors, get_cpu_object(i), ICACHE_STREAM_PREFETCH));
		//icache_dev[i] = icache_index[i];
		join_devices(proc_index[i], icache_index[i], 1, proc_icache_link, 1, icache_proc_link);
        devices[icache_index[i]]->create_direct_link(L1prot_t::interconnect_link, 
            static_cast<device_t *> (icn));
	}
	
    // Set the interconnect id of Icaches and Dcaches together
    for (uint32 i = 0; i < num_processors; i++)
    {
        devices[dcache_index[i]]->set_interconnect_id(icn->get_interconnect_id(devices[dcache_index[i]]));
        devices[icache_index[i]]->set_interconnect_id(icn->get_interconnect_id(devices[icache_index[i]]));
    }
	
    
    // L2 Setup
	cache_conf.assoc = g_conf_l2_assoc;
	cache_conf.lsize = g_conf_l2_lsize;
	cache_conf.size = g_conf_l2_size;
	cache_conf.banks = g_conf_l2_banks;
	cache_conf.bank_bw = g_conf_l2_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l2_req_q_size;
	cache_conf.wait_q_size = g_conf_l2_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l2_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l2_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l2_writeback_buffers;
	cache_conf.bank_stripe_bits = g_conf_l2_stripe_size;

	uint32 sharedL2_index = add_device(new cmp_incl_l2_cache_t("sharedL2", &cache_conf, 2 * num_processors + 1, 0, 10));
	devices[sharedL2_index]->set_interconnect_id(icn->get_clustered_id(devices[sharedL2_index], g_conf_l2_banks));
    devices[sharedL2_index]->create_fifo_link(L2prot_t::interconnect_link, 
        static_cast<device_t *> (icn), g_conf_l2_latency, 0);
        
    
    icn->setup_interconnect();
    uint32 mainmem_index = add_device(new main_mem_t<L2prot_t, simple_mainmem_msg_t>("mainmem", num_processors + 1));
    
    link_desc_t L2_mem[2];
	L2_mem[0] = buffered_link;
	L2_mem[0].link_latency = g_conf_default_link_latency;
	L2_mem[0].link_id = cmp_incl_l2_cache_t::mainmem_addr_link; 
     
	L2_mem[1] = buffered_link;
	L2_mem[1].link_latency = g_conf_default_link_latency;
	L2_mem[1].link_id = cmp_incl_l2_cache_t::mainmem_data_link; 
    
    link_desc_t mem_L2[1];
    mem_L2[0] = buffered_link;
    mem_L2[0].link_latency = g_conf_default_link_latency;
    mem_L2[0].link_id = 0;

    join_devices(sharedL2_index, mainmem_index, 2, L2_mem, 1, mem_L2);  
    // Set the L2 cache ptr
    cmp_incl_l2_cache_t *l2cache = static_cast<cmp_incl_l2_cache_t *>
        (devices[sharedL2_index]);
    for (uint32 i = 0; i < num_processors; i++)
    {
        cmp_incl_l1_cache_t *l1cache = static_cast<cmp_incl_l1_cache_t *>
            (devices[dcache_index[i]]);
        l1cache->set_L2_cache_ptr(l2cache);
        l1cache = static_cast<cmp_incl_l1_cache_t *>(devices[icache_index[i]]);
        l1cache->set_L2_cache_ptr(l2cache);
    }
  	num_devices = deviceindex;
	num_links = linkindex;
    
#endif
    
    
}

void mem_hier_t::create_two_level_split_mp()
{
	
#ifndef COMPILE_MP_TWO_MSI
	ASSERT_MSG(0, "COMPILE_MP_TWO_MSI not defined\n");
#else

	typedef mp_two_msi_L1prot_sm_t L1prot_t;
	typedef mp_two_msi_L2prot_sm_t L2prot_t;
	typedef unip_one_prot_t prot_t;
	
	link_desc_t normal_link;
	link_desc_t buffered_link;
	normal_link.buffered = false;
	normal_link.link_id = 0;
    normal_link.link_latency = 0;
	buffered_link.buffered = true;
	buffered_link.link_latency = g_conf_default_link_latency;
	buffered_link.msgs_per_link = g_conf_default_messages_per_link;
	buffered_link.link_id = 0;
	uint32 proc_index[num_processors];
	
	char num[2];
	string proc = "proc";
	for (uint32 i = 0; i < num_processors; i++) {
		ASSERT(get_cpu_object(i));
		sprintf(num, "%u", i);
		proc_index[i] = add_device(new proc_t<L1prot_t, simple_proc_msg_t>(proc+string(num), get_cpu_object(i)));
	}
	
	uint32 dcache_index[num_processors];
	
	// Add up all the D-Cache
	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l1d_writeback_buffers;
	
	// Set up link descriptor between proc and dcache
	link_desc_t proc_dcache_link[1];
	proc_dcache_link[0] = normal_link;
	proc_dcache_link[0].link_id = proc_t<prot_t, simple_proc_msg_t>::dcache_link;
	
	link_desc_t dcache_proc_link[1];
	if (g_conf_delay_zero_latency || g_conf_l1d_latency > 0) {
		dcache_proc_link[0] = buffered_link;
        //dcache_proc_link[0].msgs_per_link = 0;
		dcache_proc_link[0].link_latency = g_conf_l1d_latency;
	}
	else
		dcache_proc_link[0] = normal_link;
	dcache_proc_link[0].link_id =  unip_two_l1d_cache_t::proc_link;
	
	string cache = "Dcache";
	
	// create the d-caches and set up the links
	for (uint32 i = 0; i < num_processors; i++)
	{
	    sprintf(num, "%u", i);	
		dcache_index[i] = add_device(new mp_two_msi_L1cache_t(cache+num,&cache_conf, i, get_cpu_object(i), 0));
		join_devices(proc_index[i], dcache_index[i], 1, proc_dcache_link, 1, dcache_proc_link);
	}
	
	// I-CACHE set up
	
	uint32 icache_index[num_processors];
	
	cache_conf.assoc = g_conf_l1i_assoc;
	cache_conf.lsize = g_conf_l1i_lsize;
	cache_conf.size = g_conf_l1i_size;
	cache_conf.banks = g_conf_l1i_banks;
	cache_conf.bank_bw = g_conf_l1i_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1i_req_q_size;
	cache_conf.wait_q_size = g_conf_l1i_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1i_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1i_requests_per_mshr;
	
	link_desc_t proc_icache_link[1];
	proc_icache_link[0] = normal_link;
	proc_icache_link[0].link_id = proc_t<prot_t, simple_proc_msg_t>::icache_link;
	
	link_desc_t icache_proc_link[1];
	if (g_conf_delay_zero_latency || g_conf_l1i_latency > 0) { 
		icache_proc_link[0] = buffered_link;
        //icache_proc_link[0].msgs_per_link = 0;
		icache_proc_link[0].link_latency = g_conf_l1i_latency;
	}
	else
		icache_proc_link[0] = normal_link;
	icache_proc_link[0].link_id =  unip_two_l1i_cache_t::proc_link;
	
	cache = "Icache";
	for (uint32 i = 0; i < num_processors; i++) 
	{
		sprintf(num, "%u", i);	
		icache_index[i] = add_device(new mp_two_msi_L1cache_t(cache+num,&cache_conf, i + num_processors, get_cpu_object(i), ICACHE_STREAM_PREFETCH));
		//icache_dev[i] = icache_index[i];
		join_devices(proc_index[i], icache_index[i], 1, proc_icache_link, 1, icache_proc_link);
	}
	
	// L2 Setup
	cache_conf.assoc = g_conf_l2_assoc;
	cache_conf.lsize = g_conf_l2_lsize;
	cache_conf.size = g_conf_l2_size;
	cache_conf.banks = g_conf_l2_banks;
	cache_conf.bank_bw = g_conf_l2_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l2_req_q_size;
	cache_conf.wait_q_size = g_conf_l2_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l2_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l2_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l2_writeback_buffers;

	uint32 L1_L2bus_index = add_device(new mp_bus_t<L1prot_t, mp_two_msi_msg_t>("L1-L2bus",  2 * num_processors, 1));
	uint32 sharedL2_index = add_device(new mp_two_msi_L2cache_t("sharedL2", &cache_conf, 2 * num_processors + 1, 0));
	
	link_desc_t L1cache_busl[2];
	// Address link
	L1cache_busl[0].link_id = 1;
	L1cache_busl[0].buffered = false;
	// Data Link
	L1cache_busl[1].link_id = 2;
	L1cache_busl[1].buffered = true;
	L1cache_busl[1].link_latency = 0;
	L1cache_busl[1].msgs_per_link = g_conf_default_messages_per_link;
	
	// Bus to L1 Cache Link
	link_desc_t bus_L1cachel[1];
	bus_L1cachel[0] = buffered_link;
	bus_L1cachel[0].link_latency = 0;
	
	// Connect to bus to L1 Caches
	for (uint32 i = 0; i < num_processors; i++) {
		bus_L1cachel[0].link_id = i;
		join_devices(dcache_index[i], L1_L2bus_index, 2, L1cache_busl, 1, bus_L1cachel);
		bus_L1cachel[0].link_id = i + num_processors;
		join_devices(icache_index[i], L1_L2bus_index, 2, L1cache_busl, 1, bus_L1cachel);
	}
	
	link_desc_t L2cache_busl[2];
	// Address link
	L2cache_busl[0].link_id = 0;
	L2cache_busl[0].buffered = false;
	// Data Link
	L2cache_busl[1].buffered = true;
	L2cache_busl[1].link_latency = g_conf_l2_latency;
	L2cache_busl[1].msgs_per_link = g_conf_default_messages_per_link;
	L2cache_busl[1].link_id = 2;
	
	// Bus to L2 Link
	link_desc_t bus_L2cachel[1];
	bus_L2cachel[0] = buffered_link;
	bus_L2cachel[0].link_latency = 0;
	bus_L2cachel[0].link_id = 2 * num_processors; // = Aggregrate number of I-cache and D-cache
	
	join_devices(sharedL2_index,L1_L2bus_index, 2, L2cache_busl, 1, bus_L2cachel);
	
	// Connection below -- memory, mem-bus
	uint32 membus_index = add_device(new mp_bus_t<prot_t, simple_mainmem_msg_t>("mem-bus", 1, 1));
	uint32 mainmem_index = add_device(new main_mem_t<prot_t, simple_mainmem_msg_t>("mainmem", num_processors + 1));

	// Shared L2 to mem-bus links
	link_desc_t mem_L2link[1];
	mem_L2link[0] = buffered_link;
	mem_L2link[0].link_id = 0;
	link_desc_t L2_memlink[1];
	L2_memlink[0] = buffered_link;
	L2_memlink[0].link_id = 1;
	
	join_devices(sharedL2_index, membus_index, 1, L2_memlink, 1 ,mem_L2link);
	
	// Mem-bus to Main memory Link
	link_desc_t bus_memlink[1];
	bus_memlink[0] = buffered_link;
	bus_memlink[0].link_id = 1;
	bus_memlink[0].link_latency = g_conf_l2_latency;
	bus_memlink[0].msgs_per_link = g_conf_default_messages_per_link;
	link_desc_t mem_buslink[1];
	mem_buslink[0] = buffered_link;
	mem_buslink[0].link_id = 0;
	mem_buslink[0].link_latency = g_conf_l2_latency;
	mem_buslink[0].msgs_per_link = g_conf_default_messages_per_link;
	
	join_devices(membus_index, mainmem_index, 1, bus_memlink, 1, mem_buslink);
	num_devices = deviceindex;
	num_links = linkindex;
    
#endif // COMPILE_MP_TWO_MSI	
}



void
mem_hier_t::create_two_level_mp_bus()
{

#ifndef COMPILE_MP_TWO_MSI
	ASSERT_MSG(0, "COMPILE_MP_TWO_MSI not defined\n");
#else

	  
	
	typedef mp_two_msi_L1prot_sm_t L1prot_t;
	typedef mp_two_msi_L2prot_sm_t L2prot_t;
	typedef unip_one_prot_t prot_t;
	
	link_desc_t normal_link;
	link_desc_t buffered_link;
	normal_link.buffered = false;
	normal_link.link_id = 0;
	buffered_link.buffered = true;
	buffered_link.link_latency = g_conf_default_link_latency;
	buffered_link.msgs_per_link = g_conf_default_messages_per_link;
	
	uint32 proc_index[num_processors];
	uint32 cache_index[num_processors];
	
	// Link Descriptor types between cache and procs
	link_desc_t proc_cache_link[1];
	proc_cache_link[0] = normal_link;
	
	
	
	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.banks = g_conf_l1d_banks;
	cache_conf.bank_bw = g_conf_l1d_bank_bw_inv;
	cache_conf.req_q_size = g_conf_l1d_req_q_size;
	cache_conf.wait_q_size = g_conf_l1d_req_q_size; // same
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l1d_writeback_buffers;
	
	// Create the procs level 1 caches and create the links between them
	
	char num[2];
	string proc = "proc";
	string cache = "cache";
	
	for (uint32 i = 0; i < num_processors; i++) {
		ASSERT(get_cpu_object(i));
		sprintf(num, "%u", i);
		proc_index[i] = add_device(new proc_t<L1prot_t, simple_proc_msg_t>(proc+string(num), get_cpu_object(i)));
	}
	
	for (uint32 i = 0; i < num_processors; i++) {
		sprintf(num, "%u", i);
		cache_index[i] = add_device(new mp_two_msi_L1cache_t(cache+string(num),&cache_conf, i, get_cpu_object(i), 0));
		join_devices(proc_index[i], cache_index[i], 1, proc_cache_link, 1, proc_cache_link);
		
	}
	
    cache_conf.assoc = g_conf_l2_assoc;
	cache_conf.lsize = g_conf_l2_lsize;
	cache_conf.size = g_conf_l2_size;
	cache_conf.num_mshrs = g_conf_l2_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l2_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l2_writeback_buffers;
	
	//uint32 L1_L2bus_index = add_device(new split_msi_bus_t<L1prot_t, mp_two_msi_msg_t>("L1-L2bus", num_processors, 1));
    uint32 L1_L2bus_index = add_device(new mp_bus_t<mp_two_msi_L1prot_sm_t, mp_two_msi_msg_t>("L1-L2bus", num_processors, 1));
	uint32 sharedL2_index = add_device(new mp_two_msi_L2cache_t("sharedL2", &cache_conf, 2 * num_processors + 1, 16));
	
	// Cache to bus links
	link_desc_t L1cache_busl[2];
	// Address link
	L1cache_busl[0].link_id = 1;
	L1cache_busl[0].buffered = false;
	// Data Link
	L1cache_busl[1].link_id = 2;
	L1cache_busl[1].buffered = true;
	L1cache_busl[1].link_latency = 0;
	L1cache_busl[1].msgs_per_link = g_conf_default_messages_per_link;
	
	// Bus to L1 Cache Link
	link_desc_t bus_L1cachel[1];
	bus_L1cachel[0] = buffered_link;
	bus_L1cachel[0].link_latency = 0;
	
	// Connect to bus to L1 Caches
	for (uint32 i = 0; i < num_processors; i++) {
		bus_L1cachel[0].link_id = i;
		join_devices(cache_index[i], L1_L2bus_index, 2, L1cache_busl, 1, bus_L1cachel);
	}
	
	// Connect L1-L2 bus to shared L2
	link_desc_t L2cache_busl[2];
	// Address link
	L2cache_busl[0].link_id = 0;
	L2cache_busl[0].buffered = false;
	// Data Link
	L2cache_busl[1].buffered = true;
	L2cache_busl[1].link_latency = 0;
	L2cache_busl[1].msgs_per_link = g_conf_default_messages_per_link;
	L2cache_busl[1].link_id = 2;
	
	// Bus to L2 Link
	link_desc_t bus_L2cachel[1];
	bus_L2cachel[0] = buffered_link;
	bus_L2cachel[0].link_latency = 0;
	bus_L2cachel[0].link_id = num_processors;
	
	join_devices(sharedL2_index,L1_L2bus_index, 2, L2cache_busl, 1, bus_L2cachel);
	
	// Connection below -- memory, mem-bus
	uint32 membus_index = add_device(new split_unip_bus_t<prot_t, simple_mainmem_msg_t>("mem-bus", 1, 1));
	uint32 mainmem_index = add_device(new main_mem_t<prot_t, simple_mainmem_msg_t>("mainmem", num_processors + 1));

	// Shared L2 to mem-bus links
	link_desc_t mem_L2link[1];
	mem_L2link[0] = buffered_link;
	mem_L2link[0].link_id = 0;
	link_desc_t L2_memlink[1];
	L2_memlink[0] = buffered_link;
	L2_memlink[0].link_id = 1;
	
	join_devices(sharedL2_index, membus_index, 1, L2_memlink, 1 ,mem_L2link);
	
	// Mem-bus to Main memory Link
	link_desc_t bus_memlink[1];
	bus_memlink[0] = buffered_link;
	bus_memlink[0].link_id = 1;
	link_desc_t mem_buslink[1];
	mem_buslink[0] = buffered_link;
	mem_buslink[0].link_id = 0;
	
	join_devices(membus_index, mainmem_index, 1, bus_memlink, 1, mem_buslink);
	num_devices = deviceindex;
	num_links = linkindex;
    
#endif // COMPILE_MP_TWO_MSI
}


/*
void mem_hier_t::create_one_level_split_mp()
{
	
	typedef mp_one_msi_prot_sm_t prot_t;
	typedef proc_t<prot_t, simple_proc_msg_t> proc_t;
	
	// Proc to I/Dcache Links
	link_desc_t proc_dcache_link;
	proc_dcache_link.buffered = false;
	proc_dcache_link.link_id = proc_t::dcache_link;
	link_desc_t proc_icache_link;
	proc_icache_link.buffered = false;
	proc_icache_link.link_id = proc_t::icache_link;

	// DCache to Proc links
	link_desc_t dcache_proc_link;
	dcache_proc_link.id = prot_t::proc_link;   // Cache's link ID for proc
	if (g_conf_delay_zero_latency || g_conf_l1d_latency > 0) { 
		dcache_proc_link.buffered = true;
		dcache_proc_link.link_latency = g_conf_l1d_latency;
		dcache_proc_link.msgs_per_link = 0;  // infinite
	} else { 
		dcache_proc_link.buffered = false;
	}
	
	// ICache to Proc links
	link_desc_t icache_proc_link;
	icache_proc_link.id = prot_t::proc_link;   // Cache's link ID for proc
	if (g_conf_delay_zero_latency || g_conf_l1i_latency > 0) { 
		icache_proc_link.buffered = true;
		icache_proc_link.link_latency = g_conf_l1i_latency;
		icache_proc_link.msgs_per_link = 0;  // infinite
	} else { 
		icache_proc_link.buffered = false;
	}
	
	// Cache to Bus links (Normal)
	link_desc_t cache_bus_link;
	cache_proc_link.id = prot_t::bus_link;   // Cache's link ID for bus
	cache_proc_link.buffered = false;
	
	// Cache to Bus links (Buffered)
	link_desc_t cache_bus_buffered_link;
	cache_proc_buffered_link.id = prot_t::buffered_bus_link;   // Cache's link ID for bus
	cache_proc_buffered_link.buffered = true;
	cache_proc_buffered_link.link_latency = 0;
	cache_proc_buffered_link.msgs_per_link = 0;  // infinite

	// Bus to cache links
	link_desc_t bus_cache_link;
	bus_cache_link.id = prot_t::bus_link;   // Cache's link ID for bus
	bus_cache_link.buffered = false;
	

	uint32 proc_index[num_processors];
	
	// Create Procs
	char *num = new char[2];
	string proc = "proc";
	for (uint32 i = 0; i < num_processors; i++) {
		ASSERT(get_cpu_object(i));
		sprintf(num, "%u", i);
		proc_index[i] = add_device(new proc_t(proc+num, get_cpu_object(i)));
	}
	
	// Create Dcaches
	uint32 dcache_index[num_processors];

	cache_config_t cache_conf;
	cache_conf.assoc = g_conf_l1d_assoc;
	cache_conf.lsize = g_conf_l1d_lsize;
	cache_conf.size = g_conf_l1d_size;
	cache_conf.num_mshrs = g_conf_l1d_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1d_requests_per_mshr;
	cache_conf.writeback_buffersize = g_conf_l1d_writeback_buffers;

	string cache = "Dcache";
	for (uint32 i = 0; i < num_processors; i++)
	{
	    sprintf(num, "%u", i);	
		dcache_index[i] = add_device(new mp_on_msi_cache_t(cache+num,&cache_conf, i));
		join_devices(proc_index[i], dcache_index[i], 1, proc_dcache_link, 1, dcache_proc_link);
	}
	
	// Create Icaches
	uint32 icache_index[num_processors];

	cache_conf.assoc = g_conf_l1i_assoc;
	cache_conf.lsize = g_conf_l1i_lsize;
	cache_conf.size = g_conf_l1i_size;
	cache_conf.num_mshrs = g_conf_l1i_num_mshrs;
	cache_conf.requests_per_mshr = g_conf_l1i_requests_per_mshr;

	string cache = "Icache";
	for (uint32 i = 0; i < num_processors; i++)
	{
	    sprintf(num, "%u", i);	
		icache_index[i] = add_device(new mp_on_msi_cache_t(cache+num,&cache_conf, i+num_processors));
		join_devices(proc_index[i], icache_index[i], 1, proc_icache_link, 1, icache_proc_link);
	}
	
	// Bus and cache-bus links
	uint32 bus_index = add_device(new split_msi_bus_t<prot_t, mp_one_msi_msg_t>("bus",  2 * num_processors, 1));

	for (uint32 i = 0; i < num_processors; i++) {
		bus_L1cachel[0].link_id = i;
		join_devices(dcache_index[i], L1_L2bus_index, 2, L1cache_busl, 1, bus_L1cachel);
								   g_conf_default_link_latency,
								   g_conf_default_messages_per_link);


	// Set proc link to cache
	devices[0]->set_link(0, links[0]);  // IFetch and Data down same link
	devices[0]->set_link(1, links[0]);
	
	// Set io link to cache and mem
	devices[1]->set_link(0, links[7]);
	devices[1]->set_link(1, links[5]);

	// Set cache links to proc and mem and io
	devices[2]->set_link(0, links[1]);
	devices[2]->set_link(1, links[2]);
	devices[2]->set_link(2, links[4]);

	// Set mem link to cache and io
	devices[3]->set_link(0, links[3]);
	devices[3]->set_link(1, links[6]);

#endif // COMPILE_UNIP_ONE_COMPLEX
}*/
