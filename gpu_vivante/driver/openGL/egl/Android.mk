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
include $(LOCAL_PATH)/../../../Android.mk.def


#
# libEGL_$(TAG)
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	source/gc_egl.c \
	source/gc_egl_composition.c \
	source/gc_egl_config.c \
	source/gc_egl_context.c \
	source/gc_egl_image.c \
	source/gc_egl_init.c \
	source/gc_egl_query.c \
	source/gc_egl_surface.c \
	source/gc_egl_swap.c \
	source/gc_egl_sync.c

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-DLOG_TAG=\"v_egl\" \
	-D__EGL_EXPORTS

ifeq ($(VIVANTE_NO_3D),1)
LOCAL_CFLAGS += \
	-DVIVANTE_NO_3D=1
endif

ifeq ($(gcdDUMP_FRAME_TGA),1)
LOCAL_CFLAGS += \
	-DgcdDUMP_FRAME_TGA=1
endif

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/inc \
	$(LOCAL_PATH)/api \
	$(LOCAL_PATH)/os \
	$(LOCAL_PATH)/source \
	$(AQROOT)/sdk/inc \
	$(AQROOT)/hal/inc \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/os/linux/user \
	$(AQROOT)/driver/gralloc \

LOCAL_LDFLAGS := \
	-Wl,-z,defs \
	-Wl,--version-script=$(LOCAL_PATH)/source/libEGL.map

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libdl \
	libhardware \
	libGAL

ifeq ($(gcdDUMP_FRAME_TGA),1)
LOCAL_SHARED_LIBRARIES += \
	libcutils
endif

ifneq ($(USE_LINUX_MODE_FOR_ANDROID), 1)
LOCAL_SRC_FILES += \
	api/gc_egl_android_new.c
else
LOCAL_SRC_FILES += \
	api/gc_egl_platform.c

endif


LOCAL_MODULE         := libEGL_$(TAG)
LOCAL_MODULE_TAGS    := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)


#
# Copy libEGL_$(TAG)
#
INSTALLED_EGL_LIBRARY := \
	$(LOCAL_INSTALLED_MODULE)

TARGET_EGL_LIBRARY    := \
	$(TARGET_OUT_SHARED_LIBRARIES)/egl/libEGL_$(TAG).so

# Use ALL_PREBUILT for eclair,froyo; extra ALL_MODULES for
# gingerbread
ifeq ($(filter 6 7 8,$(PLATFORM_SDK_VERSION)),)
ALL_MODULES  += $(TARGET_EGL_LIBRARY)
endif
ALL_PREBUILT += $(TARGET_EGL_LIBRARY)

$(TARGET_EGL_LIBRARY) : $(INSTALLED_EGL_LIBRARY) $(ACP)
	@echo creating $(TARGET_OUT_SHARED_LIBRARIES)/egl folder
	@mkdir -p $(TARGET_OUT_SHARED_LIBRARIES)/egl
	@$(ACP) $(INSTALLED_EGL_LIBRARY) $(TARGET_EGL_LIBRARY)

