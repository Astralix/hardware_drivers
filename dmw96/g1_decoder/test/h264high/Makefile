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
EFENCE_PATH=/home/atna/local/electric-fence-2.1.13/lib

# set this to 'y' for md5 checksum calculation of the frames instead of yuv
USE_MD5SUM = n

SRCS = dectestbench.c

OBJS    = $(SRCS:.c=.o)

INCLUDE = -I../../source/h264high -I../../source/inc -I../../source/config \
          -I../../source/h264high/legacy -I../../source/dwl -I../common/swhw \
          -I../../source/common -I../../linux/h264high

DECLIBDIR = -L../../linux/h264high \
            -L../../linux/dwl \
            -L../common \
            -L../common/swhw \
            -L../../../system/models/golden
            
DECLIB = libdecx170h.a
DWLLIB = libdwlx170.a
TBLIB = libtbcommon.a
HWSIMLIB = lib8170hw.a

TESTDEC=hx170dec
TARGET_ENV=integrator

# Enable SW performance  measurement
#CFLAGS = -DSW_PERFORMANCE

# Electric Fence
ifeq ($(USE_EFENCE), y)
        EFENCE= -L$(EFENCE_PATH) -lefence -lpthread
endif

ifeq ($(USE_MD5SUM), y)
        MD5SUM = -DMD5SUM
endif

ifeq ($(shell uname -m),x86_64)
	ifneq (,$(findstring pclinux,$(MAKECMDGOALS)))
		M32 = -m32
	endif
	ifneq (,$(findstring coverage,$(MAKECMDGOALS)))
		M32 = -m32
	endif
	ifneq (,$(findstring verification,$(MAKECMDGOALS)))
		M32 = -m32
	endif
endif

ifeq ($(ENABLE_2ND_CHROMA_FLAG), y)
	ENABLE_2ND_CHROMA=-D_ENABLE_2ND_CHROMA
endif

ifeq ($(DISABLE_PIC_FREEZE_FLAG), y)
	DISABLE_PIC_FREEZE=-D_DISABLE_PIC_FREEZE
endif
	
export M32
export _DWL_PCLINUX
export ASIC_TRACE_SUPPORT
export SET_EMPTY_PICTURE_DATA
export ENABLE_2ND_CHROMA
export DISABLE_PIC_FREEZE

ARMCPU=ARM1136J-S

all: versatile

.PHONY: versatile
versatile: CC=arm-none-linux-gnueabi-gcc
versatile: CFLAGS += -O -g -mtune=arm926ej-s -fpic $(INCLUDE) $(MD5SUM) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE
versatile: TESTDEC=hx170dec_versatile
versatile: TARGET_ENV=versatile
versatile: LIBS = $(DECLIBDIR) -lpthread -ldecx170h -ldwlx170  -ltbcommon $(EFENCE)
versatile: test

.PHONY: integrator
integrator: CC=arm-linux-gcc
integrator: CFLAGS += -O -g -mtune=arm920tdmi -fpic $(INCLUDE) $(MD5SUM)
integrator: TESTDEC=hx170dec_integrator
integrator: TARGET_ENV=integrator
integrator: LIBS = $(DECLIBDIR) -lpthread -ldecx170h -ldwlx170 -ltbcommon $(EFENCE)
integrator: test

.PHONY: ads
ads: CC=armcc
ads: CFLAGS = --cpu $(ARMCPU) -g -O2 -Otime --no_inline $(INCLUDE) -DADS_PERFORMANCE_SIMULATION
ads: TESTDEC=hx170dec_ads
ads: TARGET_ENV=ads
ads: LIBS = ../../linux/h264high/$(DECLIB) \
            ../../linux/dwl/$(DWLLIB) \
            ../common/swhw/$(TBLIB) \
           ../../../system/models/golden/$(HWSIMLIB)
ads: ASIC_TRACE_SUPPORT=
ads: lib $(OBJS)
	armlink --noremove $(OBJS) $(LIBS) -o $(TESTDEC).axf

ads9: override ARMCPU=ARM926EJ-S
ads9: ads

ads11: override ARMCPU=ARM1136J-S
ads11: ads

.PHONY: pclinux
pclinux: CC=gcc
pclinux: CFLAGS = $(M32) -O -g $(INCLUDE) $(MD5SUM) -DASIC_TRACE_SUPPORT -D_FILE_OFFSET_BITS=64 -DEXPIRY_DATE=1$(expiry) -D_LARGEFILE64_SOURCE
pclinux: TESTDEC=hx170dec_pclinux
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(DECLIBDIR) -ldecx170h -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux: _DWL_PCLINUX = y
pclinux: ASIC_TRACE_SUPPORT = -DASIC_TRACE_SUPPORT
pclinux: test

.PHONY: coverage
coverage: CC=covc --abs --retain gcc
coverage: CFLAGS = -O -g $(INCLUDE) $(MD5SUM) $(M32)
coverage: TESTDEC=hx170dec_coverage
coverage: TARGET_ENV=coverage
coverage: LIBS = $(DECLIBDIR) -ldecx170h -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
coverage: _DWL_PCLINUX = y
coverage: test

.PHONY: pclinux_eval_8170
pclinux_eval_8170: CFLAGS += -DH264_EVALUATION_8170
pclinux_eval_8170: pclinux_eval

.PHONY: pclinux_eval_8190
pclinux_eval_8190: CFLAGS += -DH264_EVALUATION_8190
pclinux_eval_8190: pclinux_eval

.PHONY: pclinux_eval_9170
pclinux_eval_9170: CFLAGS += -DH264_EVALUATION_9170
pclinux_eval_9170: pclinux_eval

.PHONY: pclinux_eval_9190
pclinux_eval_9190: CFLAGS += -DH264_EVALUATION_9190
pclinux_eval_9190: pclinux_eval

.PHONY: pclinux_eval_G1
pclinux_eval_G1: CFLAGS += -DH264_EVALUATION_G1
pclinux_eval_G1: pclinux_eval

.PHONY: pclinux_eval
pclinux_eval: CC=gcc
pclinux_eval: CFLAGS += $(M32) -O $(INCLUDE) $(MD5SUM) -DH264_EVALUATION -D_FILE_OFFSET_BITS=64 -DEXPIRY_DATE=1$(expiry) -D_LARGEFILE64_SOURCE
pclinux_eval: TESTDEC=hx170dec_pclinux_eval
pclinux_eval: TARGET_ENV=pclinux_eval
pclinux_eval: LIBS = $(DECLIBDIR) -ldecx170h -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux_eval: _DWL_PCLINUX = y
pclinux_eval: DEBFLAGS = -O3 -DNDEBUG
pclinux_eval: test

test: CFLAGS += $(ENABLE_2ND_CHROMA)
test: lib $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TESTDEC)
        
lib:
	make -C ../../linux/h264high $(TARGET_ENV) ARMCPU=$(ARMCPU)
	make -C ../../linux/dwl $(TARGET_ENV) ARMCPU=$(ARMCPU)
	make -C ../common/swhw $(TARGET_ENV) ARMCPU=$(ARMCPU)       

clean:
	rm -f *.o
	rm -f .depend
	rm -f hx170dec_*

libclean:
	make -C ../../linux/h264high clean
	make -C ../../linux/dwl clean
	make -C ../common/swhw clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
