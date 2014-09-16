LOCAL_PATH := $(call my-dir)

fw_printenv_SOURCES = fw_env.c  fw_env_main.c

L_CFLAGS= -O3

local_target_dir := $(TARGET_OUT)/etc/

##############################################
include $(CLEAR_VARS)
LOCAL_MODULE := fw_env.config
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := fw_env.config
ifeq ($(TARGET_DEVICE),tf_evb_nand)
LOCAL_SRC_FILES := fw_env.config.tf_evb_nand
endif
ifeq ($(TARGET_DEVICE),tf_mid)
LOCAL_SRC_FILES := fw_env.config.emmc
endif
ifeq ($(TARGET_DEVICE),evb96_pixcir_emmc)
LOCAL_SRC_FILES := fw_env.config.emmc
endif
ifeq ($(TARGET_DEVICE),evb96_emmc)
LOCAL_SRC_FILES := fw_env.config.emmc
endif
include $(BUILD_PREBUILT)

include $(CLEAR_VARS)

FW_PRINTENV_TOOLS :=  fw_setenv

LOCAL_SRC_FILES := $(fw_printenv_SOURCES)
LOCAL_MODULE:= fw_printenv
LOCAL_ARM_MODE := arm
LOCAL_STATIC_LIBRARIES := libz libstdc++ libc
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_CFLAGS += $(L_CFLAGS) 
include $(BUILD_EXECUTABLE)



SYMLINKS := $(addprefix $(TARGET_OUT_OPTIONAL_EXECUTABLES)/,$(FW_PRINTENV_TOOLS))
$(SYMLINKS): FW_PRINTENV_BINARY := $(LOCAL_MODULE)
$(SYMLINKS): $(LOCAL_INSTALLED_MODULE)
	@echo "Symlink: $@ -> $(FW_PRINTENV_BINARY)"
	@mkdir -p $(dir $@)
	@rm -rf $@
	$(hide) ln -sf $(FW_PRINTENV_BINARY) $@

ALL_DEFAULT_INSTALLED_MODULES += $(SYMLINKS)

# We need this so that the installed files could be picked up based on the
# local module name
ALL_MODULES.$(LOCAL_MODULE).INSTALLED := \
    $(ALL_MODULES.$(LOCAL_MODULE).INSTALLED) $(SYMLINKS)
