#include <iostream>
using namespace std;
#include <assert.h>

#include "Search.h"
#include "Eval.h"
#include "MoveList.h"
#include "IO.h"
#include "TimeManager.h"
#include "Protocols.h"
#include "Hash.h"
#include "SEE.h"

#define ply (NI - RootNI)
extern int PieceValueEg[];

Move BestMove;

// ����� ������� ������� ����������� ��������� �������
// ��� ������������� ���������� ������� �� ��������
const uint64 NodesCheckIntervalMin = 10000;
const uint64 NodesCheckIntervalMax = 1000000;
// ������ ��������� �������� �������
uint64 CheckNodes;
// ���������� ������������� �������
uint64 Nodes;
// ����������� ���������� ������������ �������
uint64 NodesLimit = 0;

// ���������� �� ����� ������
NodeInfo NodesInfo[1024];
// ��������� �� ������� ����
NodeInfo *NI = NodesInfo;
// ��������� �� �������� ����
NodeInfo *RootNI;

// ����������� ������� ��������
uint8 DepthLimit = 99;

// ��������� ���������� ��������
bool SearchAborted;

// �������
uint32 History[13][64];

Move NullMove = {0};

// ����������� �� ������� ������
inline bool Repetition()
{
	// 3 ��������� �������� �� ������ �� ���������,
	// �.�. ��� ����� ���� �� �����
	if (NI - NodesInfo < 4) return false;
	if (!NI->Reversible) return false;
	if (!(NI - 1)->Reversible) return false;
	if (!(NI - 2)->Reversible) return false;

	// ������� ��� ������� �� ������������ ���� (������, ��� ������, ���������)
	for (NodeInfo *ni = NI - 4; ni >= NodesInfo; ni -= 2)
	{
		if (!(ni + 1)->Reversible) return false;
		if (ni->HashKey == NI->HashKey) return true;

		// � �������� � ������ ������������ ���� ��������� ������ �����������
		if (!ni->Reversible) return false;
	}
	return false;
}

// ��������� �� ����� �� �������� �������
inline bool SearchNeedAbort()
{
	
	if (NodesLimit)
	{
		if (Nodes < NodesLimit) return false;
		return true;
	}

	// ����� �������� ��� �� ���������
	if (Nodes < CheckNodes) return false;

	// �� ����� ����� �������
	while (InputAvailable())
	{
		char str[128];
		cin.getline(str, 128);
		if (!*str) return false;

		Flog << ">>" << str << endl;
		Flog.flush();

		if (UciMode)
		{
			if (!strcmp(str, "isready"))
			{
				cout << "readyok\n";
				cout.flush();
				Flog << "readyok\n";
				Flog.flush();
			}
			else if (!strcmp(str, "stop"))
			{
				return true;
			}
			else if (!strcmp(str, "ponderhit"))
			{
			}
		}
		else // XBoardMode
		{
			// ����� �������
			if (AnalyzeMode)
			{
				if (!stricmp(str, "exit"))
				{
					AnalyzeMode = false;
					return true;
				}
				else if (!strcmp(str, "."))
				{
				}
				else if (!stricmp(str, "bk"))
				{
				}
				else if (!stricmp(str, "hint"))
				{
				}
				// ������� ������� ����� ��������� � �������� ������� ��������� WB
				else
				{
					strcpy(AnalyzeCommand, str);
					return true;
				}
			}

			// ������� �����
			if (!stricmp(str, "quit")) exit(0);
			else if (!strcmp(str, "?")) return true;
			else if (!strcmp(str, "draw"))
			{
			}
			// ������� ������� ����� ��������� � �������� ������� ��������� WB
			else
			{
				strcpy(AnalyzeCommand, str);
				return true;
			}
		}
	}
	
	// ����� �������
	if (TimeLimitHard)
	{
		uint32 te = TimeElapsed();
		if (te >= TimeLimitHard)
		{
			//cout << "TimeLimitHard (2.17.8) " << te << " " << Nodes << endl;
			return true;
		}

		// ������������� �������� ��� ��������� �������� �������
		if (te)
		{
			uint64 knps = Nodes / te;
			uint64 interval = knps * (TimeLimitHard - te) / 5;
			if (interval > NodesCheckIntervalMax) interval = NodesCheckIntervalMax;
			else if (interval < NodesCheckIntervalMin) interval = NodesCheckIntervalMin;
			CheckNodes = Nodes + interval;
		}
		else if (TimeLimitHard - te < 5000) CheckNodes = Nodes + NodesCheckIntervalMin;
		else CheckNodes = Nodes + NodesCheckIntervalMax;
	}
	else CheckNodes = Nodes + NodesCheckIntervalMax;

	return false;
}

