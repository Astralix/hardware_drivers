LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

common_src_dir := $(LOCAL_PATH)/..
common_linux_dir := $(LOCAL_PATH)/../../linux/

LOCAL_SRC_FILES := h264decapi.c \
		   h264hwd_asic.c \
		   h264hwd_cavlc.c \
		   h264hwd_decoder.c \
		   h264hwd_inter_prediction.c \
		   h264hwd_macroblock_layer.c \
		   h264hwd_storage.c \
		   h264decapi_e.c \
		   h264hwd_cabac.c \
		   h264hwd_conceal.c \
		   h264hwd_dpb.c \
		   h264hwd_intra_prediction.c \
		   h264hwd_slice_data.c \
		   h264_pp_pipeline.c \
		   legacy/h264hwd_byte_stream.c \
		   legacy/h264hwd_neighbour.c \
		   legacy/h264hwd_pic_param_set.c \
		   legacy/h264hwd_slice_group_map.c \
		   legacy/h264hwd_stream.c \
		   legacy/h264hwd_vlc.c \
		   legacy/h264hwd_nal_unit.c \
		   legacy/h264hwd_pic_order_cnt.c \
		   legacy/h264hwd_seq_param_set.c \
		   legacy/h264hwd_slice_header.c \
		   legacy/h264hwd_util.c \
		   legacy/h264hwd_vui.c

LOCAL_C_INCLUDES := $(LOCAL_PATH)/ \
		    $(LOCAL_PATH)/legacy \
		    $(common_src_dir)/inc \
		    $(common_src_dir)/config \
		    $(common_src_dir)/common \
		    $(common_linux_dir)/dwl

LOCAL_CFLAGS := -DDEC_X170_OUTPUT_FORMAT=DEC_X170_OUTPUT_FORMAT_RASTER_SCAN \
		-DANDROID_MOD
#		-DANDROID_LOG \

#LOCAL_SHARED_LIBRARIES := $(common_shared_libraries)

LOCAL_MODULE_TAGS := eng

LOCAL_MODULE := libdecx170h264

include $(BUILD_STATIC_LIBRARY)

