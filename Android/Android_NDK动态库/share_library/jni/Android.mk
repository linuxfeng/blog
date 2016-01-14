LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libandroidNdkShare
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.c))
LOCAL_LDLIBS    := -llog #日志库
#LOCAL_C_INCLUDES += $(NDK_HOME)platforms/android-9/arch-arm/usr/include/
include $(BUILD_SHARED_LIBRARY)


