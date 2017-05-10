LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := pigeon
LOCAL_C_INCLUDES := pigeon/src/
 
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -D__ANDROID__ -w -fPIE -fexceptions -std=c++11
LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -D__ANDROID__ -w -fPIE -fexceptions -std=c++11
LOCAL_LDFLAGS += -fPIE -pie

LOCAL_ARM_MODE  := arm
 
LOCAL_SRC_FILES := 	pigeon/src/pigeon.cpp

include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)

LOCAL_MODULE    := pigeon-noPIE
LOCAL_C_INCLUDES := pigeon/src/

LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -D__ANDROID__ -w -fexceptions -std=c++11
LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -D__ANDROID__ -w -fexceptions -std=c++11

LOCAL_ARM_MODE  := arm

LOCAL_SRC_FILES := 	pigeon/src/pigeon.cpp

include $(BUILD_EXECUTABLE)
