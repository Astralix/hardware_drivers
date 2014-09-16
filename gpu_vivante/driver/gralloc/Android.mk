##############################################################################
#
#    Copyright (c) 2005 - 2012 by Vivante Corp.  All rights reserved.
#
#    The material in this file is confidential and contains trade secrets
#    of Vivante Corporation. This is proprietary information owned by
#    Vivante Corporation. No part of this work may be disclosed,
#    reproduced, copied, transmitted, or used in any way for any purpose,
#    without the express written permission of Vivante Corporation.
#
##############################################################################


LOCAL_PATH := $(call my-dir)
include $(LOCAL_PATH)/../../Android.mk.def

#
# gralloc.<property>.so
#
ifneq ($(USE_LINUX_MODE_FOR_ANDROID), 1)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	gralloc.cpp \
	gc_gralloc_alloc.cpp \
	gc_gralloc_fb.cpp \
	gc_gralloc_map.cpp

# With front buffer rendering, gralloc always provides the same buffer  when
# GRALLOC_USAGE_HW_FB. Obviously there is no synchronization with the display.
# Can be used to test non-VSYNC-locked rendering.
LOCAL_CFLAGS := \
	$(CFLAGS) \
	-DLOG_TAG=\"v_gralloc\" \
	-DDISABLE_FRONT_BUFFER

LOCAL_CFLAGS += \
	-DLOG_NDEBUG=1

LOCAL_C_INCLUDES := \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/os/linux/user \
	$(AQROOT)/hal/inc

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libGAL \
	libbinder \
	libutils

LOCAL_SHARED_LIBRARIES += libosdm

LOCAL_C_INCLUDES += kernel/include/video \
		    hardware/dspg/dmw96/libosdm/

LOCAL_CFLAGS += -DUSE_OSDM

LOCAL_MODULE         := gralloc.$(PROPERTY)
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_PATH    := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

endif
