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
USE_MD5SUM = y

#source search path
vpath %.c

SRCS = mpeg4dectest.c rm_length.c
OBJS    = $(SRCS:.c=.o)

INCLUDE = -I../../source/inc -I../../source/mpeg4/ -I../../source/config \
          -I../../source/common  -I../common/swhw

DECLIBDIR = -L../../linux/mpeg4 -L../common/swhw \
            -L../../../system/models/golden -L../../linux/dwl

DECLIB =  libdecx170m.a
DWLLIB = libdwlx170.a
TBLIB = libtbcommon.a
MODLIB = lib8170hw.a

ALLTESTDECS= mx170dec_versatile mx170dec_integrator \
             mx170dec_ads11 mx170dec_pclinux mx170dec_pclinux_eval \
	     mx170dec_ads9 mx170dec_coverage
TARGET_ENV=

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
versatile: CFLAGS += -O -g -mtune=arm926ej-s -fpic $(INCLUDE) $(MD5SUM) -D_LARGEFILE64_SOURCE
versatile: TESTDEC=mx170dec_versatile
versatile: TARGET_ENV=versatile
versatile: LIBS = $(DECLIBDIR) -lpthread -ldecx170m -ldwlx170 -ltbcommon $(EFENCE) $(MD5SUM)
versatile: test


.PHONY: integrator
integrator: CC=arm-linux-gcc
integrator: CFLAGS += -O -g -mtune=arm920tdmi -fpic  $(INCLUDE) $(MD5SUM)
integrator: TESTDEC=mx170dec_integrator
integrator: TARGET_ENV=integrator
integrator: LIBS = $(DECLIBDIR) -lpthread -ldecx170m -ldwlx170 -ltbcommon $(EFENCE)
integrator: test


.PHONY: ads11
ads11: CC=armcc
ads11: CFLAGS = -g -cpu ARM1136J-S -O2 -Otime --no_inline -DMP4DEC_EXTERNAL_ALLOC_DISABLE $(INCLUDE)
ads11: TESTDEC=mx170dec_ads11
ads11: TARGET_ENV=ads11
ads11: _DWL_PCLINUX = y
ads11: LIBS = -l../../linux/mpeg4/$(DECLIB) -l../../linux/dwl/$(DWLLIB) -l../../../system/models/golden/$(MODLIB) -l../common/swhw/$(TBLIB) -D_ADS
ads11: test

.PHONY: ads9
ads9: CC=armcc
ads9: CFLAGS = -g -cpu ARM926EJ-S -O2 -Otime --no_inline  -DMP4DEC_EXTERNAL_ALLOC_DISABLE $(INCLUDE)
ads9: TESTDEC=mx170dec_ads9
ads9: TARGET_ENV=ads9
ads9: _DWL_PCLINUX = y
ads9: LIBS = -l../../linux/mpeg4/$(DECLIB) -l../../linux/dwl/$(DWLLIB) -l../../../system/models/golden/$(MODLIB) -l../common/swhw/$(TBLIB) -D_ADS
ads9: test

.PHONY: pclinux
pclinux: CC=gcc
pclinux: CFLAGS = -g $(INCLUDE) $(MD5SUM) $(M32) -DASIC_TRACE_SUPPORT -D_FILE_OFFSET_BITS=64 -DEXPIRY_DATE=1$(expiry) -D_LARGEFILE64_SOURCE
pclinux: TESTDEC=mx170dec_pclinux
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(DECLIBDIR) -ldecx170m -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux: _DWL_PCLINUX = y
pclinux: ASIC_TRACE_SUPPORT = -DASIC_TRACE_SUPPORT
pclinux: test

.PHONY: verification
verification: SYSTEM_VERIFICATION_TEST = -DSYSTEM_VERIFICATION
verification: pclinux

.PHONY: coverage
coverage: CC=covc --abs --retain gcc
coverage: CFLAGS = -O -g $(INCLUDE) $(MD5SUM) $(M32)
coverage: TESTDEC=mx170dec_coverage
coverage: TARGET_ENV=coverage
coverage: LIBS = $(DECLIBDIR) -ldecx170m -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
coverage: _DWL_PCLINUX = y
coverage: test

.PHONY: pclinux_eval_8170
pclinux_eval_8170: CFLAGS += -DMPEG4_EVALUATION_8170
pclinux_eval_8170: pclinux_eval

.PHONY: pclinux_eval_8190
pclinux_eval_8190: CFLAGS += -DMPEG4_EVALUATION_8190
pclinux_eval_8190: pclinux_eval

.PHONY: pclinux_eval_9170
pclinux_eval_9170: CFLAGS += -DMPEG4_EVALUATION_9170
pclinux_eval_9170: pclinux_eval

.PHONY: pclinux_eval_9190
pclinux_eval_9190: CFLAGS += -DMPEG4_EVALUATION_9190
pclinux_eval_9190: pclinux_eval

.PHONY: pclinux_eval_G1
pclinux_eval_G1: CFLAGS += -DMPEG4_EVALUATION_G1
pclinux_eval_G1: pclinux_eval

.PHONY: pclinux_eval
pclinux_eval: CC=gcc
pclinux_eval: CFLAGS += $(INCLUDE) $(MD5SUM) $(M32) -DMPEG4_EVALUATION -D_FILE_OFFSET_BITS=64 -DEXPIRY_DATE=1$(expiry)
pclinux_eval: TESTDEC=mx170dec_pclinux_eval
pclinux_eval: TARGET_ENV=pclinux_eval
pclinux_eval: LIBS = $(DECLIBDIR) -ldecx170m -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux_eval: _DWL_PCLINUX = y
pclinux_eval: DEBFLAGS = -O3 -DNDEBUG
pclinux_eval: test

export _DWL_PCLINUX
export SYSTEM_VERIFICATION_TEST
export ASIC_TRACE_SUPPORT
export CUSTOM_FMT_SUPPORT

test: lib  $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TESTDEC)

lib:
	make -C ../../linux/mpeg4 $(TARGET_ENV)
	make -C ../../linux/dwl $(TARGET_ENV)
	make -C ../common/swhw $(TARGET_ENV)

clean:
	rm -f *.o
	rm -f .depend
	rm -f $(ALLTESTDECS)

libclean:
	make -C ../../linux/dwl clean
	make -C ../../linux/mpeg4 clean
	make -C ../common/swhw clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
