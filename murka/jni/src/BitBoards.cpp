#include <stdlib.h>
#include <iostream>
using namespace std;

#include "BitBoards.h"
#include "Board.h"
#include "IO.h"

// �������� ��� �����
BitBoard WhitePieces, BlackPieces;
BitBoard BitBoards[13];
BitBoard &WhitePawns   = BitBoards[ptWhitePawn];
BitBoard &WhiteKnights = BitBoards[ptWhiteKnight];
BitBoard &WhiteBishops = BitBoards[ptWhiteBishop];
BitBoard &WhiteRooks   = BitBoards[ptWhiteRook];
BitBoard &WhiteQueens  = BitBoards[ptWhiteQueen];
BitBoard &WhiteKing    = BitBoards[ptWhiteKing];
BitBoard &BlackPawns   = BitBoards[ptBlackPawn];
BitBoard &BlackKnights = BitBoards[ptBlackKnight];
BitBoard &BlackBishops = BitBoards[ptBlackBishop];
BitBoard &BlackRooks   = BitBoards[ptBlackRook];
BitBoard &BlackQueens  = BitBoards[ptBlackQueen];
BitBoard &BlackKing    = BitBoards[ptBlackKing];

// ����
BitBoard KnightMoves[64], KingMoves[64];
BitBoard BishopMagicMoves[5248], RookMagicMoves[102400];

// ������� ��� ���������� ���������� �����
uint8 BishopMagicShift[64], RookMagicShift[64];
uint16 BishopMagicOffset[64];
uint32 RookMagicOffset[64];

// �����
BitBoard WPawnAtk[64], BPawnAtk[64];

// �� ������ ���� �������� ������� � ������������� ����� �� ������ ����
BitBoard SBM[64]; // Single Bit Mask

// �����
//------

// ���������
BitBoard WPasserMask[64], BPasserMask[64];
BitBoard SupportWKing[64], SupportBKing[64];
BitBoard WKingQuad[64], WKingQuadExt[64];
BitBoard BKingQuad[64], BKingQuadExt[64];
// ���������
BitBoard WCandidateMask[64], BCandidateMask[64];
// �������������
BitBoard IsolatedMask[64];
// ��������
BitBoard WBackwardMask[64], WBackwardAtkMask[64];
BitBoard BBackwardMask[64], BBackwardAtkMask[64];
// �����������
BitBoard ForwardMask[64], BackwardMask[64]; // ����� �����/����
BitBoard UpMask[64], DownMask[64]; // ������� ����/����
// ������ �� ����
BitBoard EvasionsMask[64][64];
BitBoard InterceptMask[64][64];
// �������
BitBoard BishopTrapBQ, BishopTrapBK, BishopTrapWQ, BishopTrapWK;
BitBoard RookBoxBQ, RookBoxBK, RookBoxWQ, RookBoxWK, RookBoxKBQ, RookBoxKBK, RookBoxKWQ, RookBoxKWK;
//-----------------------------------------------

// ��� ���� �� ����� �����/���������
uint8 OnLine[64][64];

// ����� �� ��� ����?
bool KnightMove(uint8 from, uint8 to)
{
	if (to > 63) return false;
	int fd = abs(File(from) - File(to));
	int rd = abs(Rank(from) - Rank(to));
	if (fd == 1 && rd == 2 || fd == 2 && rd == 1) return true;
	return false;
}

// ����� �� ��� ������?
bool KingMove(uint8 from, uint8 to)
{
	if (to > 63) return false;
	if (abs(File(from) - File(to)) > 1) return false;
	return true;
}

// �������� ������������ ����� ������� ������
BitBoard EnumMask(BitBoard mask, uint16 num)
{
	BitBoard x = 0;
	while (num)
	{
		uint8 sq = ELSB(mask);
		if (num & 1) x |= SBM[sq];
		num >>= 1;
	}
	return x;	
}

// �������� �������
void ShiftBitBoard(BitBoard &b, __int8 v)
{
	if (v > 0) b <<= v;
	else b >>= -v;
}

// ������������� ���� ���� �� ��������� �� ������� ����
// ��� ������ �� ������� ����� (���������� � ������� �����)
void TraceLine(uint8 sq, __int8 dir, BitBoard &b, BitBoard occ, BitBoard mask)
{
	BitBoard x = SBM[sq];
	x &= mask;
	ShiftBitBoard(x, dir);
	while (x)
	{
		b |= x;
		if (x & occ) break;
		x &= mask;
		ShiftBitBoard(x, dir);
	}
}

