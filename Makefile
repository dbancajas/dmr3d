# Copyright (c) 2003 by Gurindar S. Sohi for the Wisconsin
# Multiscalar Project.  ALL RIGHTS RESERVED.
#
# This software is furnished under the Multiscalar license.
# For details see the LICENSE.mscalar file in the top-level source
# directory, or online at http://www.cs.wisc.edu/mscalar/LICENSE
#
# $Id: Makefile,v 1.1.1.1.10.6 2005/11/14 18:03:18 pwells Exp $

MODULE_DIR = ms2sim_smt
MODULE_NAME = ms2sim_smt

MODULE_CFLAGS = -DTARGET_SPARC_V9 -Wall -g -O0 -g3
MODULE_CFLAGS = -DTARGET_SPARC_V9 -Wall  -O3
MODULE_LDFLAGS = 
MODULE_CLASSES = ms2sim


# Include source definitions
#include $(SRC_BASE)/$(MODULE_DIR)/power/Makefile.sources
#include $(SRC_BASE)/$(MODULE_DIR)/mem-hier/Makefile.sources
#include $(SRC_BASE)/$(MODULE_DIR)/common/Makefile.sources
#include $(SRC_BASE)/$(MODULE_DIR)/processor/Makefile.sources


COMMON_VPATH = $(SRC_BASE)/$(MODULE_DIR)/common

SRC_FILES += transaction.cc \
			 stats.cc \
			 counter.cc \
			 histogram.cc \
			 config.cc \
			 debugio.cc \
			 profiles.cc \
			 checkp_util.cc \
			 instruction_info.cc
			 
			 

PROTOCOLS = 
#PROTOCOLS += UNIP_ONE
#PROTOCOLS += MP_ONE_MSI
#PROTOCOLS += MP_TWO_MSI
PROTOCOLS += UNIP_TWO
#PROTOCOLS += UNIP_TWO_DRAM
#PROTOCOLS += UNIP_ONE_COMPLEX
#PROTOCOLS += CMP_INCL
#PROTOCOLS += CMP_INCL_WT
#PROTOCOLS += CMP_EXCL
#PROTOCOLS += CMP_EX_3L
#PROTOCOLS += CMP_INCL_3L

MODULE_CFLAGS += $(foreach prot, $(PROTOCOLS), -DCOMPILE_$(prot))  

MEMHIER_VPATH = $(SRC_BASE)/$(MODULE_DIR)/mem-hier:$(SRC_BASE)/$(MODULE_DIR)/mem-hier/protocols:$(SRC_BASE)/$(MODULE_DIR)/mem-hier/profile:$(SRC_BASE)/$(MODULE_DIR)/mem-hier/DRAMSim2

SRC_FILES += external.cc \
			 mem_driver.cc \
			 event.cc \
			 meventq.cc \
			 link.cc \
			 mem_hier.cc \
			 device.cc \
			 message.cc \
			 line.cc \
			 cache.cc \
			 messages.cc \
			 networks.cc \
			 networks.cc \
			 trans_rec.cc \
			 cache_bank.cc \
			 interconnect.cc \
			 random_tester.cc \
			 mem_tracer.cc \
			 spinlock.cc \
			 codeshare.cc \
			 sms.cc stackheap.cc function_profile.cc \
			 mesh_simple.cc \
			 mesh_sandwich.cc \
			 warmup_trace.cc\


ifneq (,$(findstring UNIP_ONE,$(PROTOCOLS)))
	SRC_FILES += unip_one_cache.cc
endif

ifneq (,$(findstring MP_ONE_MSI,$(PROTOCOLS)))
	SRC_FILES += mp_one_msi_cache.cc \
				 mp_one_msi_mainmem.cc
endif

ifneq (,$(findstring MP_TWO_MSI,$(PROTOCOLS)))
	SRC_FILES += mp_two_msi_L1cache.cc \
				 mp_two_msi_sharedL2cache.cc \
				 mp_one_msi_mainmem.cc
endif

ifneq (,$(findstring UNIP_TWO,$(PROTOCOLS)))
	MEMHIER_VPATH := $(MEMHIER_VPATH):$(SRC_BASE)/$(MODULE_DIR)/mem-hier/protocols/unip_two
	SRC_FILES += unip_two_l1d_cache.cc \
				 unip_two_l1i_cache.cc \
				 unip_two_l2_cache.cc
endif

ifneq (,$(findstring UNIP_ONE_COMPLEX,$(PROTOCOLS)))
	SRC_FILES += unip_one_complex_cache.cc
endif

ifneq (,$(findstring CMP_INCL,$(PROTOCOLS)))
	MEMHIER_VPATH := $(MEMHIER_VPATH):$(SRC_BASE)/$(MODULE_DIR)/mem-hier/protocols/cmp_incl
	SRC_FILES += cmp_incl_l2_cache.cc  \
		     cmp_incl_l1_cache.cc
