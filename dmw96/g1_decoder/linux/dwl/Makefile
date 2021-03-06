#-------------------------------------------------------------------------------
#-                                                                            --
#-       This software is confidential and proprietary and may be used        --
#-        only as expressly authorized by a licensing agreement from          --
#-                                                                            --
#-                            Hantro Products Oy.                             --
#-                                                                            --
#-                   (C) COPYRIGHT 2011 HANTRO PRODUCTS OY                    --
#-                            ALL RIGHTS RESERVED                             --
#-                                                                            --
#-                 The entire notice above must be reproduced                 --
#-                  on all copies and should not be removed.                  --
#-                                                                            --
#-------------------------------------------------------------------------------
#-
#--  Abstract : Makefile for DWL
#--
#-------------------------------------------------------------------------------

# Comment/uncomment the following line to disable/enable debugging
DEBUG = y

# set this to 'y' for enabling IRQ mode for the decoder. You will need
# the hx170dec kernel driver loaded and a /dev/hx170 device node created
USE_DEC_IRQ = n

# set this 'y' for enabling sw reg tracing. NOTE! not all sw reagisters are 
# traced; only hw "return" values.
INTERNAL_TEST = y

# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DDEBUG
else
  DEBFLAGS = -O3 -DNDEBUG
endif

#DEBFLAGS += -D_STREAM_POS_TRACE
#DEBFLAGS += -D_DWL_DEBUG # for wrapper layer trace
#DEBFLAGS += -DMEMORY_USAGE_TRACE #trace all memory allocations
#DEBFLAGS += -D_READ_DEBUG_REGS
#DEBFLAGS += -D_DWL_FAKE_HW_TIMEOUT # Change stream error interrupt to hw timeouts

# Enable HW performance measurements (only in interrupt mode)
ifeq ($(USE_DEC_IRQ),y)
#  DEBFLAGS += -D_DWL_HW_PERFORMANCE
endif

ifeq ($(INTERNAL_TEST),y)
   DEBFLAGS += -DINTERNAL_TEST
endif

# path to efence headers
EFENCE_PATH = /afs/hantro.com/projects/adder/users/mahe/efence_2_4_13

# system independent code location
COMMON_SOURCE_DIR = ../../source

# Architecture flags for gcc
ARCH =
CROSS_COMPILER =

# C -compiler name, can be replaced by another compiler(replace gcc)
CC = $(CROSS_COMPILER)gcc
# MACRO for cleaning object -files
RM  = rm -f

# MACRO for creating library that includes all the object files
AR  = $(CROSS_COMPILER)ar rcs

# show the path to compiler, where to find header files and libraries
INCLUDE = -I. -I$(COMMON_SOURCE_DIR)/inc -I$(COMMON_SOURCE_DIR)/dwl \
			-I../memalloc -I../ldriver/kernel_26x/ \
			-I$(COMMON_SOURCE_DIR)/common

# compiler switches
CFLAGS  = -Wall -ansi $(ARCH) -D_GNU_SOURCE $(DEBFLAGS) $(INCLUDE) $(M32)

# list of used sourcefiles
ifeq ($(USE_DEC_IRQ),y)
        SRC_DWL_ARM := dwl_x170_linux_irq.c # use this when decoder generates IRQs.
else
        SRC_DWL_ARM := dwl_x170_linux_no_drv.c # use this for polling mode operation
endif

SRC_DWL_SIMULATION := dwl_x170_pc.c

ifeq ($(_DWL_PCLINUX), y)
    INCLUDE += -I../../../system/models/golden
    CFLAGS += -D_DWL_PCLINUX
endif

#set these for evaluation
ifeq ($(_PRODUCT_8170), y)
    CFLAGS += -DDWL_EVALUATION_8170
endif

ifeq ($(_PRODUCT_8190), y)
    CFLAGS += -DDWL_EVALUATION_8190
endif

ifeq ($(_PRODUCT_9170), y)
    CFLAGS += -DDWL_EVALUATION_9170
endif

ifeq ($(_PRODUCT_9190), y)
    CFLAGS += -DDWL_EVALUATION_9190
endif

ifeq ($(_PRODUCT_G1), y)
    CFLAGS += -DDWL_EVALUATION_G1
endif

ifeq ($(_DWL_VP6_PCLINUX), y)
    CFLAGS += -D_DWL_VP6_PCLINUX
endif

ifneq (,$(findstring ads, $(MAKECMDGOALS)))
    SRCS += $(SRC_DWL_SIMULATION)
else
    ifneq (,$(findstring pc, $(MAKECMDGOALS)))
        SRCS += $(SRC_DWL_SIMULATION)
    else
		ifneq (,$(findstring coverage, $(MAKECMDGOALS)))
        	SRCS += $(SRC_DWL_SIMULATION)
		else
        	SRCS += $(SRC_DWL_ARM) dwl_linux_lock.c dwl_linux.c
		endif
    endif
