LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/..
common_linux_dir := $(LOCAL_PATH)/../../linux/

LOCAL_SRC_FILES := avsdecapi.c \
		   avsdecapi_internal.c \
		   avs_headers.c \
		   avs_strm.c \
		   avs_utils.c \
		   avs_vlc.c

LOCAL_C_INCLUDES := . \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/config \
		    $(common_src_dir)/common \
		    $(common_linux_dir)/dwl

LOCAL_CFLAGS := -DDEC_X170_OUTPUT_FORMAT=DEC_X170_OUTPUT_FORMAT_RASTER_SCAN \
		-DANDROID_MOD

#LOCAL_SHARED_LIBRARIES := $(common_shared_libraries)

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libdecx170avs

include $(BUILD_STATIC_LIBRARY)

