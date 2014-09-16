# for SW model
CC=gcc
# or covc for coverage build
#CC=covc --abs --retain gcc
CFLAGS =-O0 -finline-functions -Wno-inline -Wall -fPIC -Wunused-variable -DDEBUG -g
LDFLAGS=-Wl #,--strip-all

INSTALL_DIR=../../../libomxil-bellagio-0.9.3/build/lib/omxilcomponents

# for HW
#CC=arm-none-linux-gnueabi-gcc
#CFLAGS =-mtune=arm926ej-s -fpic -DMEMALLOCHW
#CFLAGS =-O3 -mtune=arm926ej-s -fpic -DMEMALLOCHW -DNDEBUG
#DECODER_API_VERSION = 7190
#INSTALL_DIR=/misc/export/work/sapi/omx
#MEMALLOC_RELEASE = ../../../7190_decoder/software/linux/memalloc

#CFLAGS += -DOMX_DECODER_TRACE
#CFLAGS += -DOMX_DECODER_TUNNELING_SUPPORT

# If the machine is 64-bit and we are compiling for PC, force 32-bit compilation
ifeq ($(shell uname -m),x86_64)
        ifneq (,$(findstring arm,$(MAKECMDGOALS)))
                export M32 = -m32
        endif
endif

M32=-m32

CFLAGS +=-pthread
CFLAGS += $(M32)
CFLAGS += -I.. \
    -I../../../headers \
    -I../../../libomxil-bellagio-0.9.3/src \
    -I../../../../../opencore_x86/build_config/opencore_dynamic \
    -I../../../../../opencore_x86/build_config/opencore_dynamic/build/installed_include \
    -I../../../../../opencore_x86/oscl/oscl/config/linux \
    -I../../../../../opencore_x86/oscl/oscl/config/shared \
    -I../config_parser/inc
#    -I../../../opencore_x86/oscl/oscl/osclbase/src
LDFLAGS +=-pthread $(M32) -L./ -L../

all: libhantro_omx_core.so

install: libhantro_omx_core.so
	cp -vf libhantro_omx_core.so $(INSTALL_DIR)/

clean:
	rm -f $(libhantro_omx_core_OBJS) $(libVendor_hantro_omx_OBJS) \
    libhantro_omx_core.so libVendor_hantro_omx.so $(INSTALL_DIR)/libhantro_omx_core.so


config_parser_SRCS = ../config_parser/src/hantro_m4v_config_parser.cpp \
                     ../config_parser/src/hantro_omx_config_parser.cpp \
                     ../config_parser/src/hantro_video_config_parser.cpp

libhantro_omx_core_OBJS = $(config_parser_SRCS:.cpp=.o) hantro_omx_core.o

libVendor_hantro_omx_OBJS = hantro_omx_interface.o

libhantro_omx_core.so: $(libhantro_omx_core_OBJS)
	$(CC) $(LDFLAGS) -o libhantro_omx_core.so -lhantrovideodec -Wl,-h -Wl,libhantro_omx_core.so -shared -Wl,--start-group $(libhantro_omx_core_OBJS) -lrt -Wl,--end-grou

libVendor_hantro_omx.so: $(libVendor_hantro_omx_OBJS)
	$(CC) $(LDFLAGS) -o libVendor_hantro_omx.so -lhantro_omx_core -Wl,-h -Wl,libVendor_hantro_omx.so -shared -Wl,--start-group $(libVendor_hantro_omx_OBJS) -lrt -Wl,--end-group

hantro_omx_core.o : hantro_omx_core.c
	$(CC) $(CFLAGS) -DSTATIC_TABLE -DTRACE -c $< -o $@

#pv_omx_interface.o : pv_omx_interface.cpp
#	$(CC) $(CFLAGS) -c $< -o $@

hantro_omx_interface.o : hantro_omx_interface.cpp
	$(CC) $(CFLAGS) -c $< -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o : %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

