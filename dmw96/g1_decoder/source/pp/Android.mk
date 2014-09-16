###############################################
# this is the pp module with pipeline support #
###############################################

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/..
common_linux_dir := $(LOCAL_PATH)/../../linux/

PIPELINE_SUPPORT := 	

LOCAL_SRC_FILES := ppapi.c \
		   ppinternal.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/config \
		    $(common_src_dir)/common \
		    $(common_src_dir)/mpeg4 \
		    $(common_src_dir)/jpeg \
		    $(common_src_dir)/vc1 \
		    $(common_src_dir)/mpeg2 \
		    $(common_src_dir)/vp6 \
		    $(common_src_dir)/vp8 \
		    $(common_src_dir)/h264high \
	            $(common_src_dir)/rv \
		    $(common_src_dir)/avs \
		    $(common_linux_dir)/dwl

PIPELINE_SUPPORT := 	-DPP_H264DEC_PIPELINE_SUPPORT \
			-DPP_MPEG4DEC_PIPELINE_SUPPORT \
			-DPP_JPEGDEC_PIPELINE_SUPPORT \
			-DPP_VC1DEC_PIPELINE_SUPPORT \
			-DPP_MPEG2DEC_PIPELINE_SUPPORT \
			-DPP_VP6DEC_PIPELINE_SUPPORT \
			-DPP_VP8DEC_PIPELINE_SUPPORT \
			-DPP_RVDEC_PIPELINE_SUPPORT \
			-DPP_AVSDEC_PIPELINE_SUPPORT

LOCAL_CFLAGS := -DDEC_X170_OUTPUT_FORMAT=DEC_X170_OUTPUT_FORMAT_RASTER_SCAN \
		$(PIPELINE_SUPPORT) \
		-DANDROID_MOD
#		-DANDROID_LOG
#		-DPP_TRACE 


LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libx170pp

include $(BUILD_STATIC_LIBRARY)


###############################################
# this is the pp module with pipeline support #
###############################################

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/..
common_linux_dir := $(LOCAL_PATH)/../../linux/

LOCAL_SRC_FILES := ppapi.c \
		   ppinternal.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/config \
		    $(common_src_dir)/common \
		    $(common_linux_dir)/dwl

LOCAL_CFLAGS := -DDEC_X170_OUTPUT_FORMAT=DEC_X170_OUTPUT_FORMAT_RASTER_SCAN \
		-DANDROID_MOD
#		-DANDROID_LOG
#		-DPP_TRACE 


LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libx170pp.standalone

include $(BUILD_STATIC_LIBRARY)

