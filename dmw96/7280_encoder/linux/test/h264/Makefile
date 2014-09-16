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
#--  Abstract : Makefile for 7280 encoder testbench
#--
#-------------------------------------------------------------------------------
#--
#--  Version control information, please leave untouched.
#--
#--  $RCSfile: Makefile,v $
#--  $Date: 2009/09/15 11:36:04 $
#--  $Revision: 1.13 $
#--
#-----------------------------------------------------------------------------*/

# Comment/uncomment the following line to disable/enable debugging
DEBUG = y

# Include API extension and test ID for Hantro internal testing
INTERNAL_TEST = y

# set this to 'y' for Electric Fence checking
USE_EFENCE = n

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O1 -g -DDEBUG
else
  DEBFLAGS = -O2 -DNDEBUG
endif

#DEBFLAGS+=-DNO_OUTPUT_WRITE

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

# the path where to find header files
INCFLAGS = -I../../../inc -I../../../source/h264 \
           -I../../../source/common -I../../../source/camstab \
           -I../../debug_trace

ifeq ($(USE_EFENCE), y)
        EFENCE= -DUSE_EFENCE -I/afs/hantro.com/projects/adder/users/atna/efence_2_4_13 \
                -L/afs/hantro.com/projects/adder/users/atna/efence_2_4_13 \
                -lefence -lpthread
endif

CC = $(CROSS_COMPILE)gcc

# compiler switches
CFLAGS  = $(ARCH) -Wall -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -D_FILE_OFFSET_BITS=64 \
           $(DEBFLAGS) $(INCFLAGS)

# list of used sourcefiles
SRCS = H264TestBench.c EncGetOption.c

ifeq ($(INTERNAL_TEST),y)
# for internal tests
SRCS += h264encapi_ext.c
CFLAGS += -DINTERNAL_TEST
endif

vpath %.c
vpath %.c ../../../source/h264 

OBJS = $(SRCS:.c=.o)

# name of the control code library 
LIB = ../../lib7280enc.a

# System model library
MODELLIB = ../../../../system/models/enc7280_asic_model.a

# System model library that writes test data traces
TESTDATALIB = ../../../../system/models/enc7280_asic_model_trace.a

# name of the output executable
TARGET = h264_testenc

# MACRO for cleaning object -files
RM  = rm -f


#Here are rules for building codes and generating executable
all:    tags
	@echo ---------------------------------------
	@echo "Usage: make [ pcfile | system | testdata | ads | versatile ]"
	@echo "pcfile     - PC linux with file input"
	@echo "system     - PC system model (== pclinux)"
	@echo "testdata   - PC system model for test data creation"
	@echo "eval       - PC system model for evaluation with frame limit"
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
	$(MAKE) -w -C ../.. $(TARGETENV) INCLUDE_JPEG=n INCLUDE_MPEG4=n INCLUDE_VIDSTAB=y USE_EFENCE=$(USE_EFENCE)

.PHONY: pcfile
pcfile: TARGETENV = pcfile
pcfile: CFLAGS += -DNO_INPUT_READ
pcfile: $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) -o $(TARGET)

.PHONY: ads
ads: TARGETENV = ads
ads: CC = armcc
ads: ARMCPU=ARM1136J-S
ads: CFLAGS = -g -O1 --cpu $(ARMCPU) -DNO_OUTPUT_WRITE -DNO_INPUT_READ $(DEBFLAGS) $(INCFLAGS)
ads: $(LIB) $(OBJS) 
	$(CC) $(CFLAGS) $(OBJS) -L$(LIB) -o $(TARGET).axf

.PHONY: eval
evaluation: eval
eval: TARGET = h264_enc_7280_eval
eval: TARGETENV = eval
eval: $(MODELLIB) $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) -o $(TARGET)

pclinux: system

.PHONY: system
system: TARGETENV = system
system: $(MODELLIB) $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) -o $(TARGET)

.PHONY: testdata
testdata: TARGETENV = testdata
testdata: $(TESTDATALIB) $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(TESTDATALIB) -o $(TARGET)

.PHONY: versatile
versatile: TARGETENV = versatile
versatile: CROSS_COMPILE = arm-none-linux-gnueabi-
versatile: ARCH = -mcpu=arm926ej-s -mtune=arm926ej-s
versatile: LIB += -lpthread
versatile: $(OBJS)
	$(MAKE) -w -C ../.. $@ INCLUDE_JPEG=n INCLUDE_MPEG4=n INCLUDE_VIDSTAB=y USE_EFENCE=$(USE_EFENCE) \
                CROSS_COMPILE=$(CROSS_COMPILE) ARCH="$(ARCH)"
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(EFENCE) -o $(TARGET) 

.PHONY: integrator
integrator: TARGETENV = integrator
integrator: CROSS_COMPILE= arm-linux-
integrator: ARCH = -mcpu=arm9tdmi -mtune=arm9tdmi
integrator: LIB += -lpthread
integrator: $(OBJS)
	$(MAKE) -w -C ../.. $@ INCLUDE_JPEG=n INCLUDE_MPEG4=n INCLUDE_VIDSTAB=y USE_EFENCE=$(USE_EFENCE) \
                CROSS_COMPILE=$(CROSS_COMPILE) ARCH="$(ARCH)"
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(EFENCE) -o $(TARGET)

.PHONY: pci
pci: TARGETENV = pci
pci: CROSS_COMPILE=
pci: ARCH=
pci: $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) -o $(TARGET)

system_cov: CC = covc --retain -t!H264TestBench.c,!EncGetOption.c gcc -lstdc++
system_cov: TARGETENV = system_cov
system_cov: $(MODELLIB) $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) -o $(TARGET)

        
.PHONY: clean
clean:
	$(RM) *.o core* *~ $(TARGET) $(TARGET).*

.PHONY: libclean
libclean: clean
	$(MAKE) -w -C ../.. clean

.PHONY: tags
tags:
	ctags ../../../inc/*.h *.c ../../../source/h264/*.[ch] \
            ../../../source/common/*.[ch] ../../ewl/*c

