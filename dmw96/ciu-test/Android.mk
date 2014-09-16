LOCAL_PATH:= $(call my-dir)


##############################################
include $(CLEAR_VARS)
LOCAL_SRC_FILES := capture.c
LOCAL_MODULE:= capture
LOCAL_ARM_MODE := arm
LOCAL_C_INCLUDES += $(INCLUDES) kernel 
LOCAL_MODULE_TAGS := optional
include $(BUILD_EXECUTABLE)