__int16 QuiescenceSearch(__int16 alpha, __int16 beta, uint8 lvl);

// �� ��� �����
__int16 QuiescenceSearchCheck(__int16 alpha, __int16 beta)
{
	assert(alpha < beta);
	assert(NI->InCheck);
	if (Repetition()) return 0;

	MoveList ml;
	__int16 bs = ply - MATE; // ������ �� ������ ������ ������ - best score
	if (bs >= beta) return bs;
	ml.GenEvasions();
	ml.SetQEvasionValues();

	// ���������� ����
	while (Move *m = ml.NextSortedMove())
	{
		if (!MakeMove(*m)) continue;
		__int16 ms;
		if (NI->InCheck) ms = -QuiescenceSearchCheck(-beta, -alpha);
		else ms = -QuiescenceSearch(-beta, -alpha, 0);
		UnmakeMove(*m);
		if (SearchAborted) return 0;
		if (ms > bs)
		{
			if (ms > alpha)
			{
				if (ms >= beta) return ms;
				alpha = ms;
			}
			bs = ms;
		}
	}

	if (SearchNeedAbort()) SearchAborted = true;
	return bs;
}

const __int16 Delta = 110;

// ��
__int16 QuiescenceSearch(__int16 alpha, __int16 beta, uint8 lvl)
{
	assert(alpha < beta);
	assert(!NI->InCheck);
	if (Repetition()) return 0;

	BitBoard t = WTM? BlackPieces : WhitePieces;
	MoveList ml;
	__int16 bs = Eval();
	if (bs > alpha)
	{
		if (bs >= beta) return bs;
		alpha = bs;
		ml.GenCaps();
	}
	// ��������� ����������� ������
	else if (bs < alpha - PieceValueEg[ptWhitePawn]/32 - Delta)
	{
		if (WTM)
		{
			t &= ~BlackPawns;
			if (bs < alpha - PieceValueEg[ptWhiteKnight]/32 - Delta)
			{
				t ^= BlackKnights;
				__int32 margin = PieceValueEg[ptWhiteBishop]/32 + Delta;
				if (PopCnt(BlackBishops) > 1) margin += 47;
				if (bs < alpha - margin)
				{
					t ^= BlackBishops;
					if (bs < alpha - PieceValueEg[ptWhiteRook]/32 - Delta)
					{
						t ^= BlackRooks;
						if (bs < alpha - PieceValueEg[ptWhiteQueen]/32 - Delta)
						{
							t ^= BlackQueens;
							bs += PieceValueEg[ptWhiteQueen]/32 + Delta;
						} else bs += PieceValueEg[ptWhiteRook]/32 + Delta;
					} else bs += margin;
				} else bs += PieceValueEg[ptWhiteKnight]/32 + Delta;
			} else bs += PieceValueEg[ptWhitePawn]/32 + Delta;
			ml.GenWhiteCaps(t);
		}
		else
		{
			t &= ~WhitePawns;
			if (bs < alpha - PieceValueEg[ptWhiteKnight]/32 - Delta)
			{
				t ^= WhiteKnights;
				__int32 margin = PieceValueEg[ptWhiteBishop]/32 + Delta;
				if (PopCnt(WhiteBishops) > 1) margin += 47;
				if (bs < alpha - margin)
				{
					t ^= WhiteBishops;
					if (bs < alpha - PieceValueEg[ptWhiteRook]/32 - Delta)
					{
						t ^= WhiteRooks;
						if (bs < alpha - PieceValueEg[ptWhiteQueen]/32 - Delta)
						{
							bs += PieceValueEg[ptWhiteQueen]/32 + Delta;
							t ^= WhiteQueens;
						} else bs += PieceValueEg[ptWhiteRook]/32 + Delta;
					} else bs += margin;
				} else bs += PieceValueEg[ptWhiteKnight]/32 + Delta;
			} else bs += PieceValueEg[ptWhitePawn]/32 + Delta;
			ml.GenBlackCaps(t);
		}
	}
	else ml.GenCaps();
	ml.SetQMoveValues();

	// ���������� ����
	while (Move *m = ml.NextSortedMove())
	{
		if (!SEE(*m)) continue;
		if (!MakeMove(*m)) continue;
		__int16 ms;
		if (NI->InCheck) ms = -QuiescenceSearchCheck(-beta, -alpha);
		else ms = -QuiescenceSearch(-beta, -alpha, 0);
		UnmakeMove(*m);
		if (SearchAborted) return 0;
		if (ms > bs)
		{
			if (ms > alpha)
			{
				if (ms >= beta) return ms;
				alpha = ms;
			}
			bs = ms;
		}
	}

	if (lvl)
	{
		ml.GenChecks(~t);
		while (Move *m = ml.NextMove())
		{
			if (!SEE(*m)) continue;
			if (!MakeMove(*m)) continue;
			__int16 ms = -QuiescenceSearchCheck(-beta, -alpha);
			UnmakeMove(*m);
			if (SearchAborted) return 0;
			if (ms > bs)
			{
				if (ms > alpha)
				{
					if (ms >= beta) return ms;
					alpha = ms;
				}
				bs = ms;
			}
		}
	}

	if (SearchNeedAbort()) SearchAborted = true;
	return bs;
}

