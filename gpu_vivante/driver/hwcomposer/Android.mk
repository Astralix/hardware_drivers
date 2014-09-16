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
# hwcomposer.<property>.so
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	gc_hwc.cpp \
	gc_hwc_debug.cpp \
	gc_hwc_prepare.cpp \
	gc_hwc_set.cpp \
	gc_hwc_compose.cpp \
	gc_hwc_overlay.cpp

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-Wall \
	-Wextra \
	-DLOG_TAG=\"v_hwc\"

LOCAL_C_INCLUDES := \
	$(AQROOT)/sdk/inc \
	$(AQROOT)/driver/gralloc \
	$(AQROOT)/hal/inc

LOCAL_LDFLAGS := \
	-Wl,--version-script=$(LOCAL_PATH)/hwcomposer.map

LOCAL_SHARED_LIBRARIES := \
	libcutils \
	liblog \
	libEGL \
	libGAL

LOCAL_MODULE         := hwcomposer.$(PROPERTY)
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_PATH    := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

