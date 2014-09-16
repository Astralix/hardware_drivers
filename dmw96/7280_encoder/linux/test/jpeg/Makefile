#-------------------------------------------------------------------------------
#-                                                                            --
#-       This software is confidential and proprietary and may be used        --
#-        only as expressly authorized by a licensing agreement from          --
#-                                                                            --
#-                            Hantro Products Oy.                             --
#-                                                                            --
#-                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
#-                            ALL RIGHTS RESERVED                             --
#-                                                                            --
#-                 The entire notice above must be reproduced                 --
#-                  on all copies and should not be removed.                  --
#-                                                                            --
#-------------------------------------------------------------------------------
#-
#--  Abstract : Makefile for 7280 JPEG encoder testbench
#--
#-------------------------------------------------------------------------------
#--
#--  Version control information, please leave untouched.
#--
#--  $RCSfile: Makefile,v $
#--  $Date: 2008/09/08 10:15:58 $
#--  $Revision: 1.6 $
#--
#-----------------------------------------------------------------------------*/

# Comment/uncomment the following line to disable/enable debugging
DEBUG = y

# set this to 'y' for Electric Fence checking
USE_EFENCE = n

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DDEBUG
else
  DEBFLAGS = -O3 -DNDEBUG
endif

# This is used for testing with system model
# encoder uses comment header spefified in test bench
# if you want to use own comment header data, comment this out!
DEBFLAGS +=-DTB_DEFINED_COMMENT 

# this is used only for integrator tracing
#DEBFLAGS += -DASIC_WAVE_TRACE_TRIGGER

# Architecture flags for gcc
#ARCH = 
#CROSS_COMPILE =

ifeq ($(shell uname -m),x86_64)
	ifneq (,$(findstring pcfile,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
	ifneq (,$(findstring pclinux,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
	ifneq (,$(findstring system,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
	ifneq (,$(findstring testdata,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
	ifneq (,$(findstring eval,$(MAKECMDGOALS)))
		export ARCH = -m32
	endif
endif

# C -compiler name, can be replaced by another compiler(replace gcc)
CC = $(CROSS_COMPILE)gcc

# the path where to find header files
INCFLAGS = -I../../../inc -I../../../source/jpeg -I../../../source/common \
           -I../../debug_trace

ifeq ($(USE_EFENCE), y)
        EFENCE= -DUSE_EFENCE -I/afs/hantro.com/projects/adder/users/atna/efence_2_4_13 \
                -L/afs/hantro.com/projects/adder/users/atna/efence_2_4_13 \
                -lefence -lpthread
endif

# compiler switches
CFLAGS  = $(ARCH) -Wall -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE \
           $(DEBFLAGS) $(INCFLAGS)
# list of used sourcefiles
SRCS = JpegTestBench.c EncGetOption.c 

OBJS = $(SRCS:.c=.o)

# name of the input library 
LIB = ../../lib7280enc.a

# System model library
MODELLIB = ../../../../system/models/enc7280_asic_model.a

# System model library that writes test data traces
TESTDATALIB = ../../../../system/models/enc7280_asic_model_trace.a

# name of the output executable
TARGET = jpeg_testenc

# MACRO for cleaning object -files
RM  = rm -f

#Here are rules for building codes and generating executable
all:    tags
	@echo ---------------------------------------
	@echo "Usage: make [ pcfile | system | testdata | ads | versatile ]"
	@echo "pcfile     - PC linux with file input"
	@echo "system     - PC system model (==pclinux)"
	@echo "testdata   - PC system model for test data creation"
	@echo "ads        - ARM simulator with file input"
	@echo "integrator - ARM integrator with FPGA HW"
	@echo "versatile  - ARM versatile with FPGA HW"
	@echo "NOTE! Make sure to do 'make libclean'"
	@echo "between compiling to different targets!"
	@echo ---------------------------------------

# Use other makefiles to build the libraries
$(MODELLIB):
	$(MAKE) -w -C ../../../../system/models

$(TESTDATALIB):
	$(MAKE) -w -C ../../../../system/models testdata

$(LIB):
	$(MAKE) -w -C ../.. $(TARGETENV) INCLUDE_H264=n INCLUDE_MPEG4=n INCLUDE_VIDSTAB=n USE_EFENCE=$(USE_EFENCE)

pcfile: TARGETENV = pcfile
pcfile: CFLAGS += -DJPEG_SIMULATION
pcfile: $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) -o $(TARGET)

pclinux: system
.PHONY: system
system: TARGETENV = system
system: $(MODELLIB) $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) -o $(TARGET)

eval: TARGET = jpeg_enc_7280_eval
eval: system

.PHONY: testdata
testdata: TARGETENV = testdata
testdata: $(TESTDATALIB) $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(TESTDATALIB) -o $(TARGET)

.PHONY: ads
ads: TARGETENV = ads
ads: CC = armcc
#ads: ARMCPU=ARM1136J-S
ads: ARMCPU=ARM926EJ-S
ads: CFLAGS = -g -O1 --cpu $(ARMCPU) -DJPEG_SIMULATION -DNO_OUTPUT_WRITE $(DEBFLAGS) $(INCFLAGS)
ads: $(LIB) $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -L$(LIB) -o $(TARGET).axf

versatile: TARGETENV = versatile
versatile: CFLAGS+=-DSDRAM_LM_BASE=0x00000000 -DMEMALLOC_MODULE_PATH=\"/tmp/dev/memalloc\"
versatile: CROSS_COMPILE = arm-none-linux-gnueabi-
versatile: ARCH = -mcpu=arm926ej-s -mtune=arm926ej-s
versatile: LIB += -lpthread
versatile: $(OBJS)
	$(MAKE) -w -C ../.. $@ INCLUDE_H264=n INCLUDE_MPEG4=n INCLUDE_VIDSTAB=n USE_EFENCE=$(USE_EFENCE) \
                CROSS_COMPILE=$(CROSS_COMPILE) ARCH="$(ARCH)"
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(EFENCE) -o $(TARGET)

integrator: TARGETENV = integrator
integrator: CROSS_COMPILE= arm-linux-
integrator: ARCH = -mcpu=arm9tdmi -mtune=arm9tdmi
integrator: CFLAGS+=-DSDRAM_LM_BASE=0x80000000 -DMEMALLOC_MODULE_PATH=\"/dev/memalloc\"
integrator: LIB += -lpthread
integrator: $(OBJS)
	$(MAKE) -w -C ../.. $@ INCLUDE_H264=n INCLUDE_MPEG4=n INCLUDE_VIDSTAB=n USE_EFENCE=$(USE_EFENCE) \
                CROSS_COMPILE=$(CROSS_COMPILE) ARCH="$(ARCH)"
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(EFENCE) -o $(TARGET)

system_cov: CC = covc --retain -t!JpegTestBench.c,!EncGetOption.c gcc -lstdc++
system_cov: TARGETENV = system_cov
system_cov: $(MODELLIB) $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) -o $(TARGET)

.PHONY: clean
clean:
	$(RM) *.o core* *~ $(TARGET)*

.PHONY: libclean
libclean: clean
	$(MAKE) -w -C ../.. clean

.PHONY: tags
tags:
	ctags ../../../inc/*.h *.c ../../../source/jpeg/*.[ch] \
            ../../../source/common/*.[ch] ../../ewl/*c