// �������� ���������� �� ������� �����
inline void UpdateGoodMoveStats(Move m, uint8 depth)
{
	// ��������� ������� ��������
	if (!MoveCmp(m, NI->Killer1))
	{
		NI->Killer2 = NI->Killer1;
		NI->Killer1 = m;
	}
	
	// ��������� ������� �������
	History[Piece[m.from]][m.to] += depth * depth;
}

// ���������� �� ��������� ��� ������� ����
inline bool MaterialForNull()
{
	if (WTM) return WhiteRooks || WhiteKnights || WhiteBishops || WhiteQueens;
	return BlackRooks || BlackBishops || BlackKnights || BlackQueens;
}

__int16 SearchCheck(__int16 beta, uint8 depth);

// ������� � ������� �����
__int16 SearchCut(__int16 beta, uint8 depth)
{
	assert (depth > 0);

	if (Repetition()) return 0;

	// ������� ������������
	Move tm = NullMove;
	TransEntry *he = Trans + (NI->HashKey & HashMask);
	if (he->Key == NI->HashKey)
	{
		tm = he->Move;
		if (he->Depth >= depth)
		{
			__int16 ts = he->Score;
			if (ts > 32000) ts -= ply;
			else if (ts < -32000) ts += ply;
			if (he->Flags & tfExact) return ts;
			if (he->Flags & tfUpperBound)
			{
				if (ts < beta) return ts;
			}
			else
			{
				if (ts >= beta) return ts;
			}
		}
	}
	else tm = NullMove;

	__int16 ev = Eval();
	
	// ���������
	if (depth <= 3)
	{
		// ���� ������� ������ �������
		// � �� ����� ������� ������ ��� ������ ��
		// �� ������ �� �������
		const __int16 raz_margin[4] = {0, 64, 128, 192};
		if (beta < 31000 && ev + raz_margin[depth] < beta && MoveCmp(tm, NullMove))
		{
			BitBoard mask = WTM? WhitePawns & Rank7 : BlackPawns & Rank2;
			if (!mask)
			{
				__int16 rbeta = beta - raz_margin[depth];
				__int16 rs = QuiescenceSearch(rbeta - 1, rbeta, 1);
				if (SearchAborted) return 0;
				if (rs < rbeta) return rs;
			}
		}

		// ������� ������� �������
		// ���� � ��� ���� ���� ���� ������
		// �� ������ �� �������
		const __int16 fut_margin[4] = {0, 80, 160, 320};
		if (beta > -31000 && ev - fut_margin[depth] >= beta)
		{
			BitBoard mask = WTM? WhitePieces & ~WhitePawns & ~WhiteKing : BlackPieces & ~BlackPawns & ~BlackKing;
			if (mask) return ev - fut_margin[depth];
		}
	}

	// ������ ���
	if (depth >= 2 && beta > -31000 && ev >= beta && MaterialForNull())
	{
		MakeNullMove();
		__int16 score;
		uint8 r = 3 + depth / 4;
		if (ev - 100 > beta) r++;

		if (depth <= r) score = -QuiescenceSearch(-beta, 1 - beta, 1);
		else score = -SearchCut(1 - beta, depth - r);

		UnmakeNullMove();
		if (SearchAborted) return 0;

		if (score >= beta)
		{
			TransStore(score, depth, ply, tfLowerBound, NullMove);
			return score;
		}
	}

	// ������� ��������� �����
	MoveList ml;
	ml.IncInit(tm);
	
	__int16 bs = ply - MATE; // ������ �� ������ ������ ������ - best score
	if (bs >= beta) return bs;

	// ���������� ����
	uint8 cnt = 0;
	while (Move *m = ml.NextIncMove())
	{
		if (!MakeMove(*m)) continue;

		__int16 ms;
		uint8 nd = depth - 1;
		
		// ��������� �����
		if (NI->InCheck) nd++;
		// ���������� ��������� ����� (LMR)
		else if (nd > 0 && ml.Stage >= nmsQuiets && cnt > 2)
		{
			if (ml.Stage == nmsQuiets && cnt > 3 + depth * depth)
			{
				UnmakeMove(*m);
				continue;
			}
			nd--;
			if (ml.Stage == nmsQuiets)
			{
				if (nd > 0 && cnt > 6)
				{
					nd--;
					if (nd > 0 && cnt > 14)
					{
						nd--;
						if (nd > 0 && cnt > 30) nd--;
					}
				}
			}
		}

		// ����������� ����� ��������
		if (nd == 0) ms = -QuiescenceSearch(-beta, 1 - beta, 1);
		else if (NI->InCheck) ms = -SearchCheck(1 - beta, nd);
		else ms = -SearchCut(1 - beta, nd);
		if (SearchAborted)
		{
			UnmakeMove(*m);
			return 0;
		}

		// ����������� LMR
		if (nd < depth - 1 && ms >= beta)
		{
			ms = -SearchCut(1 - beta, depth - 1);
			if (SearchAborted)
			{
				UnmakeMove(*m);
				return 0;
			}
		}

		UnmakeMove(*m);
		cnt++;
		
		// ������ ��������
		if (ms > bs)
		{
			// ��������� �� ����
			if (ms >= beta)
			{
				TransStore(ms, depth, ply, tfLowerBound, *m);
				if (!m->cap && !(m->flags & mfPromo)) UpdateGoodMoveStats(*m, depth);
				return ms;
			}
			bs = ms;
		}
	}

	// ���� ��� ��������� �����, �� ���
	if (!cnt) return 0;

	TransStore(bs, depth, ply, tfUpperBound, NullMove);
	if (SearchNeedAbort()) SearchAborted = true;

	return bs;
}

