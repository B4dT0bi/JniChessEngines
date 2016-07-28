#include <assert.h>

#include "MoveList.h"
#include "BitBoards.h"
#include "Search.h"
#include "Protocols.h"
#include "IO.h"
#include "SEE.h"

// ������ ��������� ��� ���������������� ���������� �����
Move *MoveList::NextIncMove()
{
	Move *m;
	switch (Stage)
	{
	case nmsTrans:
		if (*(uint32*)&Trans != *(uint32*)&NullMove)
		{
			Stage = nmsGenCaps;
			if (ProperMove(Trans)) return &Trans;
			assert(0);
		}
	case nmsGenCaps:
		GenCaps();
		RemoveTrans();
		SetQMoveValues();
		Stage = nmsGoodCaps;
		pb = BadCaps;
	case nmsGoodCaps:
		while (m = NextSortedMove())
		{
			if (!SEE(*m)) *pb++ = *(cm - 1);
			else return m;
		}
		Stage = nmsKiller2;
		if ((*(uint32*)&NI->Killer1 != *(uint32*)&Trans) && ProperMove(NI->Killer1)) return &NI->Killer1;
	case nmsKiller2:
		Stage = nmsGenQuiets;
		if ((*(uint32*)&NI->Killer2 != *(uint32*)&Trans) && ProperMove(NI->Killer2)) return &NI->Killer2;
	case nmsGenQuiets:
		GenQuiets();
		RemoveTransKillers();
		Stage = nmsQuiets;
	case nmsQuiets:
		if (m = NextSortedMove()) return m;
		Stage = nmsBadCaps;
		cm = BadCaps;
		pm = pb;
	case nmsBadCaps:
		return NextSortedMove();
	}
	return 0;
}

// ���������� ������ �����
void MoveList::Sort()
{
	for (EMove *m1 = List; m1 != pm; m1++)
	{
		EMove *b = m1;
		for (EMove *m2 = m1 + 1; m2 != pm; m2++)
		{
			if (m2->value > b->value) b = m2;
		}
		EMove bm = *b;
		for (EMove *m2 = b; m2 > m1; m2--)
			*m2 = *(m2 - 1);
		*m1 = bm;
	}

	for (EMove *m = List; m != pm - 1; m++)
	{
		assert(m->value >= (m+1)->value);
	}
}

// ���������� ��� ����������� ����
BitBoard WhiteAttacks()
{
	BitBoard fb, res;

	res  = (WhitePawns & ~FileA) >> 9;
	res |= (WhitePawns & ~FileH) >> 7;

	fb = WhiteKnights;
	while (fb)
	{
		res |= KnightMoves[ELSB(fb)];
	}

	fb = WhiteBishops | WhiteQueens;
	while (fb)
	{
		res |= BishopMoves(ELSB(fb));
	}

	fb = WhiteRooks | WhiteQueens;
	while (fb)
	{
		res |= RookMoves(ELSB(fb));
	}

	return res | KingMoves[LSB(WhiteKing)];
}

BitBoard BlackAttacks()
{
	BitBoard fb, res;

	res  = (BlackPawns & ~FileA) << 7;
	res |= (BlackPawns & ~FileH) << 9;

	fb = BlackKnights;
	while (fb)
	{
		res |= KnightMoves[ELSB(fb)];
	}

	fb = BlackBishops | BlackQueens;
	while (fb)
	{
		res |= BishopMoves(ELSB(fb));
	}

	fb = BlackRooks | BlackQueens;
	while (fb)
	{
		res |= RookMoves(ELSB(fb));
	}

	return res | KingMoves[LSB(BlackKing)];
}

