LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/../../source

LOCKING := ioctl
#LOCKING := normal


LOCAL_SRC_FILES := dwl_linux.c \
	           dwl_x170_linux_irq.c \

LOCAL_C_INCLUDES := . \
		    $(common_src_dir)/inc \
		    $(LOCAL_PATH)/../memalloc \
		    $(LOCAL_PATH)/../ldriver/kernel_26x \
		    $(common_src_dir)/common

LOCAL_CFLAGS := -DDEC_MODULE_PATH=\"/dev/hx170dec\" \
                -DMEMALLOC_MODULE_PATH=\"/dev/memalloc\" \
		-DANDROID_MOD

#LOCAL_CFLAGS += -D_DWL_DEBUG
#LOCAL_CFLAGS += -D_DWL_HW_PERFORMANCE 
LOCAL_CFLAGS += -DSUPPORT_ZERO_COPY

ifeq ($(LOCKING), ioctl)
  LOCAL_CFLAGS += -DUSE_LINUX_LOCK_IOCTL
  LOCAL_SRC_FILES += dwl_linux_lock_ioctl.c
else
  LOCAL_SRC_FILES += dwl_linux_lock.c
endif

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libdwlx170

include $(BUILD_STATIC_LIBRARY)

