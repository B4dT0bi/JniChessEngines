#ifndef Protocols_H
#define Protocols_H

#include <fstream>
using namespace std;

#include "Moves.h"

#ifdef _M_X64
#define MY_NAME "Murka 3 x64"
#else
#define MY_NAME "Murka 3 w32"
#endif

void UCI();
void XBoard();
void MoveToStr(Move m, char *str);
void OpenLog(char *mode);
Move StrToMove(char *str);

extern bool Log;
extern ofstream Flog;
extern bool PostMode;
extern bool UciMode;
extern bool XBoardMode;
extern bool AnalyzeMode;
extern char AnalyzeCommand[128];

#endif
