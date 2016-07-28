#include <windows.h>
#include <iostream>
using namespace std;

#include "BitBoards.h"
#include "Test.h"
#include "Search.h"
#include "Eval.h"
#include "Protocols.h"
#include "Hash.h"
#include "AutoTuning.h"
#include "SEE.h"

void __cdecl main(int argc, char **argv)
{
	cout << MY_NAME;
	cout << "\nCopyright (C) 2010-2013 Igor Korshunov" << endl;

	uint8 i = 1;
	uint16 trans_size = 128;
	while (i < argc)
	{
		if (!strcmp(argv[i], "-l")) Log = true;
		else if (!strcmp(argv[i], "-h")) trans_size = atoi(argv[++i]);
		i++;
	}

	InitBitBoards();
	InitSEE();
	SetTransSize(trans_size);
	InitHashKeys();
	InitEval();

	SetBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");

#ifdef AUTOTUNING
	SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
	AutoTuning();
	return;
#endif

	//SetBoard("6r1/8/8/8/4p3/2P1k2K/R4p2/8 w - - 0 74");
	//cout << Eval() << endl;
	//TestEvalSymmetry();
	
	char str[32];
	cin >> str;
	if (!stricmp(str, "xboard")) XBoard();
	else if (!stricmp(str, "uci")) UCI();
	else if (!stricmp(str, "perft")) TestPerft();
	else if (!stricmp(str, "test")) TestSet();
	else if (!stricmp(str, "base"))
	{
		int n;
		cin >> str >> n;
		Base(str, n);
	}
}
