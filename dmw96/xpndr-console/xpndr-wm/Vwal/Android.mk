LOCAL_PATH:= $(call my-dir)

INCLUDES =  $(LOCAL_PATH)
INCLUDES += $(LOCAL_PATH)/../include $(LOCAL_PATH)/../../include $(LOCAL_PATH)/include  $(LOCAL_PATH)/../../../../../../external/libnl-1.1/include/


vwal_SOURCES = src/vwal.c include/vwal.h
 

##############################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(vwal_SOURCES)
LOCAL_MODULE:= libvwal
#LOCAL_ARM_MODE:= arm
LOCAL_CFLAGS += -mabi=aapcs-linux
LOCAL_C_INCLUDES += $(INCLUDES)
STLPORT_FORCE_REBUILD := true
include $(BUILD_STATIC_LIBRARY)
##############################################

