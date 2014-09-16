#BELLAGIO_ROOT = ../../../libomxil-bellagio-0.9.2/
BELLAGIO_ROOT =/afs/vsi.fi/user/eu0012/work/libomxil-bellagio-0.9.3/

DECODER_RELEASE = ../../../g1_decoder
#VP6_DECODER_RELEASE = ../../../9190_vp6_libs/$(DECODER_API_VERSION)_decoder

# for SW model
CC=gcc
# or covc for coverage build
#CC=covc --abs --retain gcc
#CFLAGS =-O2 -lm -finline-functions -Wno-inline -Wall -fPIC -Wunused-variable -DDEBUG -g
CFLAGS =-g -Wall -fPIC -Wunused-variable -DDEBUG -D_ASSERT_USED
LDFLAGS=-Wl #,--strip-all
#DECODER_API_VERSION = 8170

INSTALL_DIR=$(BELLAGIO_ROOT)/build/lib/bellagio
MEMALLOC_RELEASE = ../../../g1_decoder/software/linux/memalloc

# for HW
#CC=arm-none-linux-gnueabi-gcc
#CFLAGS =-mtune=arm926ej-s -fpic -DMEMALLOCHW
#CFLAGS =-O3 -mtune=arm926ej-s -fpic -DMEMALLOCHW -DNDEBUG
#DECODER_API_VERSION = 7190
#INSTALL_DIR=/misc/export/work/sapi/omx
#MEMALLOC_RELEASE = ../../../7190_decoder/software/linux/memalloc

# If the machine is 64-bit and we are compiling for PC, force 32-bit compilation
ifeq ($(shell uname -m),x86_64)
        ifneq (,$(findstring arm,$(MAKECMDGOALS)))
                export M32 = -m32
        endif
endif

M32 = -m32

#CFLAGS += -DOMX_DECODER_TRACE
CFLAGS += -DINCLUDE_TB
#CFLAGS += -DOMX_DECODER_TUNNELING_SUPPORT

#CFLAGS += -DIS_8190
CFLAGS += -DIS_G1_DECODER
CFLAGS += -DHANTRO_TESTBENCH
#CFLAGS += -DOMX_MEM_TRC

## Enable only for OMX IL conformance testing ##
#CFLAGS += -DCONFORMANCE

CODECS += -DENABLE_CODEC_VP6
CODECS += -DENABLE_CODEC_H263
CODECS += -DENABLE_CODEC_MPEG4
CODECS += -DENABLE_CODEC_CUSTOM_1
CODECS += -DENABLE_CODEC_MPEG2
CODECS += -DENABLE_CODEC_VC1
CODECS += -DENABLE_CODEC_JPEG
CODECS += -DENABLE_CODEC_RV
CODECS += -DENABLE_CODEC_H264
CODECS += -DENABLE_PP
CODECS += -DENABLE_CODEC_AVS
CODECS += -DENABLE_CODEC_VP8
CODECS += -DENABLE_CODEC_WEBP
CODECS += -DMVC_SUPPORT
#CODECS += -DENABLE_CODEC_MJPEG

CFLAGS +=-pthread
#CFLAGS +=-DOMX_DECODER_$(DECODER_API_VERSION)
CFLAGS += $(M32)
ifneq (,$(findstring -DIS_8190, $(CFLAGS)) || ,$(findstring -DIS_G1_DECODER, $(CFLAGS)))
CFLAGS += -I$(DECODER_RELEASE)/software/source/h264high
endif
CFLAGS += -I.. \
    -I../../headers \
    -I$(BELLAGIO_ROOT)/src \
    -I$(DECODER_RELEASE)/software/source/inc \
    -I$(DECODER_RELEASE)/software/source/common \
    -I$(DECODER_RELEASE)/software/test/common \
    -I$(DECODER_RELEASE)/software/test/common/swhw

CFLAGS += $(CODECS)
LDFLAGS +=-pthread $(M32)

all: libhantrovideodec.so libhantroimagedec.so

install: libhantrovideodec.so libhantroimagedec.so
	cp -vf libhantrovideodec.so $(INSTALL_DIR)/
	cp -vf libhantroimagedec.so $(INSTALL_DIR)/
	$(BELLAGIO_ROOT)/build/bin/omxregister-bellagio $(INSTALL_DIR)/

install_video: libhantrovideodec.so
	cp -vf libhantrovideodec.so $(INSTALL_DIR)/

clean:
	rm -f $(libhantrovideodec_OBJS) libhantrovideodec.so $(INSTALL_DIR)/libhantrovideodec.so
	rm -f $(libhantroimagedec_OBJS) libhantroimagedec.so $(INSTALL_DIR)/libhantroimagedec.so

HANTRO_LIBS_VIDEO = $(DECODER_RELEASE)/software/linux/dwl/libdwlx170.a \
	$(DECODER_RELEASE)/system/models/golden/lib8170hw.a \
	$(DECODER_RELEASE)/software/test/common/swhw/libtbcommon.a \
	$(DECODER_RELEASE)/software/linux/pp/libdecx170p.a
HANTRO_LIBS_IMAGE = $(DECODER_RELEASE)/software/linux/dwl/libdwlx170.a \
	$(DECODER_RELEASE)/system/models/golden/lib8170hw.a \
	$(DECODER_RELEASE)/software/test/common/swhw/libtbcommon.a \
	$(DECODER_RELEASE)/software/linux/pp/libdecx170p.a

