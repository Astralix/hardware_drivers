LOCAL_PATH:= $(call my-dir)

INCLUDES = $(LOCAL_PATH)/src
INCLUDES += bionic/libc/include
syssettings_SOURCES = src/SysSettings.cpp  src/SysSettings.h


##############################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(syssettings_SOURCES)
LOCAL_MODULE:= libsyssettings
LOCAL_ARM_MODE:= arm
LOCAL_C_INCLUDES += $(INCLUDES)
LOCAL_C_INCLUDES += external/sqlite/dist
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../include 
LOCAL_SHARED_LIBRARIES+= libstlport libsqlite
LOCAL_STATIC_LIBRARIES+= libsqlitewrapped
include external/stlport/libstlport.mk
STLPORT_FORCE_REBUILD := true
LOCAL_PRELINK_MODULE := false
# LOCAL_CFLAGS += -Wnon-virtual-dtor
# LOCAL_CPPFLAGS += -Wnon-virtual-dtor  
include $(BUILD_STATIC_LIBRARY)
##############################################

