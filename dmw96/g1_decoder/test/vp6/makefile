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
#--  Abstract : Makefile for decoder testbench
#--
#-------------------------------------------------------------------------------


# set this to 'y' for Electric Fence checking
USE_EFENCE = n

# set this to 'y' to enable md5 checksum calculation (comandline option)
USE_MD5SUM = y

SRCS = on2vp6dec.c

OBJS    = $(SRCS:.c=.o)

INCLUDE = -I. -I../../source/inc \
          -I../../source/vp6 \
          -I../../source/config \
          -I../../source/common \
          -I../common/swhw          

LIBDECDIR = -L../../linux/vp6 \
            -L../../linux/dwl \
	    -L../common \
	    -L../common/swhw

LIBDEC = dec8190vp6

TESTDEC=vp6dec
TARGET_ENV=integrator

# reference coded root  for hw model building; export

LIB_REF=on2vpplugin6
LIB_MODEL=vp6hwmodel

DECLIBDIR = -L../../linux/vp6 -L../../linux/dwl -L../common/swhw \
	    -L../../../system/models/golden
DECLIB = libdec8190vp6.a

# Enable SW performance  measurement
#CFLAGS = -DSW_PERFORMANCE

ifeq ($(USE_EFENCE), y)
        EFENCE= -DUSE_EFENCE -L/afs/hantro.com/projects/adder/users/mahe/efence_2_4_13 \
                -lefence -lpthread
endif

ifeq ($(USE_MD5SUM), y)
        MD5SUM = -DMD5SUM
endif

ifeq ($(shell uname -m),x86_64)
	ifneq (,$(findstring pclinux,$(MAKECMDGOALS)))
		M32 = -m32
	endif
endif

export M32
export ASIC_TRACE_SUPPORT
export SET_EMPTY_PICTURE_DATA

ARMCPU=ARM1136J-S

all: versatile

.PHONY: versatile
versatile: CC=arm-none-linux-gnueabi-gcc
versatile: CFLAGS += -O -g -mtune=arm926ej-s -fpic $(INCLUDE) $(MD5SUM) -D_FILE_OFFSET_BITS=64
versatile: TESTDEC=vp6dec_versatile
versatile: TARGET_ENV=versatile
versatile: DWL_TARGET_ENV=versatile
versatile: LIBS = $(LIBDECDIR) -lpthread -l$(LIBDEC) -ldwlx170  -ltbcommon $(EFENCE)
versatile: test

.PHONY: integrator
integrator: CC=arm-linux-gcc
integrator: CFLAGS += -O -g -mtune=arm920tdmi -fpic $(INCLUDE) $(MD5SUM)
integrator: TESTDEC=vp6dec_integrator
integrator: TARGET_ENV=integrator
integrator: DWL_TARGET_ENV=integrator
integrator: LIBS = $(LIBDECDIR) -lpthread -l$(LIBDEC) -ldwlx170 -ltbcommon $(EFENCE)
integrator: test

.PHONY: ads
ads: CC=armcc
ads: CFLAGS = --cpu $(ARMCPU) -g -O2 -Otime --no_inline $(INCLUDE) -DADS_PERFORMANCE_SIMULATION
ads: TESTDEC=vp6dec_ads
ads: TARGET_ENV=ads
ads: DWL_TARGET_ENV=ads
ads: LIBS = $(LIBDECDIR) -lpthread -l$(LIBDEC) -ldwlx170 -ltbcommon $(EFENCE)
ads: ASIC_TRACE_SUPPORT=
ads: lib $(OBJS)
	armlink --noremove $(OBJS) $(LIBS) -o $(TESTDEC).axf

ads9: override ARMCPU=ARM926EJ-S
ads9: ads

ads11: override ARMCPU=ARM1136J-S
ads11: ads

.PHONY: pclinux
pclinux: CC=gcc
pclinux: CFLAGS = -g $(INCLUDE) $(MD5SUM) $(M32) -DASIC_TRACE_SUPPORT -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DEXPIRY_DATE=1$(expiry)
pclinux: TESTDEC=vp6dec_pclinux
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(DECLIBDIR) -ldec8190vp6 -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux: _DWL_PCLINUX = y
pclinux: _DWL_VP6_PCLINUX = n
pclinux: ASIC_TRACE_SUPPORT = -DASIC_TRACE_SUPPORT
pclinux: test

.PHONY: pclinux_eval_9190
pclinux_eval_9190: CFLAGS += -DVP6_EVALUATION_9190
pclinux_eval_9190: _PRODUCT_9190 = y
pclinux_eval_9190: pclinux_eval

.PHONY: pclinux_eval_G1
pclinux_eval_G1: CFLAGS += -DVP6_EVALUATION_G1
pclinux_eval_G1: _PRODUCT_G1 = y
pclinux_eval_G1: pclinux_eval

.PHONY: pclinux_eval
pclinux_eval: CFLAGS = -g $(INCLUDE) $(MD5SUM) $(M32) -DASIC_TRACE_SUPPORT -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DEXPIRY_DATE=1$(expiry) -DVP6_EVALUATION
pclinux_eval: TESTDEC=vp6dec_pclinux_eval
pclinux_eval: TARGET_ENV=pclinux_eval
pclinux_eval: DWL_TARGET_ENV=vp6pclinux
pclinux_eval: LIBDWL=dwlvp6hwmodel
pclinux_eval: _DWL_PCLINUX = y
pclinux_eval: _DWL_VP6_PCLINUX = n
pclinux_eval: DEBFLAGS = -O3 -DNDEBUG
pclinux_eval: LIBS = $(DECLIBDIR) -ldec8190vp6 -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux_eval: test

export _DWL_PCLINUX
export _DWL_VP6_PCLINUX
export _PRODUCT_9190
export _PRODUCT_G1

test: lib $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TESTDEC)

lib:
	make -C ../../linux/vp6 $(TARGET_ENV)
	make -C ../../linux/dwl $(TARGET_ENV)
	make -C ../common/swhw $(TARGET_ENV)

clean:
	rm -f *.o core.*
	rm -f .depend
	rm -f vp6dec_*

libclean:
	make -C ../../linux/vp6 clean
	make -C ../common/swhw clean
	make -C ../../linux/dwl clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
