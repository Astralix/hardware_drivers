LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/../..

LOCAL_SRC_FILES := H264CodeFrame.c \
                   h264encapi_ext.c \
		   H264NalUnit.c \
		   H264PutBits.c \
		   H264Sei.c \
		   H264Slice.c \
		   H264EncApi.c \
		   H264Init.c \
		   H264PictureParameterSet.c \
		   H264RateControl.c \
		   H264SequenceParameterSet.c \
		   H264TestId.c


LOCAL_C_INCLUDES := . \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/source/common

LOCAL_CFLAGS := -DANDROID_MOD
#		-DH264ENC_TRACE \
#		-DTIME_ENCODE

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libx280h264

include $(BUILD_STATIC_LIBRARY)

