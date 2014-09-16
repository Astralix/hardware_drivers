LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/../../../source

LOCAL_SRC_FILES := tb_stream_corrupt.c \
		tb_params.c \
		tb_cfg.c \
		tb_md5.c \
		md5.c \
		tb_tiled.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/common \
		    $(common_src_dir)/config

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libtbcommon

include $(BUILD_STATIC_LIBRARY)

