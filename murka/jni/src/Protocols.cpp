#include <string.h>
#include <windows.h>
#include <iostream>
using namespace std;

#include "Protocols.h"
#include "Types.h"
#include "Search.h"
#include "IO.h"
#include "TimeManager.h"
#include "Eval.h"
#include "UCT.h"
#include "Hash.h"

// протокол
bool UciMode = false;
bool XBoardMode = false;
// выводить ли инфу о переборе под WinBoard
bool PostMode = true;
// сюда сохраняем сделанные ходы, чтобы можно было откатывать их под WinBoard
Move GameMoves[1024];
uint16 GameMovesCnt = 0;

// режим анализа под WB
bool AnalyzeMode = false;
// команда полученная во время перебора
// но не выполненная сразу
char AnalyzeCommand[128];

// лог
bool Log = false;
ofstream Flog;

// получаем строковое обазначение хода
void MoveToStr(Move m, char *str)
{
	str[0] = File(m.from) + 'a';
	str[1] = 8 - Rank(m.from) + '0';
	str[2] = File(m.to) + 'a';
	str[3] = 8 - Rank(m.to) + '0';
	if (m.flags & mfPromo)
	{
		str[4] = tolower(PieceLetter[m.flags]);
		str[5] = 0;
	}
	else str[4] = 0;
}
void MoveToSan1(Move m, char *str)
{
	if (Piece[m.from] != ptWhitePawn && Piece[m.from] != ptBlackPawn) *str++ = PieceLetter[Piece[m.from]];
	if (m.cap != ptNone)
	{
		if (Piece[m.from] == ptWhitePawn || Piece[m.from] == ptBlackPawn) *str++ = 'a' + File(m.from);
		*str++ = 'x';
	}
	*str++ = File(m.to) + 'a';
	*str++ = 8 - Rank(m.to) + '0';
	if (m.flags & mfPromo)  *str++ = PieceLetter[m.flags & mfPromo];
	*str++ = 0;
}

void MoveToSan2(Move m, char *str)
{
	if (Piece[m.from] != ptWhitePawn && Piece[m.from] != ptBlackPawn)
	{
		*str++ = PieceLetter[Piece[m.from]];
		*str++ = File(m.from) + 'a';
	}
	if (m.cap != ptNone)
	{
		if (Piece[m.from] == ptWhitePawn || Piece[m.from] == ptBlackPawn) *str++ = 'a' + File(m.from);
		*str++ = 'x';
	}
	*str++ = File(m.to) + 'a';
	*str++ = 8 - Rank(m.to) + '0';
	if (m.flags & mfPromo)  *str++ = PieceLetter[m.flags & mfPromo];
	*str++ = 0;
}

void MoveToSan3(Move m, char *str)
{
	if (Piece[m.from] != ptWhitePawn && Piece[m.from] != ptBlackPawn)
	{
		*str++ = PieceLetter[Piece[m.from]];
		*str++ = File(m.from) + 'a';
		*str++ = 8 - Rank(m.from) + '0';
	}
	if (m.cap != ptNone)
	{
		if (Piece[m.from] == ptWhitePawn || Piece[m.from] == ptBlackPawn) *str++ = 'a' + File(m.from);
		*str++ = 'x';
	}
	*str++ = File(m.to) + 'a';
	*str++ = 8 - Rank(m.to) + '0';
	if (m.flags & mfPromo)  *str++ = PieceLetter[m.flags & mfPromo];
	*str++ = 0;
}

// распознаем ход
// если не получилось обнуляем поля from и to
Move StrToMove(char *str)
{
	MoveList ml;
	ml.GenMoves();
	while (Move *mp = ml.NextMove())
	{
		char sm[8];
		MoveToStr(*mp, sm);
		if (!stricmp(str, sm)) return *mp;
		MoveToSan3(*mp, sm);
		if (!stricmp(str, sm)) return *mp;
		MoveToSan2(*mp, sm);
		if (!stricmp(str, sm) && MakeMove(*mp))
		{
			UnmakeMove(*mp);
			return *mp;
		}
		MoveToSan1(*mp, sm);
		if (!stricmp(str, sm) && MakeMove(*mp))
		{
			UnmakeMove(*mp);
			return *mp;
		}
	}

	Move m;
	m.from = m.to = 0;
	return m;
}

// открываем лог
void OpenLog(char *mode)
{
	char fname[512];
	char *p;

	if (!Log) return;

	// берем имя исполнимого файла
	GetModuleFileName(0, fname, 8192);
	
	// отрезаем путь
	p = strrchr(fname, '\\');
	if (p) strcpy(fname, p + 1);
	
	// отрезаем расширение
	p = strrchr(fname, '.');
	if (p) *p = 0;

	// добавляем имя протокола и расширение '.log'
	strcat(fname, mode);
	strcat(fname, ".log");

	// ну и открываем сам лог-файл
	Flog.open(fname);
}

