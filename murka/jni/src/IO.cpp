#include <iostream>
#include <fstream>
using namespace std;
#include <windows.h>
#include <conio.h>

#include "IO.h"
#include "Search.h"
#include "TimeManager.h"
#include "Protocols.h"

// вывод поля
void PrintSquare(uint8 sq)
{
	cout << char (File(sq) + 'a') << char (8 - Rank(sq) + '0');
	Flog << char (File(sq) + 'a') << char (8 - Rank(sq) + '0');
}

// вывод хода
void PrintMove(Move m)
{
	PrintSquare(m.from);
	PrintSquare(m.to);
	if (m.flags & mfPromo)
	{
		cout << PieceLetter[m.flags];
		Flog << PieceLetter[m.flags];
	}
}

// вывод битборда
void PrintBitBoard(BitBoard b)
{
	cout << endl;
	Flog << endl;
	for (uint8 sq = 0; sq < 64 ; sq++)
	{
		cout << (b & 1)? "1" : "0";
		Flog << (b & 1)? "1" : "0";
		b >>= 1;
		if (File(sq) == 7)
		{
			cout << endl;
			Flog << endl;
		}
	}
	cout << "--------\n";
	Flog << "--------\n";
}

// вывод списка ходов
void PrintMoveList(const MoveList &ml)
{
	cout << "Move list:\n";
	Flog << "Move list:\n";
	for (const EMove *mp = ml.List; mp != ml.pm; mp++)
	{
		PrintMove((*mp).move);
		cout << endl;
		Flog << endl;
	}
	cout << "----\n";
	Flog << "----\n";
	Flog.flush();
}

// вывод основного варианта
void PrintBestLine(uint8 depth, __int16 score, Move m)
{
	if (!PostMode) return;

	char str[256];
	char sm[8];
	MoveToStr(m, sm);

	if (UciMode)
	{
		uint32 t = TimeElapsed();
		uint32 nps = t? Nodes * 1000 / t : 0;
		// матовая оценка
		if (abs(score) > 32000) sprintf(str, "info depth %u score mate %d time %u nodes %I64u nps %u pv %s\n", depth, (MATE + (score > 0? -score + 1 : score)) / 2, t, Nodes, nps, sm);
		// обычная оценка в сантипешках
		else sprintf(str, "info depth %u score cp %d time %u nodes %I64u nps %u pv %s\n", depth, score, t, Nodes, nps, sm);
	}
	if (XBoardMode)
	{
		sprintf(str, "%u %d %u %I64u %s\n", depth, score, TimeElapsed() / 10, Nodes, sm);
	}
	cout << str;
	cout.flush();
	Flog << str;
	Flog.flush();
}

// появилась ли на входе новая команда
bool InputAvailable()
{
	if (stdin->_cnt > 0) return 1;

	static HANDLE hInput = GetStdHandle(STD_INPUT_HANDLE);
	DWORD mode;
	static BOOL console = GetConsoleMode(hInput, &mode);

	if (!console)
	{
		DWORD total_bytes_avail;
		if (!PeekNamedPipe(hInput, 0, 0, 0, &total_bytes_avail, 0))	return true;
		return total_bytes_avail;
	}
	else return _kbhit();
}
