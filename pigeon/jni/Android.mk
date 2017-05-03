LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := pigeon
LOCAL_C_INCLUDES := pigeon/src/
 
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -w -mpopcnt -D__ANDROID__ -DTB -DW32_BUILD -DNO_SSE -DTB_NO_HW_POP_COUNT -DTB_CUSTOM_LSB -std=c++11
LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -w -mpopcnt -D__ANDROID__ -DTB -DW32_BUILD -DNO_SSE -DTB_NO_HW_POP_COUNT -DTB_CUSTOM_LSB -std=c++11
LOCAL_LDLIBS := -lm -latomic
LOCAL_LDFLAGS += -Wl,--gc-sections
LOCAL_ARM_MODE  := arm
 
LOCAL_SRC_FILES := 	pigeon/src/pigeon.cpp

include $(BUILD_EXECUTABLE)
