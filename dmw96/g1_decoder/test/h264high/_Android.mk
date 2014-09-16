LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/../../source

LOCAL_SRC_FILES := dectestbench.c

LOCAL_C_INCLUDES := . \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/h264high \
		    $(common_src_dir)/h264high/legacy \
		    $(LOCAL_PATH)/../memalloc \
		    $(LOCAL_PATH)/../ldriver/kernel_26x \
		    $(common_src_dir)/common \
		    $(LOCAL_PATH)/../common/swhw \
		    $(common_src_dir)/config

LOCAL_CFLAGS := -DDEC_MODULE_PATH=\"/dev/hx170dec\" \
                -DMEMALLOC_MODULE_PATH=\"/dev/memalloc\" \
		-DUSE_MD5SUM=n \
		-DANDROID_MOD \
		-D_DWL_DEBUG

LOCAL_SHARED_LIBRARIES := libc	\
			  libcutils

LOCAL_STATIC_LIBRARIES := libdwlx170 \
			  libtbcommon \
			  libdecx170h264 \
			  libg1common

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := test_h264

include $(BUILD_EXECUTABLE)

