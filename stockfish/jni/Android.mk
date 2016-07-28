LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
 
LOCAL_MODULE    := stockfish
LOCAL_C_INCLUDES := stockfish
 
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -D__ANDROID__ -std=c++11 
LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -D__ANDROID__ -std=c++11 
LOCAL_LDLIBS := -lm -latomic
LOCAL_LDFLAGS += -Wl,--gc-sections
LOCAL_ARM_MODE  := arm
 
LOCAL_SRC_FILES := 	stockfish/benchmark.cpp\
                    stockfish/bitbase.cpp\
                   	stockfish/bitboard.cpp\
                   	stockfish/endgame.cpp\
                   	stockfish/evaluate.cpp\
                   	stockfish/main.cpp\
                   	stockfish/material.cpp\
                   	stockfish/misc.cpp\
                   	stockfish/movegen.cpp\
                   	stockfish/movepick.cpp\
                   	stockfish/pawns.cpp\
                   	stockfish/position.cpp\
                   	stockfish/psqt.cpp\
                   	stockfish/search.cpp\
                   	stockfish/thread.cpp\
                   	stockfish/timeman.cpp\
                   	stockfish/tt.cpp\
                   	stockfish/uci.cpp\
                   	stockfish/ucioption.cpp\
                   	stockfish/syzygy/tbprobe.cpp
 
include $(BUILD_EXECUTABLE)
