LOCAL_PATH:= $(call my-dir)

INCLUDES = $(LOCAL_PATH)/include

dwphy_SOURCES = source/hw-phy.c source/DwPhyApp.c source/DwPhyUtil.c

XPNDR_CLIAPP_VERSION=2.0.3a

##############################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(dwphy_SOURCES)
LOCAL_MODULE:= xpndr-dwphy
LOCAL_ARM_MODE:= arm
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng
#LOCAL_CFLAGS += -DXPNDR_CLI_VERSION="\"\\\"$(XPNDR_CLIAPP_VERSION)\\\"\""	
LOCAL_CFLAGS += -DBER_LINUX -DBER_LINUX_DRIVER -DDWPHY_NOT_DRIVER
LOCAL_C_INCLUDES += $(INCLUDES)
LOCAL_SHARED_LIBRARIES += libstlport liblog libnl
LOCAL_STATIC_LIBRARIES += libvwal
#include external/stlport/libstlport.mk
STLPORT_FORCE_REBUILD := true
include $(BUILD_EXECUTABLE)
##############################################


