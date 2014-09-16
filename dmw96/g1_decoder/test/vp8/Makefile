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

# set this to 'y' to enable WebM file format support
ENABLE_WEBM = n

SRCS = dectestbench.c ivf.c
#SRCS = decpp.c
#SRCS += yuvtorgb16.c yuvtorgbinit.c
OBJS = $(SRCS:.c=.o)

INCLUDE = -I../../source/inc -I../../source/vp8 -I../../source/config \
          -I../../source/dwl -I../../source/common -I../common/swhw

DECLIBDIR = -L../../linux/vp8 -L../../linux/dwl -L../common/swhw \
	    -L../../../system/models/golden
DECLIB = libdecx170vp8.a
DWLLIB = libdwlx170.a
TBLIB = libtbcommon.a

TESTDEC=vp8x170dec
TARGET_ENV=integrator

# Enable SW performance  measurement
#CFLAGS = -DSW_PERFORMANCE

ifeq ($(USE_EFENCE), y)
        EFENCE= -DUSE_EFENCE -lefence -lpthread
endif

ifeq ($(USE_MD5SUM), y)
        MD5SUM = -DMD5SUM
endif

ifeq ($(ENABLE_WEBM), y)
	WEBM = -DWEBM_ENABLED
	SRCS += nestegg/src/nestegg.c nestegg/halloc/src/halloc.c
 	INCLUDE+=-Inestegg/include -Inestegg/halloc
	DIRCHECK = dircheck
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
versatile: CFLAGS += -O -g -mtune=arm926ej-s -fpic $(INCLUDE) $(MD5SUM) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE $(WEBM)
versatile: TESTDEC=vp8x170dec_versatile
versatile: TARGET_ENV=versatile
versatile: LIBS = $(DECLIBDIR) -lpthread -ldecx170vp8 -ldwlx170 -ltbcommon $(EFENCE) 
versatile: test

.PHONY: integrator
integrator: CC=arm-linux-gcc
integrator: CFLAGS += -O -g -mtune=arm920tdmi -fpic $(INCLUDE) $(MD5SUM) $(WEBM)
integrator: TESTDEC=vp8x170dec_integrator
integrator: TARGET_ENV=integrator
integrator: LIBS = $(DECLIBDIR) -lpthread -ldecx170vp8 -ldwlx170 -ltbcommon $(EFENCE)
integrator: test

.PHONY: ads11
ads11: CC=armcc
ads11: CFLAGS = --no_inline --cpu ARM1136J-S -O2 -Otime $(INCLUDE) -DADS_PERFORMANCE_SIMULATION $(WEBM)
ads11: TESTDEC=vp8x170dec_ads11
ads11: TARGET_ENV=ads11
ads11: LIBS = -L../../linux/vp8/$(DECLIB) -L../../linux/dwl/$(DWLLIB) -L../common/swhw/$(TBLIB)
ads11: test

.PHONY: ads9
ads9: CC=armcc
ads9: CFLAGS = --no_inline --cpu ARM926EJ-S -O2 -Otime $(INCLUDE) -DADS_PERFORMANCE_SIMULATION $(WEBM)
ads9: TESTDEC=vp8x170dec_ads9
ads9: TARGET_ENV=ads9
ads9: LIBS = -L../../linux/vp8/$(DECLIB) -L../../linux/dwl/$(DWLLIB) -L../common/swhw/$(TBLIB)
ads9: test

.PHONY: ads
ads: CC=armcc
ads: CFLAGS = --no_inline --cpu ARM1136J-S -O3 -Otime $(INCLUDE) -DADS_PERFORMANCE_SIMULATION $(WEBM)
ads: TESTDEC=vp8x170dec_ads
ads: TARGET_ENV=ads
ads: LIBS = -L../../linux/vp8/$(DECLIB) -L../../linux/dwl/$(DWLLIB) -L../common/swhw/$(TBLIB)
ads: test

.PHONY: pclinux
pclinux: CC=gcc
pclinux: CFLAGS = -g $(INCLUDE) $(MD5SUM) $(M32) -DASIC_TRACE_SUPPORT -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DEXPIRY_DATE=1$(expiry) $(WEBM)
pclinux: TESTDEC=vp8x170dec_pclinux
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(DECLIBDIR) -ldecx170vp8 -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux: _DWL_PCLINUX = y
pclinux: ASIC_TRACE_SUPPORT = -DASIC_TRACE_SUPPORT
pclinux: test

.PHONY: coverage
coverage: CC=covc --abs --retain gcc
coverage: CFLAGS = -O -g $(INCLUDE) $(MD5SUM) $(M32) -DASIC_TRACE_SUPPORT $(WEBM)
coverage: TESTDEC=vp8x170dec_coverage
coverage: TARGET_ENV=coverage
coverage: LIBS = $(DECLIBDIR) -ldecx170vp8 -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
coverage: _DWL_PCLINUX = y
coverage: test

.PHONY: pclinux_eval_G1
pclinux_eval_G1: CFLAGS += -DVP8_EVALUATION_G1 $(WEBM)
pclinux_eval_G1: pclinux_eval

.PHONY: pclinux_eval
pclinux_eval: CC=gcc
pclinux_eval: CFLAGS += $(INCLUDE) $(MD5SUM) $(M32) -DVP8_EVALUATION -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DEXPIRY_DATE=1$(expiry) $(WEBM)
pclinux_eval: TESTDEC=vp8x170dec_pclinux_eval
pclinux_eval: TARGET_ENV=pclinux_eval
pclinux_eval: LIBS = $(DECLIBDIR) -ldecx170vp8 -ldwlx170 -l8170hw -ltbcommon $(EFENCE)
pclinux_eval: _DWL_PCLINUX = y
pclinux_eval: DEBFLAGS = -O3 -DNDEBUG
pclinux_eval: test

export _DWL_PCLINUX
export ASIC_TRACE_SUPPORT

test: $(DIRCHECK) lib $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TESTDEC)

.PHONY: dircheck
dircheck:
	@ if ! test -d ./nestegg; then\
		echo "";\
		echo "In order to enable WebM support, nestegg library must be installed:";\
		echo "";\
		echo "git clone https://github.com/kinetiknz/nestegg.git";\
		echo "cd nestegg";\
		echo "autoreconf --install";\
		echo "./configure";\
		echo "cd ..";\
		echo "";\
		echo "Note: nestegg configuration uses autoconf and libtool. These can be downloaded from:";\
		echo "";\
		echo "  - http://www.gnu.org/s/autoconf/";\
		echo "  - http://www.gnu.org/software/libtool/";\
		echo "";\
		exit 1;\
	fi

lib:
	make -C ../../linux/vp8 $(TARGET_ENV)
	make -C ../../linux/dwl $(TARGET_ENV)
	make -C ../common/swhw $(TARGET_ENV)

clean:
	rm -f *.o nestegg/src/nestegg.o nestegg/halloc/src/halloc.o
	rm -f .depend
	rm -f vp8x170dec_*

libclean:
	make -C ../../linux/vp8 clean
	make -C ../../linux/dwl clean
	make -C ../common/swhw clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
