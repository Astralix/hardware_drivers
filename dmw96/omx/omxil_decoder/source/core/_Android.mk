LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

HANTRO_TOP := external/hantro/openmax_il/
ifneq ($(BUILD_WITHOUT_PV),true)
PV_TOP := external/opencore
endif

LOCAL_MODULE := libhantro_omx_core
LOCAL_MODULE_TAGS := optional

LOCAL_PRELINK_MODULE := false

LOCAL_SRC_FILES := \
    hantro_omx_core.c

LOCAL_STATIC_LIBRARIES := \

ifeq ($(HANTRO_PARSER),1)
LOCAL_SHARED_LIBRARIES := \
    libdl \
    liblog \
    libVendor_hantro_omx_config_parser

LOCAL_CFLAGS := \
    -DHANTRO_PARSER

else
LOCAL_SHARED_LIBRARIES := \
    libdl \
    liblog

LOCAL_CFLAGS := \

endif

LOCAL_CFLAGS := \
    -DOMX_CORE_TRACE

LOCAL_C_INCLUDES := \
    $(HANTRO_TOP)/headers \
    $(HANTRO_TOP)/source/81x0_decoder/config_parser/inc

ifneq ($(BUILD_WITHOUT_PV),true)
LOCAL_C_INCLUDES +=
    $(PV_TOP)/build_config/opencore_dynamic \
    $(PV_TOP)/build_config/opencore_dynamic/build/installed_include \
    $(PV_TOP)/oscl/oscl/config/android \
    $(PV_TOP)/oscl/oscl/config/shared \
    $(PV_TOP)/oscl/oscl/osclbase/src \
    $(PV_TOP)/oscl/oscl/osclerror/src \
    $(PV_TOP)/oscl/oscl/osclmemory/src \
    $(PV_TOP)/oscl/oscl/osclutil/src \
    $(PV_TOP)/oscl/oscl/osclproc/src \
    $(PV_TOP)/oscl/oscl/oscllib/src \
    $(PV_TOP)/oscl/pvlogger/src \
    $(PV_TOP)/codecs_v2/omx/omx_common/include \
    $(PV_TOP)/codecs_v2/omx/omx_queue/src \
    $(PV_TOP)/codecs_v2/utilities/pv_config_parser/include \
    $(PV_TOP)/codecs_v2/omx/omx_proxy/src \
    $(PV_TOP)/codecs_v2/omx/omx_mastercore/include \
    $(PV_TOP)/pvmi/pvmf/include
endif

include $(BUILD_SHARED_LIBRARY)
