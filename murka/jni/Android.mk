LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
 
LOCAL_MODULE    := murka
LOCAL_C_INCLUDES := src
 
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -fexceptions -D__ANDROID__ -std=c++11
LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -Wall -fexceptions -D__ANDROID__ -std=c++11
LOCAL_LDLIBS := -lm -latomic
LOCAL_LDFLAGS += -Wl,--gc-sections
LOCAL_ARM_MODE  := arm
 
LOCAL_SRC_FILES := 	        src/BitBoards.cpp\
                            src/Board.cpp\
                            src/Eval.cpp\
                            src/EvalValues.cpp\
                            src/Hash.cpp\
                            src/IO.cpp\
                            src/Main.cpp\
                            src/MoveList.cpp\
                            src/Moves.cpp\
                            src/Protocols.cpp\
                            src/Search.cpp\
                            src/SEE.cpp\
                            src/Test.cpp\
                            src/TimeManager.cpp

 
include $(BUILD_EXECUTABLE)
