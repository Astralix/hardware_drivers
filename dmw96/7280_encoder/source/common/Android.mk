LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/../..

LOCAL_SRC_FILES := encasiccontroller.c \
                   encasiccontroller_v2.c \
		   encbuffering.c \
                   encpreprocess.c

LOCAL_C_INCLUDES := . \
		    $(common_src_dir)/inc \

LOCAL_CFLAGS := -DMPEG4_HW_RLC_MODE_ENABLED \
		-DANDROID_MOD
#		-DTRACE_REGS

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libx280common

include $(BUILD_STATIC_LIBRARY)

