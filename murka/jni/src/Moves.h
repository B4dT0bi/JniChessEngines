#ifndef Moves_H
#define Moves_H

#include "BitBoards.h"
#include "Board.h"

// ����� ����� - Move Flag
const uint8 mfNone     = 0;    // ������� ���
const uint8 mfPromo    = 0x0f; // �����������
const uint8 mfCastle   = 0x10; // ���������
const uint8 mfEP       = 0x20; // ������ �� �������
const uint8 mfPawnJump = 0x40; // ������ ����� ����� ������ (������) ���

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

// ������ ���
inline bool MakeMove(Move m)
{
	return WTM? MakeWhiteMove(m) : MakeBlackMove(m);
};

// ����� ����
inline void UnmakeMove(Move m)
{
	if (WTM) UnmakeBlackMove(m);
	else UnmakeWhiteMove(m);
}

// �������� ����������� ����
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
