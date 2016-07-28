#ifndef Hash_H
#define Hash_H

#include "Types.h"
#include "Moves.h"

void SetTransSize(uint16 size);
void InitHashKeys();
void CalcHashKey();
void TransStore(__int16 score, uint8 depth, uint8 ply, uint8 flags, Move m);

const uint8 tfLowerBound = 1;
const uint8 tfExact      = 2;
const uint8 tfUpperBound = 4;

struct PawnHashEntry
{
	uint64 Key;
	__int16 op;
	__int16 eg;

	__int16 ShieldWK;
	__int16 ShieldWQ;
	__int16 ShieldWC;
	__int16 ShieldBK;
	__int16 ShieldBQ;
	__int16 ShieldBC;
	BitBoard passersw, passersb;
};

const uint32 PawnHashMask = 0x7ff;
extern PawnHashEntry *PawnHash;

struct TransEntry
{
	uint64 Key;
	Move Move;
	__int16 Score;
	uint8 Flags;
	uint8 Depth;

#ifdef TEST
	uint64 Nodes;
#endif
};

extern uint32 HashMask;
extern TransEntry *Trans;
extern uint64 HashKeys[13][64];
extern uint64 HashKeysEP[64];
extern uint64 HashKeysCR[16];
extern uint64 HashKeyWTM;

#endif
