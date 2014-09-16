LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/../..

LOCAL_SRC_FILES := EncCoder.c \
                   EncGroupOfVideoObjectPlane.c \
		   EncProfile.c \
		   EncTestId.c \
		   EncVideoObjectLayer.c \
		   EncVlc.c \
		   EncCodeVop.c \
		   EncInit.c \
		   encputbits.c \
		   EncTimeCode.c \
		   EncVideoObjectPlane.c \
		   MP4EncApi.c \
		   EncCodeVop_v2.c \
		   EncMbHeader.c \
		   EncRateControl.c \
		   EncUserData.c \
		   EncVisualObject.c \
		   mp4encapi_ext.c \
		   EncDifferentialMvCoding.c \
		   EncPrediction.c \
		   EncShortVideoHeader.c \
		   EncVideoObject.c \
		   EncVisualObjectSequence.c

LOCAL_C_INCLUDES := . \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/source/common \
		    $(common_src_dir)/linux/debug_trace

LOCAL_CFLAGS := -DMPEG4_HW_VLC_MODE_ENABLED \
		-DANDROID_MOD

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libx280mpeg4

include $(BUILD_STATIC_LIBRARY)

