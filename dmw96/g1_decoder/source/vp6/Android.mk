LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/..
common_linux_dir := $(LOCAL_PATH)/../../linux/

LOCAL_SRC_FILES :=	vp6booldec.c \
			vp6decodemode.c \
			vp6gconst.c \
			vp6hwd_api.c \
			vp6_pp_pipeline.c \
			vp6strmbuffer.c \
			vp6dec.c \
			vp6decodemv.c \
			vp6huffdec.c \
			vp6hwd_asic.c \
			vp6scanorder.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/config \
		    $(common_src_dir)/common \
		    $(common_linux_dir)/dwl

LOCAL_CFLAGS := -DDEC_X170_OUTPUT_FORMAT=DEC_X170_OUTPUT_FORMAT_RASTER_SCAN \
		-DDEC_X170_TIMEOUT_LENGTH=-1 \
		-DANDROID_MOD

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libx170vp6

include $(BUILD_STATIC_LIBRARY)