void UCI()
{
	UciMode = true;

	cout << "id name " MY_NAME " UCI\n";
	cout << "id author Igor Korshunov\n";
	cout << "option name Hash type spin default 128 min 1 max 1024\n";
	cout << "uciok\n";
	cout.flush();
	OpenLog("_uci");

	char str[8192];
	char *p;
	
	for (;;)
	{
		cin.getline(str, 8192);
		if (!*str) continue;

		Flog << "> " << str << endl;
		Flog.flush();

		if (!(p = strtok(str, " "))) continue;

		if (!strcmp(p, "quit")) return;
		else if (!strcmp(p, "debug"))
		{
		}
		else if (!strcmp(p, "isready"))
		{
			cout << "readyok\n";
			cout.flush();
			Flog << "readyok\n";
			Flog.flush();
		}
		else if (!strcmp(p, "setoption"))
		{
			p = strtok(0, " ");
			p = strtok(0, " ");
			if (!stricmp(p, "Hash"))
			{
				p = strtok(0, " ");
				p = strtok(0, " ");
				uint16 size = atoi(p);
				SetTransSize(size);
				cout << "info string hash " << size << endl;
			}
		}
		else if (!strcmp(p, "ucinewgame"))
		{
		}
		else if (!strcmp(p, "position"))
		{
			p = strtok(0, " ");
			if (!strcmp(p, "fen"))
			{
				p = strtok(0, "m"); // веделяем строку до 'moves'
				SetBoard(p);
			}
			else SetBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
			if (p = strtok(0, " ")) // ожидается 'moves'
			{
				while (p = strtok(0, " "))
				{
					NI->InCheck = InCheck();
					Move m = StrToMove(p);
					MakeMove(m);
				}
			}
		}
		else if (!strcmp(p, "go"))
		{
			EngineTime = 0;
			IncrementTime = 0;
			MovesToControl = 0;
			DepthLimit = 100;
			TimeLimited = 0;
			while (p = strtok(0, " "))
			{
				if (!strcmp(p, "wtime") && WTM)
				{
					p = strtok(0, " ");
					EngineTime = atoi(p);
				}
				else if (!strcmp(p, "winc") && WTM)
				{
					p = strtok(0, " ");
					IncrementTime = atoi(p);
				}
				else if (!strcmp(p, "btime") && !WTM)
				{
					p = strtok(0, " ");
					EngineTime = atoi(p);
				}
				else if (!strcmp(p, "binc") && !WTM)
				{
					p = strtok(0, " ");
					IncrementTime = atoi(p);
				}
				else if (!strcmp(p, "movestogo"))
				{
					p = strtok(0, " ");
					MovesToControl = atoi(p);
				}
				else if (!strcmp(p, "depth"))
				{
					p = strtok(0, " ");
					DepthLimit = atoi(p);
				}
				else if (!strcmp(p, "movetime"))
				{
					p = strtok(0, " ");
					TimeLimited = (uint32)atoi(p);
				}
			}
			Move m = RootSearch();
			char sm[8];
			MoveToStr(m, sm);
			sprintf(str, "bestmove %s\n", sm);
			cout << str;
			cout.flush();
			Flog << str;
			Flog.flush();
		}
		else if (!strcmp(str, "ponderhit"))
		{
		}
	}
}

// под WB движок должен сообщать оболочке об окончании игры
bool GameEnded()
{
	if (Mate())
	{
		if (WTM) cout << "0-1";
		else cout << "1-0";
		cout << " {mate}\n";
		cout.flush();
		return true;
	}
	if (Stalemate())
	{
		cout << "1/2-1/2 {stalemate}\n";
		cout.flush();
		return true;
	}
	if (Repetition3())
	{
		cout << "1/2-1/2 {repetition}\n";
		cout.flush();
		return true;
	}
	if (Rule50Moves())
	{
		cout << "1/2-1/2 {50 moves}\n";
		cout.flush();
		return true;
	}
	if (InsufficientMaterial())
	{
		cout << "1/2-1/2 {insufficient material}\n";
		cout.flush();
		return true;
	}
	
	return false;
}

