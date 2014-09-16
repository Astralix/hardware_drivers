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
include $(LOCAL_PATH)/../../../../../Android.mk.def


#
# libGLSLC
#
include $(CLEAR_VARS)

ifndef FIXED_ARCH_TYPE
LOCAL_SRC_FILES := \
	common/gc_glsl_common.c \
	compiler/gc_glsl_built_ins.c \
	compiler/gc_glsl_compiler.c \
	compiler/gc_glsl_emit_code.c \
	compiler/gc_glsl_gen_code.c \
	compiler/gc_glsl_ir.c \
	compiler/gc_glsl_parser.c \
	compiler/gc_glsl_parser_misc.c \
	compiler/gc_glsl_scanner.c \
	compiler/gc_glsl_scanner_misc.c \
	compiler/gc_glsl_ast_walk.c \
	preprocessor/gc_glsl_api.c \
	preprocessor/gc_glsl_base.c \
	preprocessor/gc_glsl_expression.c \
	preprocessor/gc_glsl_hide_set.c \
	preprocessor/gc_glsl_input_stream.c \
	preprocessor/gc_glsl_macro_expand.c \
	preprocessor/gc_glsl_macro_manager.c \
	preprocessor/gc_glsl_preprocessor.c \
	preprocessor/gc_glsl_syntax.c \
	preprocessor/gc_glsl_syntax_util.c \
	preprocessor/gc_glsl_token.c \
	entry/gc_glsl_entry.c
else
LOCAL_WHOLE_STATIC_LIBRARIES := \
	libglslPreprocessor \
	libglslCompiler \
	libGLSLC
endif

LOCAL_CFLAGS := \
	$(CFLAGS)

LOCAL_C_INCLUDES := \
	$(AQROOT)/hal/inc \
	$(AQROOT)/hal/user \
	$(AQROOT)/hal/os/linux/user \
	$(LOCAL_PATH)/inc

LOCAL_LDFLAGS := \
	-Wl,-z,defs

LOCAL_SHARED_LIBRARIES := \
	liblog \
	libdl \
	libGAL

LOCAL_MODULE         := libGLSLC
LOCAL_MODULE_TAGS    := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)


ifdef FIXED_ARCH_TYPE

#
# Prebuilt libGLSLC
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	entry/$(FIXED_ARCH_TYPE)/libGLSLC.a

LOCAL_MODULE        := libGLSLC
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_TAGS   := optional
LOCAL_MODULE_CLASS  := STATIC_LIBRARIES
include $(BUILD_PREBUILT)


#
# Prebuilt libglslPreprocessor
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	preprocessor/$(FIXED_ARCH_TYPE)/libglslPreprocessor.a

LOCAL_MODULE        := libglslPreprocessor
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_TAGS   := optional
LOCAL_MODULE_CLASS  := STATIC_LIBRARIES
include $(BUILD_PREBUILT)


#
# Prebuilt libglslCompiler
#
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	compiler/$(FIXED_ARCH_TYPE)/libglslCompiler.a

LOCAL_MODULE        := libglslCompiler
LOCAL_MODULE_SUFFIX := .a
LOCAL_MODULE_TAGS   := optional
LOCAL_MODULE_CLASS  := STATIC_LIBRARIES
include $(BUILD_PREBUILT)

endif

