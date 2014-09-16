LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

BELLAGIO_INCLUDE = $(LOCAL_PATH)/include
OMXIL_HEADERS = $(LOCAL_PATH)/../omxil_decoder/headers

LOCAL_SRC_FILES := \
	DSPG_OMX_Core.c \
	st_static_component_loader.c

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH) \
	$(BELLAGIO_INCLUDE) \
	$(OMXIL_HEADERS)

LOCAL_SHARED_LIBRARIES :=       \
        libdl			\
	libutils

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := DSPG_OMX_Core
LOCAL_PRELINK_MODULE:=false

include $(BUILD_SHARED_LIBRARY)

