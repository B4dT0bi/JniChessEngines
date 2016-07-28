LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
 
LOCAL_MODULE    := crafty
LOCAL_C_INCLUDES := src
 
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -DUNIX -fexceptions -D__ANDROID__ -std=c99
LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -DUNIX -fexceptions -D__ANDROID__ -std=c++11
LOCAL_LDLIBS := -lm -latomic
LOCAL_LDFLAGS += -Wl,--gc-sections
LOCAL_ARM_MODE  := arm
 
LOCAL_SRC_FILES := 	        src/analyze.c\
                            src/annotate.c\
                            src/attacks.c\
                            src/autotune.c\
                            src/bench.c\
                            src/book.c\
                            src/boolean.c\
                            src/crafty.c\
                            src/data.c\
                            src/drawn.c\
                            src/edit.c\
                            src/epd.c\
                            src/epdglue.c\
                            src/evaluate.c\
                            src/evtest.c\
                            src/hash.c\
                            src/history.c\
                            src/init.c\
                            src/input.c\
                            src/interrupt.c\
                            src/iterate.c\
                            src/learn.c\
                            src/main.c\
                            src/make.c\
                            src/movgen.c\
                            src/next.c\
                            src/option.c\
                            src/output.c\
                            src/ponder.c\
                            src/probe.c\
                            src/quiesce.c\
                            src/repeat.c\
                            src/resign.c\
                            src/root.c\
                            src/search.c\
                            src/see.c\
                            src/setboard.c\
                            src/test.c\
                            src/thread.c\
                            src/time.c\
                            src/unmake.c\
                            src/utility.c\
                            src/validate.c\
                            src/egtb.cpp

 
include $(BUILD_EXECUTABLE)
