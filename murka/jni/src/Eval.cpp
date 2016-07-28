#include <stdlib.h>
#include <iostream>
using namespace std;
#include <assert.h>

#include "Eval.h"
#include "Board.h"
#include "BitBoards.h"
#include "Search.h"
#include "IO.h"
#include "Hash.h"


const uint8 efWhiteKingSafety = 1;
const uint8 efBlackKingSafety = 2;
const uint8 efBishopEnding = 4;

MaterialEntry *Material = new MaterialEntry[MaxMaterial];

int bp, wp, bn, wn, bb, wb, br, wr, bq, wq;

// сигнатура для индексации таблицы материала
void InitMaterialSignature()
{
	NI->MS = 0;
	for (uint8 i = 0; i < 64; i++)
	{
		NI->MS += MSValue[Piece[i]];
	}
}

// ничейность материального соотношения
bool DrawishMaterial()
{
	int w = 3 * (wn + wb) + 5 * wr + 10 * wq;
	int b = 3 * (bn + bb) + 5 * br + 10 * bq;
	if (!wp && w > b + bp)
	{
		if (w == 6 && wn == 2 && !bp) return true;
		if (w > b + 3) return false;
		if (w == b + 3 && wb == 2 && !bb) return false;
		return true;
	}
	if (!bp && b > w + wp)
	{
		if (b == 6 && bn == 2 && !wp) return true;
		if (b > w + 3) return false;
		if (b == w + 3 && bb == 2 && !wb) return false;
		return true;
	}
	return false;
}

// инициализируем таблицу материала
#define Mix(op, eg) (((eg) * Material[ms].phase + (op) * (24 - Material[ms].phase)) / 24)
void InitMaterial()
{

	for (wq = 0; wq < 2; wq++)
	for (bq = 0; bq < 2; bq++)
	for (wr = 0; wr < 3; wr++)
	for (br = 0; br < 3; br++)
	for (wb = 0; wb < 3; wb++)
	for (bb = 0; bb < 3; bb++)
	for (wn = 0; wn < 3; wn++)
	for (bn = 0; bn < 3; bn++)
	for (wp = 0; wp < 9; wp++)
	for (bp = 0; bp < 9; bp++)
	{
		uint32 ms =
			wq * MSValue[ptWhiteQueen]  +
			bq * MSValue[ptBlackQueen]  +
			wr * MSValue[ptWhiteRook]   +
			br * MSValue[ptBlackRook]   +
			wb * MSValue[ptWhiteBishop] +
			bb * MSValue[ptBlackBishop] +
			wn * MSValue[ptWhiteKnight] +
			bn * MSValue[ptBlackKnight] +
			wp * MSValue[ptWhitePawn]   +
			bp * MSValue[ptBlackPawn];

		// фаза игры
		Material[ms].phase = 24 - (wq*2 + bq*2 + wr + br) * 2 - (wb + bb + wn + bn);
		 
		 int mat_op = 
			(wq - bq) * PieceValueOp[ptWhiteQueen]  +
			(wr - br) * PieceValueOp[ptWhiteRook]   +
			(wb - bb) * PieceValueOp[ptWhiteBishop] +
			(wn - bn) * PieceValueOp[ptWhiteBishop] +
			(wp - bp) * PieceValueOp[ptWhitePawn];

		 int mat_eg = 
			(wq - bq) * PieceValueEg[ptWhiteQueen]  +
			(wr - br) * PieceValueEg[ptWhiteRook]   +
			(wb - bb) * PieceValueEg[ptWhiteBishop] +
			(wn - bn) * PieceValueEg[ptWhiteBishop] +
			(wp - bp) * PieceValueEg[ptWhitePawn];

		int mat = (mat_eg * Material[ms].phase + mat_op * (24 - Material[ms].phase)) / 24;

		// коррекция материала
		if (DrawishMaterial()) Material[ms].bonus = -mat;
		else
		{
			Material[ms].bonus = 0;

			// пара слонов
			if (wb > 1)
			{
				Material[ms].bonus += Mix(ebBishopPairOp, ebBishopPairEg);
				Material[ms].bonus -= Mix(0, (wp - 5) * BishopPairPawnMat);
				if (bb + bn == 0) Material[ms].bonus += ebBishopPairVsNoLight;
			}
			if (bb > 1)
			{
				Material[ms].bonus -= Mix(ebBishopPairOp, ebBishopPairEg);
				Material[ms].bonus += Mix(0, (bp - 5) * BishopPairPawnMat);
				if (wb + wn == 0) Material[ms].bonus -= ebBishopPairVsNoLight;
			}
			
			// пара ладей
			if (wr > 1) Material[ms].bonus -= Mix(epRookPairOp, epRookPairEg);
			if (br > 1) Material[ms].bonus += Mix(epRookPairOp, epRookPairEg);
			
			// ладья + ферзь
			if (wr + wq > 1) Material[ms].bonus -= Mix(epRookQueenOp, epRookQueenEg);
			if (br + bq > 1) Material[ms].bonus += Mix(epRookQueenOp, epRookQueenEg);
			
			// конь/пешки
			Material[ms].bonus += Mix(0, (wp - 5) * wn * KnightPawnMat);
			Material[ms].bonus -= Mix(0, (bp - 5) * bn * KnightPawnMat);
			
			// ладья/пешки
			Material[ms].bonus -= Mix((wp - 5) * wr * BishopPawnMat, 0);
			Material[ms].bonus += Mix((bp - 5) * br * BishopPawnMat, 0);
		}

		// флаги
		Material[ms].flags = 0;
		// безопасность королей
		if (bq && br + bb + bn > 0) Material[ms].flags |= efWhiteKingSafety;
		if (wq && wr + wb + wn > 0) Material[ms].flags |= efBlackKingSafety;
		// слоновое окончание (для проверки разноцвета)
		if (wn + wr + wq + bn + br + bq == 0 && wb == 1 && bb == 1 && abs(wp - bp) < 3)
			Material[ms].flags |= efBishopEnding;
	}
}
#undef Mix

