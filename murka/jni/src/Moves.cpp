#include <iostream>
using namespace std;
#include <assert.h>

#include "Moves.h"
#include "BitBoards.h"
#include "Board.h"
#include "Hash.h"
#include "Eval.h"
#include "Search.h"
#include "IO.h"
#include "Test.h"

// маски для определения потери рокировок
// массив расчитывался исходя из констант
//const uint8 crWhiteKingside  = 1;
//const uint8 crWhiteQueenside = 2;
//const uint8 crBlackKingside  = 4;
//const uint8 crBlackQueenside = 8;
uint8 CastleMask[64] =
{
	 7, 15, 15, 15,  3, 15, 15, 11,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	13, 15, 15, 15, 12, 15, 15, 14,
};

// перемещаем фигуру по доске
inline void MovePiece(uint8 p, uint8 from, uint8 to, BitBoard &c)
{
	c            ^= SBM[from] ^ SBM[to];
	BitBoards[p] ^= SBM[from] ^ SBM[to];
	Piece[to]     = Piece[from];
	Piece[from]   = ptNone;
}

// добавляем фигуру
inline void XorPiece(uint8 sq, uint8 p, BitBoard &c)
{
	c ^= SBM[sq];
	BitBoards[p] ^= SBM[sq];
}

// откат сделанного ранее хода
void UnmakeWhiteMove(Move m)
{
	// превращение
	if (m.flags & mfPromo)
	{
		Piece[m.to] = ptNone;
		XorPiece(m.to, m.flags, WhitePieces);
		Piece[m.from] = ptWhitePawn;
		XorPiece(m.from, ptWhitePawn, WhitePieces);
	}
	else MovePiece(Piece[m.to], m.to, m.from, WhitePieces);

	// взятие
	if (m.cap)
	{
		// на проходе
		if (m.flags == mfEP)
		{
			Piece[m.to + 8] = ptBlackPawn;
			XorPiece(m.to + 8, ptBlackPawn, BlackPieces);
		}
		else
		{
			Piece[m.to] = m.cap;
			XorPiece(m.to, m.cap, BlackPieces);
		}
	}

	// рокировка
	if (m.flags == mfCastle)
	{
		if (m.to == G1) MovePiece(ptWhiteRook, F1, H1, WhitePieces);
		else MovePiece(ptWhiteRook, D1, A1, WhitePieces);
	}

	WTM = true;
	NI--;

	assert(TestBoard());
}

void UnmakeBlackMove(Move m)
{
	// превращение
	if (m.flags & mfPromo)
	{
		Piece[m.to] = ptNone;
		XorPiece(m.to, m.flags, BlackPieces);
		Piece[m.from] = ptBlackPawn;
		XorPiece(m.from, ptBlackPawn, BlackPieces);
	}
	else MovePiece(Piece[m.to], m.to, m.from, BlackPieces);
	
	// взятие
	if (m.cap)
	{
		// на проходе
		if (m.flags == mfEP)
		{
			Piece[m.to - 8] = ptWhitePawn;
			XorPiece(m.to - 8, ptWhitePawn, WhitePieces);
		}
		else
		{
			Piece[m.to] = m.cap;
			XorPiece(m.to, m.cap, WhitePieces);
		}
	}

	// рокировка
	if (m.flags == mfCastle)
	{
		if (m.to == G8) MovePiece(ptBlackRook, F8, H8, BlackPieces);
		else MovePiece(ptBlackRook, D8, A8, BlackPieces);
	}

	WTM = false;
	NI--;

	assert(TestBoard());
}

