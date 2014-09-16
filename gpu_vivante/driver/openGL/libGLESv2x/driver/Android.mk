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
include $(LOCAL_PATH)/../../../../Android.mk.def


#
# libGLESv2_$(TAG)
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	gc_glsh_buffer.c \
	gc_glsh.c \
	gc_glsh_clear.c \
	gc_glsh_compiler.c \
	gc_glsh_database.c \
	gc_glsh_debug.c \
	gc_glsh_draw.c \
	gc_glsh_egl.c \
	gc_glsh_flush.c \
	gc_glsh_framebuffer.c \
	gc_glsh_object.c \
	gc_glsh_pixels.c \
	gc_glsh_query.c \
	gc_glsh_renderbuffer.c \
	gc_glsh_shader.c \
	gc_glsh_state.c \
	gc_glsh_texture.c \
	gc_glsh_vertex.c \
	gc_glsh_profiler.c \

LOCAL_CFLAGS := \
	$(CFLAGS)

LOCAL_C_INCLUDES := \
	$(AQROOT)/hal/inc \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/optimizer \
	$(AQROOT)/hal/os/linux/user \
	$(AQROOT)/sdk/inc \
	$(AQROOT)/driver/openGL/egl/inc

LOCAL_LDFLAGS := \
	-Wl,-z,defs \
	-Wl,--version-script=$(LOCAL_PATH)/libGLESv2.map

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libdl \
	libGAL \
	libEGL_$(TAG)

LOCAL_MODULE         := libGLESv2_$(TAG)
LOCAL_MODULE_TAGS    := optional
ifneq ($(USE_LINUX_MODE_FOR_ANDROID), 1)
LOCAL_MODULE_PATH    := $(TARGET_OUT_SHARED_LIBRARIES)/egl
endif
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