__int32 PST[13][64][2];

// инициализация таблицы ценности полей
void InitPST()
{
	int KnightLine[8] = {-4,-2, 0, 1, 1, 0,-2,-4};
	int KnightRank[8] = {-2,-1, 0, 1, 2, 3, 2, 1};
	int BishopLine[8] = {-3,-1, 0, 1, 1, 0,-1,-3};
	int RookFile[8]   = {-2,-1, 0, 1, 1, 0,-1,-2};
	int KingLine[8]   = {-3,-1, 0, 1, 1, 0,-1,-3};
	int KingFile[8]   = { 3 ,4, 2, 0, 0, 2, 4, 3};

	uint8 i, j, k;
	for (i = 0; i < 64; i++)
	{
		uint8 f = File(i);
		uint8 r = 7 - Rank(i);

		PST[ptWhiteKnight][i][0] = KnightLine[f] * KnightCenterOp + KnightLine[r] * KnightCenterOp + KnightRank[r] * KnightRankOp;
		PST[ptWhiteKnight][i][1] = KnightLine[f] * KnightCenterEg + KnightLine[r] * KnightCenterEg + KnightRank[r] * KnightRankEg;

		PST[ptWhiteBishop][i][0] = BishopLine[f] * BishopCenterOp + BishopLine[r] * BishopCenterOp;
		PST[ptWhiteBishop][i][1] = BishopLine[f] * BishopCenterEg + BishopLine[r] * BishopCenterEg;

		PST[ptWhiteRook][i][0] = RookFile[f] * RookFileOp;
		PST[ptWhiteRook][i][1] = RookFile[f] * RookFileEg;

		PST[ptWhiteKing][i][0] = KingFile[f] * KingFileOp;
		PST[ptWhiteKing][i][1] = KingLine[f] * KingCenterEg + KingLine[r] * KingCenterEg;
	}

	for (i = 0; i < 8; i++)
	{
		PST[ptWhiteBishop][A1 + i][0]             -= BishopBackRankOp;
		PST[ptWhiteBishop][i << 3 | i][0]         += BishopDiagonalOp;
		PST[ptWhiteBishop][i << 3 | i][0]         += BishopDiagonalEg;
		PST[ptWhiteBishop][Mirror[i << 3 | i]][0] += BishopDiagonalOp;
		PST[ptWhiteBishop][Mirror[i << 3 | i]][1] += BishopDiagonalEg;
	}

	// добавляем материал
	for (i = 1; i < 6; i++)
	{
		for (j = 0; j < 64; j++)
		{
			PST[i][j][0] += PieceValueOp[i];
			PST[i][j][1] += PieceValueEg[i];
		}
	}

	// заполняем таблицы для черных
	for (i = 7; i <= 12; i++)
	{
		for (j = 0; j < 64; j++)
		{
			for (k = 0; k < 2; k++)
			{
				PST[i][j][k] = -PST[i - 6][Mirror[j]][k];
			}
		}
	}
}