// делаем ход
bool MakeWhiteMove(Move m)
{
	assert(m.cap != ptBlackKing);
	assert(!WhitePiece(m.cap));
	assert(m.from < 64);
	assert(m.to < 64);
	assert(WhitePiece(Piece[m.from]));
	assert(WTM);

	NI->move = m;
	NI++;
	assert(NI-NodesInfo < 1024);
	NI->CastleRights = (NI-1)->CastleRights & CastleMask[m.from] & CastleMask[m.to];
	NI->HashKey = (NI-1)->HashKey ^ HashKeysCR[(NI-1)->CastleRights];
	if ((NI-1)->EP) NI->HashKey ^= HashKeysEP[(NI - 1)->EP];
	NI->PawnHashKey = (NI-1)->PawnHashKey;
	NI->OP = (NI-1)->OP;
	NI->EG = (NI-1)->EG;
	NI->MS = (NI-1)->MS;
	
	NI->Reversible = !m.cap && Piece[m.from] != ptWhitePawn && m.flags != mfCastle;

	// взятие
	if (m.cap)
	{
		// на проходе
		if (m.flags == mfEP)
		{
			assert(Piece[m.to + 8] == ptBlackPawn);
			NI->HashKey ^= HashKeys[ptBlackPawn][m.to + 8];
			NI->PawnHashKey ^= HashKeys[ptBlackPawn][m.to + 8];
			Piece[m.to + 8] = ptNone;
			XorPiece(m.to + 8, ptBlackPawn, BlackPieces);
			NI->OP -= PST[ptBlackPawn][m.to + 8][0];
			NI->EG -= PST[ptBlackPawn][m.to + 8][1];
			NI->MS -= MSValue[ptBlackPawn];
		}
		else
		{
			assert(m.cap == Piece[m.to]);
			NI->HashKey ^= HashKeys[Piece[m.to]][m.to];
			if (Piece[m.to] == ptBlackPawn) NI->PawnHashKey ^= HashKeys[Piece[m.to]][m.to];
			XorPiece(m.to, m.cap, BlackPieces);
			NI->OP -= PST[m.cap][m.to][0];
			NI->EG -= PST[m.cap][m.to][1];
			NI->MS -= MSValue[m.cap];
		}
	}
	else assert(Piece[m.to] == ptNone);

	// превращение
	if (m.flags & mfPromo)
	{
		assert(Piece[m.from] == ptWhitePawn);
		// убираем пешку
		NI->HashKey ^= HashKeys[ptWhitePawn][m.from];
		NI->PawnHashKey ^= HashKeys[ptWhitePawn][m.from];
		Piece[m.from] = ptNone;
		XorPiece(m.from, ptWhitePawn, WhitePieces);
		NI->OP -= PST[ptWhitePawn][m.from][0];
		NI->EG -= PST[ptWhitePawn][m.from][1];
		NI->MS -= MSValue[ptWhitePawn];

		// добавляем фигуру
		NI->HashKey ^= HashKeys[m.flags][m.to];
		Piece[m.to] = m.flags;
		assert(WhitePiece(Piece[m.to]));
		assert(Piece[m.to] != ptWhitePawn);
		XorPiece(m.to, m.flags, WhitePieces);
		NI->OP += PST[m.flags][m.to][0];
		NI->EG += PST[m.flags][m.to][1];
		NI->MS += MSValue[m.flags];
	}
	else
	{
		NI->OP += PST[Piece[m.from]][m.to][0] - PST[Piece[m.from]][m.from][0];
		NI->EG += PST[Piece[m.from]][m.to][1] - PST[Piece[m.from]][m.from][1];
		NI->HashKey ^= HashKeys[Piece[m.from]][m.from];
		NI->HashKey ^= HashKeys[Piece[m.from]][m.to];
		if (Piece[m.from] == ptWhitePawn)
		{
			NI->PawnHashKey ^= HashKeys[Piece[m.from]][m.from];
			NI->PawnHashKey ^= HashKeys[Piece[m.from]][m.to];
		}
		MovePiece(Piece[m.from], m.from, m.to, WhitePieces);
	}

	// запоминаем поле для взятия на проходе
	if (m.flags == mfPawnJump && (BPawnAtk[m.to + 8] & BlackPawns))
	{
		NI->EP = m.to + 8;
		NI->HashKey ^= HashKeysEP[m.to + 8];
	}
	else NI->EP = 0;

	// рокировка
	if (m.flags == mfCastle)
	{
		if (m.to == G1)
		{
			NI->HashKey ^= HashKeys[ptWhiteRook][H1];
			NI->HashKey ^= HashKeys[ptWhiteRook][F1];
			assert(Piece[H1] == ptWhiteRook);
			assert(Piece[F1] == ptNone);
			MovePiece(ptWhiteRook, H1, F1, WhitePieces);
			NI->OP += PST[ptWhiteRook][F1][0] - PST[ptWhiteRook][H1][0];
			NI->EG += PST[ptWhiteRook][F1][1] - PST[ptWhiteRook][H1][1];
		}
		else
		{
			NI->HashKey ^= HashKeys[ptWhiteRook][A1];
			NI->HashKey ^= HashKeys[ptWhiteRook][D1];
			assert(Piece[A1] == ptWhiteRook);
			assert(Piece[B1] == ptNone);
			assert(Piece[D1] == ptNone);
			MovePiece(ptWhiteRook, A1, D1, WhitePieces);
			NI->OP += PST[ptWhiteRook][D1][0] - PST[ptWhiteRook][A1][0];
			NI->EG += PST[ptWhiteRook][D1][1] - PST[ptWhiteRook][A1][1];
		}
	}

	NI->HashKey ^= HashKeysCR[NI->CastleRights];
	NI->HashKey ^= HashKeyWTM;

	// проверка легальности хода
	if (WhiteInCheck())
	{
		UnmakeWhiteMove(m);
		return false;
	}

	NI->InCheck = BlackInCheck();
	WTM = false;
	Nodes++;

	assert(TestBoard());
	assert(TestEvalSymmetry());

	return true;
}