// ������� ����� � ���� ��� �����
__int16 SearchCheck(__int16 beta, uint8 depth)
{
	assert(depth > 0);
	if (Repetition()) return 0;

	// ������� ������������
	Move tm = NullMove;
	TransEntry *he = Trans + (NI->HashKey & HashMask);
	if (he->Key == NI->HashKey)
	{
		tm = he->Move;
		if (he->Depth >= depth)
		{
			__int16 ts = he->Score;
			if (ts > 32000) ts -= ply;
			else if (ts < -32000) ts += ply;
			if (he->Flags & tfExact) return ts;
			if (he->Flags & tfUpperBound)
			{
				if (ts < beta) return ts;
			}
			else
			{
				if (ts >= beta) return ts;
			}
		}
	}
	else tm = NullMove;

	__int16 bs = ply - MATE; // ������ �� ������ ������ ������ - best score
	if (bs >= beta) return bs;

	// ���������� ����
	MoveList ml;
	ml.GenEvasions();
	ml.SetEvasionValues(tm);

	// ���������� ����
	while (Move *m = ml.NextSortedMove())
	{
		if (!MakeMove(*m)) continue;

		__int16 ms;
		uint8 nd = depth - 1;
		if (NI->InCheck) nd++;
		if (!nd) ms = -QuiescenceSearch(-beta, 1 - beta, 1);
		else if (NI->InCheck) ms = -SearchCheck(1 - beta, nd);
		else ms = -SearchCut(1 - beta, nd);
		UnmakeMove(*m);
		
		if (SearchAborted) return 0;
		
		// ������ ��������
		if (ms > bs)
		{
			// ��������� �� ����
			if (ms >= beta)
			{
				TransStore(ms, depth, ply, tfLowerBound, *m);
				return ms;
			}
			bs = ms;
		}
	}

	TransStore(bs, depth, ply, tfUpperBound, NullMove);
	if (SearchNeedAbort()) SearchAborted = true;
	return bs;
}

