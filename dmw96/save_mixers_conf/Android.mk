ifneq ($(TARGET_SIMULATOR),true)

LOCAL_PATH:= $(call my-dir)


include $(CLEAR_VARS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := save_mixers_conf

LOCAL_CFLAGS := \
	-fPIC -D_POSIX_SOURCE \
	-DALSA_CONFIG_DIR=\"/system/usr/share/alsa\" \
	-DALSA_PLUGIN_DIR=\"/system/usr/lib/alsa-lib\" \
	-DALSA_DEVICE_DIRECTORY=\"/dev/snd/\"

LOCAL_SHARED_LIBRARIES := \
	libasound \
	libc

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/android \
	external/alsa-lib/include

LOCAL_SRC_FILES:= save_mixers_conf.c

LOCAL_STATIC_LIBRARIES := 
include $(BUILD_EXECUTABLE)

endif  # TARGET_SIMULATOR != true