// расчитываем текущее значение ценности занимаемых полей
void CalcPST()
{
	NI->OP = NI->EG = 0;
	for (uint8 sq = 0; sq < 64; sq++)
	{
		if (!Piece[sq]) continue;
		NI->OP += PST[Piece[sq]][sq][0];
		NI->EG += PST[Piece[sq]][sq][1];
	}
}

// пешечный щит
//-------------

__int32 *PawnShield = new __int32[4096];

inline __int32 CalcShieldWC()
{
	return PawnShield[(WhitePawns >> 18 & 0x0e00) | (WhitePawns >> 29 & 0x01c0) | (WhitePawns >> 40 & 0x0038) | (WhitePawns >> 51 & 0x0007)];
}

inline __int32 CalcShieldWK()
{
	return PawnShield[(WhitePawns >> 20 & 0x0e00) | (WhitePawns >> 31 & 0x01c0) | (WhitePawns >> 42 & 0x0038) | (WhitePawns >> 53 & 0x0007)];
}

inline __int32 CalcShieldWQ()
{
	return PawnShield[(WhitePawns >> 15 & 0x0e00) | (WhitePawns >> 26 & 0x01c0) | (WhitePawns >> 37 & 0x0038) | (WhitePawns >> 48 & 0x0007)];
}

inline __int32 CalcShieldBC()
{
	return PawnShield[(BlackPawns >> 26 & 0x0e00) | (BlackPawns >> 21 & 0x01c0) | (BlackPawns >> 16 & 0x0038) | (BlackPawns >> 11 & 0x0007)];
}

inline __int32 CalcShieldBK()
{
	return PawnShield[(BlackPawns >> 28 & 0x0e00) | (BlackPawns >> 23 & 0x01c0) | (BlackPawns >> 18 & 0x0038) | (BlackPawns >> 13 & 0x0007)];
}

inline __int32 CalcShieldBQ()
{
	return PawnShield[(BlackPawns >> 23 & 0x0e00) | (BlackPawns >> 18 & 0x01c0) | (BlackPawns >> 13 & 0x0038) | (BlackPawns >>  8 & 0x0007)];
}

__int32 InitShieldLine(uint8 mask)
{
	if (mask & 1) return 0;
	if (mask & 2) return epMissingPawn1;
	if (mask & 4) return epMissingPawn2;
	if (mask & 8) return epMissingPawn3;
	return epMissingPawn;
}

__int32 InitShield(uint16 mask)
{
	__int32 res = InitShieldLine((mask >> 0 & 1) | (mask >> 2 & 2) | (mask >> 4 & 4) | (mask >> 6 & 8));
	res +=    2 * InitShieldLine((mask >> 1 & 1) | (mask >> 3 & 2) | (mask >> 5 & 4) | (mask >> 7 & 8));
	res +=        InitShieldLine((mask >> 2 & 1) | (mask >> 4 & 2) | (mask >> 6 & 4) | (mask >> 8 & 8));
	if (!res) return epWeakBackRank;
	return res;
}

