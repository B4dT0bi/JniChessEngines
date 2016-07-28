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

// стадии инкрементального генератора - Next Move Stage
const uint8 nmsTrans     = 0; // ход из таблицы перестановок
const uint8 nmsGenCaps   = 1; // генерация взятий
const uint8 nmsGoodCaps  = 2; // хорошие взятия
const uint8 nmsKiller2   = 3; // второй киллер (первый выдается сразу после хороших взятий)
const uint8 nmsGenQuiets = 4; // генерация тихих ходов
const uint8 nmsQuiets    = 5; // тихие ходы
const uint8 nmsBadCaps   = 6; // плохие взятия

struct MoveList
{
	EMove List[256];
	EMove *pm; // указатель для генерации, после генерации указывает на конец списка
	EMove *cm; // указатель для перебора ходов
	uint8 Stage;
	Move Trans;

	EMove BadCaps[16]; // плохие взятия
	EMove *pb; // указатель для плохих взятий

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

	// генерация всех ходов
	void GenMoves()
	{
		pm = cm = List;
		if (WTM) GenWhiteMoves();
		else GenBlackMoves();
	}

	// только взятия
	void GenCaps()
	{
		if (WTM) GenWhiteCaps(BlackPieces);
		else GenBlackCaps(WhitePieces);
	}

	// защиты от шаха
	void GenEvasions()
	{
		pm = cm = List;
		if (WTM) GenWhiteEvasions();
		else GenBlackEvasions();
	}

	// тихие ходы
	void GenQuiets()
	{
		pm = cm = List;
		if (WTM) GenWhiteQuiets();
		else GenBlackQuiets();
	}

	// только шахи
	void GenChecks(BitBoard t)
	{
		pm = cm = List;
		if (WTM) GenWhiteChecks(t);
		else GenBlackChecks(t);
	}

	// следующий ход без упорядочения
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

	// следующий ход в отсортированном порядке
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

	// следующий инкрементально сгенерированный ход
	Move *NextIncMove();

	// инициализация инкрементального генератора
	void IncInit(Move trans)
	{
		Trans = trans;
		Stage = nmsTrans;
	}

	// текущий ход ставим в начало списка
	void MoveToTop()
	{
		EMove sm = *(cm - 1);
		for (EMove *mp = cm - 1; mp != List; mp--)
		{
			*mp = *(mp - 1);
		}
		*List = sm;
	}

	// текущий ход удаляем из списка
	void Remove()
	{
		for (EMove *m = cm; m != pm; m++)
		{
			*(m - 1) = *m;
		}
		cm--;
		pm--;
	}

	// оцениваем ходы для сортировки
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

	// оцениваем ходы под шахом
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

	// оцениваем взятия в ФВ
	void SetQMoveValues()
	{
		for (EMove *mp = List; mp != pm; mp++)
		{
			mp->value = MVVLVA(mp->move);
		}
	}

	// оценка уклонений от шаха в ФВ
	void SetQEvasionValues()
	{
		for (EMove *mp = List; mp != pm; mp++)
		{
			mp->value = MVVLVA(mp->move) * mp->move.cap;
		}
	}

	// добавляем ход
	void AddMove(uint8 from, uint8 to, uint8 f = 0)
	{
		pm->move.from = from;
		pm->move.to = to;
		pm->move.cap = Piece[to];
		pm->move.flags = f;
		pm++;
	}

	// добавляем тихий ход
	void AddMoveQuiet(uint8 from, uint8 to, uint8 f = 0)
	{
		pm->move.from = from;
		pm->move.to = to;
		pm->move.cap = Piece[to];
		pm->move.flags = f;
		pm->value = History[Piece[from]][to];
		pm++;
	}

	// добавляем взятие на проходе
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