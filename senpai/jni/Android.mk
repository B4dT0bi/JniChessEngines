LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
 
LOCAL_MODULE    := senpai
LOCAL_C_INCLUDES := senpai
 
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -fexceptions -D__ANDROID__ -std=c++11
LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -fexceptions -D__ANDROID__ -std=c++11
LOCAL_LDLIBS := -lm -latomic
LOCAL_LDFLAGS += -Wl,--gc-sections
LOCAL_ARM_MODE  := arm
 
LOCAL_SRC_FILES := 	src/senpai_10.cpp
 
include $(BUILD_EXECUTABLE)
