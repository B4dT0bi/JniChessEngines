__int32 PieceValueOp[] = {0, 2176,  8160,  8160, 10880, 22848, 0, 2176,  8160,  8160, 10880, 22848, 0};
__int32 PieceValueEg[] = {0, 3400, 11050, 11050, 18700, 33150, 0, 3400, 11050, 11050, 18700, 33150, 0};

__int32 ebBishopPairOp = 952;
__int32 ebBishopPairEg = 1496;
__int32 BishopPairPawnMat = 96;
__int32 ebBishopPairVsNoLight = 160;
__int32 KnightPawnMat =  160;
__int32 BishopPawnMat =  160;
__int32 epRookPairOp = 448;
__int32 epRookPairEg = 896;
__int32 epRookQueenOp = 224;
__int32 epRookQueenEg = 448;

__int32 ebKnightMobilityOp = 64;
__int32 ebKnightMobilityEg = 64;
__int32 ebBishopMobilityOp = 64;
__int32 ebBishopMobilityEg = 64;
__int32 ebRookMobilityOp   = 64;
__int32 ebRookMobilityEg   = 64;
__int32 ebQueenMobilityOp  = 64;
__int32 ebQueenMobilityEg  = 64;

__int32 ebRookHalfOpenOp =   160;
__int32 ebRookHalfOpenEg =   160;
__int32 ebRookOpenOp =       640;
__int32 ebRookOpenEg =       640;

__int32 KnightCenterOp   = 160;
__int32 KnightCenterEg   = 160;
__int32 KnightRankOp     = 160;
__int32 KnightRankEg     =   0;

__int32 BishopCenterOp   = 160;
__int32 BishopCenterEg   =  64;
__int32 BishopBackRankOp = 256;
__int32 BishopDiagonalOp = 320;
__int32 BishopDiagonalEg =   0;

__int32 RookFileOp       =  96;
__int32 RookFileEg       =   0;
__int32 KingFileOp       = 320;
__int32 KingCenterEg     = 320;


__int32 ebPassedPawnOp[8] = { 0,   0,   0, 400, 800, 1600, 3200, 0};
__int32 ebPassedPawnEg[8] = { 0,   0,   0, 400, 800, 1600, 3200, 0};
__int32 ebCandidatePawnOp[8] = { 0,  0,  0, 256, 512,   0, 0, 0};
__int32 ebCandidatePawnEg[8] = { 0,  0,  0, 256, 512,   0, 0, 0};
__int32 ebPassedNoOwn[8] = {0, 0, 0, 32,  64,  128,  256, 0};
__int32 ebPassedNoEnemy[8] = {0, 0, 0, 192, 384, 768,1536, 0};
__int32 ebPassedFreePath[8] = {0, 0, 0,192, 384, 768,1536, 0};
__int32 ebPassedOwnKingDist[8] = {0, 0, 0, 64, 192, 384, 640, 0};
__int32 ebPassedEnemyKingDist[8] = {0, 0, 0, 128, 384, 768, 1280, 0};

__int32 epIsolatedPawnOp = 96;
__int32 epIsolatedPawnEg =  288;
__int32 epDoubledPawnOp = 96;
__int32 epDoubledPawnEg = 160;
__int32 epOpenIsolatedPawnOp = 480;
__int32 epOpenIsolatedPawnEg = 800;
__int32 epOpenBackwardPawnOp = 320;
__int32 epOpenBackwardPawnEg = 640;
__int32 epBackwardPawnOp =  64;
__int32 epBackwardPawnEg = 224;

__int32 epMissingPawn1 =  256;
__int32 epMissingPawn2 =  640;
__int32 epMissingPawn3 =  800;
__int32 epMissingPawn  =  1024;
__int32 epWeakBackRank =  512;

__int32 ebUnstopablePasser = 24000;
__int32 epBishopTraped = 1600;
__int32 epBishopBoxed = 1600;
__int32 epRookBoxed = 1600;

__int32 KnightAttackVal = 288;
__int32 BishopAttackVal = 288;
__int32 RookAttackVal = 480;
__int32 QueenAttackVal = 864;
__int32 KingAttackVal[16] = {0,  0,  50,  75, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100};

__int32 ebSideToMoveOp = 96;
__int32 ebSideToMoveEg = 96;

__int32 BlockedPawnOp = 92;
__int32 BlockedPawnEg = 320;
