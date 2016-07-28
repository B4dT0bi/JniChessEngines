#include <locale>
#include <iostream>
#include <stdint.h>
using namespace std;

#include "Board.h"
#include "BitBoards.h"
#include "Board.h"
#include "Search.h"
#include "Protocols.h"
#include "Hash.h"
#include "MoveList.h"
#include "Eval.h"

// ���������� ����� ������ ����������� �� ������ ����
uint8 Piece[64];

// ����������� ���� �����
bool WTM;

// ��������� ����������� �����
char PieceLetter[] = {'.', 'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'};

// ������ ������ �� �����
void PutPiece(uint8 to, uint8 p)
{
	Piece[to] = p;
	BitBoards[p] ^= SBM[to];
	if (WhitePiece(p)) WhitePieces ^= SBM[to];
	else BlackPieces ^= SBM[to];
	NI->HashKey ^= HashKeys[p][to];
	if (p == ptWhitePawn || p == ptBlackPawn) NI->PawnHashKey ^= HashKeys[p][to];
	if (p == ptWhiteKing || p == ptBlackKing) NI->PawnHashKey ^= HashKeys[p][to];
	NI->OP += PST[p][to][0];
	NI->EG += PST[p][to][1];
	NI->MS += MSValue[p];
}

// ����������� ����� � ������������ � FEN
void SetBoard(char *fen)
{
	int i, j;

	NI = NodesInfo;
	NI->HashKey = NI->PawnHashKey = 0;
	NI->OP = NI->EG = 0;
	NI->MS = 0;

	for (i = 0; i < 64; i++) Piece[i] = ptNone;
	for (i = 1; i <= ptBlackKing; i++) BitBoards[i] = 0;
	WhitePieces = BlackPieces = 0;

	// ����������� ������
	i = 0;
	while (i < 64)
	{
		char c = *fen++;
		if (c == '/') continue;
		if (isdigit(c))
		{
			i += c - '0';
			continue;
		}
		for (j = 1; j <= ptBlackKing; j++)
		{
			if (PieceLetter[j] == c)
			{
				PutPiece(i, j);
				break;
			}
		}
		i++;
	}

	// ����������� ����
	if (toupper(*(++fen)++) == 'W')
	{
		WTM = true;
		NI->HashKey ^= HashKeyWTM;
	}
	else WTM = false;
	fen++;

	// ����� �� ���������
	NI->CastleRights = 0;
	while (*fen != ' ')
	{
		switch (*fen)
		{
			case 'Q': NI->CastleRights |= crWhiteQueenside; break;
			case 'K': NI->CastleRights |= crWhiteKingside;  break;
			case 'q': NI->CastleRights |= crBlackQueenside; break;
			case 'k': NI->CastleRights |= crBlackKingside;  break;
		}
		fen++;
	}
	NI->HashKey ^= HashKeysCR[NI->CastleRights];
	fen++;

	// ���� ��� ������ �� �������
	if (*fen == '-') NI->EP = 0;
	else
	{
		NI->EP = *fen - 'a' + (7 + '1' - *(fen + 1)) * 8;
		NI->HashKey ^= HashKeysEP[NI->EP];
	}

	// ������������� ���� ����
	NI->InCheck = InCheck();
}

// �������������� �����
void InvertBoard()
{
	uint8 i;
	uint8 b2[64];

	// ������������� �������� �����
	for (i = 0; i < 32; i++)
	{
		uint8 p          = Piece[i];
		Piece[i]         = Piece[Mirror[i]];
		Piece[Mirror[i]] = p;
	}
	for (i = 1; i <= ptBlackKing; i++) BitBoards[i] = 0;
	WhitePieces = BlackPieces = 0;

	// ������ ���� ����� � ��������� ��������
	for (i = 0; i < 64; i++)
	{
		if (Piece[i] == ptNone) continue;
		if (WhitePiece(Piece[i]))
		{
			Piece[i] += 6;
			BlackPieces |= SBM[i];
		}
		else
		{
			Piece[i] -= 6;
			WhitePieces |= SBM[i];
		}
		BitBoards[Piece[i]] |= SBM[i];
	}
	WTM = !WTM;
	uint8 cr = NI->CastleRights & 3;
	NI->CastleRights = (NI->CastleRights & 12) >> 2;
	NI->CastleRights |= cr << 2;
	NI->OP = -NI->OP;
	NI->EG = -NI->EG;
	InitMaterialSignature();
}

char BitBoardName[][16] = 
{
	"None",
	"WhitePawns", 
	"WhiteKnights", 
	"WhiteBishops", 
	"WhiteRooks", 
	"WhiteQueens", 
	"WhiteKing", 
	"BlackPawns", 
	"BlackKnights", 
	"BlackBishops", 
	"BlackRooks", 
	"BlackQueens", 
	"BlackKing"
};

