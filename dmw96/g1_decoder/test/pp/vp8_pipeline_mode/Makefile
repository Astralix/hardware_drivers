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
vpath %.c ../external_mode
vpath %.c ../../vp8

SRCS = dectestbench.c ivf.c pptestbench.c params.c cfg.c frame.c
OBJS    = $(SRCS:.c=.o)

INCLUDE = -I../../../source/inc \
          -I../../../source/pp \
	  -I../../../source/vp8 \
	  -I../../../source/config \
	  -I../../../source/common \
	  -I../external_mode \
	  -I../../../linux/memalloc \
          -I../../common/swhw \
	  -I../../vp8

LIBDECDIR = -L../../../linux/pp \
            -L../../../linux/dwl \
            -L../../../linux/vp8 \
            -L../../common/swhw \
	    -L../../../../system/models/golden
            
TESTBENCH=pp_vp8_pipe

TARGET_ENV=

ifeq ($(USE_EFENCE), y)
        EFENCE= -DUSE_EFENCE -L/afs/hantro.com/projects/adder/users/atna/efence_2_4_13 \
                -lefence -lpthread
endif

ifeq ($(USE_MD5SUM), y)
        MD5SUM = -DMD5SUM
endif

ifeq ($(ENABLE_WEBM), y)
	WEBM = -DWEBM_ENABLED
	SRCS += nestegg.c halloc.c
	DIRCHECK = dircheck
endif
	
ifeq ($(shell uname -m),x86_64)
	ifneq (,$(findstring pclinux,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
	ifneq (,$(findstring coverage,$(MAKECMDGOALS)))
		export M32 = -m32
	endif
endif

all: versatile

.PHONY: versatile
versatile: CC=arm-none-linux-gnueabi-gcc
versatile: CFLAGS = -O0 -g -mtune=arm926ej-s -fpic -DPP_PIPELINE_ENABLED $(INCLUDE) $(MD5SUM) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE $(WEBM)
versatile: TARGET_ENV=versatile
versatile: DWL_TARGET_ENV=versatile
versatile: LIBS = $(LIBDECDIR) $(EFENCE) -lpthread -ldecx170p -ldwlx170 -ldecx170vp8 -ltbcommon
versatile: test

.PHONY: pci
pci: CFLAGS = -O0 -g -fpic -DPP_PIPELINE_ENABLED $(INCLUDE) $(WEBM)
pci: TARGET_ENV=pci
pci: DWL_TARGET_ENV=pci
pci: LIBS = $(LIBDECDIR) $(EFENCE) -lpthread -lppx170 -ldwlx170 -ldecx170vp8 -ltbcommon
pci: test

.PHONY: pclinux
pclinux: CC=gcc
pclinux: CFLAGS = $(M32) -g -O0 -DPP_PIPELINE_ENABLED $(INCLUDE) -DASIC_TRACE_SUPPORT $(MD5SUM) -DEXPIRY_DATE=1$(expiry) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE $(WEBM) 
pclinux: TARGET_ENV=pclinux
pclinux: LIBS = $(LIBDECDIR) -ldecx170p -ldwlx170 -ldecx170vp8 -l8170hw -ltbcommon
pclinux: _DWL_PCLINUX = y
pclinux: ASIC_TRACE_SUPPORT = -DASIC_TRACE_SUPPORT
pclinux: test

export ASIC_TRACE_SUPPORT
export _DWL_PCLINUX

test: $(DIRCHECK) lib $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TESTBENCH)

.PHONY: dircheck
dircheck:
	@ if ! test -d ../../vp8/nestegg; then\
		echo "WebM support missing";\
		echo "EXITING";\
		exit 1;\
	fi

lib:
	make -C ../../../linux/pp clean
	make -C ../../../linux/pp $(TARGET_ENV) PIPELINE_SUPPORT=-DPP_VP8DEC_PIPELINE_SUPPORT
	make -C ../../../linux/vp8 $(TARGET_ENV)
	make -C ../../../linux/dwl $(TARGET_ENV)
	make -C ../../common/swhw $(TARGET_ENV)

clean:
	rm -f *.o core.*
	rm -f .depend
	rm -f $(TESTBENCH)

libclean:
	make -C ../../../linux/pp clean
	make -C ../../../linux/dwl clean
	make -C ../../../linux/vp8 clean
	make -C ../../common/swhw clean

depend .depend dep: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

ifeq (.depend, $(wildcard .depend))
include .depend
endif
