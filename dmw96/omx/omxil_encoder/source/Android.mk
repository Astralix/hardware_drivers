LOCAL_PATH := $(call my-dir)

#
# Build video decoder
#

include $(CLEAR_VARS)

OMX_CORE = $(LOCAL_PATH)/../../omx_core/
BELLAGIO_INCLUDE = $(OMX_CORE)/include

ENCODER_NAME = 7280
DECODER_NAME = g1
ENCODER_API_VERSION = 7280

ENCODER_RELEASE = $(LOCAL_PATH)/../../../$(ENCODER_NAME)_encoder
DECODER_RELEASE = $(LOCAL_PATH)/../../../$(DECODER_NAME)_decoder

WRITE_OUTPUT_BUFFERS = false
#WRITE_OUTPUT_BUFFERS = true

EXTRA_FLAGS =

ifeq ($(WRITE_OUTPUT_BUFFERS),true)
	EXTRA_FLAGS += -DDUMP_OUTPUT_BUFFER \
		       -DOUTPUT_BUFFER_PREFIX=\"/tmp/output-\"
endif



LOCAL_CFLAGS := -DOMX_ENCODER_VIDEO_DOMAIN \
                -DENC$(ENCODER_API_VERSION) \
		-DMEMALLOCHW \
                -DANDROID_MOD \
		$(EXTRA_FLAGS)
#		-DANDROID_TRACE
#		-DENCODER_TRACE
#		-DALWAYS_CODE_INTRA_FRAMES

# uncomment below to use temporary output buffer
#LOCAL_CFLAGS += -DUSE_TEMP_OUTPUT_BUFFER

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(LOCAL_PATH)/../headers \
		    $(ENCODER_RELEASE)/inc \
		    $(DECODER_RELEASE)/linux/memalloc \
                    $(BELLAGIO_INCLUDE)


LOCAL_SHARED_LIBRARIES :=       \
        libbinder               \
        libutils                \
        libcutils               \
        libui                   \
        libdl			\
	libc			

# source files:
base_SRCS = msgque.c OSAL.c basecomp.c port.c util.c
libhantrovideoenc_SRCS = $(base_SRCS) \
                         7280_encoder/encoder_constructor_video.c \
                         7280_encoder/encoder_h264.c \
			 7280_encoder/encoder_mpeg4.c \
			 7280_encoder/encoder_h263.c \
                         7280_encoder/encoder.c \
			 7280_encoder/library_entry_point.c

LOCAL_SRC_FILES := $(libhantrovideoenc_SRCS)

# static libraries:
LOCAL_STATIC_LIBRARIES := libx280mpeg4 \
			  libx280h264 \
			  libx280ewl \
			  libx280common

LOCAL_MODULE := libhantrovideoenc
LOCAL_PRELINK_MODULE:=false
LOCAL_MODULE_TAGS := eng

include $(BUILD_SHARED_LIBRARY)

#
# Build image decoder
#

include $(CLEAR_VARS)

OMX_CORE = $(LOCAL_PATH)/../../omx_core/
BELLAGIO_INCLUDE = $(OMX_CORE)/include

ENCODER_NAME = 7280
DECODER_NAME = g1
ENCODER_API_VERSION = 7280

ENCODER_RELEASE = $(LOCAL_PATH)/../../../$(ENCODER_NAME)_encoder
DECODER_RELEASE = $(LOCAL_PATH)/../../../$(DECODER_NAME)_decoder

LOCAL_CFLAGS := -DOMX_ENCODER_IMAGE_DOMAIN \
                -DENC$(ENCODER_API_VERSION) \
                -DANDROID_MOD 

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
                    $(LOCAL_PATH)/../headers \
		    $(ENCODER_RELEASE)/inc \
                    $(BELLAGIO_INCLUDE)


LOCAL_SHARED_LIBRARIES :=       \
        libbinder               \
        libutils                \
        libcutils               \
        libui                   \
        libdl			\
	libc			

# source files:
base_SRCS = msgque.c OSAL.c basecomp.c port.c util.c
libhantrovideoenc_SRCS = $(base_SRCS) \
                         7280_encoder/encoder_constructor_image.c \
                         7280_encoder/encoder_jpeg.c \
                         7280_encoder/encoder.c \
			 7280_encoder/library_entry_point.c

LOCAL_SRC_FILES := $(libhantrovideoenc_SRCS)

# static libraries:
LOCAL_STATIC_LIBRARIES := libx280jpeg \
			  libx280ewl \
			  libx280common

LOCAL_MODULE := libhantroimageenc
LOCAL_PRELINK_MODULE:=false
LOCAL_MODULE_TAGS := eng

include $(BUILD_SHARED_LIBRARY)