bool MakeBlackMove(Move m)
{
	assert(m.cap != ptBlackKing);
	assert(!BlackPiece(m.cap));
	assert(m.from < 64);
	assert(m.to < 64);
	assert(BlackPiece(Piece[m.from]));
	assert(!WTM);

	NI->move = m;
	NI++;
	assert(NI-NodesInfo < 1024);
	NI->CastleRights = (NI-1)->CastleRights & CastleMask[m.from] & CastleMask[m.to];
	NI->HashKey = (NI-1)->HashKey ^ HashKeysCR[(NI-1)->CastleRights];
	if ((NI-1)->EP) NI->HashKey ^= HashKeysEP[(NI - 1)->EP];
	NI->PawnHashKey = (NI-1)->PawnHashKey;
	NI->OP = (NI-1)->OP;
	NI->EG = (NI-1)->EG;
	NI->MS = (NI-1)->MS;

	NI->Reversible = !m.cap && Piece[m.from] != ptBlackPawn && m.flags != mfCastle;

	// взятие
	if (m.cap)
	{
		// на проходе
		if (m.flags == mfEP)
		{
			assert(Piece[m.to - 8] == ptWhitePawn);
			NI->HashKey ^= HashKeys[ptWhitePawn][m.to - 8];
			NI->PawnHashKey ^= HashKeys[ptWhitePawn][m.to - 8];
			Piece[m.to - 8] = ptNone;
			XorPiece(m.to - 8, ptWhitePawn, WhitePieces);
			NI->OP -= PST[ptWhitePawn][m.to - 8][0];
			NI->EG -= PST[ptWhitePawn][m.to - 8][1];
			NI->MS -= MSValue[ptWhitePawn];
		}
		else
		{
			assert(m.cap == Piece[m.to]);
			NI->HashKey ^= HashKeys[Piece[m.to]][m.to];
			if (Piece[m.to] == ptWhitePawn) NI->PawnHashKey ^= HashKeys[Piece[m.to]][m.to];
			XorPiece(m.to, m.cap, WhitePieces);
			NI->OP -= PST[m.cap][m.to][0];
			NI->EG -= PST[m.cap][m.to][1];
			NI->MS -= MSValue[m.cap];
		}
	}
	else assert(Piece[m.to] == ptNone);

	// превращение
	if (m.flags & mfPromo)
	{
		assert(Piece[m.from] == ptBlackPawn);
		// убираем пешку
		NI->HashKey ^= HashKeys[ptBlackPawn][m.from];
		NI->PawnHashKey ^= HashKeys[ptBlackPawn][m.from];
		Piece[m.from] = ptNone;
		XorPiece(m.from, ptBlackPawn, BlackPieces);
		NI->OP -= PST[ptBlackPawn][m.from][0];
		NI->EG -= PST[ptBlackPawn][m.from][1];
		NI->MS -= MSValue[ptBlackPawn];

		// добавляем фигуру
		NI->HashKey ^= HashKeys[m.flags][m.to];
		Piece[m.to] = m.flags;
		assert(BlackPiece(Piece[m.to]));
		assert(Piece[m.to] != ptBlackPawn);
		XorPiece(m.to, m.flags, BlackPieces);
		NI->OP += PST[m.flags][m.to][0];
		NI->EG += PST[m.flags][m.to][1];
		NI->MS += MSValue[m.flags];
	}
	else
	{
		NI->OP += PST[Piece[m.from]][m.to][0] - PST[Piece[m.from]][m.from][0];
		NI->EG += PST[Piece[m.from]][m.to][1] - PST[Piece[m.from]][m.from][1];
		NI->HashKey ^= HashKeys[Piece[m.from]][m.from];
		NI->HashKey ^= HashKeys[Piece[m.from]][m.to];
		if (Piece[m.from] == ptBlackPawn)
		{
			NI->PawnHashKey ^= HashKeys[Piece[m.from]][m.from];
			NI->PawnHashKey ^= HashKeys[Piece[m.from]][m.to];
		}
		MovePiece(Piece[m.from], m.from, m.to, BlackPieces);
	}

	// запоминаем поле для взятия на проходе
	if (m.flags == mfPawnJump && (WPawnAtk[m.to - 8] & WhitePawns))
	{
		NI->EP = m.to - 8;
		NI->HashKey ^= HashKeysEP[m.to - 8];
	}
	else NI->EP = 0;

	// рокировка
	if (m.flags == mfCastle)
	{
		if (m.to == G8)
		{
			NI->HashKey ^= HashKeys[ptBlackRook][H8];
			NI->HashKey ^= HashKeys[ptBlackRook][F8];
			assert(Piece[H8] == ptBlackRook);
			assert(Piece[F8] == ptNone);
			MovePiece(ptBlackRook, H8, F8, BlackPieces);
			NI->OP += PST[ptBlackRook][F8][0] - PST[ptBlackRook][H8][0];
			NI->EG += PST[ptBlackRook][F8][1] - PST[ptBlackRook][H8][1];
		}
		else
		{
			NI->HashKey ^= HashKeys[ptBlackRook][A8];
			NI->HashKey ^= HashKeys[ptBlackRook][D8];
			assert(Piece[A8] == ptBlackRook);
			assert(Piece[B8] == ptNone);
			assert(Piece[D8] == ptNone);
			MovePiece(ptBlackRook, A8, D8, BlackPieces);
			NI->OP += PST[ptBlackRook][D8][0] - PST[ptBlackRook][A8][0];
			NI->EG += PST[ptBlackRook][D8][1] - PST[ptBlackRook][A8][1];
		}
	}

	NI->HashKey ^= HashKeysCR[NI->CastleRights];
	NI->HashKey ^= HashKeyWTM;

	// проверка легальности хода
	if (BlackInCheck())
	{
		UnmakeBlackMove(m);
		return false;
	}

	NI->InCheck = WhiteInCheck();
	WTM = true;
	Nodes++;

	assert(TestBoard());
	assert(TestEvalSymmetry());

	return true;
}

