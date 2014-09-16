BASE_PATH := $(call my-dir)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

# first camera
ifdef BOARD_CAMERA_DEVICE
	LOCAL_CFLAGS += -DENABLE_FIRST_CAMERA
	LOCAL_CFLAGS += -DDSPG_FIRST_CAMERA_DEVICE=\"$(BOARD_CAMERA_DEVICE)\"
	LOCAL_CFLAGS += -DDSPG_FIRST_CAMERA_FACING=$(BOARD_CAMERA_FACING)
	LOCAL_CFLAGS += -DDSPG_FIRST_CAMERA_ROTATE_ANGLE=$(BOARD_CAMERA_ROTATE_ANGLE)
endif

# second camera
ifdef BOARD_SECOND_CAMERA_DEVICE
	LOCAL_CFLAGS += -DENABLE_SECOND_CAMERA
	LOCAL_CFLAGS += -DDSPG_SECOND_CAMERA_DEVICE=\"$(BOARD_SECOND_CAMERA_DEVICE)\"
	LOCAL_CFLAGS += -DDSPG_SECOND_CAMERA_FACING=$(BOARD_SECOND_CAMERA_FACING)
	LOCAL_CFLAGS += -DDSPG_SECOND_CAMERA_ROTATE_ANGLE=$(BOARD_SECOND_CAMERA_ROTATE_ANGLE)
endif

# Enable this to enable frame profiling using counters and timers
#LOCAL_CFLAGS += -DENABLE_PROFILING

LOCAL_ARM_MODE := arm

LOCAL_SRC_FILES:= \
	CameraHardware.cpp \
	V4L2Camera.cpp \
	JpegEncoder.cpp
#	V4L2Camera_dummy.cpp

LOCAL_STATIC_LIBRARIES := libx280jpeg \
			  libx280ewl \
			  libx280common

LOCAL_SHARED_LIBRARIES := \
        libcutils \
        libutils \
        libui \
        libcamera_client \
        libbinder \
		libpp.dspg

DECODER_RELEASE := $(LOCAL_PATH)/../g1_decoder
ENCODER_RELEASE := $(LOCAL_PATH)/../7280_encoder/

LOCAL_C_INCLUDES += \
        $(ENCODER_RELEASE)/inc \
        $(ENCODER_RELEASE)/source/jpeg \
        $(ENCODER_RELEASE)/source/common \
        $(DECODER_RELEASE)/source/inc \
        hardware/dspg/dmw96/liboverlay \
        $(LOCAL_PATH)/../libpp	\
	kernel

LOCAL_CFLAGS += -DSUPPORT_ZERO_COPY

LOCAL_MODULE:= libcamera

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


