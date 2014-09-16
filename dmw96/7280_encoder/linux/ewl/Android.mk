LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/../../source

LOCKING := ioctl
#LOCKING := normal


LOCAL_SRC_FILES := ewl_x280_common.c \
	           ewl_x280_irq.c

LOCAL_C_INCLUDES := . \
                    $(LOCAL_PATH)/../../inc \
                    $(LOCAL_PATH)/../../../g1_decoder/linux/memalloc \
                    $(LOCAL_PATH)/../../../g1_decoder/linux/ldriver/kernel_26x/

LOCAL_CFLAGS := -DENC_MODULE_PATH=\"/dev/hx280enc\" \
                -DMEMALLOC_MODULE_PATH=\"/dev/memalloc\" \
		-DDEC_MODULE_PATH=\"/dev/hx170dec\" \
		-DEWL_NO_HW_TIMEOUT
#		-DANDROID_MOD
#		-D_DWL_DEBUG
#		-D_DWL_HW_PERFORMANCE \

ifeq ($(LOCKING), ioctl)
  LOCAL_CFLAGS += -DUSE_LINUX_LOCK_IOCTL
  LOCAL_SRC_FILES += ewl_linux_lock_ioctl.c
else
  LOCAL_SRC_FILES += ewl_linux_lock.c
endif

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libx280ewl

include $(BUILD_STATIC_LIBRARY)

