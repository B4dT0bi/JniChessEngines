#define _CRT_RAND_S

#include <stdlib.h>
#include <iostream>
using namespace std;

#include "Types.h"
#include "Board.h"
#include "Search.h"
#include "Hash.h"
#include "Moves.h"
#include "Protocols.h"

#ifdef AUTOTUNING
uint32 HashMask = 0xffff;
#else
uint32 HashMask = 0x7fffff;
#endif

// таблица перестановок
TransEntry *Trans;

// хеш-ключи
uint64 HashKeys[13][64]; // ключи для фигур
uint64 HashKeysEP[64];   // ключи для взятия на проходе
uint64 HashKeysCR[16];   // ключи для прав на рокировки
uint64 HashKeyWTM;       // ключ  для очередности хода

// случайное 64-битное число
uint64 Rand64()
{
	uint64 res = 0;

	for (int i = 0; i < 64; i++)
	{
		res <<= 1;
		res |= rand() & 1;
	}
	return res;
}

// инициализируем хеш
void InitHashKeys()
{
	uint8 i, j;

	for (i = 1; i < 13; i++) for (j = 0; j < 64; j++) HashKeys[i][j] = Rand64();
	for (i = 0; i < 8; i++)
	{
		HashKeysEP[A6 + i] = Rand64();
		HashKeysEP[A3 + i] = Rand64();
	}
	for (i = 0; i < 16; i++)
	{
		HashKeysCR[i] = Rand64();
	}
	HashKeyWTM = Rand64();
}

// вычисляем хеш-ключ
void CalcHashKey()
{
	uint8 i;

	NI->HashKey = 0;
	NI->PawnHashKey = 0;
	for (i = 0; i < 64; i++)
	{
		if (Piece[i]) NI->HashKey ^= HashKeys[Piece[i]][i];
		if (Piece[i] == ptWhitePawn || Piece[i] == ptBlackPawn)
			NI->PawnHashKey ^= HashKeys[Piece[i]][i];
	}
	NI->HashKey ^= HashKeysCR[NI->CastleRights];
	NI->HashKey ^= HashKeysEP[NI->EP];
	if (WTM) NI->HashKey ^= HashKeyWTM;
}

void SetTransSize(uint16 size)
{
	if (size > 1024) size = 1024;
	HashMask = 1;
	while (HashMask * sizeof(TransEntry) < size * 1024 * 1024)
	{
		HashMask <<= 1;
		HashMask |= 1;
	}
	if (HashMask * sizeof(TransEntry) > size * 1024 * 1024) HashMask >>= 1;
	if (Trans) delete[] Trans;
	Trans = new TransEntry[HashMask + 1];
	memset(Trans, 0, sizeof(TransEntry) * (HashMask + 1));
}

// сохраняем инфу по перебору в таблице перестановок
void TransStore(__int16 score, uint8 depth, uint8 ply, uint8 flags, Move m)
{
	TransEntry *he = Trans + (NI->HashKey & HashMask);
	he->Key = NI->HashKey;
	he->Move = m;
	he->Flags = flags;
	// коррекция матовой оценки
	if (score > 32000) he->Score = score + ply;
	else if (score < -32000) he->Score = score - ply;
	else he->Score = score;
	he->Depth = depth;
}
