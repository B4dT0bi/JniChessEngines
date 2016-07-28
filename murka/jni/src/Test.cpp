#include <iostream>
using namespace std;

#include "Types.h"
#include "MoveList.h"
#include "TimeManager.h"
#include "Search.h"
#include "Eval.h"
#include "Board.h"
#include "Hash.h"
#include "Protocols.h"

uint64 PerftNodes;

// проверка симметричности ОФ
bool TestEvalSymmetry()
{
	// проверяем, чтобы была корректная индексация по таблице материала
	if (PopCnt(WhiteQueens) < 2 && PopCnt(BlackQueens) < 2 &&
		PopCnt(WhiteRooks) < 3 && PopCnt(BlackRooks) < 3 &&
		PopCnt(WhiteBishops) < 3 && PopCnt(BlackBishops) < 3 &&
		PopCnt(WhiteKnights) < 3 && PopCnt(BlackKnights) < 3)
	{
		__int16 ev1 = Eval();
		InvertBoard();
		CalcHashKey();
		__int16 ev2 = Eval();
		InvertBoard();
		CalcHashKey();
		if (ev1 != ev2) return false;
	}
	return true;
}

//стандартная процедура проверки генератора и исполнителя ходов
void Perft(uint8 depth)
{
	depth--;

#ifdef TEST
	TransEntry *he = Trans + (NI->HashKey & HashMask);
	if (he->Key == NI->HashKey && he->Depth == depth)
	{
		PerftNodes += he->Nodes;
		return;
	}
	uint64 nodes = PerftNodes;
#endif

	MoveList ml;
	ml.GenMoves();

	while (Move *m = ml.NextMove())
	{
		if (!MakeMove(*m)) continue;

#ifdef TEST
		// проверка хеш-ключа
		uint64 hk = NI->HashKey;
		uint64 phk = NI->PawnHashKey;
		CalcHashKey();
		if (hk != NI->HashKey || phk != NI->PawnHashKey)
		{
			PrintBoard();
			cout << "ERROR: wrong HashKey in Perft()\n";
			CalcHashKey();
		}
#endif

		if (depth) Perft(depth);
		else PerftNodes++;
		UnmakeMove(*m);
	}

#ifdef TEST
	he->Key = NI->HashKey;
	he->Depth = depth;
	he->Nodes = PerftNodes - nodes;
#endif
}

// засекаем время и запускаем Perft
// после выводим статистику
void DoPerft(uint8 depth)
{
	PerftNodes = 0;
	StartTimer();
	Perft(depth);
	unsigned time = TimeElapsed();
	cout << "depth = " << int(depth) << ", nodes = ";
	cout.width(10);
	cout << PerftNodes << ", time = ";
	cout.width(8);
	cout.setf(ios::fixed);
	cout.precision(2);
	cout << time / 1000.0;
	if (time) cout << '\t' << int (PerftNodes * 1000.0 / time) / 1000 << " kN / s";
	cout << endl;
}

// Perft-тест 4-х позиций
void TestPerft()
{
	cout << "119060324\n193690690\n34336777\n178633661\n";

	SetBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
	DoPerft(6);
	InvertBoard();
	CalcHashKey();
	DoPerft(6);
	cout << "--------------------------------------------------\n";
	
	SetBoard("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
	DoPerft(5);
	InvertBoard();
	CalcHashKey();
	DoPerft(5);
	cout << "--------------------------------------------------\n";

	SetBoard("8/PPP4k/8/8/8/8/4Kppp/8 w - -");
	DoPerft(6);
	InvertBoard();
	CalcHashKey();
	DoPerft(6);
	cout << "--------------------------------------------------\n";

	SetBoard("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
	DoPerft(7);
	InvertBoard();
	CalcHashKey();
	DoPerft(7);
	cout << "--------------------------------------------------\n";
}

// тестирование на наборе позиций
void TestSet()
{
	XBoardMode = true;
	//UciMode = true;
	TimeLimited = 60000;
	//DepthLimit = 16;

	SetBoard("8/7p/5k2/5p2/p1p2P2/Pr1pPK2/1P1R3P/8 b - -");
	cout << "\n1 ---\n";
	RootSearch();

	SetBoard("7k/p7/1R5K/6r1/6p1/6P1/8/8 w - -");
	cout << "\n2 ---\n";
	RootSearch();

	SetBoard("r4q1k/p2bR1rp/2p2Q1N/5p2/5p2/2P5/PP3PPP/R5K1 w - -");
	cout << "\n3 ---\n";
	RootSearch();

	SetBoard("r1b2rk1/ppbn1ppp/4p3/1QP4q/3P4/N4N2/5PPP/R1B2RK1 w - -");
	cout << "\n4 ---\n";
	RootSearch();

	SetBoard("r1bqk2r/ppp1nppp/4p3/n5N1/2BPp3/P1P5/2P2PPP/R1BQK2R w KQkq -");
	cout << "\n5 ---\n";
	RootSearch();

	SetBoard("3r2k1/1p1b1pp1/pq5p/8/3NR3/2PQ3P/PP3PP1/6K1 b - -");
	cout << "\n6 ---\n";
	RootSearch();

	SetBoard("r2q2k1/pp1rbppp/4pn2/2P5/1P3B2/6P1/P3QPBP/1R3RK1 w - -");
	cout << "\n7 ---\n";
	RootSearch();

	SetBoard("1r3r2/4q1kp/b1pp2p1/5p2/pPn1N3/6P1/P3PPBP/2QRR1K1 w - -");
	cout << "\n8 ---\n";
	RootSearch();

	SetBoard("6k1/p4p1p/1p3np1/2q5/4p3/4P1N1/PP3PPP/3Q2K1 w - -");
	cout << "\n9 ---\n";
	RootSearch();

	SetBoard("2r5/2rk2pp/1pn1pb2/pN1p4/P2P4/1N2B3/nPR1KPPP/3R4 b - -");
	cout << "\n10 ---\n";
	RootSearch();
}