void XBoard()
{
	XBoardMode = true;
	bool force_mode = true;
	// флаг играет ли движок белыми
	bool ew = false;
	bool ponder_mode = false;
	OpenLog("_wb");

	char str[512];
	char *p;

	for (;;)
	{
		// ход движка
		if (!force_mode && ew == WTM)
		{
			Move m = RootSearch();
			// если прислана команда 'new', то ход отсылать не надо
			if (stricmp(AnalyzeCommand, "new"))
			{
				char sm[8];
				MoveToStr(m, sm);
				sprintf(str, "move %s\n", sm);
				cout << str;
				cout.flush();
				Flog << str;
				Flog.flush();
				MakeMove(m);
				GameMoves[GameMovesCnt++] = m;
				if (MovesToControl && !--MovesToControl) MovesToControl = MovesPerControl;
				if (GameEnded()) force_mode = true;

				Flog << "Eval = " << Eval() << endl;
			}
		}

		// режим анализа
		if (AnalyzeMode && !*AnalyzeCommand)
		{
			RootSearch();
		}

		if (*AnalyzeCommand)
		{
			strcpy(str, AnalyzeCommand);
			*AnalyzeCommand = 0;
		}
		else
		{
			cin.getline(str, 512);
			if (!*str) continue;
			Flog << "> " << str << endl;
			Flog.flush();
		}

		if (!(p = strtok(str, " "))) continue;

		if (!strcmp(p, "quit")) return;
		else if (!strcmp(p, "protover"))
		{
			cout << "feature myname=\"" << MY_NAME << "\"" << endl;
			cout << "feature ping=1\n";
			cout << "feature setboard=1\n";
			cout << "feature playother=1\n";
			cout << "feature draw=0\n";
			cout << "feature analyze=1\n";
			cout << "feature variants=\"normal\"\n";
			cout << "feature name=0\n";
			cout << "feature done=1\n";
			cout.flush();
		}
		else if (!strcmp(p, "new"))
		{
			SetBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
			if (!AnalyzeMode) force_mode = false;
			ew = false;
			DepthLimit = 100;
			GameMovesCnt = 0;
		}
		else if (!strcmp(p, "setboard"))
		{
			p = strtok(0, "");
			SetBoard(p);
			ew = !WTM;
			DepthLimit = 100;
			GameMovesCnt = 0;
		}
		else if (!strcmp(p, "force"))
		{
			force_mode = true;
		}
		else if (!strcmp(p, "white"))
		{
			WTM = true;
			ew = false;
		}
		else if (!strcmp(p, "black"))
		{
			WTM = false;
			ew = true;
		}
		else if (!strcmp(p, "playother"))
		{
			ew = !WTM;
			force_mode = false;
		}
		else if (!strcmp(p, "level"))
		{
			TimeLimited = 0;
			p = strtok(0, " ");
			MovesToControl = MovesPerControl = atoi(p);
			p = strtok(0, " ");
			p = strtok(0, " ");
			char *tmp;
			IncrementTime = strtod(p, &tmp) * 1000;
		}
		else if (!strcmp(p, "st"))
		{
			p = strtok(0, " ");
			TimeLimited = atoi(p) * 1000;
		}
		else if (!strcmp(p, "sd"))
		{
			p = strtok(0, " ");
			DepthLimit = atoi(p);
		}
		else if (!strcmp(p, "time"))
		{
			p = strtok(0, " ");
			EngineTime = atoi(p) * 10;
		}
		else if (!strcmp(p, "go"))
		{
			force_mode = false;
			ew = WTM;
		}
		else if (!strcmp(p, "undo"))
		{
			UnmakeMove(GameMoves[--GameMovesCnt]);
		}
		else if (!strcmp(p, "remove"))
		{
			UnmakeMove(GameMoves[--GameMovesCnt]);
			UnmakeMove(GameMoves[--GameMovesCnt]);
			if (MovesToControl && ++MovesToControl > MovesPerControl) MovesToControl = 1;
		}
		else if (!strcmp(p, "hard"))
		{
			ponder_mode = true;
		}
		else if (!strcmp(p, "easy"))
		{
			ponder_mode = false;
		}
		else if (!strcmp(p, "post"))
		{
			PostMode = true;
		}
		else if (!strcmp(p, "nopost"))
		{
			PostMode = false;
		}
		else if (!strcmp(p, "ping"))
		{
			p = strtok(0, " ");
			cout << "pong " << p << endl;
			cout.flush();
			Flog << "pong " << p << endl;
			Flog.flush();
		}
		else if (!strcmp(p, "analyze"))
		{
			AnalyzeMode = true;
			force_mode = true;
		}
		// игнорируемые команды
		else if (!strcmp(p, "random")) continue;
		else if (!strcmp(p, "hint")) continue;
		else if (!strcmp(p, "bk")) continue;
		else if (!strcmp(p, "result")) continue;
		else if (!strcmp(p, "otim")) continue;
		else if (!strcmp(p, "computer")) continue;
		else if (!strcmp(p, "accepted")) continue;
		else if (!strcmp(p, "rejected")) continue;
		else if (!strcmp(p, "draw")) continue;
		// попытка распознать ход соперника
		else
		{
			Move m = StrToMove(str);
			// делаем ход соперника и проверяем не закончилась ли игра
			if (m.from || m.to)
			{
				MakeMove(m);
				GameMoves[GameMovesCnt++] = m;
				if (GameEnded()) force_mode = true;
			}
			// ход не распознали
			else
			{
				cout << "Illegal move: " << str << endl;
				cout.flush();
				Flog << "Illegal move: " << str << endl;
				Flog.flush();
			}
		}
	}
}
