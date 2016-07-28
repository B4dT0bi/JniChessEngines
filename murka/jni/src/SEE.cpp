#include <stdlib.h>

#include "Types.h"
#include "BitBoards.h"
#include "Moves.h"
#include "Eval.h"
#include "IO.h"

// ценности фигур для СОР (статическая оценка разменов)
// Simple Piece Value
uint8 SPV[13] =
{
    0,
	 1, 3, 3, 5, 10, 100,
	 1, 3, 3, 5, 10, 100
};
uint8 SPV2[13] =
{
    0,
	 1, 2, 3, 5, 10, 100,
	 1, 2, 3, 5, 10, 100
};

__int8 Dirs[64][64];
uint8 SqOnLine[64][64];
uint8 DirType[64][64];
const uint8 dtBishop = 1;
const uint8 dtRook = 2;

void InitSEE()
{
	int i, j;

	for (i = 0; i < 64; i++)
	{
		for (j = 0; j < 64; j++)
		{
			if (j == i)
			{
				Dirs[i][j] = SqOnLine[i][j] = DirType[i][j] = 0;
				continue;
			}
			if (File(i) == File(j))
			{
				DirType[i][j] = dtRook;
				if (i < j)
				{
					Dirs[i][j] = 8;
					SqOnLine[i][j] = 7 - Rank(j);
				}
				else
				{
					Dirs[i][j] = -8;
					SqOnLine[i][j] = Rank(j);
				}
			}
			else if (Rank(i) == Rank(j))
			{
				DirType[i][j] = dtRook;
				if (i < j)
				{
					Dirs[i][j] = 1;
					SqOnLine[i][j] = 7 - File(j);
				}
				else
				{
					Dirs[i][j] = -1;
					SqOnLine[i][j] = File(j);
				}
			}
			else if (File(i) - File(j) == Rank(i) - Rank(j))
			{
				DirType[i][j] = dtBishop;
				if (i < j)
				{
					Dirs[i][j] = 9;
					SqOnLine[i][j] = __min(7 - File(j), 7 - Rank(j));
				}
				else
				{
					Dirs[i][j] = -9;
					SqOnLine[i][j] = __min(File(j), Rank(j));
				}
			}
			else if (File(i) - File(j) == Rank(j) - Rank(i))
			{
				DirType[i][j] = dtBishop;
				if (i < j)
				{
					Dirs[i][j] = 7;
					SqOnLine[i][j] = __min(File(j), 7 - Rank(j));
				}
				else
				{
					Dirs[i][j] = -7;
					SqOnLine[i][j] = __min(7 - File(j), Rank(j));
				}
			}
			else Dirs[i][j] = SqOnLine[i][j] = DirType[i][j] = 0;
		}
	}
}

inline BitBoard LineAttacker(uint8 from, uint8 to)
{
	__int8 d = Dirs[to][from];
	uint8 sq = from;
	for (uint8 i = 0; i < SqOnLine[to][from]; i++)
	{
		sq += d;
		if (Piece[sq] != ptNone)
		{
			if (Piece[sq] == ptWhiteQueen || Piece[sq] == ptBlackQueen) return SBM[sq];
			if (Piece[sq] == ptWhiteRook || Piece[sq] == ptBlackRook)
			{
				if (abs(d) == 1 || abs(d) == 8) return SBM[sq];
			}
			if (Piece[sq] == ptWhiteBishop || Piece[sq] == ptBlackBishop)
			{
				if (abs(d) == 7 || abs(d) == 9) return SBM[sq];
			}
			return 0;
		}
	}
	return 0;
}

inline uint8 MinAttacker(BitBoard a)
{
	uint8 sq, val = 101;
	while (a)
	{
		uint8 x = ELSB(a);
		if (SPV2[Piece[x]] < val)
		{
			val = SPV2[Piece[x]];
			sq = x;
		}
	}
	return sq;
}

uint8 SEEW(Move m)
{
	// взятие королем
	if (Piece[m.from] == ptWhiteKing) return 1;

	__int16 value = SPV[m.cap];
	value -= SPV[Piece[m.from]];
	if (value >= 0) return 1;

	// составляем списки атакующих поле размена
	// список черных атакующих поcле размена
	BitBoard a = BPawnAtk[m.to] & BlackPawns;
	a |= KnightMoves[m.to] & BlackKnights;
	a |= KingMoves[m.to] & BlackKing;
	a |= BishopMoves(m.to) & (BlackBishops | BlackQueens);
	a |= RookMoves(m.to) & (BlackRooks | BlackQueens);
	a |= LineAttacker(m.from, m.to);
	if (!(a & BlackPieces)) return 1;

	a |= WPawnAtk[m.to] & WhitePawns;
	a |= KnightMoves[m.to] & WhiteKnights;
	a |= KingMoves[m.to] & WhiteKing;
	a |= BishopMoves(m.to) & (WhiteBishops | WhiteQueens);
	a |= RookMoves(m.to) & (WhiteRooks | WhiteQueens);
	if (Piece[m.from] != ptWhitePawn || m.cap != ptNone) a ^= SBM[m.from];


	// смотрим размены
	uint8 sq;
	for (;;)
	{
		if (!(a & BlackPieces)) return 1;
		
		sq = MinAttacker(a & BlackPieces);
		value += SPV[Piece[sq]];
		if (value < 0) return 0;
		a ^= SBM[sq];
		a |= LineAttacker(sq, m.to);

		if (!(a & WhitePieces)) return 0;
		
		sq = MinAttacker(a & WhitePieces);
		value -= SPV[Piece[sq]];
		if (value >= 0) return 1;
		a ^= SBM[sq];
		a |= LineAttacker(sq, m.to);
	}
}

uint8 SEEB(Move m)
{
	if (Piece[m.from] == ptBlackKing) return 1;
	__int16 value = SPV[m.cap];
	value -= SPV[Piece[m.from]];
	if (value >= 0) return 1;

	BitBoard a = WPawnAtk[m.to] & WhitePawns;
	a |= KnightMoves[m.to] & WhiteKnights;
	a |= KingMoves[m.to] & WhiteKing;
	a |= BishopMoves(m.to) & (WhiteBishops | WhiteQueens);
	a |= RookMoves(m.to) & (WhiteRooks | WhiteQueens);
	a |= LineAttacker(m.from, m.to);
	if (!(a & WhitePieces)) return 1;

	a |= BPawnAtk[m.to] & BlackPawns;
	a |= KnightMoves[m.to] & BlackKnights;
	a |= KingMoves[m.to] & BlackKing;
	a |= BishopMoves(m.to) & (BlackBishops | BlackQueens);
	a |= RookMoves(m.to) & (BlackRooks | BlackQueens);
	if (Piece[m.from] != ptBlackPawn || m.cap != ptNone) a ^= SBM[m.from];

	uint8 sq;
	for (;;)
	{
		if (!(a & WhitePieces)) return 1;
		
		sq = MinAttacker(a & WhitePieces);
		value += SPV[Piece[sq]];
		if (value < 0) return 0;
		a ^= SBM[sq];
		a |= LineAttacker(sq, m.to);
		
		if (!(a & BlackPieces)) return 0;
		
		sq = MinAttacker(a & BlackPieces);
		value -= SPV[Piece[sq]];
		if (value >= 0) return 1;
		a ^= SBM[sq];
		a |= LineAttacker(sq, m.to);
	}
}
