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
# libGLESv1_CM_$(TAG)
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	gc_glff_alpha.c \
	gc_glff_basic_types.c \
	gc_glff_buffer.c \
	gc_glff.c \
	gc_glff_clear.c \
	gc_glff_clip_plane.c \
	gc_glff_context.c \
	gc_glff_cull.c \
	gc_glff_depth.c \
	gc_glff_draw.c \
	gc_glff_egl.c \
	gc_glff_enable.c \
	gc_glff_fixed_func.c \
	gc_glff_fog.c \
	gc_glff_fragment_shader.c \
	gc_glff_frag_proc.c \
	gc_glff_framebuffer.c \
	gc_glff_hash.c \
	gc_glff_lighting.c \
	gc_glff_line.c \
	gc_glff_matrix.c \
	gc_glff_multisample.c \
	gc_glff_named_object.c \
	gc_glff_pixel.c \
	gc_glff_point.c \
	gc_glff_profiler.c \
	gc_glff_query.c \
	gc_glff_renderbuffer.c \
	gc_glff_states.c \
	gc_glff_stream.c \
	gc_glff_texture.c \
	gc_glff_vertex_shader.c \
	gc_glff_viewport.c \

LOCAL_CFLAGS := \
	$(CFLAGS) \
	-D__GL_EXPORTS

LOCAL_C_INCLUDES := \
	$(AQROOT)/sdk/inc \
	$(AQROOT)/hal/inc \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/os/linux/user \
	$(AQROOT)/driver/gralloc \
	$(AQROOT)/driver/openGL/egl/inc

LOCAL_LDFLAGS := \
	-Wl,-z,defs \
	-Wl,--version-script=$(LOCAL_PATH)/libGLESv11.map

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libdl \
	libGAL \
	libEGL_$(TAG)

LOCAL_MODULE         := libGLESv1_CM_$(TAG)
LOCAL_MODULE_TAGS    := optional
ifneq ($(USE_LINUX_MODE_FOR_ANDROID), 1)
LOCAL_MODULE_PATH    := $(TARGET_OUT_SHARED_LIBRARIES)/egl
endif
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

