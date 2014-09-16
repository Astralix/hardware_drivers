LOCAL_PATH:= $(call my-dir)

INCLUDES = $(LOCAL_PATH) $(LOCAL_PATH)/../xpndr-wm $(LOCAL_PATH)/../xpndr-wm/include $(LOCAL_PATH)/..  $(LOCAL_PATH)/../include


cliapp_SOURCES = XpndrCli.cpp XpndrCli.h WifiCLICommand.cpp WifiCLICommand.h \
                 TxPacketSettings.cpp TxPacketSettings.h RxSettings.cpp RxSettings.h \
		 MenuCommands.cpp MenuCommands.h Main.cpp

XPNDR_CLIAPP_VERSION=2.0.4a

##############################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(cliapp_SOURCES)
LOCAL_MODULE:= xpndr-console
LOCAL_ARM_MODE:= arm
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_MODULE_TAGS := eng
LOCAL_CFLAGS += -DXPNDR_CLI_VERSION="\"\\\"$(XPNDR_CLIAPP_VERSION)\\\"\""	
LOCAL_C_INCLUDES += $(INCLUDES)
LOCAL_SHARED_LIBRARIES += libstlport liblog libsqlite libnl
LOCAL_STATIC_LIBRARIES += libwifimgr libsyssettings libclimenu libvwal libsqlitewrapped
include external/stlport/libstlport.mk
STLPORT_FORCE_REBUILD := true
include $(BUILD_EXECUTABLE)
##############################################


