#ifndef Eval_H
#define Eval_H

#include "Types.h"

void InitPST();
void InitEval();
__int16 Eval();
void CalcPST();
void InitMaterialSignature();
void InitMaterial();
//-------------------------------------------

struct MaterialEntry
{
	__int32 bonus;
	uint8 flags;
	uint8 phase;
};
const uint32 MaxMaterial = 2*2*3*3*3*3*3*3*9*9;
const uint16 MSValue[13] = {0, 2916, 324, 36, 4, 1, 0, 26244, 972, 108, 12, 2, 0};
extern MaterialEntry *Material;

extern __int32 PieceValueOp[], PieceValueEg[];
extern __int32 PST[13][64][2];
extern __int32 KnightCenterOp;
extern __int32 KnightCenterEg;
extern __int32 KnightRankOp;
extern __int32 KnightRankEg;
extern __int32 BishopCenterOp;
extern __int32 BishopCenterEg;
extern __int32 BishopBackRankOp;
extern __int32 BishopDiagonalOp;
extern __int32 BishopDiagonalEg;
extern __int32 RookFileOp;
extern __int32 RookFileEg;
extern __int32 KingFileOp;
extern __int32 KingCenterEg;

extern __int32 ebPassedPawnOp[8], ebPassedPawnEg[8];
extern __int32 ebBishopPairOp, ebBishopPairEg;
extern __int32 ebBishopPairVsNoLight;
extern __int32 epRookPairOp, epRookPairEg, epRookQueenOp, epRookQueenEg;
extern __int32 epMissingPawn1;
extern __int32 epMissingPawn2;
extern __int32 epMissingPawn3;
extern __int32 epMissingPawn;
extern __int32 epWeakBackRank;
extern __int32 ebKnightMobilityOp;
extern __int32 ebKnightMobilityEg;
extern __int32 ebBishopMobilityOp;
extern __int32 ebBishopMobilityEg;
extern __int32 ebRookMobilityOp;
extern __int32 ebRookMobilityEg;
extern __int32 ebQueenMobilityOp;
extern __int32 ebQueenMobilityEg;
extern __int32 ebRookHalfOpenOp;
extern __int32 ebRookHalfOpenEg;
extern __int32 ebRookOpenOp;
extern __int32 ebRookOpenEg;
extern __int32 epIsolatedPawnOp;
extern __int32 epIsolatedPawnEg;
extern __int32 ebRook7Op, ebRook7Eg;
extern __int32 ebQueen7Op, ebQueen7Eg;
extern __int32 ebRookKingAttack;
extern __int32 ebRookFileKingAttack;
extern __int32 ebPassedNoOwn[8];
extern __int32 ebPassedNoEnemy[8];
extern __int32 ebPassedFreePath[8];
extern __int32 ebPassedOwnKingDist[8];
extern __int32 ebPassedEnemyKingDist[8];
extern __int32 ebUnstopablePasser;
extern __int32 epBishopTraped;
extern __int32 epBishopBoxed;
extern __int32 epRookBoxed;
extern __int32 KnightAttackVal;
extern __int32 BishopAttackVal;
extern __int32 RookAttackVal;
extern __int32 QueenAttackVal;
extern __int32 KingAttackVal[16];
extern __int32 ebSideToMoveOp;
extern __int32 ebSideToMoveEg;
extern __int32 epDoubledPawnOp;
extern __int32 epDoubledPawnEg;
extern __int32 epOpenIsolatedPawnOp;
extern __int32 epOpenIsolatedPawnEg;
extern __int32 epOpenBackwardPawnOp;
extern __int32 epOpenBackwardPawnEg;
extern __int32 epBackwardPawnOp;
extern __int32 epBackwardPawnEg;
extern __int32 ebCandidatePawnOp[8];
extern __int32 ebCandidatePawnEg[8];
extern __int32 KnightPawnMat;
extern __int32 BishopPawnMat;
extern __int32 BishopPairPawnMat;
extern __int32 BlockedPawnOp;
extern __int32 BlockedPawnEg;

#endif
