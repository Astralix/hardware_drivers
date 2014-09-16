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
# libOpenVG
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	gc_vgsh_egl.c \
	gc_vgsh_dump.c \
	gc_vgsh_context.c \
	gc_vgsh_font.c \
	gc_vgsh_image.c \
	gc_vgsh_matrix.c \
	gc_vgsh_mask_layer.c \
	gc_vgsh_math.c \
	gc_vgsh_object.c \
	gc_vgsh_paint.c \
	gc_vgsh_path.c \
	gc_vgsh_scanline.c \
	gc_vgsh_shader.c \
	gc_vgsh_tessellator.c \
	gc_vgsh_hardware.c \
	gc_vgsh_vgu.c \
	gc_vgsh_profiler.o

LOCAL_CFLAGS := \
	$(CFLAGS)

LOCAL_C_INCLUDES += \
	$(AQROOT)/hal/inc \
	$(AQROOT)/hal/user \
	$(AQROOT)/sdk/inc \
	$(AQROOT)/hal/os/linux/user \
	$(AQROOT)/driver/openGL/egl/inc

LOCAL_LDFLAGS := \
	-Wl,-z,defs \
	-Wl,--version-script=$(LOCAL_PATH)/libOpenVG.map

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libdl \
	libGAL \
	libEGL_$(TAG)

LOCAL_MODULE        := libOpenVG
LOCAL_MODULE_TAGS   := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