void InitShield()
{
	for (uint16 i = 0; i < 4096; i++)
	{
		PawnShield[i] = InitShield(i);
	}
}

inline __int32 Distance(uint8 s1, uint8 s2)
{
	return max(abs(Rank(s1) - Rank(s2)), abs(File(s1) - File(s2)));
}

__int32 Dist[64][64];

// инициализация вспомогательных массивов для оценочной функции
void InitEval()
{
	InitPST();
	InitMaterial();
	InitShield();
	for (uint8 i = 0; i < 64; i++)
	{
		for (uint8 j = 0; j < 64; j++)
		{
			Dist[i][j] = Distance(i, j);
		}
	}
}

PawnHashEntry *PawnHash = new PawnHashEntry[PawnHashMask + 1];

// проверка на корректность инкрементального вычисления ценности полей
bool TestPST(__int32 op, __int32 eg)
{
	CalcPST();
	if (op != NI->OP || eg != NI->EG) return false;
	return true;
}

// Статическая оценка позиции
__int16 Eval()
{
	BitBoard t;
	uint8 sq;

	// ценность полей + материал
	__int32 op = NI->OP;
	__int32 eg = NI->EG;

	assert(TestPST(op, eg));

	// коррекция материала
	op += Material[NI->MS].bonus;
	eg += Material[NI->MS].bonus;

	uint8 wk = LSB(WhiteKing);
	uint8 bk = LSB(BlackKing);

	// переменные для расчета атак на королей
	__int32 watk_cnt = 0;
	__int32 watk_val = 0;
	__int32 batk_cnt = 0;
	__int32 batk_val = 0;

	BitBoard wking_atk = KingMoves[wk];
	BitBoard wa = wking_atk; // атакованные белыми поля (white attacks)
	BitBoard bking_atk = KingMoves[bk];
	BitBoard ba = bking_atk; // черными
	
	// атаки белых пешек
	BitBoard wpawn_atk = ((WhitePawns & ~FileA) >> 9) | (WhitePawns & ~FileH) >> 7;
	wa |= wpawn_atk;
	if (wpawn_atk & bking_atk) watk_cnt++;
	// безопасные для черных поля
	BitBoard bsafe = ~wpawn_atk;

	BitBoard bpawn_atk = ((BlackPawns & ~FileA) << 7) | ((BlackPawns & ~FileH) << 9);
	ba |= bpawn_atk;
	if (bpawn_atk & wking_atk) batk_cnt++;
	BitBoard wsafe = ~bpawn_atk;

	// заблокированные пешки
	int wpawn_blocked = PopCnt(WhitePawns >> 8 & (WhitePieces | BlackPieces));
	int bpawn_blocked = PopCnt(BlackPawns << 8 & (WhitePieces | BlackPieces));
	op += (bpawn_blocked - wpawn_blocked) * BlockedPawnOp;
	eg += (bpawn_blocked - wpawn_blocked) * BlockedPawnEg;

	// кони
	t = WhiteKnights;
	while (t)
	{
		sq = ELSB(t);
		BitBoard a = KnightMoves[sq];
		wa |= a;
		// атака короля
		if (a & bking_atk)
		{
			watk_cnt++;
			watk_val += KnightAttackVal;
		}
		// мобильность
		a &= ~WhitePieces;
		uint8 mob = PopCnt(a & (wsafe | BlackPieces & ~BlackPawns));
		op += mob * ebKnightMobilityOp;
		eg += mob * ebKnightMobilityEg;
	}

	t = BlackKnights;
	while (t)
	{
		sq = ELSB(t);
		BitBoard a = KnightMoves[sq];
		ba |= a;
		if (a & wking_atk)
		{
			batk_cnt++;
			batk_val += KnightAttackVal;
		}
		a &= ~BlackPieces;
		uint8 mob = PopCnt(a & (bsafe | WhitePieces & ~WhitePawns));
		op -= mob * ebKnightMobilityOp;
		eg -= mob * ebKnightMobilityEg;
	}
	
	// слоны
	t = WhiteBishops;
	while (t)
	{
		sq = ELSB(t);
		BitBoard a = BishopMoves(sq);
		wa |= a;
		if (a & bking_atk)
		{
			watk_cnt++;
			watk_val += BishopAttackVal;
		}
		a &= ~WhitePieces;
		uint8 mob = PopCnt(a & (wsafe | BlackPieces & ~BlackPawns));
		op += mob * ebBishopMobilityOp;
		eg += mob * ebBishopMobilityEg;
	}

	t = BlackBishops;
	while (t)
	{
		sq = ELSB(t);
		BitBoard a = BishopMoves(sq);
		ba |= a;
		if (a & wking_atk)
		{
			batk_cnt++;
			batk_val += BishopAttackVal;
		}
		a &= ~BlackPieces;
		uint8 mob = PopCnt(a & (bsafe | WhitePieces & ~WhitePawns));
		op -= mob * ebBishopMobilityOp;
		eg -= mob * ebBishopMobilityEg;
	}

	// ладьи
	t = WhiteRooks;
	while (t)
	{
		sq = ELSB(t);
		BitBoard a = RookMoves(sq);
		wa |= a;
		if (a & bking_atk)
		{
			watk_cnt++;
			watk_val += RookAttackVal;
		}
		a &= ~WhitePieces;
		uint8 mob = PopCnt(a & (wsafe | BlackQueens | BlackRooks));
		op += mob * ebRookMobilityOp;
		eg += mob * ebRookMobilityEg;
		// открытые/полуоткрытые линии
		if (!(ForwardMask[sq] & WhitePawns))
		{
			if (!(ForwardMask[sq] & BlackPawns))
			{
				op += ebRookOpenOp;
				eg += ebRookOpenEg;
			}
			else
			{
				op += ebRookHalfOpenOp;
				eg += ebRookHalfOpenEg;
			}
		}
	}

	t = BlackRooks;
	while (t)
	{
		sq = ELSB(t);
		BitBoard a = RookMoves(sq);
		ba |= a;
		if (a & wking_atk)
		{
			batk_cnt++;
			batk_val += RookAttackVal;
		}
		a &= ~BlackPieces;
		uint8 mob = PopCnt(a & (bsafe | WhiteQueens | WhiteRooks));
		op -= mob * ebRookMobilityOp;
		eg -= mob * ebRookMobilityEg;
		if (!(BackwardMask[sq] & BlackPawns))
		{
			if (!(BackwardMask[sq] & WhitePawns))
			{
				op -= ebRookOpenOp;
				eg -= ebRookOpenEg;
			}
			else
			{
				op -= ebRookHalfOpenOp;
				eg -= ebRookHalfOpenEg;
			}
		}
	}

	// ферзи
	t = WhiteQueens;
	while (t)
	{
		sq = ELSB(t);
		BitBoard a = RookMoves(sq) | BishopMoves(sq);
		wa |= a;
		if (a & bking_atk)
		{
			watk_cnt++;
			watk_val += QueenAttackVal;
		}
		a &= ~WhitePieces;
		uint8 mob = PopCnt(a & (wsafe | BlackQueens));
		op += mob * ebQueenMobilityOp;
		eg += mob * ebQueenMobilityEg;
	}

	t = BlackQueens;
	while (t)
	{
		sq = ELSB(t);
		BitBoard a = RookMoves(sq) | BishopMoves(sq);
		ba |= a;
		if (a & wking_atk)
		{
			batk_cnt++;
			batk_val += QueenAttackVal;
		}
		a &= ~BlackPieces;
		uint8 mob = PopCnt(a & (bsafe | WhiteQueens));
		op -= mob * ebQueenMobilityOp;
		eg -= mob * ebQueenMobilityEg;
	}

	// пешки
	PawnHashEntry *phe = PawnHash + (NI->PawnHashKey & PawnHashMask);
#ifndef TEST
	if (phe->Key != NI->PawnHashKey)
#endif
	{
		phe->passersb = phe->passersw = 0;
		__int32 pop = 0;
		__int32 peg = 0;
		t = WhitePawns;
		while (t)
		{
			sq = ELSB(t);

			uint8 open;
			// открытые/закрытые
			if (ForwardMask[sq] & (WhitePawns | BlackPawns)) open = 0;
			else open = 1;
			// изолированные
			if (!(IsolatedMask[sq] & WhitePawns))
			{
				if (open)
				{
					pop -=epOpenIsolatedPawnOp;
					peg -=epOpenIsolatedPawnEg;
				}
				else
				{
					pop -= epIsolatedPawnOp;
					peg -= epIsolatedPawnEg;
				}
			}
			// отсталые
			else if (!(WBackwardMask[sq] & WhitePawns))
			{
				if ((BPawnAtk[sq -  8] & BlackPawns) ||
					(BPawnAtk[sq - 16] & BlackPawns) && !(BPawnAtk[sq] & WhitePawns))
					if (open)
					{
						pop -= epOpenBackwardPawnOp;
						peg -= epOpenBackwardPawnEg;
					}
					else
					{
						pop -= epBackwardPawnOp;
						peg -= epBackwardPawnEg;
					}
			}
			//проходные
			if (!(WPasserMask[sq] & BlackPawns) && !(ForwardMask[sq] & WhitePawns))
			{
				phe->passersw |= SBM[sq];
				pop += ebPassedPawnOp[7 - Rank(sq)];
				peg += ebPassedPawnEg[7 - Rank(sq)];
			}
			// кандидаты
			else if (open && PopCnt(WCandidateMask[sq] & WhitePawns) >= PopCnt(WPasserMask[sq] & BlackPawns))
			{
				pop += ebCandidatePawnOp[7 - Rank(sq)];
				peg += ebCandidatePawnEg[7 - Rank(sq)];
			}
		}

		t = BlackPawns;
		while (t)
		{
			sq = ELSB(t);

			uint8 open;
			if (BackwardMask[sq] & (WhitePawns | BlackPawns)) open = 0;
			else open = 1;
			if (!(IsolatedMask[sq] & BlackPawns))
			{
				if (open)
				{
					pop += epOpenIsolatedPawnOp;
					peg += epOpenIsolatedPawnEg;
				}
				else
				{
					pop += epIsolatedPawnOp;
					peg += epIsolatedPawnEg;
				}
			}
			else if (!(BBackwardMask[sq] & BlackPawns))
			{
				if ((WPawnAtk[sq +  8] & WhitePawns) ||
					(WPawnAtk[sq + 16] & WhitePawns) && !(WPawnAtk[sq] & BlackPawns))
					if (open)
					{
						pop += epOpenBackwardPawnOp;
						peg += epOpenBackwardPawnEg;
					}
					else
					{
						pop += epBackwardPawnOp;
						peg += epBackwardPawnEg;
					}
			}
			if (!(BPasserMask[sq] & WhitePawns) && !(BackwardMask[sq] & BlackPawns))
			{
				phe->passersb |= SBM[sq];
				pop -= ebPassedPawnOp[Rank(sq)];
				peg -= ebPassedPawnEg[Rank(sq)];
			}
			else if (open && PopCnt(BCandidateMask[sq] & BlackPawns) >= PopCnt(BPasserMask[sq] & WhitePawns))
			{
				pop -= ebCandidatePawnOp[Rank(sq)];
				peg -= ebCandidatePawnEg[Rank(sq)];
			}
		}
		phe->Key = NI->PawnHashKey;
		phe->op = pop;
		phe->eg = peg;

		phe->ShieldWK = CalcShieldWK();
		phe->ShieldWQ = CalcShieldWQ();
		phe->ShieldWC = CalcShieldWC();
		phe->ShieldBK = CalcShieldBK();
		phe->ShieldBQ = CalcShieldBQ();
		phe->ShieldBC = CalcShieldBC();
	}
	op += phe->op;
	eg += phe->eg;
	//-------------------------------------------
	
	// безопасность королей
	if (Material[NI->MS].flags & efWhiteKingSafety)
	{
		// атака
		op -= KingAttackVal[batk_cnt] * batk_val / 32;
		// не централизуем короля при наличии угрозы
		eg -= PST[ptWhiteKing][wk][1];

		__int32 s;
		// пешечный щит + возможности рокировать
		if (File(wk) > 4)
		{
			s = phe->ShieldWK;
		}
		else if (File(wk) < 3)
		{
			s = phe->ShieldWQ;
		}
		else
		{
			__int32 k = phe->ShieldWC;
			__int32 pk = k;
			if (NI->CastleRights & crWhiteKingside)
			{
				__int32 pkk = phe->ShieldWK - (PST[ptWhiteKing][G1] - PST[ptWhiteKing][E1] /*+ PST[ptWhiteRook][F1] - PST[ptWhiteRook][H1]*/);
				if (pkk < pk) pk = pkk;
			}
			if (NI->CastleRights & crWhiteQueenside)
			{
				__int32 pkk = phe->ShieldWQ - (PST[ptWhiteKing][B1] - PST[ptWhiteKing][E1] /*+ PST[ptWhiteRook][D1] - PST[ptWhiteRook][A1]*/);
				if (pkk < pk) pk = pkk;
			}
			s = (k + pk) / 2;
		}
		op -= s;
	}

	if (Material[NI->MS].flags & efBlackKingSafety)
	{
		op += KingAttackVal[watk_cnt] * watk_val / 32;

		eg -= PST[ptBlackKing][bk][1];

		__int32 s;
		if (File(bk) > 4)
		{
			s = phe->ShieldBK;
		}
		else if (File(bk) < 3)
		{
			s = phe->ShieldBQ;
		}
		else 
		{
			__int32 k = phe->ShieldBC;
			__int32 pk = k;
			if (NI->CastleRights & crBlackKingside)
			{
				__int32 pkk = phe->ShieldBK - (PST[ptWhiteKing][G1] - PST[ptWhiteKing][E1] /*+ PST[ptWhiteRook][F1] - PST[ptWhiteRook][H1]*/);
				if (pkk < pk) pk = pkk;
			}
			if (NI->CastleRights & crBlackQueenside)
			{
				__int32 pkk = phe->ShieldBQ - (PST[ptWhiteKing][B1] - PST[ptWhiteKing][E1] /*+ PST[ptWhiteRook][D1] - PST[ptWhiteRook][A1]*/);
				if (pkk < pk) pk = pkk;
			}
			s = (k + pk) / 2;
		}
		op += s;
	}

	// дополнительная оценка проходных
	t = phe->passersw;
	while (t)
	{
		sq = ELSB(t);
		// нет своих на пути
		if (!(ForwardMask[sq] & WhitePieces)) eg += ebPassedNoOwn[7 - Rank(sq)];
		// нет чужих на пути
		if (!(ForwardMask[sq] & BlackPieces)) eg += ebPassedNoEnemy[7 - Rank(sq)];
		// на пути нет полей атакованных только соперником
		if (!(~wa & ForwardMask[sq] & ba)) eg += ebPassedFreePath[7 - Rank(sq)];
		if (!(BlackPieces ^ BlackPawns ^ BlackKing))
		{
			// неудержимая проходная (вне квадрата или поддержанная своим королем)
			if (!(ForwardMask[sq] & WhitePieces))
			{
				BitBoard q;
				if (WTM) q = BKingQuad[sq];
				else q = BKingQuadExt[sq];
				if (!(q & BlackKing)) eg += ebUnstopablePasser;
				else if (SupportWKing[sq] & WhiteKing) eg += ebUnstopablePasser;
			}
		}
		// дистанция до королей
		eg += Dist[sq - 8][bk] * ebPassedEnemyKingDist[7 - Rank(sq)];
		eg -= Dist[sq - 8][wk] * ebPassedOwnKingDist[7 - Rank(sq)];
	}

	t = phe->passersb;
	while (t)
	{
		sq = ELSB(t);
		if (!(BackwardMask[sq] & BlackPieces)) eg -= ebPassedNoOwn[Rank(sq)];
		if (!(BackwardMask[sq] & WhitePieces)) eg -= ebPassedNoEnemy[Rank(sq)];
		if (!(~ba & BackwardMask[sq] & wa)) eg -= ebPassedFreePath[Rank(sq)];
		if (!(WhitePieces ^ WhitePawns ^ WhiteKing))
		{
			if (!(BackwardMask[sq] & BlackPieces))
			{
				BitBoard q;
				if (WTM) q = WKingQuadExt[sq];
				else q = WKingQuad[sq];
				if (!(q & WhiteKing)) eg -= ebUnstopablePasser;
				else if (SupportBKing[sq] & BlackKing) eg -= ebUnstopablePasser;
			}
		}
		eg -= Dist[sq + 8][wk] * ebPassedEnemyKingDist[Rank(sq)];
		eg += Dist[sq + 8][bk] * ebPassedOwnKingDist[Rank(sq)];
	}

	// ловушки

	// пойманный слон
	if ((WhiteBishops << 9) & BlackPawns & BishopTrapBQ)
	{
		op -= epBishopTraped;
		eg -= epBishopTraped;
	}
	if ((WhiteBishops << 7) & BlackPawns & BishopTrapBK)
	{
		op -= epBishopTraped;
		eg -= epBishopTraped;
	}
	if ((BlackBishops >> 7) & WhitePawns & BishopTrapWQ)
	{
		op += epBishopTraped;
		eg += epBishopTraped;
	}
	if ((BlackBishops >> 9) & WhitePawns & BishopTrapWK)
	{
		op += epBishopTraped;
		eg += epBishopTraped;
	}

	// запертный слон
	if (Piece[C1] == ptWhiteBishop && Piece[D2] == ptWhitePawn && Piece[D3] != ptNone)
	{
		op -= epBishopBoxed;
	}
	if (Piece[F1] == ptWhiteBishop && Piece[E2] == ptWhitePawn && Piece[E3] != ptNone)
	{
		op -= epBishopBoxed;
	}
	if (Piece[C8] == ptBlackBishop && Piece[D7] == ptBlackPawn && Piece[D6] != ptNone)
	{
		op += epBishopBoxed;
	}
	if (Piece[F8] == ptBlackBishop && Piece[E7] == ptBlackPawn && Piece[E6] != ptNone)
	{
		op += epBishopBoxed;
	}

	// запертая ладья
	if ((WhiteRooks & RookBoxWK) && (WhiteKing & RookBoxKWK))
	{
		op -= epRookBoxed;
	}
	if ((WhiteRooks & RookBoxWQ) && (WhiteKing & RookBoxKWQ))
	{
		op -= epRookBoxed;
	}
	if ((BlackRooks & RookBoxBK) && (BlackKing & RookBoxKBK))
	{
		op += epRookBoxed;
	}
	if ((BlackRooks & RookBoxBQ) && (BlackKing & RookBoxKBQ))
	{
		op += epRookBoxed;
	}
	
	// разноцвет
	if (Material[NI->MS].flags & efBishopEnding)
	{
		BitBoard m = WhiteBishops | BlackBishops;
		if ((m & LightMask) && (m & ~LightMask))
		{
			op /= 2;
			eg /= 2;
		}
	}

	// очередность хода
	if (WTM)
	{
		op += ebSideToMoveOp;
		eg += ebSideToMoveEg;
	}
	else
	{
		op -= ebSideToMoveOp;
		eg -= ebSideToMoveEg;
	}

	// смешиваем дебютную и эндшпильную оценки
	__int32 total = ((Material[NI->MS].phase * eg + (24 - Material[NI->MS].phase) * op) / 24 ) / 32;
	return WTM? total: -total;
}
