#ifndef Search_H
#define Search_H

#include "Types.h"
#include "Moves.h"

Move RootSearch();
inline bool Repetition();
__int16 QuiescenceSearch(__int16 alpha, __int16 beta, uint8 gencheck);

// ���������� �� ����� ������ ��������
struct NodeInfo
{
	// ���-�����
	uint64 HashKey;
	uint64 PawnHashKey;

	// ������ ��������� � ��������� �����
	__int32 OP;
	__int32 EG;

	// �������
	Move Killer1, Killer2;

	// ��������� ���������
	uint32 MS; // Material Signature

	// ����� �� ���������
	uint8 CastleRights;
	// ���� ��� ������ �� �������
	uint8 EP;
	// �������� ���
	uint8 InCheck;

	uint8 Reversible;

	Move move;
	__int16 eval;
};

// ������ ����
const __int16 MATE = 0x7fff;

extern uint8 DepthLimit;
extern uint64 NodesLimit;
extern uint64 Nodes;
extern NodeInfo *NI;
extern NodeInfo NodesInfo[1024];
extern uint32 History[13][64];
extern Move NullMove;

#endif
