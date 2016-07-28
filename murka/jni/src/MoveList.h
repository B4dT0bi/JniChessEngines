#ifndef MoveList_H
#define MoveList_H

#include <iostream>
using namespace std;

#include "Moves.h"
#include "Board.h"
#include "Search.h"
#include "SEE.h"

const uint32 mvHashMove = 4294967295;
const uint32 mvCapture  = 4294900000;
const uint32 mvKiller   = 4294800000;

inline
uint32 MVVLVA(Move m)
{
	return 12 + (m.cap << 4) - Piece[m.from];
}

struct EMove
{
	Move move;
	uint32 value;
};

// ������ ���������������� ���������� - Next Move Stage
const uint8 nmsTrans     = 0; // ��� �� ������� ������������
const uint8 nmsGenCaps   = 1; // ��������� ������
const uint8 nmsGoodCaps  = 2; // ������� ������
const uint8 nmsKiller2   = 3; // ������ ������ (������ �������� ����� ����� ������� ������)
const uint8 nmsGenQuiets = 4; // ��������� ����� �����
const uint8 nmsQuiets    = 5; // ����� ����
const uint8 nmsBadCaps   = 6; // ������ ������

struct MoveList
{
	EMove List[256];
	EMove *pm; // ��������� ��� ���������, ����� ��������� ��������� �� ����� ������
	EMove *cm; // ��������� ��� �������� �����
	uint8 Stage;
	Move Trans;

	EMove BadCaps[16]; // ������ ������
	EMove *pb; // ��������� ��� ������ ������

	void Sort();
	void GenWhiteMoves();
	void GenBlackMoves();
	void GenWhiteCaps(BitBoard t);
	void GenBlackCaps(BitBoard t);
	void GenWhiteEvasions();
	void GenBlackEvasions();
	void GenWhiteQuiets();
	void GenBlackQuiets();
	void GenWhiteChecks(BitBoard t);
	void GenBlackChecks(BitBoard t);

	// ��������� ���� �����
	void GenMoves()
	{
		pm = cm = List;
		if (WTM) GenWhiteMoves();
		else GenBlackMoves();
	}

	// ������ ������
	void GenCaps()
	{
		if (WTM) GenWhiteCaps(BlackPieces);
		else GenBlackCaps(WhitePieces);
	}

	// ������ �� ����
	void GenEvasions()
	{
		pm = cm = List;
		if (WTM) GenWhiteEvasions();
		else GenBlackEvasions();
	}

	// ����� ����
	void GenQuiets()
	{
		pm = cm = List;
		if (WTM) GenWhiteQuiets();
		else GenBlackQuiets();
	}

	// ������ ����
	void GenChecks(BitBoard t)
	{
		pm = cm = List;
		if (WTM) GenWhiteChecks(t);
		else GenBlackChecks(t);
	}

	// ��������� ��� ��� ������������
	Move *NextMove()
	{
		if (cm == pm) return 0;
		return &(cm++)->move;
	}

	void RemoveTrans()
	{
		for (EMove *im = cm; im < pm; im++)
		{
			if (*(uint32*)&im->move == *(uint32*)&Trans)
			{
				*im = *(--pm);
				return;
			}
		}
	}

	void RemoveTransKillers()
	{
		for (EMove *im = cm; im < pm; im++)
		{
			if (*(uint32*)&im->move == *(uint32*)&Trans
				|| *(uint32*)&im->move == *(uint32*)&NI->Killer1
				|| *(uint32*)&im->move == *(uint32*)&NI->Killer2)
			{
				*im = *(--pm);
			}
		}
	}

	// ��������� ��� � ��������������� �������
	Move *NextSortedMove()
	{
		if (cm == pm) return 0;
		EMove *bm = cm;
		for (EMove *im = cm + 1; im != pm; im++)
		{
			if (im->value > bm->value) bm = im;
		}
		
		EMove tmp = *bm;
		*bm = *cm;
		*cm = tmp;

		return &(cm++)->move;
	}

	// ��������� �������������� ��������������� ���
	Move *NextIncMove();

	// ������������� ���������������� ����������
	void IncInit(Move trans)
	{
		Trans = trans;
		Stage = nmsTrans;
	}

	// ������� ��� ������ � ������ ������
	void MoveToTop()
	{
		EMove sm = *(cm - 1);
		for (EMove *mp = cm - 1; mp != List; mp--)
		{
			*mp = *(mp - 1);
		}
		*List = sm;
	}

	// ������� ��� ������� �� ������
	void Remove()
	{
		for (EMove *m = cm; m != pm; m++)
		{
			*(m - 1) = *m;
		}
		cm--;
		pm--;
	}

	// ��������� ���� ��� ����������
	void SetMoveValues(Move hm)
	{
		for (EMove *mp = List; mp != pm; mp++)
		{
			if (MoveCmp(mp->move, hm)) mp->value = mvHashMove;
			else if (mp->move.cap) mp->value = mvCapture + MVVLVA(mp->move);
			else if (MoveCmp(mp->move, NI->Killer1)) mp->value = mvKiller;
			else if (MoveCmp(mp->move, NI->Killer2)) mp->value = mvKiller - 1;
			else mp->value = History[Piece[mp->move.from]][mp->move.to];
		}
	}

	// ��������� ���� ��� �����
	void SetEvasionValues(Move hm)
	{
		for (EMove *mp = List; mp != pm; mp++)
		{
			if (MoveCmp(mp->move, hm)) mp->value = mvHashMove;
			else if (mp->move.cap) mp->value = mvCapture + MVVLVA(mp->move);
			else if (MoveCmp(mp->move, NI->Killer1)) mp->value = mvKiller;
			else if (MoveCmp(mp->move, NI->Killer2)) mp->value = mvKiller - 1;
			else mp->value = History[Piece[mp->move.from]][mp->move.to];
		}
	}

	// ��������� ������ � ��
	void SetQMoveValues()
	{
		for (EMove *mp = List; mp != pm; mp++)
		{
			mp->value = MVVLVA(mp->move);
		}
	}

	// ������ ��������� �� ���� � ��
	void SetQEvasionValues()
	{
		for (EMove *mp = List; mp != pm; mp++)
		{
			mp->value = MVVLVA(mp->move) * mp->move.cap;
		}
	}

	// ��������� ���
	void AddMove(uint8 from, uint8 to, uint8 f = 0)
	{
		pm->move.from = from;
		pm->move.to = to;
		pm->move.cap = Piece[to];
		pm->move.flags = f;
		pm++;
	}

	// ��������� ����� ���
	void AddMoveQuiet(uint8 from, uint8 to, uint8 f = 0)
	{
		pm->move.from = from;
		pm->move.to = to;
		pm->move.cap = Piece[to];
		pm->move.flags = f;
		pm->value = History[Piece[from]][to];
		pm++;
	}

	// ��������� ������ �� �������
	void AddMoveEP(uint8 from, uint8 to, uint8 cap)
	{
		pm->move.from = from;
		pm->move.to = to;
		pm->move.cap = cap;
		pm->move.flags = mfEP;
		pm++;
	}
};

#endif