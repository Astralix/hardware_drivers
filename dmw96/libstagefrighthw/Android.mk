LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
		ON2OMXPlugin.cpp \
		stagefright_overlay_output.cpp \
		DSPGHardwareRenderer.cpp

LOCAL_C_INCLUDES:= \
	$(TOP)/frameworks/base/include/media/stagefright/openmax \
	$(TOP)/hardware/dspg/dmw96/liboverlay

LOCAL_SHARED_LIBRARIES :=       \
        libbinder               \
        libutils                \
        libcutils               \
        libui                   \
        libdl					\
        libsurfaceflinger_client

LOCAL_CFLAGS += -DSUPPORT_ZERO_COPY

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libstagefrighthw

include $(BUILD_SHARED_LIBRARY)

