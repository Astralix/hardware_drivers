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
#--  Abstract : Makefile for x170 RV decoder control SW
#--
#-------------------------------------------------------------------------------

# C -compiler name, can be replaced by another compiler(replace gcc)
CC = gcc

# show the path to compiler, where to find header files and libraries
INCLUDE = -Iinclude -Icommon/pub -Irv_depack -Irm_parser

# compiler switches
CFLAGS  = -g -Wall $(ARCH) -D_GNU_SOURCE $(DEBFLAGS) $(INCLUDE) $(SETTING_FLAGS) $(M32) 

# list of used sourcefiles
SRCS := \
helix_date.c \
memory_utils.c \
pack_utils.c \
rm_error_default.c \
rm_io_default.c \
rm_memory_default.c \
rm_packet.c \
rm_property.c \
rm_stream.c \
stream_hdr_utils.c \
string_utils.c \
rm_parser.c \
rm_parser_internal.c \
rv_decode.c \
rv_depack.c \
rv_depack_internal.c

#source search path
vpath %.c
vpath %.c common rm_parser rv_decode rv_depack

vpath %.o
vpath %.o obj

# object files will be generated from .c sourcefiles
OBJS    = $(SRCS:.c=.o)

# name of the outputfile (library)
DECLIB = librvparser.a

# MACRO for cleaning object -files
RM  = rm -f

# MACRO for creating library that includes all the object files
#AR  = ar rcv
AR  = $(CROSS_COMPILER)ar rcs


OBJDIR := obj

#Here are rules for building codes and generating object library.
all: $(DECLIB)

.PHONY: versatile
versatile: CROSS_COMPILER = arm-none-linux-gnueabi-
versatile: CC=arm-none-linux-gnueabi-gcc
versatile: CFLAGS += -O -g -mtune=arm926ej-s -fpic $(INCLUDE) $(MD5SUM)
versatile: $(DECLIB)

.PHONY: integrator
integrator: CC=arm-linux-gcc
integrator: CFLAGS = -O -g -mtune=arm920tdmi -fpic  $(INCLUDE) $(MD5SUM)
integrator: $(DECLIB)


.PHONY: ads11
ads11: CC=armcc
ads11: CFLAGS = -g --cpu ARM1136J-S --no_inline -O3 -Otime -DRVDEC_EXTERNAL_ALLOC_DISABLE $(INCLUDE)
ads11: $(DECLIB)

.PHONY: ads9
ads9: CC=armcc
ads9: CFLAGS = -g --cpu ARM926EJ-S --no_inline -O3 -Otime -DRVDEC_EXTERNAL_ALLOC_DISABLE $(INCLUDE)
ads9: $(DECLIB)

.PHONY: verification
verification: $(DECLIB)

.PHONY: coverage
coverage: $(DECLIB)

.PHONY: pclinux
pclinux: $(DECLIB)

.PHONY: pclinux_eval
pclinux_eval: $(DECLIB)

$(OBJDIR):
	mkdir $@

$(DECLIB): depend $(OBJDIR) $(OBJS)
	$(AR) $(DECLIB) $(patsubst %,$(OBJDIR)/%, $(OBJS))

$(OBJS): %.o: %.c
	$(CC) -c $(CFLAGS) $(ENVSET) $< -o $(OBJDIR)/$@


.PHONY: clean
clean:
	$(RM) $(DECLIB)
	$(RM) .depend
	$(RM) -r $(OBJDIR)

depend: $(SRCS)
	$(CC) $(CFLAGS) -M  $^ > .depend

#ifeq (.depend, $(wildcard .depend))
#include .depend
#endif

ifeq (,$(findstring clean, $(MAKECMDGOALS)))
ifeq (.depend, $(wildcard .depend))
include .depend
endif
endif
