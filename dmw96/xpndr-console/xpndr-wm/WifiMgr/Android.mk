LOCAL_PATH:= $(call my-dir)

INCLUDES =  $(LOCAL_PATH)
INCLUDES += $(LOCAL_PATH)/../include $(LOCAL_PATH)/../Vwal/include $(LOCAL_PATH)/include $(LOCAL_PATH)/src $(LOCAL_PATH)/../.. $(LOCAL_PATH)/../../include


wifimgr_SOURCES = src/Utils/WmUtils.cpp  src/Utils/WmUtils.h \
		  src/VwalWrapper/WmVwalWrapper.cpp src/VwalWrapper/WmVwalWrapper.h \
		  src/WifiTestMgr/WifiTestMgr.cpp src/WifiTestMgr/WifiTestMgr.h \
                  src/WifiTestMgr/CRandomPacketGenerator.cpp src/WifiTestMgr/CRandomPacketGenerator.h \
		  src/WifiTestMgr/WifiTPacketReceiver.cpp src/WifiTestMgr/WifiTPacketReceiver.h



##############################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(wifimgr_SOURCES)
LOCAL_MODULE:= libwifimgr
LOCAL_ARM_MODE:= arm
LOCAL_C_INCLUDES += $(INCLUDES)
#LOCAL_SHARED_LIBRARIES+= libstlport libsyssettings
LOCAL_STATIC_LIBRARIES+= libvwal 
include external/stlport/libstlport.mk
STLPORT_FORCE_REBUILD := true
include $(BUILD_STATIC_LIBRARY)
##############################################

