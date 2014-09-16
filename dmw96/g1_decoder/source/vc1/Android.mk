LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/..
common_linux_dir := $(LOCAL_PATH)/../../linux/

LOCAL_SRC_FILES :=	vc1decapi.c \
			vc1hwd_asic.c \
			vc1hwd_bitplane.c \
			vc1hwd_decoder.c \
			vc1hwd_headers.c \
			vc1hwd_picture_layer.c \
			vc1hwd_stream.c \
			vc1hwd_vlc.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/config \
		    $(common_src_dir)/common \
		    $(common_linux_dir)/dwl

LOCAL_CFLAGS := -DDEC_X170_OUTPUT_FORMAT=DEC_X170_OUTPUT_FORMAT_RASTER_SCAN \
		-DANDROID_MOD

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libx170vc1

include $(BUILD_STATIC_LIBRARY)