endif

#source search path
vpath %.o
vpath %.o obj

# object files will be generated from .c sourcefiles
OBJS    = $(SRCS:.c=.o)

# name of the outputfile (library)
DECLIB = libdwlx170.a

OBJDIR := obj

#Here are rules for building codes and generating object library.
all:
	@echo
	@echo ERROR! run one of:
	@echo "    $$ make integrator"
	@echo "    $$ make versatile"
	@echo "    $$ make pclinux"
	@echo "    $$ make pclinux_eval"
	@echo "    $$ make pci"
	@echo "    $$ make ads"
	@echo

$(OBJDIR):
	mkdir $@

ARMCPU=ARM1136J-S
ads: CC=armcc
ads: AR=armar -rcs
ads: INCLUDE += -I../../../system/models/golden
ads: CFLAGS = --cpu $(ARMCPU) -g --gnu -O2 -Otime $(INCLUDE) -D_DWL_PCLINUX -D_DWL_PERFORMANCE
ads: $(DECLIB)
	make -C ../../../system/models/golden ads

ads9: CC=armcc
ads9: AR=armar -rcs
ads9: INCLUDE += -I../../../system/models/golden
ads9: CFLAGS = --cpu ARM926EJ-S -g --gnu -O2 -Otime $(INCLUDE) -D_DWL_PCLINUX -D_DWL_PERFORMANCE
ads9: $(DECLIB)
	make -C ../../../system/models/golden ads9

ads11: CC=armcc
ads11: AR=armar -rcs
ads11: INCLUDE += -I../../../system/models/golden
ads11: CFLAGS = --cpu ARM1136J-S -g --gnu -O2 -Otime $(INCLUDE) -D_DWL_PCLINUX -D_DWL_PERFORMANCE
ads11: $(DECLIB)
	make -C ../../../system/models/golden ads11

pclinux: $(DECLIB)
	make -C ../../../system/models/golden

pclinux_eval: DEBFLAGS = -O3 -DNDEBUG
pclinux_eval: $(DECLIB)
	make -C ../../../system/models/golden pclinux_eval

.PHONY: coverage
coverage: CC=covc --abs --retain gcc
coverage: $(DECLIB)
	make -C ../../../system/models/golden


integrator: ENVSET  = -DSDRAM_LM_BASE=0x80000000 \
	              -DDEC_MODULE_PATH=\"/tmp/dev/hx170\" \
                     -DMEMALLOC_MODULE_PATH=\"/tmp/dev/memalloc\"
integrator: INCLUDE += -I../../source/ldriver/kernel_24x # linux kernel 2.4.x
integrator: ARCH = -mtune=arm9tdmi -fpic
integrator: CROSS_COMPILER = arm-linux-
integrator: $(DECLIB)

versatile: ENVSET  = -DSDRAM_LM_BASE=0x00000000 \
                     -DDEC_MODULE_PATH=\"/tmp/dev/hx170\" \
                     -DMEMALLOC_MODULE_PATH=\"/tmp/dev/memalloc\"
versatile: INCLUDE += -I../../source/ldriver/kernel_26x # linux kernel 2.6.x
versatile: CROSS_COMPILER = arm-none-linux-gnueabi-
versatile: CC = arm-none-linux-gnueabi-gcc
versatile: ARCH = -mtune=arm926ej-s -fpic
versatile: $(DECLIB)

pci: ENVSET  = -DSDRAM_LM_BASE=0x20000000 \
               -DDEC_MODULE_PATH=\"/tmp/dev/hx170\" \
               -DMEMALLOC_MODULE_PATH=\"/tmp/dev/memalloc\"
pci: INCLUDE += -I../../source/ldriver/kernel_26x # linux kernel 2.6.x
pci: $(DECLIB)



$(DECLIB): .depend $(OBJDIR) $(OBJS)
	$(AR) $(DECLIB) $(patsubst %,$(OBJDIR)/%, $(OBJS))

$(OBJS): %.o: %.c
	$(CC) -c $(CFLAGS) $(ENVSET) $< -o $(OBJDIR)/$(@F)

clean:
	$(RM) $(DECLIB)
	$(RM) .depend
	$(RM) -r $(OBJDIR)
	@-make -C ../../../system/models/golden/ clean 2>/dev/null

depend .depend: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

tar:
	$(RM) dwlx170.tar
	tar -cf dwlx170.tar $(DECLIB)

.PHONY: ads ads9 ads11 pclinux pclinux_eval pc_plain_lib integrator versatile pci clean lint tar

ifneq ( , $( findstring clean , $(MAKECMDGOALS) ))
ifeq (.depend, $(wildcard .depend))
include .depend
endif
endif

