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
# VDK library backend for Android.
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	gc_vdk_egl.c \
	gc_vdk_gl.c

ifeq ($(USE_LINUX_MODE_FOR_ANDROID),1)
LOCAL_SRC_FILES += \
	gc_vdk_fbdev.c
else
LOCAL_SRC_FILES += \
	gc_vdk_android.cpp
endif

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-DLOG_TAG=\"vdk\" \
	-D__VDK_EXPORT

LOCAL_C_INCLUDES += \
	$(AQROOT)/sdk/inc

LOCAL_LDFLAGS := \
	-Wl,-z,defs \
	-Wl,--version-script=$(LOCAL_PATH)/libVDK.map \
	-DANDROID_SDK_VERSION=$(PLATFORM_SDK_VERSION)

LOCAL_SHARED_LIBRARIES := \
	libdl

ifneq ($(USE_LINUX_MODE_FOR_ANDROID),1)
ifneq ($(filter 14 15,$(PLATFORM_SDK_VERSION)),)
LOCAL_SHARED_LIBRARIES += \
	libui \
	libutils \
	libcutils \
	libbinder \
	libgui
else
LOCAL_SHARED_LIBRARIES += \
    libui \
	libutils \
	libcutils \
	libbinder \
	libsurfaceflinger_client
endif
endif

LOCAL_MODULE         := libVDK
LOCAL_MODULE_TAGS    := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

