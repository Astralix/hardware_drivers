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
include $(LOCAL_PATH)/Android.mk.def

ifndef FIXED_ARCH_TYPE
VIVANTE_MAKEFILES := \

else
VIVANTE_MAKEFILES :=

endif

VIVANTE_MAKEFILES += \
	$(LOCAL_PATH)/hal/user/Android.mk \
	$(LOCAL_PATH)/driver/gralloc/Android.mk

ifeq ($(BUILD_GALCORE_IN_ANDROID),yes)
VIVANTE_MAKEFILES += \
	$(LOCAL_PATH)/hal/kernel/Android.mk
endif

# Build hwcomposer for Honeycomb and later
ifeq ($(shell expr $(PLATFORM_SDK_VERSION) ">=" 11),1)
VIVANTE_MAKEFILES += \
	$(LOCAL_PATH)/driver/hwcomposer/Android.mk

endif


ifneq ($(VIVANTE_NO_3D),1)
VIVANTE_MAKEFILES += \
	$(LOCAL_PATH)/driver/openGL/egl/Android.mk \
	$(LOCAL_PATH)/driver/openGL/libGLESv11/Android.mk \
	$(LOCAL_PATH)/driver/openGL/libGLESv2x/driver/Android.mk \
	$(LOCAL_PATH)/driver/openGL/libGLESv2x/compiler/libGLESv2SC/Android.mk

endif

ifeq ($(USE_2D_COMPOSITION),1)
VIVANTE_MAKEFILES += \
	$(LOCAL_PATH)/driver/copybit/Android.mk

endif

include $(VIVANTE_MAKEFILES)