// �������� �������
__int16 SearchPV(__int16 alpha, __int16 beta, uint8 depth)
{
	assert(alpha < beta);

	if (!depth) return QuiescenceSearch(alpha, beta, 1);
	if (ply > 100) return Eval();
	if (Repetition()) return 0;

	__int16 bs = ply - MATE; // ������ �� ������ ������ ������ - best score
	if (bs >= beta)
	{
		BestMove = NullMove;
		return bs;
	}

	// ������� ������������
	Move tm = NullMove;
	TransEntry *he = Trans + (NI->HashKey & HashMask);
	if (he->Key == NI->HashKey)	tm = he->Move;
	else tm = NullMove;

	// IID
	if (depth >= 3 && MoveCmp(tm, NullMove))
	{
		SearchPV(alpha, beta, depth - 2);
		if (SearchAborted) return 0;
		tm = BestMove;
	}

	// ���������� ����
	MoveList ml;
	if (NI->InCheck)
	{
		ml.GenEvasions();
		ml.SetEvasionValues(tm);
		// ����
		ml.Stage = nmsBadCaps;
	}
	else ml.IncInit(tm);
	
	NI->eval = Eval();

	Move bm = NullMove; // ������ �� ������ ������ ��� - best move
	bool first = true; // ������ ��� ��� �� ����������
	// ���������� ����
	while (Move *m = ml.NextIncMove())
	{
		uint8 ext = 0;
		if (m->to == (NI-1)->move.to && NI->eval + 50 < -(NI - 1)->eval && SEE(*m)) ext = 1;

		if (!MakeMove(*m)) continue;

		uint8 nd = depth - 1;
		if (NI->InCheck || ext) nd++;

		__int16 ms;
		if (first) ms = -SearchPV(-beta, -alpha, nd);
		else
		{
			if (!nd) ms = -QuiescenceSearch(-alpha - 1, -alpha, 1);
			else if (NI->InCheck) ms = -SearchCheck(-alpha, nd);
			else ms = -SearchCut(-alpha, nd);
			if (SearchAborted)
			{
				UnmakeMove(*m);
				return 0;
			}
			if (ms > alpha) ms = -SearchPV(-beta, -alpha, nd);
		}
		first = false;

		UnmakeMove(*m);
		if (SearchAborted) return 0;
		
		// ������ ��������
		if (ms > bs)
		{
			if (ms > alpha)
			{
				// ��������� �� ����
				if (ms >= beta)
				{
					BestMove = *m;
					TransStore(ms, depth, ply, tfLowerBound, *m);
					if (!m->cap && !(m->flags & mfPromo)) UpdateGoodMoveStats(*m, depth);
					return ms;
				}
				alpha = ms;
				bm = *m;
			}
			bs = ms;
		}
	}

	BestMove = bm;
	// ��� ��� ���
	if (first) return NI->InCheck ? bs : 0;
	else
	{
		if (!MoveCmp(bm, NullMove))
		{
			assert(bs < beta);
			TransStore(bs, depth, ply, tfExact, bm);
			if (bm.cap == ptNone && !(bm.flags & mfPromo)) UpdateGoodMoveStats(bm, depth);
		}
		else
		{
			assert(bs <= alpha);
			TransStore(bs, depth, ply, tfUpperBound, NullMove);
		}
	}

	if (SearchNeedAbort()) SearchAborted = true;
	return bs;
}

int LastScore;