void PrintError(char *str)
{
	cout << endl << str << endl << endl;
}

void PrintError(uint8 p, char *s2)
{
	cout << endl << "ERROR: " << BitBoardName[p] << s2 << endl << endl;
}

// ��������������� ������ ������� ������ ���� ����������
// ��������� ��������
bool TestBitBoardSet(uint8 sq)
{
	int i;

	for (i = 1; i <= ptBlackKing; i++)
	{
		if (i == Piece[sq])
		{
			if (!(SBM[sq] & BitBoards[i])) return false;
		}
		else
		{
			if (SBM[sq] & BitBoards[i]) return false;
		}
	}
	return true;
}

// ��������� ������������ ��������� ������� ���� � ���� ���������
bool TestSquare(uint8 sq)
{
	uint8 i;

	bool r = TestBitBoardSet(sq);
	if (!r) return r;

	// ������ ����
	if (Piece[sq] == ptNone)
	{
		if (SBM[sq] & WhitePieces) return false;
		if (SBM[sq] & BlackPieces) return false;
	}
	// ����� ������
	else if (WhitePiece(Piece[sq]))
	{
		if (!(SBM[sq] & WhitePieces)) return false;
		if (SBM[sq]   & BlackPieces)  return false;
	}
	// ������
	else
	{
		if (SBM[sq] & WhitePieces)    return false;
		if (!(SBM[sq] & BlackPieces)) return false;
	}
	return true;
}

bool TestBoard()
{
	uint8 i;
	for (i = 0; i < 64; i++)
	{
		bool r = TestSquare(i);
		if (!r) return r;
	}
	return true;
}

// ������ �����
void PrintBoard()
{
	uint8 i, j;

	cout << endl;
	Flog << endl;
	for (i = 0; i < 64; i++)
	{
		cout << PieceLetter[Piece[i]];
		Flog << PieceLetter[Piece[i]];
		if (File(i) == 7)
		{
			cout << endl;
			Flog << endl;
		}
	}
	if (WTM)
	{
		cout << "white";
		Flog << "white";
	}
	else
	{
		cout << "black";
		Flog << "black";
	}
	cout << " to move" << endl << endl;
	Flog << " to move" << endl << endl;
}

// ���� ��������� ������?
uint8 AttackedByWhite(uint8 sq)
{
	if (KnightMoves[sq] & WhiteKnights) return 1;
	if (WPawnAtk[sq] & WhitePawns) return 1;
	if (KingMoves[sq]   & WhiteKing) return 1;
	if (RookMoves(sq)   & (WhiteQueens | WhiteRooks)) return 1;
	if (BishopMoves(sq) & (WhiteQueens | WhiteBishops)) return 1;
	return 0;
}

// ���� ��������� �������?
uint8 AttackedByBlack(uint8 sq)
{
	if (KnightMoves[sq] & BlackKnights) return 1;
	if (BPawnAtk[sq] & BlackPawns) return 1;
	if (KingMoves[sq] & BlackKing) return 1;
	if (RookMoves(sq)   & (BlackQueens | BlackRooks)) return 1;
	if (BishopMoves(sq) & (BlackQueens | BlackBishops)) return 1;
	return 0;
}

// ������� ��� ����������� ��������� ���� ��� WB
//----------------------------------------------

// ���� � ������� ��� �����
bool NoLegalMoves()
{
	MoveList ml;
	ml.GenMoves();
	while (Move *m = ml.NextMove())
	{
		if (MakeMove(*m))
		{
			UnmakeMove(*m);
			return false;
		}
	}
	return true;
}

// ���
bool Mate()
{
	if (!NI->InCheck) return false;
	return NoLegalMoves();
}

// ���
bool Stalemate()
{
	if (NI->InCheck) return false;
	return NoLegalMoves();
}

// 3-������� ���������� �������
bool Repetition3()
{
	if (NI - NodesInfo < 4) return false;
	if (!NI->Reversible) return false;
	if (!(NI - 1)->Reversible) return false;
	if (!(NI - 2)->Reversible) return false;
	if (!(NI - 3)->Reversible) return false;

	uint8 reps = 1;
	for (NodeInfo *ni = NI - 4; ni >= NodesInfo; ni -= 2)
	{
		if (!ni->Reversible) break;
		if (ni->HashKey == NI->HashKey) reps++;
		
		// ���������� ������� � ������ ������������ ����
		if (ni > NodesInfo && !(ni - 1)->Reversible) break;
	}
	return reps >= 3;
}

// ������� 50 �����
bool Rule50Moves()
{
	if (NI - NodesInfo < 100) return false;
	uint8 cnt = 0;
	for (NodeInfo *ni = NI; ni >= NodesInfo && cnt < 100; ni--)
	{
		if (!ni->Reversible) break;
		cnt++;
	}
	return cnt >= 100;
}