// ��������� ����� �����
void MoveList::GenWhiteQuiets()
{
	BitBoard fb, tb;
	uint8 from, to;

	if (!NI->InCheck)
	{
		// ���������
		if (NI->CastleRights & crWhiteKingside && Piece[F1] == ptNone && Piece[G1] == ptNone)
		{
			if (!AttackedByBlack(F1))
			if (!AttackedByBlack(G1))
			{
				AddMoveQuiet(E1, G1, mfCastle);
			}
		}
		if (NI->CastleRights & crWhiteQueenside && Piece[D1] == ptNone && Piece[C1] == ptNone && Piece[B1] == ptNone)
		{
			if (!AttackedByBlack(D1))
			if (!AttackedByBlack(C1))
			{
				AddMoveQuiet(E1, C1, mfCastle);
			}
		}
	}

	BitBoard t = ~(WhitePieces | BlackPieces);

	tb = WhitePawns >> 8 & t & ~Rank8;
	while (tb)
	{
		to = ELSB(tb);
		if (Rank(to) == 5 && Piece[to - 8] == ptNone) AddMoveQuiet(to + 8, to - 8, mfPawnJump);
		AddMoveQuiet(to + 8, to);
	}

	fb = WhiteKnights;
	while (fb)
	{
		from = ELSB(fb);
		tb = KnightMoves[from] & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMoveQuiet(from, to);
		}
	}

	fb = WhiteBishops;
	while (fb)
	{
		from = ELSB(fb);
		tb = BishopMoves(from) & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMoveQuiet(from, to);
		}
	}

	fb = WhiteRooks;
	while (fb)
	{
		from = ELSB(fb);
		tb = RookMoves(from) & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMoveQuiet(from, to);
		}
	}

	fb = WhiteQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = (BishopMoves(from) | RookMoves(from)) & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMoveQuiet(from, to);
		}
	}

	from = LSB(WhiteKing);
	tb = KingMoves[from] & t;
	while (tb)
	{
		to = ELSB(tb);
		AddMoveQuiet(from, to);
	}
}

void MoveList::GenBlackQuiets()
{
	BitBoard fb, tb;
	uint8 from, to;

	BitBoard t = ~(WhitePieces | BlackPieces);

	if (!NI->InCheck)
	{
		if (NI->CastleRights & crBlackKingside && Piece[F8] == ptNone && Piece[G8] == ptNone)
		{
			if (!AttackedByWhite(F8))
			if (!AttackedByWhite(G8))
			{
				AddMoveQuiet(E8, G8, mfCastle);
			}
		}
		if (NI->CastleRights & crBlackQueenside && Piece[D8] == ptNone && Piece[C8] == ptNone && Piece[B8] == ptNone)
		{
			if (!AttackedByWhite(D8))
			if (!AttackedByWhite(C8))
			{
				AddMoveQuiet(E8, C8, mfCastle);
			}
		}
	}

	tb = BlackPawns << 8 & t & ~Rank1;
	while (tb)
	{
		to = ELSB(tb);
		if (Rank(to) == 2 && Piece[to + 8] == ptNone) AddMoveQuiet(to - 8, to + 8, mfPawnJump);
		AddMoveQuiet(to - 8, to);
	}

	fb = BlackKnights;
	while (fb)
	{
		from = ELSB(fb);
		tb = KnightMoves[from] & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMoveQuiet(from, to);
		}
	}

	fb = BlackBishops;
	while (fb)
	{
		from = ELSB(fb);
		tb = BishopMoves(from) & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMoveQuiet(from, to);
		}
	}

	fb = BlackRooks;
	while (fb)
	{
		from = ELSB(fb);
		tb = RookMoves(from) & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMoveQuiet(from, to);
		}
	}

	fb = BlackQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = (BishopMoves(from) | RookMoves(from)) & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMoveQuiet(from, to);
		}
	}

	from = LSB(BlackKing);
	tb = KingMoves[from] & t;
	while (tb)
	{
		to = ELSB(tb);
		AddMoveQuiet(from, to);
	}
}

// ��������� ����� �� ����
void MoveList::GenWhiteEvasions()
{
	BitBoard fb, tb;
	uint8 from, to;

	uint8 ksq = LSB(WhiteKing);

	// ���������� �������� ������
	BitBoard a =
		KnightMoves[ksq] & BlackKnights |
		BPawnAtk[ksq] & BlackPawns |
		RookMoves(ksq)   & (BlackQueens | BlackRooks) |
		BishopMoves(ksq) & (BlackQueens | BlackBishops);

	// ������� ��������� ������ ������
	tb = KingMoves[ksq] & ~WhitePieces & ~BlackAttacks();
	uint8 asq = ELSB(a);
	if (Piece[asq] != ptBlackPawn) tb &= EvasionsMask[ksq][asq];
	// ������� ���
	if (a)
	{
		asq = LSB(a);
		if (Piece[asq] != ptBlackPawn) tb &= EvasionsMask[ksq][asq];
		// ������� ���� ������
		while (tb)
		{
			AddMove(ksq, ELSB(tb));
		}
		return;
	}
	// ������� ���� ������
	while (tb)
	{
		AddMove(ksq, ELSB(tb));
	}
	// ������ �������
	fb = WPawnAtk[asq] & WhitePawns;
	while (fb)
	{
		AddMove(ELSB(fb), asq, Rank(asq) == 0? ptWhiteQueen : 0);
	}
	// ������ �� �������
	if (NI->EP && NI->EP == asq - 8)
	{
		fb = WPawnAtk[asq - 8] & WhitePawns;
		while (fb)
		{
			AddMoveEP(ELSB(fb), asq - 8, ptBlackPawn);
		}
	}
	// ����������
	BitBoard ib = InterceptMask[ksq][asq];

	tb = WhitePawns >> 8 & ~(WhitePieces | BlackPieces);

	BitBoard jumps = Rank4 & tb >> 8 & ~(WhitePieces | BlackPieces) & ib;
	tb &= ib;
	// ����������� ����� �� ���� ������ ������
	while (tb)
	{
		to = ELSB(tb);
		AddMove(to + 8, to, Rank(to) == 0? ptWhiteQueen : 0);
	}

	// ������ ����� ������ ���
	while (jumps)
	{
		to = ELSB(jumps);
		AddMove(to + 16, to, mfPawnJump);
	}

	fb = WhiteKnights;
	while (fb)
	{
		from = ELSB(fb);
		tb = KnightMoves[from] & ib;
		while (tb)
		{
			AddMove(from, ELSB(tb));
		}
	}

	fb = WhiteBishops | WhiteQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = BishopMoves(from) & ib;
		while (tb)
		{
			AddMove(from, ELSB(tb));
		}
	}

	fb = WhiteRooks | WhiteQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = RookMoves(from) & ib;
		while (tb)
		{
			AddMove(from, ELSB(tb));
		}
	}
}

