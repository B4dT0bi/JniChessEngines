LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
 
LOCAL_MODULE    := ethereal
LOCAL_C_INCLUDES := ethereal
 
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -D__ANDROID__ -std=c99
LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -D__ANDROID__ -std=c99
LOCAL_LDLIBS := -lm
LOCAL_ARM_MODE  := arm
 
LOCAL_SRC_FILES := 	ethereal/bitboards.c       \
                    ethereal/bitutils.c        \
                    ethereal/board.c           \
                    ethereal/castle.c          \
                    ethereal/evaluate.c        \
                    ethereal/magics.c          \
                    ethereal/masks.c           \
                    ethereal/move.c            \
                    ethereal/movegen.c         \
                    ethereal/psqt.c            \
                    ethereal/search.c          \
                    ethereal/time.c            \
                    ethereal/transposition.c   \
                    ethereal/uci.c             \
                    ethereal/zorbist.c
 
include $(BUILD_EXECUTABLE)