ifneq (,$(findstring -DENABLE_CODEC_H264, $(CODECS)))
VIDEO_SRCS += codec_h264.c
ifneq (,$(findstring -DIS_8190, $(CFLAGS)) || ,$(findstring -DIS_G1_DECODER, $(CFLAGS)))
HANTRO_LIBS_VIDEO += $(DECODER_RELEASE)/software/linux/h264high/libdecx170h.a
else
HANTRO_LIBS_VIDEO += $(DECODER_RELEASE)/software/linux/h264/libdecx170h.a
endif
endif

ifneq (,$(findstring -DENABLE_CODEC_MPEG4, $(CODECS)))
VIDEO_SRCS += codec_mpeg4.c
HANTRO_LIBS_VIDEO += $(DECODER_RELEASE)/software/linux/mpeg4/libdecx170m.a
endif

ifneq (,$(findstring -DENABLE_CODEC_MPEG2, $(CODECS)))
VIDEO_SRCS += codec_mpeg2.c
HANTRO_LIBS_VIDEO += $(DECODER_RELEASE)/software/linux/mpeg2/libdecx170m2.a
endif

ifneq (,$(findstring -DENABLE_CODEC_VC1, $(CODECS)))
VIDEO_SRCS += codec_vc1.c
HANTRO_LIBS_VIDEO += $(DECODER_RELEASE)/software/linux/vc1/libdecx170v.a
endif

ifneq (,$(findstring -DENABLE_CODEC_VP6, $(CODECS)))
VIDEO_SRCS += codec_vp6.c
HANTRO_LIBS_VIDEO += $(DECODER_RELEASE)/software/linux/vp6/libdec8190vp6.a
endif

ifneq (,$(findstring -DENABLE_CODEC_MJPEG, $(CODECS)))
VIDEO_SRCS += codec_jpeg.c
HANTRO_LIBS_VIDEO += $(DECODER_RELEASE)/software/linux/jpeg/libx170j.a
endif

ifneq (,$(findstring -DENABLE_CODEC_VP8, $(CODECS)))
VIDEO_SRCS += codec_vp8.c
HANTRO_LIBS_VIDEO += $(DECODER_RELEASE)/software/linux/vp8/libdecx170vp8.a
endif

ifneq (,$(findstring -DENABLE_CODEC_AVS, $(CODECS)))
VIDEO_SRCS += codec_avs.c
HANTRO_LIBS_VIDEO += $(DECODER_RELEASE)/software/linux/avs/libdecx170a.a
endif

ifneq (,$(findstring -DENABLE_CODEC_RV, $(CODECS)))
VIDEO_SRCS += codec_rv.c
HANTRO_LIBS_VIDEO += $(DECODER_RELEASE)/software/linux/rv/libdecx170rv.a
endif

ifneq (,$(findstring -DENABLE_CODEC_JPEG, $(CODECS)))
IMAGE_SRCS = codec_jpeg.c
HANTRO_LIBS_IMAGE += $(DECODER_RELEASE)/software/linux/jpeg/libx170j.a
endif

ifneq (,$(findstring -DENABLE_CODEC_WEBP, $(CODECS)))
IMAGE_SRCS += codec_webp.c
HANTRO_LIBS_IMAGE += $(DECODER_RELEASE)/software/linux/vp8/libdecx170vp8.a
endif

base_SRCS = ../msgque.c ../OSAL.c ../basecomp.c ../port.c ../util.c
libhantrovideodec_SRCS = $(VIDEO_SRCS) codec_pp.c post_processor.c
libhantrovideodec_OBJS = $(base_SRCS:.c=.o) $(libhantrovideodec_SRCS:.c=.o) decoder_video.o library_entry_point_video.o
libhantroimagedec_SRCS = $(IMAGE_SRCS) codec_pp.c post_processor.c
libhantroimagedec_OBJS = $(base_SRCS:.c=.o) $(libhantroimagedec_SRCS:.c=.o) decoder_image.o library_entry_point_image.o


test: test.o
	$(CC) $(CFLAGS) $(LDFLAGS) -Wl,--start-group $(HANTRO_LIBS_IMAGE) -lrt -W1,--end-group -o jpegtest test.c codec_jpeg.c post_processor.c ../OSAL.c
	cp -vf jpegtest $(INSTALL_DIR)/

libhantrovideodec.so: $(libhantrovideodec_OBJS) $(HANTRO_LIBS_VIDEO)
	$(CC) $(LDFLAGS) -o libhantrovideodec.so -Wl,-h -Wl,libhantrovideodec.so -shared -Wl,--start-group $(libhantrovideodec_OBJS) $(HANTRO_LIBS_VIDEO) -lrt -Wl,--end-group

libhantroimagedec.so: $(libhantroimagedec_OBJS) $(HANTRO_LIBS_IMAGE)
	$(CC) $(LDFLAGS) -o libhantroimagedec.so -Wl,-h -Wl,libhantroimagedec.so -shared -Wl,--start-group $(libhantroimagedec_OBJS) $(HANTRO_LIBS_IMAGE) -lrt -Wl,--end-group

decoder_video.o : decoder.c
	$(CC) $(CFLAGS) -DOMX_DECODER_VIDEO_DOMAIN -c $< -o $@

decoder_image.o : decoder.c
	$(CC) $(CFLAGS) -DOMX_DECODER_IMAGE_DOMAIN -c $< -o $@

library_entry_point_video.o : library_entry_point.c
	$(CC) $(CFLAGS) -DOMX_DECODER_VIDEO_DOMAIN -c $< -o $@

library_entry_point_image.o : library_entry_point.c
	$(CC) $(CFLAGS) -DOMX_DECODER_IMAGE_DOMAIN -c $< -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

