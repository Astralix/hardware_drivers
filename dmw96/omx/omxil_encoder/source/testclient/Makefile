# If the machine is 64-bit and we are compiling for PC, force 32-bit compilation
ifeq ($(shell uname -m),x86_64)
#	ifneq (,$(findstring arm,$(MAKECMDGOALS)))
		export M32 = -m32
#	endif
endif

CC=gcc
INSTALL_DIR=../../libomxil-B-0.3.2/build/bin
MEMALLOC_RELEASE = ../../../8170_decoder/software/linux/memalloc
CORE=../../libomxil-B-0.3.2/build/lib/libomxil.a

#Use this for HW
#CC=arm-none-linux-gnueabi-gcc
#INSTALL_DIR=/tmp
#CFLAGS +=-mtune=arm926ej-s -fpic -DMEMALLOCHW
#MEMALLOC_RELEASE = ../../../7190_decoder/software/linux/memalloc
#CORE=../../libomxil-B-0.3.2/src/.libs/libomxil.a

CFLAGS += -O0 -fno-inline -Wall -g -pthread -Wall $(M32)
CFLAGS += -I. \
	-I.. \
	-I../../headers \
	-I$(MEMALLOC_RELEASE)

base_SRCS = ../msgque.c ../OSAL.c ../basecomp.c ../port.c ../util.c

omxdec_HDRS = omxtestcommon.h
omxdec_SRCS = omxdectest.c omxtestcommon.c
omxdec_OBJS = $(base_SRCS:.c=.o) $(omxdec_SRCS:.c=.o)
omxdec_LIBS = $(CORE)

omxenc_HDRS = omxencparameters.h omxtestcommon.h
omxenc_SRCS = omxencparameters.c omxenctest.c omxtestcommon.c
omxenc_OBJS = $(base_SRCS:.c=.o) $(omxenc_SRCS:.c=.o)
omxenc_LIBS = $(CORE)

all: omxdectest omxenctest

clean:
	rm -f $(omxdec_OBJS) $(omxenc_OBJS) omxenctest omxdectest \
	rm -f $(INSTALL_DIR)/omxenctest


install: omxdectest omxenctest
	cp -vf omxenctest $(INSTALL_DIR)
	cp -vf omxdectest $(INSTALL_DIR)

omxdectest: $(omxdec_OBJS) $(omxdec_LIBS)
	$(CC) -o omxdectest $(omxdec_OBJS) $(omxdec_LIBS) -ldl -lpthread $(M32)

omxenctest: $(omxenc_OBJS) $(omxenc_LIBS)
	$(CC) -o omxenctest $(omxenc_OBJS) $(omxenc_LIBS) -ldl -lpthread $(M32)

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@