void MoveList::GenBlackEvasions()
{
	BitBoard fb, tb;
	uint8 from, to;

	uint8 ksq = LSB(BlackKing);

	// ���������� �������� ������
	BitBoard a =
		KnightMoves[ksq] & WhiteKnights |
		WPawnAtk[ksq] & WhitePawns |
		RookMoves(ksq)   & (WhiteQueens | WhiteRooks) |
		BishopMoves(ksq) & (WhiteQueens | WhiteBishops);

	// ������� ��������� ������ ������
	tb = KingMoves[ksq] & ~BlackPieces & ~WhiteAttacks();
	uint8 asq = ELSB(a);
	if (Piece[asq] != ptWhitePawn) tb &= EvasionsMask[ksq][asq];
	// ������� ���
	if (a)
	{
		asq = LSB(a);
		if (Piece[asq] != ptWhitePawn) tb &= EvasionsMask[ksq][asq];
		// ������� ���� ������
		while (tb)
		{
			AddMove(ksq, ELSB(tb));
		}
		return;
	}
	// ������� ���� ������
	while (tb)
	{
		AddMove(ksq, ELSB(tb));
	}
	// ������ �������
	fb = BPawnAtk[asq] & BlackPawns;
	while (fb)
	{
		AddMove(ELSB(fb), asq, Rank(asq) == 7? ptBlackQueen : 0);
	}
	// ������ �� �������
	if (NI->EP && NI->EP == asq + 8)
	{
		fb = BPawnAtk[asq + 8] & BlackPawns;
		while (fb)
		{
			AddMoveEP(ELSB(fb), asq + 8, ptWhitePawn);
		}
	}
	// ����������
	BitBoard ib = InterceptMask[ksq][asq];

	tb = BlackPawns << 8 & ~(WhitePieces | BlackPieces);

	BitBoard jumps = Rank5 & tb << 8 & ~(WhitePieces | BlackPieces) & ib;
	tb &= ib;
	// ����������� ����� �� ���� ������ ������
	while (tb)
	{
		to = ELSB(tb);
		AddMove(to - 8, to, Rank(to) == 7? ptBlackQueen : 0);
	}

	// ������ ����� ������ ���
	while (jumps)
	{
		to = ELSB(jumps);
		AddMove(to - 16, to, mfPawnJump);
	}

	fb = BlackKnights;
	while (fb)
	{
		from = ELSB(fb);
		tb = KnightMoves[from] & ib;
		while (tb)
		{
			AddMove(from, ELSB(tb));
		}
	}

	fb = BlackBishops | BlackQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = BishopMoves(from) & ib;
		while (tb)
		{
			AddMove(from, ELSB(tb));
		}
	}

	fb = BlackRooks | BlackQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = RookMoves(from) & ib;
		while (tb)
		{
			AddMove(from, ELSB(tb));
		}
	}
}


