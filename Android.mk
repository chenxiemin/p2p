LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
# Use the sub library as a prebuilt shared library
LOCAL_MODULE := p2p
LOCAL_SRC_FILES := ./build.android/common/libcommon.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/common $(LOCAL_PATH)/util
include $(PREBUILT_STATIC_LIBRARY)

