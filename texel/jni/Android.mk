LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
 
LOCAL_MODULE    := texel
LOCAL_C_INCLUDES := src src/gtb src/util src/syzygy src/gtb/sysport src/gtb/compression src/gtb/compression/lzma
 
LOCAL_CFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -fexceptions -Wall -D__ANDROID__ -std=c99
LOCAL_CPPFLAGS := $(LOCAL_C_INCLUDES:%=-I%) -O2 -fexceptions -Wall -frtti -D__ANDROID__ -std=c++11
LOCAL_LDLIBS := -lm -latomic
LOCAL_LDFLAGS += -Wl,--gc-sections
LOCAL_ARM_MODE  := arm
 
LOCAL_SRC_FILES := 	        src/gtb/gtb-att.c\
                            src/gtb/gtb-dec.c\
                            src/gtb/gtb-probe.c\
                            src/gtb/compression/wrap.c\
                            src/gtb/compression/lzma/Alloc.c\
                            src/gtb/compression/lzma/Bra86.c\
                            src/gtb/compression/lzma/LzFind.c\
                            src/gtb/compression/lzma/Lzma86Dec.c\
                            src/gtb/compression/lzma/Lzma86Enc.c\
                            src/gtb/compression/lzma/LzmaDec.c\
                            src/gtb/compression/lzma/LzmaEnc.c\
                            src/gtb/sysport/sysport.c\
        src/bitBoard.cpp\
        src/book.cpp\
        src/computerPlayer.cpp\
        src/endGameEval.cpp\
        src/enginecontrol.cpp\
        src/evaluate.cpp\
        src/game.cpp\
        src/history.cpp\
        src/humanPlayer.cpp\
        src/killerTable.cpp\
        src/kpkTable.cpp\
        src/krkpTable.cpp\
        src/krpkrTable.cpp\
        src/material.cpp\
        src/move.cpp\
        src/moveGen.cpp\
        src/numa.cpp\
        src/parallel.cpp\
        src/parameters.cpp\
        src/piece.cpp\
        src/position.cpp\
        src/search.cpp\
        src/tbprobe.cpp\
        src/texel.cpp\
        src/textio.cpp\
        src/transpositionTable.cpp\
        src/treeLogger.cpp\
        src/tuigame.cpp\
        src/uciprotocol.cpp\
        src/syzygy/rtb-probe.cpp\
        src/util/logger.cpp\
        src/util/random.cpp\
        src/util/timeUtil.cpp\
        src/util/util.cpp

 
include $(BUILD_EXECUTABLE)