void MakeNullMove()
{
	NI++;
	NI->CastleRights = (NI - 1)->CastleRights;
	NI->HashKey = (NI - 1)->HashKey ^ HashKeyWTM;
	if ((NI-1)->EP) NI->HashKey ^= HashKeysEP[(NI - 1)->EP];
	NI->PawnHashKey = (NI-1)->PawnHashKey;
	NI->EP = 0;
	NI->OP = (NI-1)->OP;
	NI->EG = (NI-1)->EG;
	NI->MS = (NI-1)->MS;
	NI->InCheck = 0;
	NI->Reversible = 0; // ???

	WTM = !WTM;
	Nodes++;
}

void UnmakeNullMove()
{
	NI--;
	WTM = !WTM;
}

// проверка возможности хода
bool ProperWhiteMove(Move m)
{
	if (Piece[m.from] == ptWhitePawn)
	{
		if (m.flags == mfEP)
		{
			return m.to == NI->EP;
		}
		if (m.cap != Piece[m.to]) return false;
		if (m.cap)
		{
			return WPawnAtk[m.to] & SBM[m.from];
		}
		else
		{
			if (Rank(m.from) == 1 && !(m.flags & mfPromo)) return false;
			if (m.from == m.to + 8)  return true;
			if (m.from != m.to + 16) return false;
			if (m.flags != mfPawnJump || Rank(m.from) != 6 || Piece[m.from - 8]) return false;
			return true;
		}
	}
	if (m.cap != Piece[m.to]) return false;
	if (m.flags == mfPawnJump) return false;
	if (m.flags == mfEP) return false;
	if (m.flags & mfPromo) return false;
	if (Piece[m.from] == ptWhiteKing)
	{
		if (m.flags == mfCastle)
		{
			if (m.from != E1) return false;
			if (m.to == G1)
			{
				if (!(NI->CastleRights & crWhiteKingside)) return false;
				if (Piece[F1] || Piece[G1]) return false;
				if (AttackedByBlack(F1) || AttackedByBlack(G1)) return false;
				return true;
			}
			else
			{
				if (!(NI->CastleRights & crWhiteQueenside)) return false;
				if (Piece[D1] || Piece[C1] || Piece[B1]) return false;
				if (AttackedByBlack(D1) || AttackedByBlack(C1)) return false;
				return true;
			}
		}
		return KingMoves[m.from] & ~WhitePieces & SBM[m.to];
	}
	if (m.flags == mfCastle) return false;
	if (Piece[m.from] == ptWhiteRook)
	{
		return RookMoves(m.from) & ~WhitePieces & SBM[m.to];
	}
	if (Piece[m.from] == ptWhiteBishop)
	{
		return BishopMoves(m.from) & ~WhitePieces & SBM[m.to];
	}
	if (Piece[m.from] == ptWhiteKnight)
	{
		return KnightMoves[m.from] & ~WhitePieces & SBM[m.to];
	}
	if (Piece[m.from] == ptWhiteQueen)
	{
		return (RookMoves(m.from) | BishopMoves(m.from)) & ~WhitePieces & SBM[m.to];
	}
	return false;
}

