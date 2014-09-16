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

SRCS = hantro_rv_test.c
OBJS    = $(SRCS:.c=.o)

INCLUDE = -I../../source/inc -I../../source/rv/ -I../../source/config \
          -I../../source/common  -I../common/swhw \
	  -Irm_parser/include

DECLIBDIR = -L../../linux/dwl -L../../linux/rv -L../common/swhw \
            -L../../../system/models/golden -Lrm_parser
DECLIB =  libdecx170rv.a
DWLLIB = libdwlx170.a
TBLIB = libtbcommon.a
MODLIB = lib8170hw.a

ALLTESTDECS= rvx170dec_versatile rvx170dec_integrator \
             rvx170dec_ads11 rvx170dec_pclinux rvx170dec_pclinux_eval \
	     rvx170dec_ads9
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
	ifneq (,$(findstring api,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
endif

all: versatile

.PHONY: versatile
versatile: CC=arm-none-linux-gnueabi-gcc
versatile: CFLAGS += -O -g -mtune=arm926ej-s -fpic $(INCLUDE) $(MD5SUM)
versatile: TESTDEC=rvx170dec_versatile
versatile: TARGET_ENV=versatile
versatile: LIBS = $(DECLIBDIR) -lpthread -ldecx170rv -ldwlx170 -ltbcommon -lrvparser $(EFENCE) $(MD5SUM)
versatile: test


.PHONY: integrator
integrator: CC=arm-linux-gcc
integrator: CFLAGS = -O -g -mtune=arm920tdmi -fpic  $(INCLUDE) $(MD5SUM)
integrator: TESTDEC=rvx170dec_integrator
integrator: TARGET_ENV=integrator
integrator: LIBS = $(DECLIBDIR) -lpthread -ldecx170rv -ldwlx170 -ltbcommon -lrvparser $(EFENCE)
integrator: test


.PHONY: ads11
ads11: CC=armcc
ads11: CFLAGS = -g --cpu ARM1136J-S --no_inline -O3 -Otime -DRVDEC_EXTERNAL_ALLOC_DISABLE $(INCLUDE)
ads11: TESTDEC=rvx170dec_ads11
ads11: TARGET_ENV=ads11
ads11: _DWL_PCLINUX = y
ads11: LIBS = -L../../linux/rv/$(DECLIB) -L../../linux/dwl/$(DWLLIB) -L../../../system/models/golden/$(MODLIB) -L../common/swhw/$(TBLIB) -D_ADS
ads11: test

.PHONY: ads9
ads9: CC=armcc
ads9: CFLAGS = -g --cpu ARM926EJ-S --no_inline -O3 -Otime -DRVDEC_EXTERNAL_ALLOC_DISABLE $(INCLUDE)
ads9: TESTDEC=rvx170dec_ads9
ads9: TARGET_ENV=ads9
ads9: _DWL_PCLINUX = y
ads9: LIBS = -L../../linux/rv/$(DECLIB) -L../../linux/dwl/$(DWLLIB) -L../../../system/models/golden/$(MODLIB) -L../common/swhw/$(TBLIB) -D_ADS
ads9: test

.PHONY: verification
verification: SYSTEM_VERIFICATION_TEST = -DSYSTEM_VERIFICATION
verification: pclinux

.PHONY: pclinux
pclinux: CC=gcc
pclinux: CFLAGS = -g $(INCLUDE) $(MD5SUM) $(M32) -D_FILE_OFFSET_BITS=64 -DASIC_TRACE_SUPPORT -DEXPIRY_DATE=1$(expiry)
pclinux: TESTDEC=rvx170dec_pclinux
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(DECLIBDIR) -ldecx170rv -ldwlx170 -l8170hw -ltbcommon -lrvparser $(EFENCE)
pclinux: _DWL_PCLINUX = y
pclinux: ASIC_TRACE_SUPPORT = -DASIC_TRACE_SUPPORT
pclinux: test

.PHONY: api
api: CC=gcc
api: CFLAGS = -g $(INCLUDE) $(MD5SUM) $(M32) -D_FILE_OFFSET_BITS=64 -DASIC_TRACE_SUPPORT -DEXPIRY_DATE=1$(expiry)
api: TESTDEC=rvx170dec_pclinux
api: TARGET_ENV=pclinux
api: LIBS = $(DECLIBDIR) -ldecx170rv -ldwlx170 -l8170hw -ltbcommon -lrvparser $(EFENCE)
api: _DWL_PCLINUX = y
api: ASIC_TRACE_SUPPORT = -DASIC_TRACE_SUPPORT
api: lib api_rv_test.o
	$(CC) $(CFLAGS) api_rv_test.o $(LIBS) -o $(TESTDEC)

.PHONY: coverage
coverage: CC=covc --abs --retain gcc
coverage: CFLAGS = -g $(INCLUDE) $(MD5SUM) $(M32) -DASIC_TRACE_SUPPORT -D_FILE_OFFSET_BITS=64
coverage: TESTDEC=rvx170dec_coverage
coverage: TARGET_ENV=coverage
coverage: LIBS = $(DECLIBDIR) -ldecx170rv -ldwlx170 -l8170hw -ltbcommon -lrvparser $(EFENCE)
coverage: _DWL_PCLINUX = y
coverage: ASIC_TRACE_SUPPORT = -DASIC_TRACE_SUPPORT
coverage: test

.PHONY: pclinux_eval_9170
pclinux_eval_9170: CFLAGS += -DRV_EVALUATION_9170
pclinux_eval_9170: _PRODUCT_9170 = y
pclinux_eval_9170: pclinux_eval

.PHONY: pclinux_eval_9190
pclinux_eval_9190: CFLAGS += -DRV_EVALUATION_9190
pclinux_eval_9190: _PRODUCT_9190 = y
pclinux_eval_9190: pclinux_eval

.PHONY: pclinux_eval_G1
pclinux_eval_G1: CFLAGS += -DRV_EVALUATION_G1
pclinux_eval_G1: _PRODUCT_G1 = y
pclinux_eval_G1: pclinux_eval

.PHONY: pclinux_eval
pclinux_eval: CC=gcc
pclinux_eval: CFLAGS += $(INCLUDE) $(MD5SUM) $(M32) -DRV_EVALUATION -D_FILE_OFFSET_BITS=64 -DEXPIRY_DATE=1$(expiry)
pclinux_eval: TESTDEC=rvx170dec_pclinux_eval
pclinux_eval: TARGET_ENV=pclinux_eval
pclinux_eval: LIBS = $(DECLIBDIR) -ldecx170rv -ldwlx170 -l8170hw -ltbcommon -lrvparser $(EFENCE)
pclinux_eval: _DWL_PCLINUX = y
pclinux_eval: DEBFLAGS = -O3 -DNDEBUG
pclinux_eval: test

export _DWL_PCLINUX
export SYSTEM_VERIFICATION_TEST
export ASIC_TRACE_SUPPORT
export _PRODUCT_9170
export _PRODUCT_9190
export _PRODUCT_G1

test: lib  $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TESTDEC)

lib:
	make -C ../../linux/rv $(TARGET_ENV)
	make -C ../../linux/dwl $(TARGET_ENV)
	make -C ../common/swhw $(TARGET_ENV)
	make -C rm_parser $(TARGET_ENV)

clean:
	rm -f *.o
	rm -f .depend
	rm -f $(ALLTESTDECS)

libclean:
	make -C ../../linux/dwl clean
	make -C ../../linux/rv clean
	make -C ../common/swhw clean
	make -C rm_parser clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
