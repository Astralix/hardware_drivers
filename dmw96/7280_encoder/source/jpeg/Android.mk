LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/../..

LOCAL_SRC_FILES := EncJpeg.c \
                   EncJpegCodeFrame.c \
		   EncJpegInit.c \
		   EncJpegPutBits.c \
		   JpegEncApi.c


LOCAL_C_INCLUDES := . \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/source/common \
		    $(common_src_dir)/linux/debug_trace

LOCAL_CFLAGS := -DANDROID_MOD

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libx280jpeg

include $(BUILD_STATIC_LIBRARY)

