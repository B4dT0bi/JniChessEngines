#ifndef IO_H
#define IO_H

#include "Types.h"
#include "Moves.h"
#include "BitBoards.h"
#include "MoveList.h"


void PrintSquare(uint8 sq);
void PrintMove(Move m);
void PrintBitBoard(BitBoard b);
void PrintMoveList(const MoveList &ml);
void PrintBestLine(uint8 depth, __int16 score, Move m);
bool InputAvailable();

#endif