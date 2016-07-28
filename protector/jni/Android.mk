LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
 
LOCAL_MODULE    := senpai
LOCAL_C_INCLUDES := senpai
 
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -fexceptions -D__ANDROID__ -std=c99
LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -fexceptions -D__ANDROID__ -std=c++11
LOCAL_LDLIBS := -lm -latomic
LOCAL_LDFLAGS += -Wl,--gc-sections
LOCAL_ARM_MODE  := arm
 
LOCAL_SRC_FILES := 	src/bitboard.c\
                    src/book.c\
                    src/coordination.c\
                    src/egtb.c\
                    src/evaluation.c\
                    src/fen.c\
                    src/hash.c\
                    src/io.c\
                    src/keytable.c\
                    src/matesearch.c\
                    src/movegeneration.c\
                    src/pgn.c\
                    src/position.c\
                    src/protector.c\
                    src/search.c\
                    src/tablebase.c\
                    src/tbdecode.c\
                    src/test.c\
                    src/tools.c\
                    src/xboard.c
 
include $(BUILD_EXECUTABLE)