bool ProperBlackMove(Move m)
{
	if (Piece[m.from] == ptBlackPawn)
	{
		if (m.flags == mfEP)
		{
			return m.to == NI->EP;
		}
		if (m.cap != Piece[m.to]) return false;
		if (m.cap) return BPawnAtk[m.to] & SBM[m.from];
		else
		{
			if (Rank(m.from) == 6 && !(m.flags & mfPromo)) return false;
			if (m.from + 8  == m.to) return true;
			if (m.from + 16 != m.to) return false;
			if (m.flags != mfPawnJump || Rank(m.from) != 1 || Piece[m.from + 8]) return false;
			return true;
		}
	}
	if (m.cap != Piece[m.to]) return false;
	if (m.flags == mfPawnJump) return false;
	if (m.flags == mfEP) return false;
	if (m.flags & mfPromo) return false;
	if (Piece[m.from] == ptBlackKing)
	{
		if (m.flags == mfCastle)
		{
			if (m.from != E8) return false;
			if (m.to == G8)
			{
				if (!(NI->CastleRights & crBlackKingside)) return false;
				if (Piece[F8] || Piece[G8]) return false;
				if (AttackedByWhite(F8) || AttackedByWhite(G8)) return false;
				return true;
			}
			else
			{
				if (!(NI->CastleRights & crBlackQueenside)) return false;
				if (Piece[D8] || Piece[C8] || Piece[B8]) return false;
				if (AttackedByWhite(D8) || AttackedByWhite(C8)) return false;
				return true;
			}
		}
		return KingMoves[m.from] & ~BlackPieces & SBM[m.to];
	}
	if (m.flags == mfCastle) return false;
	if (Piece[m.from] == ptBlackRook)
	{
		return RookMoves(m.from) & ~BlackPieces & SBM[m.to];
	}
	if (Piece[m.from] == ptBlackBishop)
	{
		return BishopMoves(m.from) & ~BlackPieces & SBM[m.to];
	}
	if (Piece[m.from] == ptBlackKnight)
	{
		return KnightMoves[m.from] & ~BlackPieces & SBM[m.to];
	}
	if (Piece[m.from] == ptBlackQueen)
	{
		return (RookMoves(m.from) | BishopMoves(m.from)) & ~BlackPieces & SBM[m.to];
	}
	return false;
}
