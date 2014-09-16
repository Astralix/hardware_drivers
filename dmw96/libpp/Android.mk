# Copyright (C) 2008 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:= $(call my-dir)
# HAL module implemenation, not prelinked and stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so

include $(CLEAR_VARS)

DECODER_RELEASE := $(LOCAL_PATH)/../g1_decoder

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils

LOCAL_SRC_FILES := PostProcessor.cpp

LOCAL_C_INCLUDES := $(DECODER_RELEASE)/source/inc

LOCAL_STATIC_LIBRARIES := libx170pp.standalone \
			  libg1common \
                          libdwlx170

# Uncomment to enable post-processor delay measurements
#LOCAL_CFLAGS	+= -DTIME_PP

# Always maintain input picture aspect ratio using FILL
LOCAL_CFLAGS	+= -DMAINTAIN_ASPECT_RATIO

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := libpp.dspg

include $(BUILD_SHARED_LIBRARY)
