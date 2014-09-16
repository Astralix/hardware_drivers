LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/..
common_linux_dir := $(LOCAL_PATH)/../../linux/

LOCAL_SRC_FILES := mp4decapi.c \
		   mp4dechwd_error_conceal.c \
		   mp4dechwd_motiontexture.c \
		   mp4dechwd_strmdec.c \
		   mp4dechwd_vlc.c \
		   mp4decapi_internal.c \
		   mp4dechwd_rvlc.c \
		   mp4dechwd_utils.c \
		   mp4dechwd_vop.c \
		   mp4dechwd_headers.c \
		   mp4dechwd_shortvideo.c \
		   mp4dechwd_videopacket.c

CUSTOM_FMT_SUPPORT := y

ifeq ($(CUSTOM_FMT_SUPPORT),y)
	LOCAL_SRC_FILES += mp4dechwd_custom.c
else
	LOCAL_SRC_FILES += mp4dechwd_generic.c
endif

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/config \
		    $(common_src_dir)/common \
		    $(common_linux_dir)/dwl

LOCAL_CFLAGS := -DDEC_X170_OUTPUT_FORMAT=DEC_X170_OUTPUT_FORMAT_RASTER_SCAN \
		-D_MP4_RLC_BUFFER_SIZE=384 \
		-DANDROID_MOD

# Enable for debugging
#LOCAL_CFLAGS += -DMP4DEC_TRACE

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libx170mpeg4

include $(BUILD_STATIC_LIBRARY)