// ������� � ����� ������
Move RootSearch()
{
	if (DepthLimit < 99) TimeLimitHard = TimeLimitSoft = 0;
	else StartTimer();
	if (TimeLimitHard && TimeLimitHard < 5000) CheckNodes = NodesCheckIntervalMin;
	else CheckNodes = NodesCheckIntervalMax;
	SearchAborted = false;
	RootNI = NI;

	// ���������� ��� ����
	MoveList ml;
	if (NI->InCheck) ml.GenEvasions();
	else
	{
		ml.GenCaps();
		if (WTM) ml.GenWhiteQuiets();
		else ml.GenBlackQuiets();
	}
	
	// ������� �����������
	while (Move *m = ml.NextMove())
	{
		if (!MakeMove(*m)) ml.Remove();
		else UnmakeMove(*m);
	}
	if (DepthLimit >= 99 && ml.pm - ml.List == 1) return ml.List[0].move;
	// ��������� ����� ����� ������� �� ������ ������
	ml.cm = ml.List;

	Move tm = NullMove;
	TransEntry *he = Trans + (NI->HashKey & HashMask);
	if (he->Key == NI->HashKey)	tm = he->Move;
	
	NI[0].Killer1 = NI[2].Killer1;
	NI[0].Killer2 = NI[2].Killer2;
	ml.SetMoveValues(tm);
	ml.Sort();
	memset(History, 0, sizeof(History));
	for (int i = 0; i < 64; i++) NI[i].Killer1 = NI[i].Killer2 = NullMove;


	Nodes = 0;

	uint8 drop, easy, change_last;
	uint8 change = 0;

	__int16 ps = NI->eval = Eval(); // previous score

	for (uint8 depth = 1; depth <= DepthLimit; depth++)
	{
		if (UciMode) cout << "info depth " << int(depth) << endl;

		drop = change_last = 0;
		__int16 alpha = -MATE;
		__int16 bs = -MATE;
		
		while (Move *m = ml.NextMove())
		{
			MakeMove(*m);

			uint8 nd = depth - 1;
			if (NI->InCheck) nd++;

			if (UciMode && TimeElapsed() > 1000)
			{
				char str[8];
				MoveToStr(*m, str);
				cout << "info currmove " << str << " currmovenumber " << ml.cm - ml.List << endl;
			}

			__int16 ms;
			if (bs != -MATE && nd > 0)
			{
				if (NI->InCheck) ms = -SearchCheck(-alpha, nd);
				else ms = -SearchCut(-alpha, nd);
				if (SearchAborted)
				{
					UnmakeMove(*m);
					return ml.List[0].move;
				}
				if (ms > alpha)
				{
					easy = 0;
					change = 1;
					change_last = 1;
					ms = -SearchPV(-MATE, -alpha, nd);
				}
			}
			else ms = -SearchPV(-MATE, +MATE, nd);

			if (depth == 1) (ml.cm - 1)->value = ms + 100000;

			UnmakeMove(*m);
			if (SearchAborted) return ml.List[0].move;

			// ������ ��������
			if (ms > bs)
			{
				if (depth > 1 && ms < ps - 50) drop = 1;
				bs = ms;
				if (bs > alpha)
				{
					alpha = bs;
					ml.MoveToTop();
					if (alpha > 32000) break;
				}
				PrintBestLine(depth, bs, ml.List[0].move);
			}
		}

		PrintBestLine(depth, bs, ml.List[0].move);
		LastScore = bs;

		ps = bs;

		// ��������� ����� ���������� ����
		if (depth == 1)
		{
			EMove *m;
			for (m = ml.List + 1; m != ml.pm; m++)
			{
				if (ml.List[0].value - m->value < 200) break;
			}
			if (m == ml.pm) easy = 1;
			else easy = 0;
		}

		// ����� � ������ ������� ������
		if (bs < -32000 || bs > 32000) break;
		
		// ��������� ����� �������
		if (depth >= 5 && TimeLimitSoft)
		{
			uint32 t = TimeElapsed();
			if (t >= TimeLimitSoft / 12 && easy) break;
			if (!drop)
			{
				if (t >= TimeLimitSoft) break;
				if (t >= TimeLimitSoft / 2 && !change_last) break;
				if (t >= TimeLimitSoft / 4 && !change) break;
			}
		}

		// ��������� ����� ����� ������� �� ������ ������
		ml.cm = ml.List;
	}

	return ml.List[0].move;
}
