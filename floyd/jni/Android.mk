LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE    := floyd
LOCAL_C_INCLUDES := floyd/Source/
 
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -w -mpopcnt -D__ANDROID__ -DTB -DW32_BUILD -DNO_SSE -DTB_NO_HW_POP_COUNT -DTB_CUSTOM_LSB -DfloydVersion=0.9 -std=c11
LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -w -mpopcnt -D__ANDROID__ -DTB -DW32_BUILD -DNO_SSE -DTB_NO_HW_POP_COUNT -DTB_CUSTOM_LSB -DfloydVersion=0.9 -std=c11
LOCAL_LDLIBS := -lm -latomic
LOCAL_LDFLAGS += -Wl,--gc-sections
LOCAL_ARM_MODE  := arm
 
LOCAL_SRC_FILES := 	floyd/Source/cplus.c\
                    floyd/Source/engine.c\
                    floyd/Source/evaluate.c\
                    floyd/Source/floydmain.c\
                    floyd/Source/format.c\
                    floyd/Source/kpk.c\
                    floyd/Source/moves.c\
                    floyd/Source/parse.c\
                    floyd/Source/search.c\
                    floyd/Source/test.c\
                    floyd/Source/ttable.c\
                    floyd/Source/uci.c\
                    floyd/Source/zobrist.c

include $(BUILD_EXECUTABLE)
