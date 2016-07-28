#ifndef Moves_H
#define Moves_H

#include "BitBoards.h"
#include "Board.h"

// флаги ходов - Move Flag
const uint8 mfNone     = 0;    // обычный ход
const uint8 mfPromo    = 0x0f; // превращение
const uint8 mfCastle   = 0x10; // рокировка
const uint8 mfEP       = 0x20; // взятие на проходе
const uint8 mfPawnJump = 0x40; // прыжок пешки через третий (шестой) ряд

struct Move
{
	uint8 from;
	uint8 to;
	uint8 flags;
	uint8 cap;
};

inline
bool MoveCmp(Move m1, Move m2)
{
	return *(uint32*)&m1 == *(uint32*)&m2;
}

bool MakeWhiteMove(Move m);
bool MakeBlackMove(Move m);
void UnmakeWhiteMove(Move m);
void UnmakeBlackMove(Move m);

// делаем ход
inline bool MakeMove(Move m)
{
	return WTM? MakeWhiteMove(m) : MakeBlackMove(m);
};

// откат хода
inline void UnmakeMove(Move m)
{
	if (WTM) UnmakeBlackMove(m);
	else UnmakeWhiteMove(m);
}

// проверка возможности хода
bool ProperWhiteMove(Move m);
bool ProperBlackMove(Move m);

inline bool ProperMove(Move m)
{
	if (WTM) return ProperWhiteMove(m);
	return ProperBlackMove(m);
}

void MakeNullMove();
void UnmakeNullMove();

#endif