// ������������� ���������
void InitBitBoards()
{
	uint8 i;
	uint16 j;

	// ��������� ��������� �������
	uint16 bishop_ofs = 0;
	uint32 rook_ofs   = 0;
	for (i = 0; i < 64; i++)
	{
		SBM[i] = SB << i;

		BishopMagicOffset[i] = bishop_ofs;
		bishop_ofs += 1 << BishopMagicBits[i];

		RookMagicOffset[i] = rook_ofs;
		rook_ofs += 1 << RookMagicBits[i];

		BishopMagicShift[i] = 64 - BishopMagicBits[i];
		RookMagicShift[i] = 64 - RookMagicBits[i];
	}

	for (i = 0; i < 64; i++)
	{
		UpMask[i] = 0;
		WKingQuad[i] = WKingQuadExt[i] = 0;
		BKingQuad[i] = BKingQuadExt[i] = 0;

		// ��������� ��������� �������
		SupportWKing[i] = SupportBKing[i] = 0;
		if (Rank(i) > 0 && Rank(i) < 4)
		{
			if (File(i) > 0)
			{
				SupportWKing[i] |= SBM[File(i) + 7];
				if (Rank(i) < 3) SupportWKing[i] |= SBM[File(i) - 1];
			}
			if (File(i) < 7)
			{
				SupportWKing[i] |= SBM[File(i) + 9];
				if (Rank(i) < 3) SupportWKing[i] |= SBM[File(i) + 1];
			}
			if (File(i) > 0 && File(i) < 7)
			{
				SupportWKing[i] |= SBM[File(i) + 8];
				if (Rank(i) < 3) SupportWKing[i] |= SBM[File(i)];
			}
		}
		if (Rank(i) < 7 && Rank(i) > 3)
		{
			if (File(i) > 0)
			{
				SupportBKing[i] |= SBM[File(i) + 47];
				if (Rank(i) > 4) SupportBKing[i] |= SBM[File(i) + 55];
			}
			if (File(i) < 7)
			{
				SupportBKing[i] |= SBM[File(i) + 49];
				if (Rank(i) > 4) SupportBKing[i] |= SBM[File(i) + 57];
			}
			if (File(i) > 0 && File(i) < 7)
			{
				SupportBKing[i] |= SBM[File(i) + 48];
				if (Rank(i) > 4) SupportBKing[i] |= SBM[File(i) + 56];
			}
		}

		// ����� �����������
		ForwardMask[i] = 0;
		TraceLine(i, -8, ForwardMask[i],  0, 0xffffffffffffff00);
		BackwardMask[i] = 0;
		TraceLine(i,  8, BackwardMask[i], 0, 0x00ffffffffffffff);

		for (j = 0; j < 64; j++)
		{
			if (Rank(i) > Rank(j)) UpMask[i]   |= SBM[j];
			if (Rank(i) < Rank(j)) DownMask[i] |= SBM[j];
			OnLine[i][j] = 0;
			if (i == j) continue;

			// ������� ������� ������
			if (Rank(i) < 6)
			{
				if (Rank(i) >= Rank(j) && abs(File(i) - File(j)) <= Rank(i))
				{
					BKingQuad[i] |= SBM[j];
				}
				if (Rank(i) + 1 >= Rank(j) && abs(File(i) - File(j)) <= Rank(i) + 1)
				{
					BKingQuadExt[i] |= SBM[j];
				}
			}
			else if (Rank(i) == 6)
			{
				if (Rank(i) > Rank(j) && abs(File(i) - File(j)) < Rank(i))
				{
					BKingQuad[i] |= SBM[j];
				}
				if (Rank(i) >= Rank(j) && abs(File(i) - File(j)) <= Rank(i))
				{
					BKingQuadExt[i] |= SBM[j];
				}
			}
			
			// ������� ������ ������
			if (Rank(i) > 1)
			{
				if (Rank(i) <= Rank(j) && abs(File(i) - File(j)) <= 7 - Rank(i))
				{
					WKingQuad[i] |= SBM[j];
				}
				if (Rank(i) <= Rank(j) + 1 && abs(File(i) - File(j)) <= 8 - Rank(i))
				{
					WKingQuadExt[i] |= SBM[j];
				}
			}
			else if (Rank(i) == 1)
			{
				if (Rank(i) < Rank(j) && abs(File(i) - File(j)) < 7 - Rank(i))
				{
					WKingQuad[i] |= SBM[j];
				}
				if (Rank(i) <= Rank(j) && abs(File(i) - File(j)) < 8 - Rank(i))
				{
					WKingQuadExt[i] |= SBM[j];
				}
			}
			
			if (Rank(i) == Rank(j))
			{
				OnLine[i][j] = 3;
				if (File(i) < File(j))
				{
					EvasionsMask[i][j] = ~((SBM[i] & ~FileA) >> 1);
					InterceptMask[i][j] = 0;
					TraceLine(i, 1, InterceptMask[i][j], SBM[j], -1);
				}
				else 
				{
					EvasionsMask[i][j] = ~((SBM[i] & ~FileH) << 1);
					InterceptMask[i][j] = 0;
					TraceLine(i, -1, InterceptMask[i][j], SBM[j], -1);
				}
			}
			else if (File(i) == File(j))
			{
				OnLine[i][j] = 4;
				if (Rank(i) < Rank(j))
				{
					EvasionsMask[i][j] = ~((SBM[i] & ~Rank8) >> 8);
					InterceptMask[i][j] = 0;
					TraceLine(i, 8, InterceptMask[i][j], SBM[j], -1);
				}
				else 
				{
					EvasionsMask[i][j] = ~((SBM[i] & ~Rank1) << 8);
					InterceptMask[i][j] = 0;
					TraceLine(i, -8, InterceptMask[i][j], SBM[j], -1);
				}
			}
			else if (File(i) + Rank(j) == Rank(i) + File(j))
			{
				OnLine[i][j] = 1;
				if (File(i) > File(j))
				{
					EvasionsMask[i][j] = ~((SBM[i] & ~Rank1 & ~FileH) << 9);
					InterceptMask[i][j] = 0;
					TraceLine(i, -9, InterceptMask[i][j], SBM[j], -1);
				}
				else
				{
					EvasionsMask[i][j] = ~((SBM[i] & ~Rank8 & ~FileA) >> 9);
					InterceptMask[i][j] = 0;
					TraceLine(i, 9, InterceptMask[i][j], SBM[j], -1);
				}
			}
			else if (File(i) + Rank(i) == File(j) + Rank(j))
			{
				OnLine[i][j] = 2;
				if (File(i) < File(j))
				{
					EvasionsMask[i][j] = ~((SBM[i] & ~Rank1 & ~FileA) << 7);
					InterceptMask[i][j] = 0;
					TraceLine(i, -7, InterceptMask[i][j], SBM[j], -1);
				}
				else
				{
					EvasionsMask[i][j] = ~((SBM[i] & ~Rank8 & ~FileH) >> 7);
					InterceptMask[i][j] = 0;
					TraceLine(i, 7, InterceptMask[i][j], SBM[j], -1);
				}
			}
			else
			{
				EvasionsMask[i][j] = -1;
				InterceptMask[i][j] = SBM[j];
			}
		}

		// ����� ���������
		WPasserMask[i] = 0;
		if (File(i) > 0) TraceLine(i - 1, -8, WPasserMask[i], 0, 0xffffffffffffff00);
		TraceLine(i, -8, WPasserMask[i], 0, 0xffffffffffffff00);
		if (File(i) < 7) TraceLine(i + 1, -8, WPasserMask[i], 0, 0xffffffffffffff00);
		
		BPasserMask[i] = 0;
		if (File(i) > 0) TraceLine(i - 1, 8, BPasserMask[i], 0, 0x00ffffffffffffff);
		TraceLine(i, 8, BPasserMask[i], 0, 0x00ffffffffffffff);
		if (File(i) < 7) TraceLine(i + 1, 8, BPasserMask[i], 0, 0x00ffffffffffffff);

		// ����� �������������
		IsolatedMask[i] = 0;
		if (File(i) > 0) IsolatedMask[i] |= FileMask[File(i) - 1];
		if (File(i) < 7) IsolatedMask[i] |= FileMask[File(i) + 1];

		// ����� ��������
		WBackwardMask[i] = WBackwardAtkMask[i] = 0;
		if (Rank(i) > 2 && Rank(i) < 7)
		{
			if (File(i) > 0)
			{
				for (j = i - 1; Rank(j) < 7; j += 8) WBackwardMask[i] |= SBM[j];
			}
			if (File(i) < 7)
			{
				for (j = i + 1; Rank(j) < 7; j += 8) WBackwardMask[i] |= SBM[j];
			}
		}
		BBackwardMask[i] = BBackwardAtkMask[i] = 0;
		if (Rank(i) < 5 && Rank(i) > 0)
		{
			if (File(i) < 7)
			{
				for (j = i + 1; Rank(j) > 0; j -= 8) BBackwardMask[i] |= SBM[j];
			}
			if (File(i) > 0)
			{
				for (j = i - 1; Rank(j) > 0; j -= 8) BBackwardMask[i] |= SBM[j];
			}
		}
		// ����� ����������
		WCandidateMask[i] = 0;
		if (Rank(i) > 1 && Rank(i) < 7)
		{
			if (File(i) > 0)
				for (j = i - 1; Rank(j) < 7; j += 8) WCandidateMask[i] |= SBM[j];
			if (File(i) < 7)
				for (j = i + 1; Rank(j) < 7; j += 8) WCandidateMask[i] |= SBM[j];
		}
		BCandidateMask[i] = 0;
		if (Rank(i) < 6 && Rank(i) > 0)
		{
			if (File(i) < 7)
				for (j = i + 1; Rank(j) > 0; j -= 8) BCandidateMask[i] |= SBM[j];
			if (File(i) > 0)
			{
				for (j = i - 1; Rank(j) > 0; j -= 8) BCandidateMask[i] |= SBM[j];
			}
		}
		
		// �����
		WPawnAtk[i] = 0;
		BPawnAtk[i] = 0;
		if (i > 15)
		{
			if (File(i) != 0) BPawnAtk[i] |= SB << i - 9;
			if (File(i) != 7) BPawnAtk[i] |= SB << i - 7;
		}
		if (i < 48)
		{
			if (File(i) != 0) WPawnAtk[i] |= SB << i + 7;
			if (File(i) != 7) WPawnAtk[i] |= SB << i + 9;
		}

		// ���� ����
		KnightMoves[i] = 0;
		if (KnightMove(i, i - 10)) KnightMoves[i] |= SB << i - 10;
		if (KnightMove(i, i - 17)) KnightMoves[i] |= SB << i - 17;
		if (KnightMove(i, i - 15)) KnightMoves[i] |= SB << i - 15;
		if (KnightMove(i, i -  6)) KnightMoves[i] |= SB << i -  6;
		if (KnightMove(i, i +  6)) KnightMoves[i] |= SB << i +  6;
		if (KnightMove(i, i + 15)) KnightMoves[i] |= SB << i + 15;
		if (KnightMove(i, i + 17)) KnightMoves[i] |= SB << i + 17;
		if (KnightMove(i, i + 10)) KnightMoves[i] |= SB << i + 10;

		// ���� ������
		KingMoves[i] = 0;
		if (KingMove(i, i - 9)) KingMoves[i] |= SB << i - 9;
		if (KingMove(i, i - 8)) KingMoves[i] |= SB << i - 8;
		if (KingMove(i, i - 7)) KingMoves[i] |= SB << i - 7;
		if (KingMove(i, i - 1)) KingMoves[i] |= SB << i - 1;
		if (KingMove(i, i + 1)) KingMoves[i] |= SB << i + 1;
		if (KingMove(i, i + 7)) KingMoves[i] |= SB << i + 7;
		if (KingMove(i, i + 8)) KingMoves[i] |= SB << i + 8;
		if (KingMove(i, i + 9)) KingMoves[i] |= SB << i + 9;

		// ���� ������
		for (j = 0; j < (1 << BishopMagicBits[i]); j++)
		{
			BitBoard occupied = EnumMask(BishopMagicMask[i], j);
			BitBoard mb = 0;
			TraceLine(i, 9, mb, occupied, 0x007f7f7f7f7f7f7f);
			TraceLine(i, 7, mb, occupied, 0x00fefefefefefefe);
			TraceLine(i,-7, mb, occupied, 0x7f7f7f7f7f7f7f00);
			TraceLine(i,-9, mb, occupied, 0xfefefefefefefe00);
			uint16 index = BishopMagicOffset[i];
			index += occupied * BishopMagicMult[i] >> BishopMagicShift[i];
			BishopMagicMoves[index] = mb;
		}
		
		// ���� �����
		for (j = 0; j < (1 << RookMagicBits[i]); j++)
		{
			BitBoard occupied = EnumMask(RookMagicMask[i], j);
			BitBoard mb = 0;
			TraceLine(i, 8, mb, occupied, 0x00ffffffffffffff);
			TraceLine(i,-8, mb, occupied, 0xffffffffffffff00);
			TraceLine(i, 1, mb, occupied, 0x7f7f7f7f7f7f7f7f);
			TraceLine(i,-1, mb, occupied, 0xfefefefefefefefe);
			uint32 index = RookMagicOffset[i];
			index += occupied * RookMagicMult[i] >> RookMagicShift[i];
			RookMagicMoves[index] = mb;
		}
	}

	BishopTrapBQ = SBM[C7] | SBM[B6] | SBM[B5];
	BishopTrapBK = SBM[F7] | SBM[G6] | SBM[G5];
	BishopTrapWQ = SBM[C2] | SBM[B3] | SBM[B4];
	BishopTrapWK = SBM[F2] | SBM[G3] | SBM[G4];
	RookBoxWK = SBM[H1] | SBM[G1] | SBM[H2];
	RookBoxWQ = SBM[A1] | SBM[B1] | SBM[A2];
	RookBoxBK = SBM[H8] | SBM[G8] | SBM[H7];
	RookBoxBQ = SBM[A8] | SBM[B8] | SBM[A7];
	RookBoxKWK = SBM[G1] | SBM[F1];
	RookBoxKWQ = SBM[B1] | SBM[C1];
	RookBoxKBK = SBM[G8] | SBM[F8];
	RookBoxKBQ = SBM[B8] | SBM[C8];
}
