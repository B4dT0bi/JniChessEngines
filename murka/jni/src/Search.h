#ifndef Search_H
#define Search_H

#include "Types.h"
#include "Moves.h"

Move RootSearch();
inline bool Repetition();
__int16 QuiescenceSearch(__int16 alpha, __int16 beta, uint8 gencheck);

// информация об узлах дерева перебора
struct NodeInfo
{
	// хеш-ключи
	uint64 HashKey;
	uint64 PawnHashKey;

	// оценки материала и положения фигур
	__int32 OP;
	__int32 EG;

	// Киллеры
	Move Killer1, Killer2;

	// сигнатура материала
	uint32 MS; // Material Signature

	// права на рокировки
	uint8 CastleRights;
	// поле для взятия на проходе
	uint8 EP;
	// объявлен шах
	uint8 InCheck;

	uint8 Reversible;

	Move move;
	__int16 eval;
};

// оценка мата
const __int16 MATE = 0x7fff;

extern uint8 DepthLimit;
extern uint64 NodesLimit;
extern uint64 Nodes;
extern NodeInfo *NI;
extern NodeInfo NodesInfo[1024];
extern uint32 History[13][64];
extern Move NullMove;

#endif
