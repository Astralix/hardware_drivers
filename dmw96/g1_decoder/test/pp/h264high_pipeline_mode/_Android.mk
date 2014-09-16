LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/../../../source

pp_src_dir := $(LOCAL_PATH)/../external_mode

h264_src_dir := $(LOCAL_PATH)/../../h264high

LOCAL_SRC_FILES := ../../h264high/dectestbench.c \
		../external_mode/pptestbench.c \
		../external_mode/params.c \
		../external_mode/cfg.c \
		../external_mode/frame.c

LOCAL_C_INCLUDES := . \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/h264high \
		    $(common_src_dir)/h264high/legacy \
		    $(LOCAL_PATH)/../../../linux/memalloc \
		    $(LOCAL_PATH)/../../ldriver/kernel_26x \
		    $(common_src_dir)/common \
		    $(LOCAL_PATH)/../../common/swhw \
		    $(common_src_dir)/config \
		    $(common_src_dir)/pp \
		    $(LOCAL_PATH)/../external_mode

LOCAL_CFLAGS := -DDEC_MODULE_PATH=\"/dev/hx170dec\" \
                -DMEMALLOC_MODULE_PATH=\"/dev/memalloc\" \
		-DUSE_MD5SUM=n \
		-DANDROID_MOD \
		-DPP_PIPELINE_ENABLED \
		-D_DWL_DEBUG

LOCAL_SHARED_LIBRARIES := libc \
			  libcutils

LOCAL_STATIC_LIBRARIES := libdwlx170 \
			  libtbcommon \
			  libdecx170h264 \
			  libg1common \
			  libx170pp

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE := test_h264_pp

include $(BUILD_EXECUTABLE)

