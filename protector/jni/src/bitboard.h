/*
    Protector -- a UCI chess engine

    Copyright (C) 2009-2010 Raimund Heid (Raimund_Heid@yahoo.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef _bitboard_h_
#define _bitboard_h_

#include "protector.h"
#include <string.h>

typedef UINT64 Bitboard;

#define EMPTY_BITBOARD ULONG_ZERO
#define IMAX_ROOK    4096
#define IMAX_BISHOP  512

typedef struct
{
   int hLane, vLane, uLane, dLane;
   BYTE hLaneSetMask, vLaneSetMask, uLaneSetMask, dLaneSetMask;
   BYTE hLaneClearMask, vLaneClearMask, uLaneClearMask, dLaneClearMask;
}
SquareLaneInfo;

typedef struct
{
   int numSetSquares;
   UINT8 setSquares[16];
}
SetSquaresInfo;

typedef struct
{
   int hLaneNumber, vLaneNumber, uLaneNumber, dLaneNumber;
   Bitboard hLane[256], vLane[256], uLane[256], dLane[256];
}
ObstacleSquareInfo;

typedef struct
{
   Bitboard preMask, magicNumber;
   Bitboard moves[IMAX_ROOK];
}
MagicSquareInfoRook;

typedef struct
{
   Bitboard preMask, magicNumber;
   Bitboard moves[IMAX_BISHOP];
}
MagicSquareInfoBishop;

#define SLI(square) (squareLaneInfo[(square)])

extern UINT64 minValue[_64_];
extern UINT64 maxValue[_64_];
extern INT8 highestBit[0x10000];
extern INT8 lowestBit[0x10000];
extern Bitboard hlane[_64_][256];
extern Bitboard vlane[_64_][256];
extern Bitboard ulane[_64_][256];
extern Bitboard dlane[_64_][256];
extern ObstacleSquareInfo obsi[_64_];
extern Bitboard castlings[2][16][256];
extern int castlingLane[2];
extern int castlingsOfColor[2];
extern SquareLaneInfo squareLaneInfo[_64_];
extern Bitboard generalMoves[0x0f][_64_];
extern const Bitboard squaresOfFile[8];
extern const Bitboard squaresOfRank[8];
extern const Bitboard squaresOfLateralFiles[8];
extern Bitboard squaresOfFileRange[8][8];
extern Bitboard squaresOfRankRange[8][8];
extern Bitboard pawnMoves[2][_64_][256];
extern Bitboard promotionCandidates[2];
extern SetSquaresInfo setSquares[4][0x10000];
extern INT8 numSetBits[0x10000];
extern UINT8 rankOverlay[0x10000];
extern UINT8 bitshiftGap[8][256];
extern Bitboard squaresBehind[_64_][_64_];
extern Bitboard squaresBetween[_64_][_64_];
extern Bitboard squaresInDistance[8][_64_];
extern Bitboard squaresInTaxiDistance[15][_64_];
extern Bitboard squaresAbove[2][_64_];
extern Bitboard squaresBelow[2][_64_];
extern Bitboard squaresLeftOf[_64_];
extern Bitboard squaresRightOf[_64_];
extern Bitboard orthoKingAttackers[_64_];
extern Bitboard diaKingAttackers[_64_];
extern Bitboard knightKingAttackers[_64_];
extern Bitboard pawnKingAttackers[2][_64_];
extern Bitboard interestedPawns[2][_64_][256];
extern Bitboard nonA, nonH, border, center, lightSquares, darkSquares,
   queenSide, kingSide, centerFiles, extendedCenter;
extern int hLaneNumber[_64_], vLaneNumber[_64_];
extern int uLaneNumber[_64_], dLaneNumber[_64_];
extern Bitboard preMaskRook[64], preMaskBishop[64];
extern Bitboard magicRookMoves[64][IMAX_ROOK];
extern Bitboard magicBishopMoves[64][IMAX_BISHOP];
extern const Bitboard magicRookNumber[64];
extern const Bitboard magicBishopNumber[64];
extern MagicSquareInfoRook magicSquareInfoRook[64];
extern MagicSquareInfoBishop magicSquareInfoBishop[64];

#define setSquare(bitboard,square) ((bitboard) |= minValue[(square)]);
#define clearSquare(bitboard,square) ((bitboard) &= maxValue[(square)]);
#define excludeSquares(bitboard,toBeExcluded) ((bitboard) &= ~(toBeExcluded))

/**
 * The number of lanes used to hold information
 * about the state of all files, rows and diagonals
 */
