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
# copybit.<property>.so
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	gc_copybit.cpp \
	gc_copybit_misc.cpp \
	gc_copybit_blit.cpp \
	gc_copybit_blit_pe1x.cpp

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-DLOG_TAG=\"v_copybit\"

LOCAL_C_INCLUDES := \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/os/linux/user \
	$(AQROOT)/hal/inc \
	$(AQROOT)/driver/gralloc \

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libGAL

LOCAL_MODULE         := copybit.$(PROPERTY)
LOCAL_MODULE_TAGS    := optional
LOCAL_MODULE_PATH    := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

