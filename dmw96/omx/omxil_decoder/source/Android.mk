LOCAL_PATH := $(call my-dir)

#
# Build video decoder
#

include $(CLEAR_VARS)

OMX_CORE = $(LOCAL_PATH)/../../omx_core/
BELLAGIO_INCLUDE = $(OMX_CORE)/include

DECODER_NAME = g1
DECODER_API_VERSION = 8170

CODECS += -DENABLE_CODEC_VP6
CODECS += -DENABLE_CODEC_MPEG4
CODECS += -DENABLE_CODEC_MPEG2
CODECS += -DENABLE_CODEC_VC1
CODECS += -DENABLE_CODEC_RV
CODECS += -DENABLE_CODEC_H264
CODECS += -DENABLE_CODEC_H263
CODECS += -DENABLE_PP
CODECS += -DENABLE_CODEC_AVS
CODECS += -DENABLE_CODEC_VP8

DECODER_RELEASE = $(LOCAL_PATH)/../../../$(DECODER_NAME)_decoder

LOCAL_CFLAGS := -DIS_G1_DECODER \
		 $(CODECS) \
		 -DOMX_DECODER_VIDEO_DOMAIN \
		 -DANDROID_MOD

# Skip scanframe for mpeg2 and vc1
LOCAL_CFLAGS += -DSKIP_SCANFRAME_MPEG2 \
                -DSKIP_SCANFRAME_VC1

# This flag will enable DSPG timestamp fix
LOCAL_CFLAGS += -DDSPG_TIMESTAMP_FIX

# Decrease video dimensions to screen size (fb0 width and height)
LOCAL_CFLAGS += -DALIGN_AND_DECREASE_OUTPUT

# Enable this for debugging decoder omxil (deprecated! to enable logs: uncomment LOG_NDEBUG in the specific file head)
#LOCAL_CFLAGS += -DOMX_DECODER_TRACE

# Enable this for dynamic scaling support
#LOCAL_CFLAGS += -DDYNAMIC_SCALING

# Dump input stream into /tmp/input.stream
#LOCAL_CFLAGS += -DDSPG_DUMP_STREAM

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ \
    			 	$(LOCAL_PATH)/../headers \
    			 	$(DECODER_RELEASE)/source/inc \
    			 	$(DECODER_RELEASE)/source/h264high \
    			 	$(DECODER_RELEASE)/source/common \
				$(DECODER_RELEASE)/linux/dwl \
				$(DECODER_RELEASE)/linux/memalloc \
    			 	$(DECODER_RELEASE)/test/common \
    			 	$(DECODER_RELEASE)/test/common/swhw \
				$(BELLAGIO_INCLUDE)/

LOCAL_SHARED_LIBRARIES :=       \
        libbinder               \
        libutils                \
        libcutils               \
        libui                   \
        libdl			\
	libc

ifneq (,$(findstring -DENABLE_CODEC_H264, $(CODECS)))
VIDEO_SRCS += 81x0_decoder/codec_h264.c
endif

ifneq (,$(findstring -DENABLE_CODEC_MPEG4, $(CODECS)))
VIDEO_SRCS += 81x0_decoder/codec_mpeg4.c
endif

ifneq (,$(findstring -DENABLE_CODEC_MPEG2, $(CODECS)))
VIDEO_SRCS += 81x0_decoder/codec_mpeg2.c
endif

ifneq (,$(findstring -DENABLE_CODEC_VC1, $(CODECS)))
VIDEO_SRCS += 81x0_decoder/codec_vc1.c
endif

ifneq (,$(findstring -DENABLE_CODEC_VP6, $(CODECS)))
VIDEO_SRCS += 81x0_decoder/codec_vp6.c
endif

ifneq (,$(findstring -DENABLE_CODEC_VP8, $(CODECS)))
VIDEO_SRCS += 81x0_decoder/codec_vp8.c
endif

ifneq (,$(findstring -DENABLE_CODEC_AVS, $(CODECS)))
VIDEO_SRCS += 81x0_decoder/codec_avs.c
endif

