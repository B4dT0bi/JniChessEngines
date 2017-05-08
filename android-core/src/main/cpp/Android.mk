LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := nHelper

LOCAL_SRC_FILES := 	nHelper.cpp

include $(BUILD_SHARED_LIBRARY)
