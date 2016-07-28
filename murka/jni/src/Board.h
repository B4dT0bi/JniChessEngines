#ifndef Board_H
#define Board_H

#include "BitBoards.h"

extern bool WTM;
extern uint8 Piece[64];
extern char PieceLetter[];

// прототипы
//----------

void SetBoard(char *fen);
void PrintBoard();
bool TestBoard();
uint8 AttackedByWhite(uint8 sq);
uint8 AttackedByBlack(uint8 sq);
void InvertBoard();
bool Mate();
bool Stalemate();
bool Repetition3();
bool Rule50Moves();
bool InsufficientMaterial();
void Base(char *pgn, int n);

// типы фигур
//-----------

const uint8 ptNone = 0;
const uint8 ptWhitePawn = 1;
const uint8 ptWhiteKnight = 2;
const uint8 ptWhiteBishop = 3;
const uint8 ptWhiteRook = 4;
const uint8 ptWhiteQueen = 5;
const uint8 ptWhiteKing = 6;
const uint8 ptBlackPawn = 7;
const uint8 ptBlackKnight = 8;
const uint8 ptBlackBishop = 9;
const uint8 ptBlackRook = 10;
const uint8 ptBlackQueen = 11;
const uint8 ptBlackKing = 12;

// права на рокировку
//-------------------

const uint8 crWhiteKingside  = 1;
const uint8 crWhiteQueenside = 2;
const uint8 crBlackKingside  = 4;
const uint8 crBlackQueenside = 8;

// пол€
//-----

const uint8 A8 =  0;
const uint8 B8 =  1;
const uint8 C8 =  2;
const uint8 D8 =  3;
const uint8 E8 =  4;
const uint8 F8 =  5;
const uint8 G8 =  6;
const uint8 H8 =  7;
const uint8 A7 =  8;
const uint8 B7 =  9;
const uint8 C7 = 10;
const uint8 D7 = 11;
const uint8 E7 = 12;
const uint8 F7 = 13;
const uint8 G7 = 14;
const uint8 H7 = 15;
const uint8 A6 = 16;
const uint8 B6 = 17;
const uint8 D6 = 19;
const uint8 E6 = 20;
const uint8 G6 = 22;
const uint8 B5 = 25;
const uint8 D5 = 27;
const uint8 E5 = 28;
const uint8 G5 = 30;
const uint8 B4 = 33;
const uint8 D4 = 35;
const uint8 E4 = 36;
const uint8 G4 = 38;
const uint8 A3 = 40;
const uint8 B3 = 41;
const uint8 D3 = 43;
const uint8 E3 = 44;
const uint8 G3 = 46;
const uint8 A2 = 48;
const uint8 B2 = 49;
const uint8 C2 = 50;
const uint8 D2 = 51;
const uint8 E2 = 52;
const uint8 F2 = 53;
const uint8 G2 = 54;
const uint8 H2 = 55;
const uint8 A1 = 56;
const uint8 B1 = 57;
const uint8 C1 = 58;
const uint8 D1 = 59;
const uint8 E1 = 60;
const uint8 F1 = 61;
const uint8 G1 = 62;
const uint8 H1 = 63;

// зеркальное отображение полей
const uint8 Mirror[64] =
{
	56, 57, 58, 59, 60, 61, 62, 63,
	48, 49, 50, 51, 52, 53, 54, 55,
	40, 41, 42, 43, 44, 45, 46, 47,
	32, 33, 34, 35, 36, 37, 38, 39,
	24, 25, 26, 27, 28, 29, 30, 31,
	16, 17, 18, 19, 20, 21, 22, 23,
	 8,  9, 10, 11, 12, 13, 14, 15,
	 0,  1,  2,  3,  4,  5,  6,  7
};

// инлайновые функции
//-------------------

inline uint8 File(uint8 sq)
{
	return sq & 7;
}

// ќбратна€ нумераци€ р€дов
// 0 - последний р€д, 7 - первый
inline uint8 Rank(uint8 sq)
{
	return sq >> 3;
}

// бела€ фигура?
inline uint8 WhitePiece(uint8 p)
{
	return p > 0 && p <= ptWhiteKing;
}

inline uint8 BlackPiece(uint8 p)
{
	return p >= ptBlackPawn;
}

// белые под шахом?
inline uint8 WhiteInCheck()
{
	return AttackedByBlack(LSB(WhiteKing));
}

// черные под шахом?
inline uint8 BlackInCheck()
{
	return AttackedByWhite(LSB(BlackKing));
}

// объ€влен ли шах
inline uint8 InCheck()
{
	return WTM? WhiteInCheck() : BlackInCheck();
}

#endif
