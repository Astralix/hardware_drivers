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

# set this to 'y' for md5 checksum calculation of the frames instead of yuv
USE_MD5SUM = n

SRCS = dectestbench.c
#SRCS = decpp.c
#SRCS += yuvtorgb16.c yuvtorgbinit.c
OBJS    = $(SRCS:.c=.o)

INCLUDE = -I../../source/inc -I../../source/vc1 -I../../source/config \
          -I../../source/dwl -I../../source/common -I../common/swhw

DECLIBDIR = -L../../linux/vc1 -L../../linux/dwl -L../common/swhw \
	    -L../../../system/models/golden
DECLIB = libdecx170v.a
DWLLIB = libdwlx170.a
TBLIB = libtbcommon.a

TESTDEC=vx170dec
TARGET_ENV=integrator

# Enable SW performance  measurement
#CFLAGS = -DSW_PERFORMANCE

ifeq ($(USE_EFENCE), y)
        EFENCE= -DUSE_EFENCE -lefence -lpthread
endif

ifeq ($(USE_MD5SUM), y)
        MD5SUM = -DMD5SUM
endif

ifeq ($(shell uname -m),x86_64)
	ifneq (,$(findstring pclinux,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
	ifneq (,$(findstring coverage,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
	ifneq (,$(findstring verification,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
endif

all: versatile

.PHONY: versatile
versatile: CC=arm-none-linux-gnueabi-gcc
versatile: CFLAGS += -O -g -mtune=arm926ej-s -fpic $(INCLUDE) $(MD5SUM) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
versatile: TESTDEC=vx170dec_versatile
versatile: TARGET_ENV=versatile
versatile: LIBS = $(DECLIBDIR) -lpthread -ldecx170v -ldwlx170 -ltbcommon $(EFENCE) 
versatile: test

.PHONY: integrator
integrator: CC=arm-linux-gcc
integrator: CFLAGS += -O -g -mtune=arm920tdmi -fpic $(INCLUDE) $(MD5SUM)
integrator: TESTDEC=vx170dec_integrator
integrator: TARGET_ENV=integrator
integrator: LIBS = $(DECLIBDIR) -lpthread -ldecx170v -ldwlx170 -ltbcommon $(EFENCE)
integrator: test


.PHONY: ads9
ads9: CC=armcc
ads9: CFLAGS = --no_inline --cpu ARM926EJ-S -O2 -Otime $(INCLUDE)
ads9: TESTDEC=vx170dec_ads9.axf
ads9: TARGET_ENV=ads9
ads9: LIBS = -L../../linux/vc1/$(DECLIB) -L../../linux/dwl/$(DWLLIB) -L../common/swhw/$(TBLIB)
ads9: test

.PHONY: ads
ads: CC=armcc
ads: CFLAGS = --no_inline --cpu ARM1136J-S -O3 -Otime $(INCLUDE)
ads: TESTDEC=vx170dec_ads.axf
ads: TARGET_ENV=ads
ads: LIBS = -L../../linux/vc1/$(DECLIB) -L../../linux/dwl/$(DWLLIB) -L../common/swhw/$(TBLIB)
ads: test

.PHONY: pclinux
pclinux: CC=gcc
pclinux: CFLAGS = -g $(INCLUDE) $(MD5SUM) $(M32) -DASIC_TRACE_SUPPORT -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DEXPIRY_DATE=1$(expiry)
pclinux: TESTDEC=vx170dec_pclinux
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(DECLIBDIR) -ldecx170v -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux: _DWL_PCLINUX = y
pclinux: ASIC_TRACE_SUPPORT = -DASIC_TRACE_SUPPORT
pclinux: test

.PHONY: pclinux_eval_8170
pclinux_eval_8170: CFLAGS += -DVC1_EVALUATION_8170
pclinux_eval_8170: _PRODUCT_8170 = y
pclinux_eval_8170: pclinux_eval

.PHONY: pclinux_eval_8190
pclinux_eval_8190: CFLAGS += -DVC1_EVALUATION_8190
pclinux_eval_8190: _PRODUCT_8190 = y
pclinux_eval_8190: pclinux_eval

.PHONY: pclinux_eval_9170
pclinux_eval_9170: CFLAGS += -DVC1_EVALUATION_9170
pclinux_eval_9170: _PRODUCT_9170 = y
pclinux_eval_9170: pclinux_eval

.PHONY: pclinux_eval_9190
pclinux_eval_9190: CFLAGS += -DVC1_EVALUATION_9190
pclinux_eval_9190: _PRODUCT_9190 = y
pclinux_eval_9190: pclinux_eval

.PHONY: pclinux_eval_G1
pclinux_eval_G1: CFLAGS += -DVC1_EVALUATION_G1
pclinux_eval_G1: _PRODUCT_G1 = y
pclinux_eval_G1: pclinux_eval

.PHONY: pclinux_eval
pclinux_eval: CC=gcc
pclinux_eval: CFLAGS += $(INCLUDE) $(MD5SUM) $(M32) -DVC1_EVALUATION -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DEXPIRY_DATE=1$(expiry)
pclinux_eval: TESTDEC=vx170dec_pclinux_eval
pclinux_eval: TARGET_ENV=pclinux_eval
pclinux_eval: LIBS = $(DECLIBDIR) -ldecx170v -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux_eval: _DWL_PCLINUX = y
pclinux_eval: DEBFLAGS = -O3 -DNDEBUG
pclinux_eval: test

export _DWL_PCLINUX
export ASIC_TRACE_SUPPORT
export _PRODUCT_8170
export _PRODUCT_8190
export _PRODUCT_9170
export _PRODUCT_9190
export _PRODUCT_G1

test: lib $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TESTDEC)

lib:
	make -C ../../linux/vc1 $(TARGET_ENV)
	make -C ../../linux/dwl $(TARGET_ENV)
	make -C ../common/swhw $(TARGET_ENV)

clean:
	rm -f *.o
	rm -f .depend
	rm -f vx170dec_*

libclean:
	make -C ../../linux/vc1 clean
	make -C ../../linux/dwl clean
	make -C ../common/swhw clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
