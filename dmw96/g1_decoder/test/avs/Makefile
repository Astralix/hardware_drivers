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

SRCS = avsdectest.c
OBJS    = $(SRCS:.c=.o)

INCLUDE = -I../../source/inc -I../../source/avs/ -I../../source/config \
          -I../../source/common  -I../common/swhw

DECLIBDIR = -L../../linux/dwl -L../../linux/avs -L../common/swhw \
            -L../../../system/models/golden
DECLIB =  libdecx170a.a
DWLLIB = libdwlx170.a
TBLIB = libtbcommon.a
MODLIB = lib8170hw.a

ALLTESTDECS= ax170dec_versatile ax170dec_integrator \
             ax170dec_ads11 ax170dec_pclinux ax170dec_ads9
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
versatile: CFLAGS += -O -g -mtune=arm926ej-s -fpic $(INCLUDE) $(MD5SUM)  -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
versatile: TESTDEC=ax170dec_versatile
versatile: TARGET_ENV=versatile
versatile: LIBS = $(DECLIBDIR) -lpthread -ldecx170a -ldwlx170 -ltbcommon $(EFENCE) $(MD5SUM)
versatile: test


.PHONY: integrator
integrator: CC=arm-linux-gcc
integrator: CFLAGS = -O -g -mtune=arm920tdmi -fpic  $(INCLUDE) $(MD5SUM)
integrator: TESTDEC=ax170dec_integrator
integrator: TARGET_ENV=integrator
integrator: LIBS = $(DECLIBDIR) -lpthread -ldecx170a -ldwlx170 -ltbcommon $(EFENCE)
integrator: test


.PHONY: ads11
ads11: CC=armcc
ads11: CFLAGS = -g --cpu ARM1136J-S --no_inline -O3 -Otime $(INCLUDE) -DADS_PERFORMANCE_SIMULATION
ads11: TESTDEC=ax170dec_ads11
ads11: TARGET_ENV=ads11
ads11: _DWL_PCLINUX = y
ads11: LIBS = -L../../linux/avs/$(DECLIB) -L../../linux/dwl/$(DWLLIB) -L../../../system/models/golden/$(MODLIB) -L../common/swhw/$(TBLIB) -D_ADS
ads11: test

.PHONY: ads9
ads9: CC=armcc
ads9: CFLAGS = -g --cpu ARM926EJ-S --no_inline -O3 -Otime $(INCLUDE) -DADS_PERFORMANCE_SIMULATION
ads9: TESTDEC=ax170dec_ads9
ads9: TARGET_ENV=ads9
ads9: _DWL_PCLINUX = y
ads9: LIBS = -L../../linux/avs/$(DECLIB) -L../../linux/dwl/$(DWLLIB) -L../../../system/models/golden/$(MODLIB) -L../common/swhw/$(TBLIB) -D_ADS
ads9: test

.PHONY: ads
ads: CC=armcc
ads: CFLAGS = -g --cpu ARM1136J-S --no_inline -O3 -Otime $(INCLUDE) -DADS_PERFORMANCE_SIMULATION
ads: TESTDEC=ax170dec_ads
ads: TARGET_ENV=ads
ads: _DWL_PCLINUX = y
ads: LIBS = -L../../linux/avs/$(DECLIB) -L../../linux/dwl/$(DWLLIB) -L../../../system/models/golden/$(MODLIB) -L../common/swhw/$(TBLIB) -D_ADS
ads: test

.PHONY: verification
verification: SYSTEM_VERIFICATION_TEST = -DSYSTEM_VERIFICATION
verification: pclinux

.PHONY: pclinux
pclinux: CC=gcc
pclinux: CFLAGS = -g $(INCLUDE) $(MD5SUM) $(M32) -DASIC_TRACE_SUPPORT  -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DEXPIRY_DATE=1$(expiry)
pclinux: TESTDEC=ax170dec_pclinux
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(DECLIBDIR) -ldecx170a -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux: _DWL_PCLINUX = y
pclinux: test

.PHONY: coverage
coverage: CC=covc --abs --retain gcc
coverage: CFLAGS = -O -g $(INCLUDE) $(MD5SUM) $(M32)
coverage: TESTDEC=ax170dec_coverage
coverage: TARGET_ENV=coverage
coverage: LIBS = $(DECLIBDIR) -ldecx170a -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
coverage: _DWL_PCLINUX = y
coverage: test

.PHONY: pclinux_eval_G1
pclinux_eval_G1: CFLAGS += -DAVS_EVALUATION_G1
pclinux_eval_G1: pclinux_eval

.PHONY: pclinux_eval
pclinux_eval: CC=gcc
pclinux_eval: CFLAGS += $(INCLUDE) $(MD5SUM) $(M32) -DAVS_EVALUATION -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DEXPIRY_DATE=1$(expiry)
pclinux_eval: TESTDEC=ax170dec_pclinux_eval
pclinux_eval: TARGET_ENV=pclinux_eval
pclinux_eval: LIBS = $(DECLIBDIR) -ldecx170a -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux_eval: _DWL_PCLINUX = y
pclinux_eval: DEBFLAGS = -O3 -DNDEBUG
pclinux_eval: test

export _DWL_PCLINUX
export SYSTEM_VERIFICATION_TEST
export ASIC_TRACE_SUPPORT

test: lib  $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TESTDEC)

lib:
	make -C ../../linux/avs $(TARGET_ENV)
	make -C ../../linux/dwl $(TARGET_ENV)
	make -C ../common/swhw $(TARGET_ENV)

clean:
	rm -f *.o
	rm -f .depend
	rm -f $(ALLTESTDECS)

libclean:
	make -C ../../linux/dwl clean
	make -C ../../linux/avs clean
	make -C ../common/swhw clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
