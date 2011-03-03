#
# Thanks to Greg Link from Penn State University 
# for his math acceleration engine.
#

# Uncomment the following math acceleration flags 
# relevant to your target and set the appropriate
# path and flag options

# default - no math acceleration
MATHACCEL	= none
INCDIRFLAG	= 
LIBDIRFLAG	= 
LIBS		= -lm
EXTRAFLAGS	= 

# Intel Machines - acceleration with the Intel
# Math Kernel Library (MKL)
#MATHACCEL	= intel
#INCDIRFLAG	= -I/uf1/ks4kk/research/scratch/mkl/8.0.2/include
#LIBDIRFLAG	= -L/uf1/ks4kk/research/scratch/mkl/8.0.2/lib/32
#LIBS		= -lmkl_lapack -lmkl_ia32 -lguide -lpthread -lm
#EXTRAFLAGS	= 

# AMD Machines - acceleration with the AMD
# Core Math Library (ACML)
#MATHACCEL	= amd
#INCDIRFLAG	= -I/uf1/ks4kk/research/scratch/acml3.1.0/gfortran32_nosse2/include
#LIBDIRFLAG	= -L/uf1/ks4kk/research/scratch/acml3.1.0/gfortran32_nosse2/lib
#LIBS		= -lacml -lgfortran -lm
#EXTRAFLAGS	= 

# Apple Machines - acceleration with the Apple
# Velocity Engine (AltiVec)
#MATHACCEL	= apple
#INCDIRFLAG	= 
#LIBDIRFLAG	= 
#LIBS		= -framework vecLib -lm
#EXTRAFLAGS	= 

# Sun Machines - acceleration with the SUN
# performance library (sunperf)
#MATHACCEL	= sun
#INCDIRFLAG	= 
#LIBDIRFLAG	= 
#LIBS		= -library=sunperf
#EXTRAFLAGS	= -dalign 

# basic compiler flags - special case for sun
ifeq ($(MATHACCEL), sun)
CC 			= CC
ifeq ($(DEBUG), 1)
OFLAGS		= -g -erroff=badargtypel2w
else
ifeq ($(DEBUG), 2)
OFLAGS		= -xpg -g -erroff=badargtypel2w
else
OFLAGS		= -xO4 -erroff=badargtypel2w
endif	# DEBUG = 2
endif	# DEBUG = 1
else	# MATHACCEL != sun	
CC 			= g++
ifeq ($(DEBUG), 1)
OFLAGS		= -O0 -ggdb -Wall
else
ifeq ($(DEBUG), 2)
OFLAGS		= -O3 -pg -ggdb -Wall
else
OFLAGS		= -O3
endif	# DEBUG = 2
endif	# DEBUG = 1
endif	# end MATHACCEL
RM			= rm -f
OEXT		= o

# Verbosity level [0-3]
ifndef VERBOSE
VERBOSE	= 1
endif

# Numerical ID for each acceleration engine
ifeq ($(MATHACCEL), none)
ACCELNUM = 0
endif
ifeq ($(MATHACCEL), intel)
ACCELNUM = 1
endif
ifeq ($(MATHACCEL), amd)
ACCELNUM = 2
endif
ifeq ($(MATHACCEL), apple)
ACCELNUM = 3
endif
ifeq ($(MATHACCEL), sun)
ACCELNUM = 4
endif

CFLAGS	= $(OFLAGS) $(EXTRAFLAGS) $(INCDIRFLAG) $(LIBDIRFLAG) -DVERBOSE=$(VERBOSE) -DMATHACCEL=$(ACCELNUM)

# sources, objects, headers and inputs

# HotFloorplan
FLPSRC	= flp.c flp_desc.c npe.c shape.c 
FLPOBJ	= flp.$(OEXT) flp_desc.$(OEXT) npe.$(OEXT) shape.$(OEXT) 
FLPHDR	= flp.h npe.h shape.h
FLPIN = ev6.desc avg.p

# HotSpot
TEMPSRC	= temperature.c RCutil.c
TEMPOBJ	= temperature.$(OEXT) RCutil.$(OEXT)
TEMPHDR = temperature.h
TEMPIN	= 

# HotSpot block model
BLKSRC = temperature_block.c 
BLKOBJ = temperature_block.$(OEXT) 
BLKHDR	= temperature_block.h
BLKIN	= ev6.flp gcc.ptrace

# HotSpot grid model
GRIDSRC = temperature_grid.c Grids.cpp FBlocks.cpp
GRIDOBJ = temperature_grid.$(OEXT) Grids.$(OEXT) FBlocks.$(OEXT)
GRIDHDR	= temperature_grid.h Grids.h FBlocks.h
GRIDIN	= layer.lcf example.lcf example.flp example.ptrace

# Miscellaneous
MISCSRC = util.c wire.c
MISCOBJ = util.$(OEXT) wire.$(OEXT)
MISCHDR = util.h wire.h
MISCIN	= hotspot.config

# all objects
OBJ	= $(TEMPOBJ) $(BLKOBJ) $(GRIDOBJ) $(FLPOBJ) $(MISCOBJ)

# targets
all:	hotspot hotfloorplan

hotspot:	hotspot.$(OEXT) $(OBJ)
	$(CC) $(CFLAGS) -o hotspot hotspot.$(OEXT) $(OBJ) $(LIBS)
ifdef LIBDIRFLAG
		@echo
		@echo
		@echo "...Done. Do not forget to include $(LIBDIRFLAG) in your LD_LIBRARY_PATH"
endif

hotfloorplan:	hotfloorplan.$(OEXT) $(OBJ)
	$(CC) $(CFLAGS) -o hotfloorplan hotfloorplan.$(OEXT) $(OBJ) $(LIBS)
ifdef LIBDIRFLAG
		@echo
		@echo
		@echo "...Done. Do not forget to include $(LIBDIRFLAG) in your LD_LIBRARY_PATH"
endif

.c.$(OEXT):
	$(CC) $(CFLAGS) -c $*.c

.cpp.$(OEXT):
	$(CC) $(CFLAGS) -c $*.cpp

filelist:
	@echo $(FLPSRC) $(TEMPSRC) $(BLKSRC) $(GRIDSRC) $(MISCSRC) \
		  $(FLPHDR) $(TEMPHDR) $(BLKHDR) $(GRIDHDR) $(MISCHDR) \
		  $(FLPIN) $(TEMPIN) $(BLKIN) $(GRIDIN) $(MISCIN) \
		  hotspot.h hotspot.c hotfloorplan.h hotfloorplan.c \
		  sim-template_block.c \
		  tofig.pl grid_thermal_map.pl \
		  Makefile
clean:
	$(RM) *.$(OEXT) *.obj core *~ Makefile.bak hotspot hotfloorplan

cleano:
	$(RM) *.$(OEXT) *.obj

