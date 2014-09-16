#
# on2's memalloc.ko
#
LOCAL_PATH	:= $(abspath $(call my-dir))

include $(CLEAR_VARS)
.PHONY: MEMALLOC_KBUILD

KERNEL_DIR	:= $(LOCAL_PATH)/../../../../../../kernel
THIS_DIR	:= $(LOCAL_PATH)/

MEMALLOC := $(LOCAL_PATH)/memalloc.ko


#$(eval "echo hahaha $CROSS_COMPILE")


$(MEMALLOC): MEMALLOC_KBUILD
MEMALLOC_KBUILD:
	$(MAKE) -C $(KERNEL_DIR) SUBDIRS=$(THIS_DIR) modules CROSS_COMPILE=../prebuilt/linux-x86/toolchain/arm-eabi-4.4.3/bin/arm-eabi-

LOCAL_MODULE := memalloc.ko
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/modules
LOCAL_SRC_FILES := memalloc.ko
LOCAL_GENERATED_SOURCES += $(MEMALLOC)
include $(BUILD_PREBUILT)

