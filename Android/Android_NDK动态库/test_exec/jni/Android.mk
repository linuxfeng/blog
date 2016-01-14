LOCAL_PATH := $(call my-dir)

##对引用的动态库进行设置
include $(CLEAR_VARS)
LOCAL_MODULE    := libandroidNdkShare
#LOCAL_SRC_FILES，引用的动态库的位置
LOCAL_SRC_FILES := ../../share_library/libs/armeabi-v7a/libandroidNdkShare.so
#引用动态库的头文件
LOCAL_EXPORT_C_INCLUDES := ../share_library/jni/include 
include $(PREBUILT_SHARED_LIBRARY)

##编译可执行文件
include $(CLEAR_VARS)  
LOCAL_MODULE := test_exec 
LOCAL_SHARED_LIBRARIES := libandroidNdkShare
LOCAL_LDLIBS    := -llog #日志库
LOCAL_SRC_FILES += $(notdir $(wildcard $(LOCAL_PATH)/*.c))
LOCAL_C_INCLUDES := $(INCLUDES)
include $(BUILD_EXECUTABLE)

