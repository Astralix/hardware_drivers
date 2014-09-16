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

#source search path
vpath %.c

SRCS = mpeg2dectest.c
OBJS    = $(SRCS:.c=.o)

INCLUDE = -I../../source/inc -I../../source/mpeg2/ -I../../source/config \
          -I../../source/common  -I../common/swhw

DECLIBDIR = -L../../linux/dwl -L../../linux/mpeg2 -L../common/swhw \
            -L../../../system/models/golden
DECLIB =  libdecx170m2.a
DWLLIB = libdwlx170.a
TBLIB = libtbcommon.a
MODLIB = lib8170hw.a

ALLTESTDECS= m2x170dec_versatile m2x170dec_integrator \
             m2x170dec_ads11 m2x170dec_pclinux m2x170dec_pclinux_eval \
	     m2x170dec_ads9
TARGET_ENV=


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
versatile: TESTDEC=m2x170dec_versatile
versatile: TARGET_ENV=versatile
versatile: LIBS = $(DECLIBDIR) -lpthread -ldecx170m2 -ldwlx170 -ltbcommon $(EFENCE) $(MD5SUM)
versatile: test


.PHONY: integrator
integrator: CC=arm-linux-gcc
integrator: CFLAGS = -O -g -mtune=arm920tdmi -fpic  $(INCLUDE) $(MD5SUM)
integrator: TESTDEC=m2x170dec_integrator
integrator: TARGET_ENV=integrator
integrator: LIBS = $(DECLIBDIR) -lpthread -ldecx170m2 -ldwlx170 -ltbcommon $(EFENCE)
integrator: test


.PHONY: ads11
ads11: CC=armcc
ads11: CFLAGS = -g --cpu ARM1136J-S --no_inline -O3 -Otime -DMPEG2DEC_EXTERNAL_ALLOC_DISABLE $(INCLUDE)
ads11: TESTDEC=m2x170dec_ads11
ads11: TARGET_ENV=ads11
ads11: _DWL_PCLINUX = y
ads11: LIBS = -L../../linux/mpeg2/$(DECLIB) -L../../linux/dwl/$(DWLLIB) -L../../../system/models/golden/$(MODLIB) -L../common/swhw/$(TBLIB) -D_ADS
ads11: test

.PHONY: ads9
ads9: CC=armcc
ads9: CFLAGS = -g --cpu ARM926EJ-S --no_inline -O3 -Otime -DMPEG2DEC_EXTERNAL_ALLOC_DISABLE $(INCLUDE)
ads9: TESTDEC=m2x170dec_ads9
ads9: TARGET_ENV=ads9
ads9: _DWL_PCLINUX = y
ads9: LIBS = -L../../linux/mpeg2/$(DECLIB) -L../../linux/dwl/$(DWLLIB) -L../../../system/models/golden/$(MODLIB) -L../common/swhw/$(TBLIB) -D_ADS
ads9: test

.PHONY: verification
verification: SYSTEM_VERIFICATION_TEST = -DSYSTEM_VERIFICATION
verification: pclinux

.PHONY: pclinux
pclinux: CC=gcc
pclinux: CFLAGS = -g $(INCLUDE) $(MD5SUM) $(M32) -DASIC_TRACE_SUPPORT -D_FILE_OFFSET_BITS=64 -DEXPIRY_DATE=1$(expiry) -D_LARGEFILE64_SOURCE
pclinux: TESTDEC=m2x170dec_pclinux
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(DECLIBDIR) -ldecx170m2 -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux: _DWL_PCLINUX = y
pclinux: test

.PHONY: coverage
coverage: CC=covc --abs --retain gcc
coverage: CFLAGS = -O -g $(INCLUDE) $(MD5SUM) $(M32)
coverage: TESTDEC=m2x170dec_coverage
coverage: TARGET_ENV=coverage
coverage: LIBS = $(DECLIBDIR) -ldecx170m2 -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
coverage: _DWL_PCLINUX = y
coverage: test

.PHONY: pclinux_eval_8170
pclinux_eval_8170: CFLAGS += -DMPEG2_EVALUATION_8170
pclinux_eval_8170: pclinux_eval

.PHONY: pclinux_eval_8190
pclinux_eval_8190: CFLAGS += -DMPEG2_EVALUATION_8190
pclinux_eval_8190: pclinux_eval

.PHONY: pclinux_eval_9170
pclinux_eval_9170: CFLAGS += -DMPEG2_EVALUATION_9170
pclinux_eval_9170: pclinux_eval

.PHONY: pclinux_eval_9190
pclinux_eval_9190: CFLAGS += -DMPEG2_EVALUATION_9190
pclinux_eval_9190: pclinux_eval

.PHONY: pclinux_eval_G1
pclinux_eval_G1: CFLAGS += -DMPEG2_EVALUATION_G1
pclinux_eval_G1: pclinux_eval

.PHONY: pclinux_eval
pclinux_eval: CC=gcc
pclinux_eval: CFLAGS += $(INCLUDE) $(MD5SUM) $(M32) -DMPEG2_EVALUATION -D_FILE_OFFSET_BITS=64 -DEXPIRY_DATE=1$(expiry) -D_LARGEFILE64_SOURCE
pclinux_eval: TESTDEC=m2x170dec_pclinux_eval
pclinux_eval: TARGET_ENV=pclinux_eval
pclinux_eval: LIBS = $(DECLIBDIR) -ldecx170m2 -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux_eval: _DWL_PCLINUX = y
pclinux_eval: DEBFLAGS = -O3 -DNDEBUG
pclinux_eval: test

export _DWL_PCLINUX
export SYSTEM_VERIFICATION_TEST
export ASIC_TRACE_SUPPORT

test: lib  $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TESTDEC)

lib:
	make -C ../../linux/mpeg2 $(TARGET_ENV)
	make -C ../../linux/dwl $(TARGET_ENV)
	make -C ../common/swhw $(TARGET_ENV)

clean:
	rm -f *.o
	rm -f .depend
	rm -f $(ALLTESTDECS)

libclean:
	make -C ../../linux/dwl clean
	make -C ../../linux/mpeg2 clean
	make -C ../common/swhw clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
