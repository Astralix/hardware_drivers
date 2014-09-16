LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/..

LOCAL_SRC_FILES := bqueue.c \
		   refbuffer.c \
		   regdrv.c \
		   tiledref.c \
		   workaround.c

LOCAL_C_INCLUDES := . \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/config

LOCAL_CFLAGS := -DANDROID_MOD

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libg1common

include $(BUILD_STATIC_LIBRARY)