// ������������ ��������� ��� ����
bool InsufficientMaterial()
{
	// �����, �����, ����� �����������
	if (WhitePawns || WhiteQueens || WhiteRooks) return false;
	if (BlackPawns || BlackQueens || BlackRooks) return false;

	uint8 cnt = PopCnt(WhitePieces | BlackPieces);
	
	// ������ ���� ������ ������
	if (cnt < 4) return true;

	// ��� ����������� �����
	if (cnt == 4 && WhiteBishops && BlackBishops)
	{
		uint8 wsq = LSB(WhiteBishops);
		uint8 bsq = LSB(BlackBishops);
		if ((File(wsq) + Rank(wsq) & 1) == (File(bsq) + Rank(bsq) & 1)) return true;
	}
	return false;
}

// �������� fen ������� �������
void GetFen(char *str)
{
	int i, j;
	int k = 0;
	for (i = 0; i < 64; i += 8)
	{
		int cnt = 0;
		for (j = 0; j < 8; j++)
		{
			if (Piece[i + j] == ptNone)
			{
				cnt++;
				continue;
			}
			if (cnt)
			{
				str[k++] = '0' + cnt;
				cnt = 0;
			}
			str[k++] = PieceLetter[Piece[i + j]];
		}
		if (cnt) str[k++] = '0' + cnt;
		if (i < 56) str[k++] = '/';
	}
	str[k++] = ' ';
	if (WTM) str[k++] = 'w';
	else str[k++] = 'b';

	str[k++] = ' ';
	int oldk = k;
	if (NI->CastleRights & crWhiteKingside) str[k++] = 'K';
	if (NI->CastleRights & crWhiteQueenside) str[k++] = 'Q';
	if (NI->CastleRights & crBlackKingside) str[k++] = 'k';
	if (NI->CastleRights & crBlackQueenside) str[k++] = 'q';
	if (oldk == k) str[k++] = '-';

	str[k++] = ' ';
	if (NI->EP)
	{
		str[k++] = 'a' + File(NI->EP);
		str[k++] = '0' + 8 - Rank(NI->EP);
	}
	else str[k++] = '-';
	
	str[k] = 0;
}

// ��������� ���� �������� �������
void Base(char *pgn, int n)
{
	int i, j;
	const int MAX_POS = 64000;
	ifstream fin(pgn);
	struct PosCounter
	{
		char fen[128];
		int cnt;
	};
	PosCounter *Ops = new PosCounter[MAX_POS];
	int cnt = 0;
	for (;;)
	{
		char str[128];
		for (;;)
		{
			fin >> str;
			if (!fin) break;
			if (!strcmp(str, "1.")) break;
		}
		if (!fin) break;
		SetBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -");
		i = 0;
		while (i < n)
		{
			fin >> str;
			if (strstr(str, ".")) continue;
			if (str[0] == '*') break;
			if (!strcmp(str, "1-0")) break;
			if (!strcmp(str, "0-1")) break;
			if (!strcmp(str, "1/2-1/2")) break;
			Move m = StrToMove(str);
			if (!m.from && !m.to) break;
			MakeMove(m);
			i++;
		}
		if (i < n) continue;
		GetFen(str);
		for (i = 0; i < cnt; i++)
		{
			if (!strcmp(Ops[i].fen, str))
			{
				Ops[i].cnt++;
				break;
			}
		}
		if (i < cnt) continue;
		if (cnt == MAX_POS) continue;
		strcpy(Ops[cnt].fen, str);
		Ops[cnt].cnt = 1;
		cnt++;
	}
	int s[MAX_POS];
	for (i = 0; i < cnt; i++) s[i] = i;
	for (i = 0; i < cnt; i++)
	{
		for (j = cnt - 1; j > i; j--)
		{
			if (Ops[s[i]].cnt < Ops[s[j]].cnt)
			{
				int t = s[i];
				s[i] = s[j];
				s[j] = t;
			}
		}
	}
	ofstream f1("A.epd");
	ofstream f2("B.epd");
	ofstream f3("C.epd");
	ofstream f4("D.epd");
	for (i = 0; i < cnt; i++)
	{
		cout << i + 1 << '\t' << Ops[s[i]].cnt << endl;
		cout << Ops[s[i]].fen << endl;
		if (i % 4 == 0) f1 << Ops[s[i]].fen << endl;
		else if (i % 4 == 1) f2 << Ops[s[i]].fen << endl;
		else if (i % 4 == 2) f3 << Ops[s[i]].fen << endl;
		else if (i % 4 == 3) f4 << Ops[s[i]].fen << endl;
	}
}