#define NUM_LANES 46

#define UHEX_FFFF 0xFFFFllu
#define UHEX_FFFFffff00000000 0xFFFFffff00000000llu
#define UHEX_00000000FFFF0000 0x00000000FFFF0000llu
#define UHEX_FFFF000000000000 0xFFFF000000000000llu
#define UHEX_0000ffff00000000 0x0000ffff00000000llu
#define UHEX_00000000FFFFffff 0x00000000FFFFffffllu
#define UHEX_000000000000ffff 0x000000000000ffffllu

bool testSquare(const Bitboard bitboard, const Square square);
Bitboard getKingMoves(const Square square);
Bitboard getCastlingMoves(const Color color,
                          const BYTE castlingRights,
                          const Bitboard obstacles);
/*
 Bitboard getQueenMoves(const Square square, const BYTE * obstacles);
*/
int getWidth(const Bitboard set);
Bitboard getMagicQueenMoves(const Square square, const Bitboard obstacles);
Bitboard getRookMoves(const Square square, const BYTE * obstacles);
Bitboard getMagicRookMoves(const Square square, const Bitboard obstacles);
Bitboard getBishopMoves(const Square square, const BYTE * obstacles);
Bitboard getMagicBishopMoves(const Square square, const Bitboard obstacles);
Bitboard getKnightMoves(const Square square);
Bitboard getPawnCaptures(const Piece piece, const Square square,
                         const Bitboard allPieces);
Bitboard getPawnAdvances(const Color color, const Square square,
                         const Bitboard obstacles);
Bitboard getInterestedPawns(const Color color,
                            const Square square, const Bitboard obstacles);
Bitboard getSquaresBetween(const Square square1, const Square square2);
Bitboard getSquaresBehind(const Square target, const Square viewpoint);
Bitboard shiftLeft(const Bitboard bitboard);
Bitboard shiftRight(const Bitboard bitboard);
Bitboard getLateralSquares(const Bitboard squares);
Bitboard getSquaresOfFile(const File file);
Bitboard getSquaresOfRank(const Rank rank);
int getNumberOfSetSquares(const Bitboard bitboard);
int getRankOverlay(const Bitboard bitboard);
Bitboard getMoves(Square square, Piece piece, Bitboard allPieces);
Bitboard getCaptureMoves(Square square, Piece piece, Bitboard allPieces);
void setObstacleSquare(Square square, BYTE obstacles[NUM_LANES]);
void clearObstacleSquare(Square square, BYTE obstacles[NUM_LANES]);
void calculateObstacles(Bitboard board, BYTE obstacles[NUM_LANES]);
Square getLastSquare(Bitboard * vector);
void floodBoard(Bitboard * board);
Bitboard getWhitePawnTargets(const Bitboard whitePawns);
Bitboard getBlackPawnTargets(const Bitboard blackPawns);
void floodBoard(Bitboard * board);
Square getLastSquare(Bitboard * vector);
Square getFirstSquare(Bitboard * vector);
int getSetSquares(const Bitboard board, UINT8 squares[_64_]);
Bitboard getMultipleSquaresBetween(const Square origin, Bitboard targets);

#define ITERATE_BITBOARD(b,sq) while ( ( sq = getLastSquare(b) ) >= 0 )

unsigned int getMinimumDistance(const Bitboard targets, const Square square);
unsigned int getMaximumDistance(const Bitboard targets, const Square square);
int getFloodValue(const Square origin, const Bitboard targets,
                  const Bitboard permittedSquares);
Bitboard getFlippedBitboard(Bitboard original);

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleBitboard(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleBitboard(void);

#endif
