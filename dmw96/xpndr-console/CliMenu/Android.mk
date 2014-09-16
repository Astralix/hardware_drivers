LOCAL_PATH:= $(call my-dir)

INCLUDES = $(LOCAL_PATH)/src $(LOCAL_PATH)/include $(LOCAL_PATH)/../include


#climenu_SOURCES = src/MenuEntry.cpp src/MenuEntry.h src/ActionCommand.cpp src/ActionCommand.h \
#                                   src/ActionCallbackCommand.cpp src/ActionCallbackCommand.h \
#				   src/SubMenu.cpp src/SubMenu.h \
#                                   src/Command.cpp src/Command.h src/CliUtils.cpp src/CliUtils.h \
#				   src/CliMenu.cpp src/CliMenu.h 

climenu_SOURCES = src/MenuEntry.cpp src/ActionCommand.cpp src/ActionCallbackCommand.cpp  \
		  src/SubMenu.cpp   src/Command.cpp src/CliUtils.cpp src/CliMenu.cpp


##############################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(climenu_SOURCES)
LOCAL_MODULE:= libclimenu
LOCAL_ARM_MODE:= arm
LOCAL_C_INCLUDES += $(INCLUDES)
#LOCAL_C_INCLUDES += external/sqlite/dist
LOCAL_SHARED_LIBRARIES+= libstlport
include external/stlport/libstlport.mk
STLPORT_FORCE_REBUILD := true
LOCAL_PRELINK_MODULE := false
include $(BUILD_STATIC_LIBRARY)
##############################################