ifneq (,$(findstring -DENABLE_CODEC_RV, $(CODECS)))
VIDEO_SRCS += 81x0_decoder/codec_rv.c
endif

# source files:
base_SRCS = msgque.c OSAL.c basecomp.c port.c util.c
libhantrovideodec_SRCS = $(VIDEO_SRCS) \
			$(base_SRCS) \
			81x0_decoder/codec_pp.c \
			81x0_decoder/post_processor.c \
			81x0_decoder/library_entry_point.c \
			81x0_decoder/decoder.c

LOCAL_SRC_FILES := $(libhantrovideodec_SRCS)

# static libraries:
HANTRO_LIBS =	libx170mpeg2 \
		libx170mpeg4 \
		libx170pp \
		libx170rv \
		libx170vc1 \
		libx170vp6 \
		libx170vp8 \
		libdecx170h264 \
		libdecx170avs \
		libx170jpeg \
		libdwlx170 \
		libtbcommon \
		libg1common

LOCAL_STATIC_LIBRARIES := $(HANTRO_LIBS)

LOCAL_MODULE := libhantrovideodec
LOCAL_PRELINK_MODULE:=false
LOCAL_MODULE_TAGS := eng

include $(BUILD_SHARED_LIBRARY)

#
# Build image decoder
#

include $(CLEAR_VARS)

OMX_CORE = $(LOCAL_PATH)/../../omx_core/
BELLAGIO_INCLUDE = $(OMX_CORE)/include

DECODER_NAME = g1
DECODER_API_VERSION = 8170

CODECS := -DENABLE_CODEC_JPEG
CODECS += -DENABLE_CODEC_WEBP


DECODER_RELEASE = $(LOCAL_PATH)/../../../$(DECODER_NAME)_decoder

LOCAL_CFLAGS := -DIS_G1_DECODER \
		 -DOMX_DECODER_$(DECODER_API_VERSION) \
		 $(CODECS) \
		 -DOMX_DECODER_IMAGE_DOMAIN \
		 -DANDROID_MOD

# Enable this for debugging image decoding
#LOCAL_CFLAGS += -DOMX_DECODER_TRACE

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ \
    			 	$(LOCAL_PATH)/../headers \
    			 	$(DECODER_RELEASE)/source/inc \
    			 	$(DECODER_RELEASE)/source/h264high \
    			 	$(DECODER_RELEASE)/source/common \
				$(DECODER_RELEASE)/linux/dwl \
				$(DECODER_RELEASE)/linux/memalloc \
    			 	$(DECODER_RELEASE)/test/common \
    			 	$(DECODER_RELEASE)/test/common/swhw \
				$(BELLAGIO_INCLUDE)/

LOCAL_SHARED_LIBRARIES :=       \
        libbinder               \
        libutils                \
        libcutils               \
        libui                   \
        libdl			\
	libc

ifneq (,$(findstring -DENABLE_CODEC_JPEG, $(CODECS)))
IMAGE_SRCS = 81x0_decoder/codec_jpeg.c
endif

ifneq (,$(findstring -DENABLE_CODEC_WEBP, $(CODECS)))
IMAGE_SRCS += 81x0_decoder/codec_webp.c
endif


# source files:
base_SRCS = msgque.c OSAL.c basecomp.c port.c util.c
libhantroimagedec_SRCS = $(IMAGE_SRCS) \
			$(base_SRCS) \
			81x0_decoder/codec_pp.c \
			81x0_decoder/post_processor.c \
			81x0_decoder/library_entry_point.c \
			81x0_decoder/decoder.c

LOCAL_SRC_FILES := $(libhantroimagedec_SRCS)

# static libraries:
HANTRO_LIBS =	libx170jpeg \
		libx170vp8 \
		libdwlx170 \
		libtbcommon \
		libg1common

LOCAL_STATIC_LIBRARIES := $(HANTRO_LIBS)

#LOCAL_LDFLAGS := -Wl,--start-group $(HANTRO_LIBS) -Wl,--end-group

LOCAL_MODULE := libhantroimagedec
LOCAL_PRELINK_MODULE:=false
LOCAL_MODULE_TAGS := eng

include $(BUILD_SHARED_LIBRARY)


