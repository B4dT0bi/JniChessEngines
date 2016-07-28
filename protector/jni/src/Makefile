#TARGET=WINDOWS_32
#TARGET=WINDOWS_64
#TARGET=LINUX_64

EXECUTABLE=".\Protector.exe"
TB_PATH="C:\Tablebases"
MAKE=make
CC=gcc
CPPC=g++
LN=g++ -m64
TARGET_FLAGS=-m64 -O3
CFLAGS=-Wall -ansi -pedantic -Wno-long-long -pthread -flto -msse -DNDEBUG
CLEAN_CMD=del /f
INDENT="C:\Program Files (x86)\GnuWin32\bin\indent.exe"
REMOVE_CRLF=
RESTORE_RCLF=

ifeq ($(TARGET),WINDOWS_32)
   MAKE=mingw32-make
   TARGET_FLAGS=-m32 -O2
   LN=g++ -m32
endif

ifeq ($(TARGET),WINDOWS_64)
   MAKE=mingw32-make
endif

ifeq ($(TARGET),LINUX_64)
   CLEAN_CMD=@rm -f
   INDENT=indent
   CFLAGS=-O3 -Wall -Wunused -Wuninitialized -Wimplicit-function-declaration \
          -Wmissing-prototypes -Wmissing-declarations -ansi -pedantic \
          -Wno-long-long -DNDEBUG -DTARGET_LINUX -DGCC_INLINE_STYLE \
		  -Wno-unused-result -std=c99
    REMOVE_CRLF=dos2unix *.c *.h
    RESTORE_CRLF=unix2dos *.c *.h
    EXECUTABLE="./Protector"
    TB_PATH="/tablebases"
endif

protectordbg: CFLAGS=-Wall -ansi -pedantic -Wno-long-long -pthread

protectorobjects = protector.o bitboard.o position.o fen.o io.o \
                   movegeneration.o matesearch.o keytable.o hash.o test.o \
                   pgn.o tools.o search.o evaluation.o coordination.o \
                   xboard.o book.o tablebase.o

protectorsources = $(protectorobjects:.o=.c) egtb.cpp

.c.o:
	$(CC) $(TARGET_FLAGS) $(CFLAGS) -c $*.c

.cpp.o:
	$(CPPC) $(TARGET_FLAGS) $(CPPFLAGS) -flto -c $*.cpp

protector: $(protectorobjects) egtb.o
	$(CLEAN_CMD) $(EXECUTABLE)
	$(LN) -lstdc++ -lm -pthread -static -flto -o Protector $(protectorobjects) egtb.o

protectordbg: $(protectorobjects) egtb.o
	$(CLEAN_CMD) $(EXECUTABLE)
	$(LN) -lstdc++ -lm -pthread -static -flto -o Protector $(protectorobjects) egtb.o

protector_pgogen: $(protectorobjects) egtb.o
	$(CLEAN_CMD) $(EXECUTABLE)
	$(LN) -lstdc++ -lm -pthread -static -flto -fprofile-generate -o Protector $(protectorobjects) egtb.o

protector_pgouse: $(protectorobjects) egtb.o
	$(CLEAN_CMD) $(EXECUTABLE)
	$(LN) -lstdc++ -lm -pthread -static -flto -fprofile-use -o Protector $(protectorobjects) egtb.o

all:
	$(MAKE) clean
	$(MAKE) protector
	$(MAKE) clean

debug:
	$(MAKE) clean
	$(MAKE) protectordbg
	$(MAKE) clean

protectorpgo:
	$(MAKE) clean
	$(MAKE) CFLAGS='-fprofile-generate -Wall -ansi -pedantic -Wno-long-long -pthread -flto -msse -DNDEBUG' CPPFLAGS='-fprofile-generate' protector_pgogen
	$(CLEAN_CMD) *.o
	$(EXECUTABLE) -e $(TB_PATH) -t pgo.pgn
	$(CLEAN_CMD) protector Protector.exe
	$(MAKE) CFLAGS='$(TARGET_FLAGS) -fprofile-use -Wall -ansi -pedantic -Wno-long-long -pthread -flto -msse -DNDEBUG' CPPFLAGS='-fprofile-use' protector_pgouse
	$(MAKE) clean

indent:
	$(REMOVE_CRLF)
	$(INDENT) -bad -bap -sob -npsl -npcs -bl -bli0 -i3 -ci3 -nut -nbbo *.c *.h
	$(RESTORE_CRLF)
	$(CLEAN_CMD) *.*~

.PHONY: clean
clean:
	$(CLEAN_CMD) *.o *.gcda *.bak *.bin *.lrn game.* log.* *~ .#*