// ��������� ���� �����
void MoveList::GenWhiteMoves()
{
	BitBoard fb, tb;
	uint8 from, to;

	// ������ �� �������
	if (NI->EP)
	{
		fb = WPawnAtk[NI->EP] & WhitePawns;
		while (fb)
		{
			from = ELSB(fb);
			AddMoveEP(from, NI->EP, ptBlackPawn);
		}
	}

	// ������ �������
	tb = (WhitePawns & ~FileH) >> 7 & BlackPieces;
	while (tb)
	{
		to = ELSB(tb);
		// �����������
		if (Rank(to) == 0)
		{
			AddMove(to + 7, to, ptWhiteQueen);
			AddMove(to + 7, to, ptWhiteRook);
			AddMove(to + 7, to, ptWhiteKnight);
			AddMove(to + 7, to, ptWhiteBishop);
		}
		else AddMove(to + 7, to);
	}

	// ������ ������
	tb = (WhitePawns & ~FileA) >> 9 & BlackPieces;
	while (tb)
	{
		to = ELSB(tb);
		// �����������
		if (Rank(to) == 0)
		{
			AddMove(to + 9, to, ptWhiteQueen);
			AddMove(to + 9, to, ptWhiteRook);
			AddMove(to + 9, to, ptWhiteKnight);
			AddMove(to + 9, to, ptWhiteBishop);
		}
		else AddMove(to + 9, to);
	}

	tb = WhitePawns >> 8 & ~(WhitePieces | BlackPieces);

	// ������ ����� ������ ���
	BitBoard jumps = Rank4 & tb >> 8 & ~(WhitePieces | BlackPieces);
	while (jumps)
	{
		to = ELSB(jumps);
		AddMove(to + 16, to, mfPawnJump);
	}

	// ����������� ����� �� ���� ������ ������
	while (tb)
	{
		to = ELSB(tb);
		// �����������
		if (Rank(to) == 0)
		{
			AddMove(to + 8, to, ptWhiteQueen);
			AddMove(to + 8, to, ptWhiteRook);
			AddMove(to + 8, to, ptWhiteKnight);
			AddMove(to + 8, to, ptWhiteBishop);
		}
		else AddMove(to + 8, to);
	}

	fb = WhiteKnights;
	while (fb)
	{
		from = ELSB(fb);
		tb = KnightMoves[from] & ~WhitePieces;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	fb = WhiteBishops | WhiteQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = BishopMoves(from) & ~WhitePieces;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	fb = WhiteRooks | WhiteQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = RookMoves(from) & ~WhitePieces;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	if (!NI->InCheck)
	{
		// ���������
		if (NI->CastleRights & crWhiteKingside && Piece[F1] == ptNone && Piece[G1] == ptNone)
		{
			if (!AttackedByBlack(F1))
			if (!AttackedByBlack(G1))
			{
				AddMove(E1, G1, mfCastle);
			}
		}
		if (NI->CastleRights & crWhiteQueenside && Piece[D1] == ptNone && Piece[C1] == ptNone && Piece[B1] == ptNone)
		{
			if (!AttackedByBlack(D1))
			if (!AttackedByBlack(C1))
			{
				AddMove(E1, C1, mfCastle);
			}
		}
	}
	
	from = LSB(WhiteKing);
	tb = KingMoves[from] & ~WhitePieces;
	while (tb)
	{
		to = ELSB(tb);
		AddMove(from, to);
	}
}

// ��������� ������/�����������
void MoveList::GenWhiteCaps(BitBoard t)
{
	BitBoard fb, tb;
	uint8 from, to;

	pm = cm = List;

	// �����������
	tb = WhitePawns & Rank7;
	while (tb)
	{
		from = ELSB(tb);
		if (Piece[from - 8] == ptNone) AddMove(from, from - 8, ptWhiteQueen);
		if (from !=  8 && BlackPiece(Piece[from - 9])) AddMove(from, from - 9, ptWhiteQueen);
		if (from != 15 && BlackPiece(Piece[from - 7])) AddMove(from, from - 7, ptWhiteQueen);
	}

	// ������ ������
	tb = (WhitePawns & ~FileA & ~Rank7) >> 9 & t;
	while (tb)
	{
		to = ELSB(tb);
		AddMove(to + 9, to, 0);
	}

	// ������ �������
	tb = (WhitePawns & ~FileH & ~Rank7) >> 7 & t;
	while (tb)
	{
		to = ELSB(tb);
		AddMove(to + 7, to, 0);
	}

	// ������ �� �������
	if (NI->EP)
	{
		fb = WPawnAtk[NI->EP] & WhitePawns;
		while (fb) AddMoveEP(ELSB(fb), NI->EP, ptBlackPawn);
	}

	fb = WhiteKnights;
	while (fb)
	{
		from = ELSB(fb);
		tb = KnightMoves[from] & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	fb = WhiteBishops;
	while (fb)
	{
		from = ELSB(fb);
		tb = BishopMoves(from) & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	fb = WhiteRooks;
	while (fb)
	{
		from = ELSB(fb);
		tb = RookMoves(from) & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	fb = WhiteQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = (RookMoves(from) | BishopMoves(from)) & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	from = LSB(WhiteKing);
	tb = KingMoves[from] & t;
	while (tb)
	{
		to = ELSB(tb);
		AddMove(from, to);
	}
}

void MoveList::GenBlackMoves()
{

	BitBoard fb, tb;
	uint8 from, to;

	// ������ �� �������
	if (NI->EP)
	{
		tb = BPawnAtk[NI->EP] & BlackPawns;
		while (tb)
		{
			from = ELSB(tb);
			AddMoveEP(from, NI->EP, ptWhitePawn);
		}
	}

	// ������ ������
	tb = (BlackPawns & ~FileA) << 7 & WhitePieces;
	while (tb)
	{
		to = ELSB(tb);
		// �����������
		if (Rank(to) == 7)
		{
			AddMove(to - 7, to, ptBlackQueen);
			AddMove(to - 7, to, ptBlackRook);
			AddMove(to - 7, to, ptBlackKnight);
			AddMove(to - 7, to, ptBlackBishop);
		}
		else AddMove(to - 7, to);
	}

	// ������ �������
	tb = (BlackPawns & ~FileH) << 9 & WhitePieces;
	while (tb)
	{
		to = ELSB(tb);
		// �����������
		if (Rank(to) == 7)
		{
			AddMove(to - 9, to, ptBlackQueen);
			AddMove(to - 9, to, ptBlackRook);
			AddMove(to - 9, to, ptBlackKnight);
			AddMove(to - 9, to, ptBlackBishop);
		}
		else AddMove(to - 9, to);
	}

	tb = BlackPawns << 8 & ~(WhitePieces | BlackPieces);

	// ������ ����� ������ ���
	BitBoard jumps = Rank5 & tb << 8 & ~(WhitePieces | BlackPieces);
	while (jumps)
	{
		to = ELSB(jumps);
		AddMove(to - 16, to, mfPawnJump);
	}

	// ����������� �� ���� ������ ������
	while (tb)
	{
		to = ELSB(tb);
		// �����������
		if (Rank(to) == 7)
		{
			AddMove(to - 8, to, ptBlackQueen);
			AddMove(to - 8, to, ptBlackRook);
			AddMove(to - 8, to, ptBlackKnight);
			AddMove(to - 8, to, ptBlackBishop);
		}
		else AddMove(to - 8, to);
	}

	fb = BlackKnights;
	while (fb)
	{
		from = ELSB(fb);
		BitBoard moves = KnightMoves[from] & ~BlackPieces;
		while (moves)
		{
			to = ELSB(moves);
			AddMove(from, to);
		}
	}

	fb = BlackBishops | BlackQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = BishopMoves(from) & ~BlackPieces;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	fb = BlackRooks | BlackQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = RookMoves(from) & ~BlackPieces;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	// ������
	if (!NI->InCheck)
	{
		// ���������
		if (NI->CastleRights & crBlackKingside && Piece[F8] == ptNone && Piece[G8] == ptNone)
		{
			if (!AttackedByWhite(F8))
			if (!AttackedByWhite(G8))
			{
				AddMove(E8, G8, mfCastle);
			}
		}
		if (NI->CastleRights & crBlackQueenside && Piece[D8] == ptNone && Piece[C8] == ptNone && Piece[B8] == ptNone)
		{
			if (!AttackedByWhite(D8))
			if (!AttackedByWhite(C8))
			{
				AddMove(E8, C8, mfCastle);
			}
		}
	}

	from = LSB(BlackKing);
	tb = KingMoves[from] & ~BlackPieces;
	while (tb)
	{
		to = ELSB(tb);
		AddMove(from, to);
	}
}

void MoveList::GenBlackCaps(BitBoard t)
{
	BitBoard fb, tb;
	uint8 from, to;

	pm = cm = List;

	// �����������
	tb = BlackPawns & Rank2;
	while (tb)
	{
		from = ELSB(tb);
		if (Piece[from + 8] == ptNone) AddMove(from, from + 8, ptBlackQueen);
		if (from != 48 && WhitePiece(Piece[from + 7])) AddMove(from, from + 7, ptBlackQueen);
		if (from != 55 && WhitePiece(Piece[from + 9])) AddMove(from, from + 9, ptBlackQueen);
	}

	// ������ ������
	tb = (BlackPawns & ~FileA & ~Rank2) << 7 & t;
	while (tb)
	{
		to = ELSB(tb);
		AddMove(to - 7, to, 0);
	}

	// ������ �������
	tb = (BlackPawns & ~FileH & ~Rank2) << 9 & t;
	while (tb)
	{
		to = ELSB(tb);
		AddMove(to - 9, to, 0);
	}

	// ������ �� �������
	if (NI->EP)
	{
		tb = BPawnAtk[NI->EP] & BlackPawns;
		while (tb) AddMoveEP(ELSB(tb), NI->EP, ptWhitePawn);
	}

	fb = BlackKnights;
	while (fb)
	{
		from = ELSB(fb);
		BitBoard moves = KnightMoves[from] & t;
		while (moves)
		{
			to = ELSB(moves);
			AddMove(from, to);
		}
	}

	fb = BlackBishops;
	while (fb)
	{
		from = ELSB(fb);
		tb = BishopMoves(from) & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	fb = BlackRooks;
	while (fb)
	{
		from = ELSB(fb);
		tb = RookMoves(from) & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	fb = BlackQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = (RookMoves(from) | BishopMoves(from)) & t;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	// ������
	from = LSB(BlackKing);
	tb = KingMoves[from] & t;
	while (tb)
	{
		to = ELSB(tb);
		AddMove(from, to);
	}
}

// ��������� �����
void MoveList::GenWhiteChecks(BitBoard t)
{
	uint8 from, to;
	BitBoard fb, tb, mask;

	uint8 king = LSB(BlackKing);
	
	mask = KnightMoves[king];
	fb = WhiteKnights;
	while (fb)
	{
		from = ELSB(fb);
		BitBoard tb = KnightMoves[from] & mask & t & ~WhitePieces;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	mask = BishopMoves(king);
	fb = WhiteBishops | WhiteQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = BishopMoves(from) & mask & t;
		while (tb)
		{
			to = ELSB(tb);
			if (Piece[to] != ptNone && WhitePiece(Piece[to]) && OnLine[from][king] && OnLine[from][king] <= 2)
			{
				// �������� ���
				if (Piece[to] == ptWhitePawn && Rank(to) > 1)
				{
					if (Piece[to - 8] == ptNone)
					{
						AddMove(to, to - 8);
						if (Rank(to) == 6 && Piece[to - 16] == ptNone) AddMove(to, to - 16, mfPawnJump);
					}
					if (File(to) < 7 && BlackPiece(Piece[to - 7]) && (SBM[to - 7] & t))
					{
						AddMove(to, to - 7, 0);
					}
					if (File(to) > 0 && BlackPiece(Piece[to - 9]) && (SBM[to - 9] & t))
					{
						AddMove(to, to - 9, 0);
					}
				}
				else if (Piece[to] == ptWhiteKnight)
				{
					tb = KnightMoves[to] & t & ~WhitePieces;
					while (tb)
					{
						uint8 to2 = ELSB(tb);
						AddMove(to, to2);
					}
				}
				else if (Piece[to] == ptWhiteRook)
				{
					tb = RookMoves(to) & t & ~WhitePieces;
					while (tb)
					{
						uint8 to2 = ELSB(tb);
						AddMove(to, to2);
					}
				}
				else if (Piece[to] == ptWhiteKing)
				{
					tb = KingMoves[to] & t & ~WhitePieces;
					while (tb)
					{
						uint8 to2 = ELSB(tb);
						if (OnLine[to][to2] != OnLine[from][king]) AddMove(to, to2);
					}
				}
			} else if (Piece[to] == ptNone || BlackPiece(Piece[to])) AddMove(from, to);
		}
		if (Piece[from] == ptWhiteQueen)
		{
			tb = (RookMoves(from) & mask | BishopMoves(from) & RookMoves(king)) & t & ~WhitePieces;
			while (tb)
			{
				to = ELSB(tb);
				AddMove(from, to);
			}
		}
	}

	mask = RookMoves(king);
	fb = WhiteRooks | WhiteQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = RookMoves(from) & mask & t;
		while (tb)
		{
			to = ELSB(tb);
			if (Piece[to] != ptNone && WhitePiece(Piece[to]) && OnLine[from][king] >= 3)
			{
				// �������� ���
				if (Piece[to] == ptWhitePawn)
				{
					if (Rank(to) > 1 && Piece[to - 8] == ptNone && OnLine[from][king] == 3)
					{
						AddMove(to, to - 8);
						if (Rank(to) == 6 && Piece[to - 16] == ptNone) AddMove(to, to - 16, mfPawnJump);
					}
					if (File(to) < 7 && BlackPiece(Piece[to - 7]) && (SBM[to - 7] & t))
					{
						AddMove(to, to - 7, Rank(to) > 1? 0 : mfPromo & ptWhiteQueen);
					}
					if (File(to) > 0 && BlackPiece(Piece[to - 9]) && (SBM[to - 9] & t))
					{
						AddMove(to, to - 9, Rank(to) > 1? 0 : mfPromo & ptWhiteQueen);
					}
				}
				else if (Piece[to] == ptWhiteKnight)
				{
					tb = KnightMoves[to] & t & ~WhitePieces;
					while (tb)
					{
						uint8 to2 = ELSB(tb);
						AddMove(to, to2);
					}
				}
				else if (Piece[to] == ptWhiteBishop)
				{
					tb = BishopMoves(to) & t & ~WhitePieces;
					while (tb)
					{
						uint8 to2 = ELSB(tb);
						AddMove(to, to2);
					}
				}
				else if (Piece[to] == ptWhiteKing)
				{
					tb = KingMoves[to] & t & ~WhitePieces;
					while (tb)
					{
						uint8 to2 = ELSB(tb);
						if (OnLine[to][to2] != OnLine[from][king]) AddMove(to, to2);
					}
				}
			} else if (Piece[to] == ptNone || BlackPiece(Piece[to])) AddMove(from, to);
		}
	}

	if ((WhitePawns & 0x00FEFEFEFEFE0000) >> 17 & BlackKing && Piece[king + 9] == ptNone) AddMove(king + 17, king + 9);
	if ((WhitePawns & 0x007F7F7F7F7F0000) >> 15 & BlackKing && Piece[king + 7] == ptNone) AddMove(king + 15, king + 7);
	if (BlackKing << 9 & (WhitePawns & 0x00FCFCFCFCFC0000) >> 9 & t & BlackPieces) AddMove(king + 18, king + 9);
	if (BlackKing << 7 & (WhitePawns & 0x00FEFEFEFEFE0000) >> 9 & t & BlackPieces) AddMove(king + 16, king + 7);
	if (BlackKing << 9 & (WhitePawns & 0x007F7F7F7F7F0000) >> 7 & t & BlackPieces) AddMove(king + 16, king + 9);
	if (BlackKing << 7 & (WhitePawns & 0x003F3F3F3F3F0000) >> 7 & t & BlackPieces) AddMove(king + 14, king + 7);
	if ((WhitePawns & 0x00FE000000000000) >> 25 & BlackKing && Piece[king + 9] == ptNone && Piece[king + 17] == ptNone) AddMove(king + 25, king + 9, mfPawnJump);
	if ((WhitePawns & 0x007F000000000000) >> 23 & BlackKing && Piece[king + 7] == ptNone && Piece[king + 15] == ptNone) AddMove(king + 23, king + 7, mfPawnJump);
}

void MoveList::GenBlackChecks(BitBoard t)
{
	uint8 from, to;
	BitBoard fb, tb, mask;

	uint8 king = LSB(WhiteKing);
	
	mask = KnightMoves[king];
	fb = BlackKnights;
	while (fb)
	{
		from = ELSB(fb);
		BitBoard tb = KnightMoves[from] & mask & t & ~BlackPieces;
		while (tb)
		{
			to = ELSB(tb);
			AddMove(from, to);
		}
	}

	mask = BishopMoves(king);
	fb = BlackBishops | BlackQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = BishopMoves(from) & mask & t;
		while (tb)
		{
			to = ELSB(tb);
			if (BlackPiece(Piece[to]) && OnLine[from][king] && OnLine[from][king] <= 2)
			{
				// �������� ���
				if (Piece[to] == ptBlackPawn && Rank(to) < 6)
				{
					if (Piece[to + 8] == ptNone)
					{
						AddMove(to, to + 8);
						if (Rank(to) == 1 && Piece[to + 16] == ptNone) AddMove(to, to + 16, mfPawnJump);
					}
					if (File(to) < 7 && Piece[to + 9] && WhitePiece(Piece[to + 9]) && (SBM[to + 9] & t))
					{
						AddMove(to, to + 9, 0);
					}
					if (File(to) > 0 && Piece[to + 7] && WhitePiece(Piece[to + 7]) && (SBM[to + 7] & t))
					{
						AddMove(to, to + 7, 0);
					}
				}
				else if (Piece[to] == ptBlackKnight)
				{
					tb = KnightMoves[to] & t & ~BlackPieces;
					while (tb)
					{
						uint8 to2 = ELSB(tb);
						AddMove(to, to2);
					}
				}
				else if (Piece[to] == ptBlackRook)
				{
					tb = RookMoves(to) & t & ~BlackPieces;
					while (tb)
					{
						uint8 to2 = ELSB(tb);
						AddMove(to, to2);
					}
				}
				else if (Piece[to] == ptBlackKing)
				{
					tb = KingMoves[to] & t & ~BlackPieces;
					while (tb)
					{
						uint8 to2 = ELSB(tb);
						if (OnLine[to][to2] != OnLine[from][king]) AddMove(to, to2);
					}
				}
			} else if (Piece[to] == ptNone || WhitePiece(Piece[to])) AddMove(from, to);
		}
		if (Piece[from] == ptBlackQueen)
		{
			tb = (RookMoves(from) & mask | BishopMoves(from) & RookMoves(king)) & t & ~BlackPieces;
			while (tb)
			{
				to = ELSB(tb);
				AddMove(from, to);
			}
		}
	}

	mask = RookMoves(king);
	fb = BlackRooks | BlackQueens;
	while (fb)
	{
		from = ELSB(fb);
		tb = RookMoves(from) & mask & t;
		while (tb)
		{
			to = ELSB(tb);
			if (BlackPiece(Piece[to]) && OnLine[from][king] >= 3)
			{
				// �������� ���
				if (Piece[to] == ptBlackPawn)
				{
					if (Rank(to) < 6 && Piece[to + 8] == ptNone && OnLine[from][king] == 3)
					{
						AddMove(to, to + 8);
						if (Rank(to) == 6 && Piece[to + 16] == ptNone) AddMove(to, to + 16, mfPawnJump);
					}
					if (File(to) < 7 && Piece[to + 9] && WhitePiece(Piece[to + 9]) && (SBM[to + 9] & t))
					{
						AddMove(to, to + 9, Rank(to) < 6? 0 : mfPromo & ptBlackQueen);
					}
					if (File(to) > 0 && Piece[to + 7] && WhitePiece(Piece[to + 7]) && (SBM[to + 7] & t))
					{
						AddMove(to, to + 7, Rank(to) < 6? 0 : mfPromo & ptBlackQueen);
					}
				}
				else if (Piece[to] == ptBlackKnight)
				{
					tb = KnightMoves[to] & t & ~BlackPieces;
					while (tb)
					{
						uint8 to2 = ELSB(tb);
						AddMove(to, to2);
					}
				}
				else if (Piece[to] == ptBlackBishop)
				{
					tb = BishopMoves(to) & t & ~BlackPieces;
					while (tb)
					{
						uint8 to2 = ELSB(tb);
						AddMove(to, to2);
					}
				}
				else if (Piece[to] == ptBlackKing)
				{
					tb = KingMoves[to] & t & ~BlackPieces;
					while (tb)
					{
						uint8 to2 = ELSB(tb);
						if (OnLine[to][to2] != OnLine[from][king]) AddMove(to, to2);
					}
				}
			} else if (Piece[to] <= ptWhiteKing) AddMove(from, to);
		}
	}
	if ((BlackPawns & 0x0000FEFEFEFEFE00) << 15 & WhiteKing && Piece[king - 7] == ptNone) AddMove(king - 15, king - 7);
	if ((BlackPawns & 0x00007F7F7F7F7F00) << 17 & WhiteKing && Piece[king - 9] == ptNone) AddMove(king - 17, king - 9);
	if (WhiteKing >> 7 & (BlackPawns & 0x0000FCFCFCFCFC00) << 7 & t & WhitePieces) AddMove(king - 14, king - 7);
	if (WhiteKing >> 9 & (BlackPawns & 0x0000FEFEFEFEFE00) << 7 & t & WhitePieces) AddMove(king - 16, king - 9);
	if (WhiteKing >> 7 & (BlackPawns & 0x00007F7F7F7F7F00) << 9 & t & WhitePieces) AddMove(king - 16, king - 7);
	if (WhiteKing >> 9 & (BlackPawns & 0x00003F3F3F3F3F00) << 9 & t & WhitePieces) AddMove(king - 18, king - 9);
	if ((BlackPawns & 0x000000000000FE00) << 23 & WhiteKing && Piece[king - 7] == ptNone && Piece[king - 15] == ptNone) AddMove(king - 23, king - 7, mfPawnJump);
	if ((BlackPawns & 0x0000000000007F00) << 25 & WhiteKing && Piece[king - 9] == ptNone && Piece[king - 17] == ptNone) AddMove(king - 25, king - 9, mfPawnJump);
}
