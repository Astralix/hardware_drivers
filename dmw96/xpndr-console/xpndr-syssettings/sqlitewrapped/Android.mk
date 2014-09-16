LOCAL_PATH:= $(call my-dir)

INCLUDES = $(LOCAL_PATH)/src


sqlitewrapped_SOURCES = src/SysLog.cpp src/SysLog.h src/StderrLog.cpp src/StderrLog.h \
                                   src/Database.cpp src/Database.h src/Query.cpp src/Query.h \
                                   src/SQLiteBinary.cpp src/SQLiteBinary.h




##############################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(sqlitewrapped_SOURCES)
LOCAL_MODULE:= libsqlitewrapped
LOCAL_ARM_MODE:= arm
LOCAL_C_INCLUDES += $(INCLUDES)
LOCAL_C_INCLUDES += external/sqlite/dist
LOCAL_SHARED_LIBRARIES+= libstlport
include external/stlport/libstlport.mk
STLPORT_FORCE_REBUILD := true
include $(BUILD_STATIC_LIBRARY)
##############################################