endif

ifneq (,$(findstring CMP_INCL_WT,$(PROTOCOLS)))
	MEMHIER_VPATH := $(MEMHIER_VPATH):$(SRC_BASE)/$(MODULE_DIR)/mem-hier/protocols/cmp_incl_wt
	SRC_FILES += cmp_incl_wt_l2_cache.cc  \
		     cmp_incl_wt_l1_cache.cc
endif

ifneq (,$(findstring CMP_EXCL,$(PROTOCOLS)))
	MEMHIER_VPATH := $(MEMHIER_VPATH):$(SRC_BASE)/$(MODULE_DIR)/mem-hier/protocols/cmp_excl
	SRC_FILES += cmp_excl_l2_cache.cc  \
		     cmp_excl_l1_cache.cc \
		     cmp_excl_l1dir_array.cc
endif

ifneq (,$(findstring CMP_EX_3L,$(PROTOCOLS)))
	MEMHIER_VPATH := $(MEMHIER_VPATH):$(SRC_BASE)/$(MODULE_DIR)/mem-hier/protocols/cmp_excl
	MEMHIER_VPATH := $(MEMHIER_VPATH):$(SRC_BASE)/$(MODULE_DIR)/mem-hier/protocols/cmp_excl_3l
	SRC_FILES += cmp_excl_3l_l3_cache.cc  \
		     cmp_excl_3l_l1_cache.cc \
		     cmp_excl_3l_l2_cache.cc \
		     cmp_excl_3l_l2dir_array.cc
endif

ifneq (,$(findstring CMP_INCL_3L,$(PROTOCOLS)))
	MEMHIER_VPATH := $(MEMHIER_VPATH):$(SRC_BASE)/$(MODULE_DIR)/mem-hier/protocols/cmp_incl
	MEMHIER_VPATH := $(MEMHIER_VPATH):$(SRC_BASE)/$(MODULE_DIR)/mem-hier/protocols/cmp_incl_3l
	SRC_FILES += cmp_incl_3l_l3_cache.cc  \
		     cmp_incl_3l_l1_cache.cc \
		     cmp_incl_3l_l2_cache.cc 
endif


PROCESSOR_VPATH = /users/dean/research/ece5750/gems-2.1/simics3workspace/modules/ms2sim_smt/processor:/users/dean/research/ece5750/gems-2.1/simics3workspace/modules/ms2sim_smt/processor/csp_alg

SRC_FILES += dynamic.cc iwindow.cc mai.cc mai_instr.cc \
             chip.cc sequencer.cc startup.cc fu.cc \
             isa.cc yags.cc cascade.cc ctrl_flow.cc ras.cc lsq.cc \
             st_buffer.cc mem_xaction.cc v9_mem_xaction.cc \
             wait_list.cc fetch_buffer.cc \
             mem_hier_handle.cc proc_stats.cc eventq.cc\
             fastsim.cc mmu.cc thread_scheduler.cc thread_context.cc \
			 disambig.cc csp_alg.cc base_alg.cc tsp_alg.cc ssp_alg.cc esp_alg.cc  \
			 msp_alg.cc shb.cc disable_timer.cc os_alg.cc hybrid_alg.cc \
			 func_alg.cc heatrun_alg.cc servc_alg.cc
	
CACTI_VPATH = $(MODULE_DIR)/power/cacti
HOTSPOT_VPATH = $(MODULE_DIR)/power/hotspot
POWER_VPATH = $(MODULE_DIR)/power:$(CACTI_VPATH):$(HOTSPOT_VPATH)

MODULE_CFLAGS += -DPOWER_COMPILE
HOTSPOT_SRCS = flp.cc flp_desc.cc npe.cc shape.cc \
				temperature.cc RCutil.cc \
				temperature_grid.cc Grids.cpp FBlocks.cc \
				util.cc wire.cc temperature_block.cc \
				hotspot_interface.cc
CACTI_SRCS = area.cc cacti_time.cc leakage.cc
SRC_FILES += power_obj.cc power_profile.cc  \
			 circuit_param.cc area_param.cc \
			 basic_circuit.cc power_util.cc \
			 core_power.cc \
			 cache_power.cc sparc_regfile.cc \
			 $(CACTI_SRCS) $(HOTSPOT_SRCS)
			 



EXTRA_VPATH = /users/dean/research/ece5750/gems-2.1/simics3workspace/modules/ms2sim_smt/common:$(PROCESSOR_VPATH):$(MEMHIER_VPATH):$(POWER_VPATH)
#EXTRA_VPATH = $(COMMON_VPATH):$(PROCESSOR_VPATH):$(MEMHIER_VPATH)

SIMICS_API = 3.0

include $(MODULE_MAKEFILE)

# Force object's dependence on these files too
# $(OBS): common/Makefile.sources processor/Makefile.sources power/Makefile.sources mem-hier/Makefile.sources

