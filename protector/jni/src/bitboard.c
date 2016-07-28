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

/*
   This module contains a bitboard-based chess move generator.
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "bitboard.h"
#include "io.h"

#define offset(sq1,sq2) ( ( (sq2)-(sq1) ) / distance(sq1,sq2) )

/**
 * Macros for internal use.
 */
#define HLANE(sq)    ( rank(sq) )
#define HLANEBIT(sq) ( FILE_H - file(sq) )
#define VLANE(sq)    ( file(sq) )
#define VLANEBIT(sq) ( RANK_8 - rank(sq) )
#define ULANE(sq)    ( file(sq) - rank(sq) + 7 )
#define ULANEBIT(sq) ( RANK_8 - rank(sq) )
#define DLANE(sq)    ( file(sq) + rank(sq) )
#define DLANEBIT(sq) ( rank(sq) )
#define HLANE_SQUARE(lane,bit) ( getSquare ( 7-(bit), (lane) ) )
#define VLANE_SQUARE(lane,bit) ( getSquare ( (lane), 7-(bit) ) )
#define ULANE_SQUARE(lane,bit) ( getSquare ( (lane)-(bit), 7-(bit) ) )
#define DLANE_SQUARE(lane,bit) ( getSquare ( (lane)-(bit), (bit) ) )

static bool isInitialized = FALSE;

UINT64 minValue[_64_];
UINT64 maxValue[_64_];
INT8 highestBit[0x10000];
INT8 lowestBit[0x10000];
INT8 numSetBits[0x10000];
UINT8 rankOverlay[0x10000];
UINT8 bitshiftGap[8][256];
int hLaneNumber[_64_], vLaneNumber[_64_];
int uLaneNumber[_64_], dLaneNumber[_64_];

Bitboard generalMoves[0x0f][_64_];

/* *INDENT-OFF* */
const Bitboard squaresOfFile[8] = {
   0x0101010101010101llu,
   0x0202020202020202llu,
   0x0404040404040404llu,
   0x0808080808080808llu,
   0x1010101010101010llu,
   0x2020202020202020llu,
   0x4040404040404040llu,
   0x8080808080808080llu
};
const Bitboard squaresOfRank[8] = {
   0x00000000000000ffllu,
   0x000000000000ff00llu,
   0x0000000000ff0000llu,
   0x00000000ff000000llu,
   0x000000ff00000000llu,
   0x0000ff0000000000llu,
   0x00ff000000000000llu,
   0xff00000000000000llu
};
const Bitboard squaresOfLateralFiles[8] = {
   0x0202020202020202llu,
   0x0505050505050505llu,
   0x0a0a0a0a0a0a0a0allu,
   0x1414141414141414llu,
   0x2828282828282828llu,
   0x5050505050505050llu,
   0xa0a0a0a0a0a0a0a0llu,
   0x4040404040404040llu
};
/* *INDENT-ON* */

Bitboard squaresOfFileRange[8][8];
Bitboard squaresOfRankRange[8][8];
Bitboard nonA, nonH, border, center, lightSquares, darkSquares, queenSide,
   kingSide, centerFiles, extendedCenter;
Bitboard squaresBehind[_64_][_64_];
Bitboard squaresBetween[_64_][_64_];
Bitboard squaresInDistance[8][_64_];
Bitboard squaresInTaxiDistance[15][_64_];
Bitboard squaresAbove[2][_64_];
Bitboard squaresBelow[2][_64_];
Bitboard squaresLeftOf[_64_];
Bitboard squaresRightOf[_64_];
Bitboard orthoKingAttackers[_64_];
Bitboard diaKingAttackers[_64_];
Bitboard knightKingAttackers[_64_];
Bitboard pawnKingAttackers[2][_64_];

SquareLaneInfo squareLaneInfo[_64_];
Bitboard hlane[_64_][256];
Bitboard vlane[_64_][256];
Bitboard ulane[_64_][256];
Bitboard dlane[_64_][256];
ObstacleSquareInfo obsi[_64_];
Bitboard pawnMoves[2][_64_][256];
Bitboard interestedPawns[2][_64_][256];
Bitboard castlings[2][16][256];
int castlingLane[] = { HLANE(E1), HLANE(E8) };
Bitboard promotionCandidates[2];
SetSquaresInfo setSquares[4][0x10000];
Bitboard preMaskRook[64], preMaskBishop[64];
Bitboard magicRookMoves[64][IMAX_ROOK];
Bitboard magicBishopMoves[64][IMAX_BISHOP];
MagicSquareInfoRook magicSquareInfoRook[64];
MagicSquareInfoBishop magicSquareInfoBishop[64];

/* *INDENT-OFF* */
const Bitboard magicRookNumber[64] = { 
   2341871876025885824llu, 5908793080399659264llu,
   2814784401577026llu, 4755810071654563842llu,
   4683749110191751172llu, 648520571176420352llu,
   288793875996213508llu, 36039242390061312llu,
   2233855853176307712llu, 360296835543278592llu,
   37470368710784llu, 2261832991510528llu,
   74595267411776576llu, 4612831726723531008llu,
   17491104022660138llu, 9015996489662496llu,
   164381455144716288llu, 4785426803991552llu,
   2603080765009821794llu, 4504870937840771llu,
   73202185810346241llu, 2594073935322554944llu,
   577059170398781952llu, 687199551504llu,
   36029072970620928llu, 4755818807417635204llu,
   2305854279342097445llu, 4654506018003485187llu,
   1369095394823243808llu, 1650609881216llu,
   35189740863840llu, 17609634498592llu,
   17867407885440llu, 1307170084179935305llu,
   5224509820511455237llu, 30399366238175240llu,
   685674177139050632llu, 4504708001565696llu,
   576777574861506688llu, 2305843147759948801llu,
   36046664624070657llu, 2305862800624326656llu,
   35201686183936llu, 76563943050448912llu,
   4828140284174931982llu, 93630429790212llu,
   2305862800758624268llu, 4918212371161546978llu,
   1197987317416064llu, 4629700452154347536llu,
   144220742299427329llu, 1152928385211832832llu,
   40132191457288llu, 9354573578272llu,
   650209674406199320llu, 36029355649925192llu,
   3470081245843620386llu, 9208682512641llu,
   586664357648667141llu, 113161891886927874llu,
   19144700892151826llu, 594756909391937601llu,
   562958551810122llu, 1152922124158043138llu,
};

const Bitboard magicBishopNumber[64] = { 
   2287018567549508llu, 20833559275112450llu,
   1157535125460746242llu, 2343279465658384466llu,
   182423557274534246llu, 91204498130747394llu,
   79165240377360llu, 18024053647361153llu,
   4756927389883318784llu, 2377974339296303121llu,
   1529092243969llu, 18016872700051744llu,
   1153204084472553600llu, 873698516757668385llu,
   1125942867148800llu, 26423192526848llu,
   289939073072784384llu, 10714208241454656llu,
   2306405960274414104llu, 54114131212519426llu,
   109212291534684230llu, 2306971108957557248llu,
   288239189760215296llu, 166643228131737628llu,
   3459894829004630016llu, 35193180131968llu,
   576478344523031040llu, 1134696069202368llu,
   298367873364066368llu, 2882596248975577091llu,
   432437927499531776llu, 72621163549300482llu,
   468396630660612192llu, 576812875222894592llu,
   3463272528741531664llu, 2612123106223325312llu,
   3242874314789487108llu, 3461016494560002066llu,
   4539883779662384llu, 5909074606504807464llu,
   4684333518172472324llu, 306280543752814656llu,
   2345387698176llu, 293016142258114592llu,
   5073215437209668llu, 79251277611040llu,
   602532708688384llu, 20310252071682068llu,
   4467607076866llu, 648573875995549696llu,
   288232580007339520llu, 1188986585695191042llu,
   9306611390751752llu, 5498105049088llu,
   4625205909830770716llu, 1152994261659095304llu,
   126118725618245632llu, 2305851875981395204llu,
   351843796394505llu, 40532672078233696llu,
   9009398294791225llu, 4611688257515749392llu,
   1152930369572702464llu, 2306989456245202960llu,
};
/* *INDENT-ON* */

bool testSquare(const Bitboard bitboard, const Square square)
{
   return (bool) (((bitboard) & minValue[(square)]) != EMPTY_BITBOARD);
}

/**
 * Get the king moves for the specified square.
 */
Bitboard getKingMoves(const Square square)
{
   return generalMoves[KING][square];
}

/**
 * Get castling moves depending from the castling rights and
 * the current position.
 */
Bitboard getCastlingMoves(const Color color, const BYTE castlingRights,
                          const Bitboard obstacles)
{
   if (color == WHITE)
   {
      return castlings[WHITE][castlingRights][obstacles & 0xFF];
   }
   else
   {
      return castlings[BLACK][castlingRights][obstacles >> 56];
   }
}

/**
 * Get the queen moves for the specified square.
 */
/*
 Bitboard getQueenMoves(const Square square, const BYTE * obstacles)
{
   const ObstacleSquareInfo *_obsi = &obsi[square];

   return _obsi->hLane[obstacles[_obsi->hLaneNumber]] |
      _obsi->vLane[obstacles[_obsi->vLaneNumber]] |
      _obsi->uLane[obstacles[_obsi->uLaneNumber]] |
      _obsi->dLane[obstacles[_obsi->dLaneNumber]];
}
*/

int getWidth(const Bitboard set)
{
   if (set == EMPTY_BITBOARD)
   {
      return 0;
   }
   else
   {
      File leftMost = FILE_A, rightMost = FILE_H;

      while ((set & squaresOfFile[leftMost]) == EMPTY_BITBOARD)
      {
         leftMost++;
      }

      while ((set & squaresOfFile[rightMost]) == EMPTY_BITBOARD)
      {
         rightMost--;
      }

      return rightMost - leftMost;
   }
}

/**
 * Get the queen moves for the specified square.
 */
Bitboard getMagicQueenMoves(const Square square, const Bitboard obstacles)
{
   MagicSquareInfoRook *msir = &magicSquareInfoRook[square];
   MagicSquareInfoBishop *msib = &magicSquareInfoBishop[square];

   return
      msir->moves[((obstacles & msir->preMask) * msir->magicNumber) >> 52] |
      msib->moves[((obstacles & msib->preMask) * msib->magicNumber) >> 55];
}

/**
 * Get the rook moves for the specified square.
 */
Bitboard getRookMoves(const Square square, const BYTE * obstacles)
{
   const ObstacleSquareInfo *_obsi = &obsi[square];

   return _obsi->hLane[obstacles[_obsi->hLaneNumber]] |
      _obsi->vLane[obstacles[_obsi->vLaneNumber]];
}

/**
 * Get the rook moves for the specified square.
 */
Bitboard getMagicRookMoves(const Square square, const Bitboard obstacles)
{
   MagicSquareInfoRook *msir = &magicSquareInfoRook[square];

   return
      msir->moves[((obstacles & msir->preMask) * msir->magicNumber) >> 52];
}

/**
 * Get the bishop moves for the specified square.
 */
Bitboard getBishopMoves(const Square square, const BYTE * obstacles)
{
   const ObstacleSquareInfo *_obsi = &obsi[square];

   return _obsi->uLane[obstacles[_obsi->uLaneNumber]] |
      _obsi->dLane[obstacles[_obsi->dLaneNumber]];
}

/**
 * Get the bishop moves for the specified square.
 */
Bitboard getMagicBishopMoves(const Square square, const Bitboard obstacles)
{
   MagicSquareInfoBishop *msib = &magicSquareInfoBishop[square];

   return
      msib->moves[((obstacles & msib->preMask) * msib->magicNumber) >> 55];
}

/**
 * Get the knight moves for the specified square.
 */
Bitboard getKnightMoves(const Square square)
{
   return generalMoves[KNIGHT][square];
}

/**
 * Get the pawn captures for the specified square.
 */
Bitboard getPawnCaptures(const Piece piece, const Square square,
                         const Bitboard allPieces)
{
   return generalMoves[piece][square] & allPieces;
}

/**
 * Get the pawn advances for the specified square.
 */
Bitboard getPawnAdvances(const Color color, const Square square,
                         const Bitboard obstacles)
{
   Bitboard advances;

   if (color == WHITE)
   {
      advances = (minValue[square] << 8) & ~obstacles;

      if (rank(square) == RANK_2)
      {
         advances |= (advances << 8) & ~obstacles;
      }
   }
   else
   {
      advances = (minValue[square] >> 8) & ~obstacles;

      if (rank(square) == RANK_7)
      {
         advances |= (advances >> 8) & ~obstacles;
      }
   }

   return advances;
}

/**
 * Get the pawns interested in advancing to the specified square.
 */
Bitboard getInterestedPawns(const Color color, const Square square,
                            const Bitboard obstacles)
{
   Bitboard interestedPawns;

   if (color == WHITE)
   {
      interestedPawns = minValue[square] >> 8;

      if (rank(square) == RANK_4 &&
          (interestedPawns & obstacles) == EMPTY_BITBOARD)
      {
         interestedPawns |= minValue[square] >> 16;
      }
   }
   else
   {
      interestedPawns = minValue[square] << 8;

      if (rank(square) == RANK_5 &&
          (interestedPawns & obstacles) == EMPTY_BITBOARD)
      {
         interestedPawns |= minValue[square] << 16;
      }
   }

   return interestedPawns;
}

/**
 * Get the squares between the two specified squares.
 */
Bitboard getSquaresBetween(const Square square1, const Square square2)
{
   return squaresBetween[square1][square2];
}

/**
 * Get the squares behind 'target', as seen from 'viewpoint'.
 */
Bitboard getSquaresBehind(const Square target, const Square viewpoint)
{
   return squaresBehind[target][viewpoint];
}

/**
 * Shift all set squares one file to the left.
 */
Bitboard shiftLeft(const Bitboard bitboard)
{
   return (bitboard & nonA) >> 1;
}

/**
 * Shift all set squares one file to the right.
 */
Bitboard shiftRight(const Bitboard bitboard)
{
   return (bitboard & nonH) << 1;
}

/**
 * Get all squares lateral to the specified squares.
 */
Bitboard getLateralSquares(const Bitboard squares)
{
   return ((squares & nonA) >> 1) | ((squares & nonH) << 1);
}

/**
 * Get the squares of the specified file.
 */
Bitboard getSquaresOfFile(const File file)
{
   return squaresOfFile[file];
}

/**
 * Get the squares of the specified rank.
 */
Bitboard getSquaresOfRank(const Rank rank)
{
   return squaresOfRank[rank];
}

/**
 * Get the number of set squares in the specified bitboard.
 */
int getNumberOfSetSquares(const Bitboard bitboard)
{
   return __builtin_popcountll(bitboard);
}

/**
 * Get the rank overlay of the specified bitboard.
 */
int getRankOverlay(const Bitboard bitboard)
{
   return rankOverlay[(bitboard) & UHEX_FFFF] |
      rankOverlay[(bitboard >> 16) & UHEX_FFFF] |
      rankOverlay[(bitboard >> 32) & UHEX_FFFF] |
      rankOverlay[(bitboard >> 48) & UHEX_FFFF];
}

/**
 * Get the moves of the the specified piece.
 */
Bitboard getMoves(Square square, Piece piece, Bitboard allPieces)
{
   switch (pieceType(piece))
   {
   case PAWN:
      return getPawnCaptures(piece, square, allPieces) |
         getPawnAdvances(pieceColor(piece), square, allPieces);
   case KING:
      return getKingMoves(square);
   case KNIGHT:
      return getKnightMoves(square);
   case ROOK:
      return getMagicRookMoves(square, allPieces);
   case BISHOP:
      return getMagicBishopMoves(square, allPieces);
   default:
      return getMagicQueenMoves(square, allPieces);
   }
}

/**
 * Get the capture moves of the the specified piece.
 */
Bitboard getCaptureMoves(Square square, Piece piece, Bitboard allPieces)
{
   switch (pieceType(piece))
   {
   case PAWN:
      return getPawnCaptures(piece, square, allPieces);
   case KING:
      return getKingMoves(square);
   case KNIGHT:
      return getKnightMoves(square);
   case ROOK:
      return getMagicRookMoves(square, allPieces);
   case BISHOP:
      return getMagicBishopMoves(square, allPieces);
   default:
      return getMagicQueenMoves(square, allPieces);
   }
}

/**
 * Set a square in the specified vector of obstacles.
 */
void setObstacleSquare(Square square, BYTE obstacles[NUM_LANES])
{
   SquareLaneInfo *sqi = &squareLaneInfo[square];

   obstacles[sqi->hLane] |= sqi->hLaneSetMask;
   obstacles[sqi->vLane] |= sqi->vLaneSetMask;
   obstacles[sqi->uLane] |= sqi->uLaneSetMask;
   obstacles[sqi->dLane] |= sqi->dLaneSetMask;
}

/**
 * Clear a square in the specified vector of obstacles.
 */
void clearObstacleSquare(Square square, BYTE obstacles[NUM_LANES])
{
   SquareLaneInfo *sqi = &squareLaneInfo[square];

   obstacles[sqi->hLane] &= sqi->hLaneClearMask;
   obstacles[sqi->vLane] &= sqi->vLaneClearMask;
   obstacles[sqi->uLane] &= sqi->uLaneClearMask;
   obstacles[sqi->dLane] &= sqi->dLaneClearMask;
}

/**
 * Calculate all obstacles according to the specified bitboard.
 */
void calculateObstacles(Bitboard board, BYTE obstacles[NUM_LANES])
{
   Square square;

   memset(obstacles, 0x00, NUM_LANES);

   ITERATE(square)
   {
      if (board & minValue[square])
      {
         setObstacleSquare(square, obstacles);
      }
   }
}

/**
 * Get the number of a set square and clear the corresponding bit.
 *
 * @return NO_SQUARE if no square was set.
 */
Square getLastSquare(Bitboard * vector);

/**
 * Extend all set bits by one square into all directions.
 */
void floodBoard(Bitboard * board);

/**
 * Get the targets of all pawns specified by 'whitePawns'.
 */
Bitboard getWhitePawnTargets(const Bitboard whitePawns)
{
   return ((whitePawns & nonA) << 7) | ((whitePawns & nonH) << 9);
}

/**
 * Get the targets of all pawns specified by 'blackPawns'.
 */
Bitboard getBlackPawnTargets(const Bitboard blackPawns)
{
   return ((blackPawns & nonA) >> 9) | ((blackPawns & nonH) >> 7);
}

/**
 * Iteration shortcuts
 */
#define ITERATE_BITBOARD(b,sq) while ( ( sq = getLastSquare(b) ) >= 0 )

void floodBoard(Bitboard * board)
{
   const Bitboard toLeft = *board & nonA, toRight = *board & nonH;

   *board = (toLeft >> 1) | (toLeft << 7) | (*board >> 8) | (toLeft >> 9) |
      (toRight << 1) | (toRight >> 7) | (*board << 8) | (toRight << 9);
}

Square getLastSquare(Bitboard * vector)
{
   if (*vector == EMPTY_BITBOARD)
   {
      return NO_SQUARE;
   }
   else
   {
      const int index = 63 - __builtin_clzll(*vector);

      *vector &= maxValue[index];

      return index;
   }
}

Square getFirstSquare(Bitboard * vector)
{
   if (*vector == EMPTY_BITBOARD)
   {
      return NO_SQUARE;
   }
   else
   {
      const int index = __builtin_ctzll(*vector);

      *vector &= maxValue[index];

      return index;
   }
}

int getSetSquares(const Bitboard board, UINT8 squares[_64_])
{
   int index;
   UINT8 *currentSquare = &squares[0];

   if ((index = (int) (0xFFFF & board)) != 0)
   {
      const SetSquaresInfo *info = &setSquares[0][index];

      memcpy(currentSquare, info->setSquares, info->numSetSquares);
      currentSquare += info->numSetSquares;
   }

   if ((index = (int) (0xFFFF & (board >> 16))) != 0)
   {
      const SetSquaresInfo *info = &setSquares[1][index];

      memcpy(currentSquare, info->setSquares, info->numSetSquares);
      currentSquare += info->numSetSquares;
   }

   if ((index = (int) (0xFFFF & (board >> 32))) != 0)
   {
      const SetSquaresInfo *info = &setSquares[2][index];

      memcpy(currentSquare, info->setSquares, info->numSetSquares);
      currentSquare += info->numSetSquares;
   }

   if ((index = (int) (board >> 48)) != 0)
   {
      const SetSquaresInfo *info = &setSquares[3][index];

      memcpy(currentSquare, info->setSquares, info->numSetSquares);
      currentSquare += info->numSetSquares;
   }

   return (int) (currentSquare - &squares[0]);
}

Bitboard getMultipleSquaresBetween(const Square origin, Bitboard targets)
{
   Bitboard squares = targets;
   Square targetSquare;
   Bitboard *sqb = &(squaresBetween[origin][0]);

   ITERATE_BITBOARD(&targets, targetSquare)
   {
      squares |= sqb[targetSquare];
   }

   return squares;
}

int getFloodValue(const Square origin, const Bitboard targets,
                  const Bitboard permittedSquares)
{
   Bitboard floodedSquares = getKingMoves(origin) & permittedSquares;
   Bitboard oldFloodedSquares = minValue[origin];
   int numSteps = 1;

   while ((targets & floodedSquares) == EMPTY_BITBOARD &&
          floodedSquares != oldFloodedSquares)
   {
      oldFloodedSquares = floodedSquares;
      floodBoard(&floodedSquares);
      floodedSquares |= oldFloodedSquares;
      floodedSquares &= permittedSquares;
      numSteps++;
   }

   return numSteps;
}

static void initializeKingMoves()
{
   int offset[] = { 1, -1, 7, -7, 8, -8, 9, -9 }, direction;
   Square square;

   ITERATE(square)
   {
      for (direction = 0; direction < 8; direction++)
      {
         Square target = (Square) (square + offset[direction]);

         if (squareIsValid(target) && distance(square, target) == 1)
         {
            generalMoves[KING][square] |= minValue[target];
         }
      }
   }
}

static void initializeRookMoves()
{
   int offset[] = { 1, -1, 8, -8 }, direction, moveLength;
   Square square;

   ITERATE(square)
   {
      for (direction = 0; direction < 4; direction++)
      {
         for (moveLength = 1; moveLength <= 7; moveLength++)
         {
            Square target =
               (Square) (square + offset[direction] * moveLength);
            bool sameFile = (bool) (file(square) == file(target));
            bool sameRow = (bool) (rank(square) == rank(target));

            if (squareIsValid(target) && (sameFile || sameRow))
            {
               generalMoves[ROOK][square] |= minValue[target];
            }
            else
            {
               break;
            }
         }
      }
   }
}

static void initializeBishopMoves()
{
   int offset[] = { 7, -7, 9, -9 }, direction, moveLength;
   Square square;

   ITERATE(square)
   {
      for (direction = 0; direction < 4; direction++)
      {
         for (moveLength = 1; moveLength <= 7; moveLength++)
         {
            Square target =
               (Square) (square + offset[direction] * moveLength);
            int hDistance = horizontalDistance(square, target);
            int vDistance = verticalDistance(square, target);

            if (squareIsValid(target) && hDistance == vDistance)
            {
               generalMoves[BISHOP][square] |= minValue[target];
            }
            else
            {
               break;
            }
         }
      }
   }
}

static void initializeQueenMoves()
{
   Square square;

   ITERATE(square)
   {
      generalMoves[QUEEN][square] =
         generalMoves[ROOK][square] | generalMoves[BISHOP][square];
   }
}

static void initializeKnightMoves()
{
   int offset[] = { 6, -6, 10, -10, 15, -15, 17, -17 }, direction;
   Square square;

   ITERATE(square)
   {
      for (direction = 0; direction < 8; direction++)
      {
         Square target = (Square) (square + offset[direction]);

         if (squareIsValid(target) && distance(square, target) <= 2)
         {
            generalMoves[KNIGHT][square] |= minValue[target];
         }
      }
   }
}

static void initializePawnMoves()
{
   Square square;

   ITERATE(square)
   {
      if (file(square) != FILE_A)
      {
         if (square <= H8 - 7)
         {
            generalMoves[WHITE_PAWN][square] |= minValue[square + 7];
         }

         if (square >= A1 + 9)
         {
            generalMoves[BLACK_PAWN][square] |= minValue[square - 9];
         }
      }

      if (file(square) != FILE_H)
      {
         if (square <= H8 - 9)
         {
            generalMoves[WHITE_PAWN][square] |= minValue[square + 9];
         }

         if (square >= A1 + 7)
         {
            generalMoves[BLACK_PAWN][square] |= minValue[square - 7];
         }
      }
   }
}

static Bitboard calculateSquaresBehind(Square square, int offset)
{
   Bitboard squares = EMPTY_BITBOARD;

   for (square = (Square) (square + offset);
        squareIsValid(square) &&
        distance(square, (Square) (square - offset)) == 1;
        square = (Square) (square + offset))
   {
      Bitboard mv = minValue[square];

      squares |= mv;
   }

   return squares;
}

static void initializeSquaresBehind()
{
   Square sq1, sq2;

   ITERATE(sq1)
   {
      Bitboard squares = generalMoves[QUEEN][sq1];

      ITERATE_BITBOARD(&squares, sq2)
      {
         int offset = offset(sq1, sq2);

         squaresBehind[sq2][sq1] = calculateSquaresBehind(sq2, offset);
      }
   }
}

static Bitboard calculateSquaresBetween(Square sq1, Square sq2)
{
   Bitboard squares = EMPTY_BITBOARD;
   int _offset = offset(sq1, sq2), square;

   for (square = sq1 + _offset; square != sq2; square += _offset)
   {
      squares |= minValue[square];
   }

   return squares;
}

static void initializeSquaresBetween()
{
   Square sq1, sq2;

   ITERATE(sq1)
   {
      Bitboard squares = generalMoves[QUEEN][sq1];

      ITERATE_BITBOARD(&squares, sq2)
      {
         squaresBetween[sq1][sq2] = calculateSquaresBetween(sq1, sq2);
      }
   }
}

static void initializeSquareLaneInfo()
{
   Square square;

   ITERATE(square)
   {
      SquareLaneInfo *sqi = &squareLaneInfo[square];

      sqi->hLane = hLaneNumber[square] = HLANE(square);
      sqi->hLaneSetMask = (BYTE) minValue[HLANEBIT(square)];
      sqi->hLaneClearMask = (BYTE) ~ sqi->hLaneSetMask;

      sqi->vLane = vLaneNumber[square] = VLANE(square) + 8;
      sqi->vLaneSetMask = (BYTE) minValue[VLANEBIT(square)];
      sqi->vLaneClearMask = (BYTE) ~ sqi->vLaneSetMask;

      sqi->uLane = uLaneNumber[square] = ULANE(square) + 16;
      sqi->uLaneSetMask = (BYTE) minValue[ULANEBIT(square)];
      sqi->uLaneClearMask = (BYTE) ~ sqi->uLaneSetMask;

      sqi->dLane = dLaneNumber[square] = DLANE(square) + 31;
      sqi->dLaneSetMask = (BYTE) minValue[DLANEBIT(square)];
      sqi->dLaneClearMask = (BYTE) ~ sqi->dLaneSetMask;
   }
}

static Bitboard getHlaneVector(int lane, BYTE mask)
{
   int i;
   Bitboard v = EMPTY_BITBOARD;

   for (i = 0; i < 8; i++)
   {
      if (mask & minValue[i])
      {
         v |= minValue[7 - i];
      }
   }

   v <<= 8 * lane;

   return v;
}

static Bitboard getVlaneVector(int lane, BYTE mask)
{
   int i;
   Bitboard v = EMPTY_BITBOARD;

   for (i = 0; i < 8; i++)
   {
      if (mask & minValue[i])
      {
         v |= minValue[8 * (7 - i)];
      }
   }

   v <<= lane;

   return v;
}

static Bitboard getUlaneVector(int lane, BYTE mask)
{
   int i;
   Bitboard v = EMPTY_BITBOARD;

   for (i = 0; i < 8; i++)
   {
      v <<= 8;

      if (mask & minValue[i])
      {
         v |= minValue[7 - i];
      }
   }

   if (lane < 7)
   {
      for (i = lane; i < 7; i++)
      {
         v = (v & nonA) >> 1;
      }
   }
   else
   {
      for (i = lane; i > 7; i--)
      {
         v = (v & nonH) << 1;
      }
   }

   return v;
}

static Bitboard getDlaneVector(int lane, BYTE mask)
{
   int i;
   Bitboard v = EMPTY_BITBOARD;

   for (i = 0; i < 8; i++)
   {
      v <<= 8;

      if (mask & minValue[7 - i])
      {
         v |= minValue[i];
      }
   }

   if (lane < 7)
   {
      for (i = lane; i < 7; i++)
      {
         v = (v & nonA) >> 1;
      }
   }
   else
   {
      for (i = lane; i > 7; i--)
      {
         v = (v & nonH) << 1;
      }
   }

   return v;
}

static Bitboard getLaneMask(Square square, int offset,
                            Bitboard legalSquares, Bitboard obstacles)
{
   Bitboard laneMask = EMPTY_BITBOARD, mv = EMPTY_BITBOARD;

   while ((square = (Square) (square + offset)) >= A1 &&
          square <= H8 && ((mv = minValue[square]) & legalSquares))
   {
      laneMask |= mv;

      if (mv & obstacles)
      {
         break;
      }
   }

   return laneMask;
}

static Bitboard getHLaneMask(Square square, BYTE mask)
{
   int lane = HLANE(square);
   Bitboard legalSquares = getHlaneVector(lane, 255);
   Bitboard obstacles = getHlaneVector(lane, mask);

   Bitboard laneMask = getLaneMask(square, 1, legalSquares, obstacles);

   laneMask |= getLaneMask(square, -1, legalSquares, obstacles);

   return laneMask;
}

static Bitboard getVLaneMask(Square square, BYTE mask)
{
   int lane = VLANE(square);
   Bitboard legalSquares = getVlaneVector(lane, 255);
   Bitboard obstacles = getVlaneVector(lane, mask);

   Bitboard laneMask = getLaneMask(square, 8, legalSquares, obstacles);

   laneMask |= getLaneMask(square, -8, legalSquares, obstacles);

   return laneMask;
}

static Bitboard getULaneMask(Square square, BYTE mask)
{
   int lane = ULANE(square);
   Bitboard legalSquares = getUlaneVector(lane, 255);
   Bitboard obstacles = getUlaneVector(lane, mask);

   Bitboard laneMask = getLaneMask(square, 9, legalSquares, obstacles);

   laneMask |= getLaneMask(square, -9, legalSquares, obstacles);

   return laneMask;
}

static Bitboard getDLaneMask(Square square, BYTE mask)
{
   int lane = DLANE(square);
   Bitboard legalSquares = getDlaneVector(lane, 255);
   Bitboard obstacles = getDlaneVector(lane, mask);

   Bitboard laneMask = getLaneMask(square, 7, legalSquares, obstacles);

   laneMask |= getLaneMask(square, -7, legalSquares, obstacles);

   return laneMask;
}

static Bitboard getWhitePawnMoves(Square square, BYTE laneMask)
{
   int lane = VLANE(square);
   Bitboard moves = EMPTY_BITBOARD, board = getVlaneVector(lane, laneMask);

   if (square >= A2 && square <= H7 && (board & minValue[square + 8]) == 0)
   {
      moves |= minValue[square + 8];

      if (square <= H2 && (board & minValue[square + 16]) == 0)
      {
         moves |= minValue[square + 16];
      }
   }

   return moves;
}

static Bitboard getInterestedWhitePawns(Square square, BYTE laneMask)
{
   int lane = VLANE(square);
   Bitboard pawns = EMPTY_BITBOARD, board = getVlaneVector(lane, laneMask);

   if (square >= A3 && square <= H8 && (board & minValue[square]) == 0)
   {
      pawns |= minValue[square - 8];

      if (rank(square) == RANK_4 && (board & minValue[square - 8]) == 0)
      {
         pawns |= minValue[square - 16];
      }
   }

   return pawns;
}

static Bitboard getBlackPawnMoves(Square square, BYTE laneMask)
{
   int lane = VLANE(square);
   Bitboard moves = EMPTY_BITBOARD, board = getVlaneVector(lane, laneMask);

   if (square >= A2 && square <= H7 && (board & minValue[square - 8]) == 0)
   {
      moves |= minValue[square - 8];

      if (square >= A7 && (board & minValue[square - 16]) == 0)
      {
         moves |= minValue[square - 16];
      }
   }

   return moves;
}

static Bitboard getInterestedBlackPawns(Square square, BYTE laneMask)
{
   int lane = VLANE(square);
   Bitboard pawns = EMPTY_BITBOARD, board = getVlaneVector(lane, laneMask);

   if (square >= A1 && square <= H6 && (board & minValue[square]) == 0)
   {
      pawns |= minValue[square + 8];

      if (rank(square) == RANK_5 && (board & minValue[square + 8]) == 0)
      {
         pawns |= minValue[square + 16];
      }
   }

   return pawns;
}

static void initializeLaneMasks()
{
   Square square;
   int i;

   ITERATE(square)
   {
      for (i = 0; i <= 255; i++)
      {
         BYTE mask = (BYTE) i;

         hlane[square][i] = getHLaneMask(square, mask);
         vlane[square][i] = getVLaneMask(square, mask);
         ulane[square][i] = getULaneMask(square, mask);
         dlane[square][i] = getDLaneMask(square, mask);
         pawnMoves[WHITE][square][i] = getWhitePawnMoves(square, mask);
         pawnMoves[BLACK][square][i] = getBlackPawnMoves(square, mask);
         interestedPawns[WHITE][square][i] =
            getInterestedWhitePawns(square, mask);
         interestedPawns[BLACK][square][i] =
            getInterestedBlackPawns(square, mask);
      }
   }

   ITERATE(square)
   {
      int i;

      obsi[square].hLaneNumber = hLaneNumber[square];
      obsi[square].vLaneNumber = vLaneNumber[square];
      obsi[square].uLaneNumber = uLaneNumber[square];
      obsi[square].dLaneNumber = dLaneNumber[square];

      for (i = 0; i < 256; i++)
      {
         obsi[square].hLane[i] = hlane[square][i];
         obsi[square].vLane[i] = vlane[square][i];
         obsi[square].uLane[i] = ulane[square][i];
         obsi[square].dLane[i] = dlane[square][i];
      }
   }
}

static Bitboard getCastlings(const Bitboard obstacles)
{
   Bitboard castlings = EMPTY_BITBOARD;

   if ((obstacles & minValue[F1]) == EMPTY_BITBOARD &&
       (obstacles & minValue[G1]) == EMPTY_BITBOARD)
   {
      castlings |= minValue[G1];
   }

   if ((obstacles & minValue[B1]) == EMPTY_BITBOARD &&
       (obstacles & minValue[C1]) == EMPTY_BITBOARD &&
       (obstacles & minValue[D1]) == EMPTY_BITBOARD)
   {
      castlings |= minValue[C1];
   }

   return castlings;
}

static void initializeCastlings()
{
   int i, c;

   for (i = 0; i <= 255; i++)
   {
      BYTE mask = (BYTE) i;
      Bitboard moves = getCastlings((Bitboard) mask);

      for (c = 0; c <= 15; c++)
      {
         castlings[WHITE][c][mask] = castlings[BLACK][c][mask] = 0;

         if (moves & minValue[G1])
         {
            if (c & WHITE_00)
            {
               castlings[WHITE][c][mask] |= minValue[G1];
            }

            if (c & BLACK_00)
            {
               castlings[BLACK][c][mask] |= minValue[G8];
            }
         }

         if (moves & minValue[C1])
         {
            if (c & WHITE_000)
            {
               castlings[WHITE][c][mask] |= minValue[C1];
            }

            if (c & BLACK_000)
            {
               castlings[BLACK][c][mask] |= minValue[C8];
            }
         }
      }
   }
}

static INT8 _getNumberOfSetBits(INT32 v)
{
   int count = 0, i;

   for (i = 0; i < 16; i++)
   {
      count += (v >> i) & 1;
   }

   return (INT8) count;
}

static UINT8 _getRankOverlay(INT32 v)
{
   return (UINT8) ((v & 0xff) | ((v >> 8) & 0xff));
}

unsigned int getMinimumDistance(const Bitboard targets, const Square square)
{
   unsigned int distance;

   for (distance = 1; distance <= 7; distance++)
   {
      if ((squaresInDistance[distance][square] & targets) != EMPTY_BITBOARD)
      {
         return distance;
      }
   }

   return 0;
}

unsigned int getMaximumDistance(const Bitboard targets, const Square square)
{
   unsigned int distance;

   for (distance = 7; distance > 0; distance--)
   {
      if ((squaresInDistance[distance][square] & targets) != EMPTY_BITBOARD)
      {
         return distance;
      }
   }

   return 0;
}

static Bitboard getPremaskExclusions(const Square square)
{
   if (testSquare(border, square))
   {
      Bitboard exclusions = EMPTY_BITBOARD;

      if (testSquare(squaresOfFile[FILE_A], square) == FALSE)
      {
         exclusions |= squaresOfFile[FILE_A];
      }

      if (testSquare(squaresOfFile[FILE_H], square) == FALSE)
      {
         exclusions |= squaresOfFile[FILE_H];
      }

      if (testSquare(squaresOfRank[RANK_1], square) == FALSE)
      {
         exclusions |= squaresOfRank[RANK_1];
      }

      if (testSquare(squaresOfRank[RANK_8], square) == FALSE)
      {
         exclusions |= squaresOfRank[RANK_8];
      }

      return exclusions;
   }
   else
   {
      return border;
   }
}

static void initializePremasks()
{
   Square square;

   ITERATE(square)
   {
      preMaskRook[square] = generalMoves[WHITE_ROOK][square] &
         ~getPremaskExclusions(square);
      preMaskBishop[square] = generalMoves[WHITE_BISHOP][square] &
         ~getPremaskExclusions(square);
   }
}

static void initializeMagicRookMoves(const Square square)
{
   Square squareOfIndex[12];
   Bitboard allMoves = preMaskRook[square];
   const Bitboard premask = preMaskRook[square];
   int i, pieceSetup;
   Square sq1;
   BYTE obstacles[NUM_LANES];
   const Bitboard magicNumber = magicRookNumber[square];

   for (i = 0; i < IMAX_ROOK; i++)
   {
      magicRookMoves[square][i] = EMPTY_BITBOARD;
   }

   for (i = 0; i < 12; i++)
   {
      squareOfIndex[i] = NO_SQUARE;
   }

   i = 0;

   ITERATE_BITBOARD(&allMoves, sq1)
   {
      assert(i < 12);

      squareOfIndex[i++] = sq1;
   }

   for (pieceSetup = 0; pieceSetup < 4096; pieceSetup++)
   {
      Bitboard currentSetup = EMPTY_BITBOARD, effectiveMoves;
      int j = 1;
      Bitboard magicIndex;

      for (i = 0; i < 12 && squareOfIndex[i] != NO_SQUARE; i++)
      {
         if ((pieceSetup & j) != 0)
         {
            setSquare(currentSetup, squareOfIndex[i]);
         }

         j *= 2;
      }

      calculateObstacles(currentSetup, obstacles);
      effectiveMoves = getRookMoves(square, obstacles);
      magicIndex = ((currentSetup & premask) * magicNumber) >> 52;
      magicRookMoves[square][magicIndex] = effectiveMoves;
   }
}

static void initializeMagicBishopMoves(const Square square)
{
   Square squareOfIndex[12];
   Bitboard allMoves = preMaskBishop[square];
   const Bitboard premask = preMaskBishop[square];
   int i, pieceSetup;
   Square sq1;
   BYTE obstacles[NUM_LANES];
   const Bitboard magicNumber = magicBishopNumber[square];

   for (i = 0; i < IMAX_BISHOP; i++)
   {
      magicBishopMoves[square][i] = EMPTY_BITBOARD;
   }

   for (i = 0; i < 12; i++)
   {
      squareOfIndex[i] = NO_SQUARE;
   }

   i = 0;

   ITERATE_BITBOARD(&allMoves, sq1)
   {
      assert(i < 12);

      squareOfIndex[i++] = sq1;
   }

   for (pieceSetup = 0; pieceSetup < 4096; pieceSetup++)
   {
      Bitboard currentSetup = EMPTY_BITBOARD, effectiveMoves;
      int j = 1;
      Bitboard magicIndex;

      for (i = 0; i < 12 && squareOfIndex[i] != NO_SQUARE; i++)
      {
         if ((pieceSetup & j) != 0)
         {
            setSquare(currentSetup, squareOfIndex[i]);
         }

         j *= 2;
      }

      calculateObstacles(currentSetup, obstacles);
      effectiveMoves = getBishopMoves(square, obstacles);
      magicIndex = ((currentSetup & premask) * magicNumber) >> 55;
      magicBishopMoves[square][magicIndex] = effectiveMoves;
   }
}

/* #define GENERATE_MAGIC_NUMBERS */
#ifdef GENERATE_MAGIC_NUMBERS

#define IMAX 4096
#define BMAX 12
Bitboard collisions[IMAX];

static bool testMagicNumber(const Bitboard magicNumber, const Square square,
                            const bool rookMoves)
{
   Square squareOfIndex[BMAX];
   const Bitboard premask =
      (rookMoves ? preMaskRook[square] : preMaskBishop[square]);
   Bitboard allMoves = premask;
   int i, pieceSetup;
   Square sq1;
   BYTE obstacles[NUM_LANES];

   for (i = 0; i < IMAX; i++)
   {
      collisions[i] = EMPTY_BITBOARD;
   }

   for (i = 0; i < BMAX; i++)
   {
      squareOfIndex[i] = NO_SQUARE;
   }

   i = 0;

   ITERATE_BITBOARD(&allMoves, sq1)
   {
      assert(i < BMAX);

      squareOfIndex[i++] = sq1;
   }

   for (pieceSetup = 0; pieceSetup < IMAX; pieceSetup++)
   {
      Bitboard currentSetup = EMPTY_BITBOARD, effectiveMoves, calcBase;
      int j = 1;
      Bitboard magicIndex;
      const int shift = (rookMoves ? 64 - 12 : 64 - 9);

      for (i = 0; i < BMAX && squareOfIndex[i] != NO_SQUARE; i++)
      {
         if ((pieceSetup & j) != 0)
         {
            setSquare(currentSetup, squareOfIndex[i]);
         }

         j *= 2;
      }

      calculateObstacles(currentSetup, obstacles);
      effectiveMoves =
         (rookMoves ? getRookMoves(square, obstacles) :
          getBishopMoves(square, obstacles));
      calcBase = currentSetup & premask;
      magicIndex = (calcBase * magicNumber) >> shift;

      if (collisions[magicIndex] == EMPTY_BITBOARD)
      {
         collisions[magicIndex] = effectiveMoves;
      }
      else
      {
         if (effectiveMoves != collisions[magicIndex])
         {
            return FALSE;       /* severe collision -> magic number not suitable */
         }
      }
   }

   return TRUE;                 /* the given magic number is valid */
}

static UINT64 get64bitRandom()
{
   const UINT64 limit = (RAND_MAX * 13) / 100;
   UINT64 rn = 0, runningOne = 1;
   int i;

   for (i = 0; i < 64; i++)
   {
      if (rand() <= limit)
      {
         rn |= runningOne;
      }

      runningOne *= 2;
   }

   return rn;
}

static void calculateMagicNumbers()
{
   Square square;

   printf("{");

   ITERATE(square)
   {
      Bitboard magicNumber = get64bitRandom();

      while (testMagicNumber(magicNumber, square, TRUE) == FALSE)
      {
         magicNumber = get64bitRandom();
      }

      printf("%llu, ", magicNumber);

      if ((square % 2) == 1)
      {
         printf("\n");
      }
   }

   printf("};\n{");

   ITERATE(square)
   {
      Bitboard magicNumber = get64bitRandom();

      while (testMagicNumber(magicNumber, square, FALSE) == FALSE)
      {
         magicNumber = get64bitRandom();
      }

      printf("%llu, ", magicNumber);

      if ((square % 2) == 1)
      {
         printf("\n");
      }
   }

   printf("};\n");
   getKeyStroke();
}

#endif /* GENREATE_MAGIC_NUMBERS */

Bitboard getFlippedBitboard(Bitboard original)
{
   Bitboard flipped = EMPTY_BITBOARD;
   Square square;

   ITERATE_BITBOARD(&original, square)
   {
      setSquare(flipped, getFlippedSquare(square));
   }

   return flipped;
}

int initializeModuleBitboard()
{
   INT32 i;
   UINT32 j;
   INT8 k, indexLow, indexHigh;
   UINT64 min = 1;
   Square square;

   if (isInitialized)
   {
      return 0;
   }
   else
   {
      isInitialized = TRUE;
   }

   ITERATE(i)
   {
      minValue[i] = min;
      maxValue[i] = ~min;

      min *= 2;
   }

   lowestBit[0] = highestBit[0] = NO_SQUARE;
   numSetBits[0] = rankOverlay[0] = 0;

   for (i = 1; i <= 0xffffL; i++)
   {
      numSetBits[i] = _getNumberOfSetBits(i);
      rankOverlay[i] = _getRankOverlay(i);

      j = i;
      indexLow = indexHigh = NO_SQUARE;

      for (k = 0x00; k <= 0x0f; k++)
      {
         if ((j & 1) == 1)
         {
            if (indexLow == NO_SQUARE)
            {
               indexLow = k;
            }

            indexHigh = k;
         }

         j >>= 1;
      }

      lowestBit[i] = indexLow;
      highestBit[i] = indexHigh;
   }

   ITERATE(square)
   {
      int distance;

      if (squareColor(square) == DARK)
      {
         setSquare(darkSquares, square);
         clearSquare(lightSquares, square);
      }
      else
      {
         clearSquare(darkSquares, square);
         setSquare(lightSquares, square);
      }

      for (distance = 0; distance <= 7; distance++)
      {
         Square square2;

         squaresInDistance[distance][square] = EMPTY_BITBOARD;

         ITERATE(square2)
         {
            if (distance(square, square2) <= distance)
            {
               setSquare(squaresInDistance[distance][square], square2);
            }
         }
      }

      for (distance = 0; distance <= 14; distance++)
      {
         Square square2;

         squaresInTaxiDistance[distance][square] = EMPTY_BITBOARD;

         ITERATE(square2)
         {
            if (taxiDistance(square, square2) <= distance)
            {
               setSquare(squaresInTaxiDistance[distance][square], square2);
            }
         }
      }
   }

   for (i = FILE_A; i <= FILE_H; i++)
   {
      int j;

      for (j = FILE_A; j <= FILE_H; j++)
      {
         int k;

         squaresOfFileRange[i][j] = squaresOfRankRange[i][j] = EMPTY_BITBOARD;

         for (k = FILE_A; k <= FILE_H; k++)
         {
            if ((k >= i && k <= j) || (k >= j && k <= i))
            {
               squaresOfFileRange[i][j] |= squaresOfFile[k];
               squaresOfRankRange[i][j] |= squaresOfRank[k];
            }
         }
      }
   }

   nonA = ~squaresOfFile[FILE_A], nonH = ~squaresOfFile[FILE_H];
   center = minValue[D4] | minValue[D5] | minValue[E4] | minValue[E5];
   extendedCenter =
      (squaresOfFile[FILE_C] | squaresOfFile[FILE_D] |
       squaresOfFile[FILE_E] | squaresOfFile[FILE_F]) &
      (squaresOfRank[RANK_3] | squaresOfRank[RANK_4] |
       squaresOfRank[RANK_5] | squaresOfRank[RANK_6]);
   border = squaresOfFile[FILE_A] | squaresOfFile[FILE_H] |
      squaresOfRank[RANK_1] | squaresOfRank[RANK_8];
   queenSide =
      squaresOfFile[FILE_A] | squaresOfFile[FILE_B] | squaresOfFile[FILE_C];
   kingSide =
      squaresOfFile[FILE_F] | squaresOfFile[FILE_G] | squaresOfFile[FILE_H];
   centerFiles = squaresOfFile[FILE_D] | squaresOfFile[FILE_E];
   promotionCandidates[WHITE] = squaresOfRank[RANK_7];
   promotionCandidates[BLACK] = squaresOfRank[RANK_2];

   initializeKingMoves();
   initializeRookMoves();
   initializeBishopMoves();
   initializeQueenMoves();
   initializeKnightMoves();
   initializePawnMoves();
   initializeSquaresBehind();
   initializeSquaresBetween();
   initializeSquareLaneInfo();
   initializeLaneMasks();
   initializeCastlings();

   ITERATE(square)
   {
      const Bitboard corona = generalMoves[KING][square];
      Square square2;

      orthoKingAttackers[square] = diaKingAttackers[square] =
         knightKingAttackers[square] = pawnKingAttackers[WHITE][square] =
         pawnKingAttackers[BLACK][square] = EMPTY_BITBOARD;
      squaresAbove[WHITE][square] = squaresAbove[BLACK][square] =
         squaresBelow[WHITE][square] = squaresBelow[BLACK][square] =
         squaresLeftOf[square] = squaresRightOf[square] = EMPTY_BITBOARD;

      ITERATE(square2)
      {
         const Bitboard orthoSquares = generalMoves[ROOK][square2];
         const Bitboard diaSquares = generalMoves[BISHOP][square2];
         const Bitboard knightSquares = generalMoves[KNIGHT][square2];
         const Bitboard whitePawnSquares = generalMoves[WHITE_PAWN][square2];
         const Bitboard blackPawnSquares = generalMoves[BLACK_PAWN][square2];

         if (square != square2 && (corona & orthoSquares) != EMPTY_BITBOARD &&
             taxiDistance(square, square2) > 1)
         {
            setSquare(orthoKingAttackers[square], square2);
         }

         if (square != square2 && (corona & diaSquares) != EMPTY_BITBOARD &&
             (distance(square, square2) > 1 ||
              taxiDistance(square, square2) != 2))
         {
            setSquare(diaKingAttackers[square], square2);
         }

         if (square != square2 &&
             (corona & knightSquares) != EMPTY_BITBOARD &&
             testSquare(knightSquares, square) == EMPTY_BITBOARD)
         {
            setSquare(knightKingAttackers[square], square2);
         }

         if (square != square2 &&
             (corona & whitePawnSquares) != EMPTY_BITBOARD &&
             testSquare(whitePawnSquares, square) == EMPTY_BITBOARD)
         {
            setSquare(pawnKingAttackers[WHITE][square], square2);
         }

         if (square != square2 &&
             (corona & blackPawnSquares) != EMPTY_BITBOARD &&
             testSquare(blackPawnSquares, square) == EMPTY_BITBOARD)
         {
            setSquare(pawnKingAttackers[BLACK][square], square2);
         }

         if (rank(square2) < rank(square))
         {
            setSquare(squaresBelow[WHITE][square], square2);
            setSquare(squaresAbove[BLACK][square], square2);
         }

         if (rank(square2) > rank(square))
         {
            setSquare(squaresAbove[WHITE][square], square2);
            setSquare(squaresBelow[BLACK][square], square2);
         }

         if (file(square2) < file(square))
         {
            setSquare(squaresLeftOf[square], square2);
         }

         if (file(square2) > file(square))
         {
            setSquare(squaresRightOf[square], square2);
         }
      }
   }

   for (i = 0; i < 256; i++)
   {
      for (j = 0; j < 8; j++)
      {
         bitshiftGap[j][i] = (i == 0 ? 0 : 8);

         for (k = 0; k < 8; k++)
         {
            if ((minValue[(int) k] & i) != 0
                && abs(j - k) < bitshiftGap[j][i])
            {
               bitshiftGap[j][i] = (UINT8) abs(j - k);
            }
         }
      }
   }

   for (i = 0; i < 4; i++)
   {
      UINT64 bitMask;

      for (bitMask = 0; bitMask < 0x10000; bitMask++)
      {
         SetSquaresInfo *info = &setSquares[i][bitMask];
         Bitboard board = bitMask << (16 * i);
         Square square;

         info->numSetSquares = 0;

         ITERATE_BITBOARD(&board, square)
         {
            info->setSquares[info->numSetSquares++] = (UINT8) square;
         }
      }
   }

   initializePremasks();

   ITERATE(square)
   {
      initializeMagicRookMoves(square);
      initializeMagicBishopMoves(square);
   }

   ITERATE(square)
   {
      int i;

      magicSquareInfoRook[square].preMask = preMaskRook[square];
      magicSquareInfoRook[square].magicNumber = magicRookNumber[square];

      for (i = 0; i < IMAX_ROOK; i++)
      {
         magicSquareInfoRook[square].moves[i] = magicRookMoves[square][i];
      }

      magicSquareInfoBishop[square].preMask = preMaskBishop[square];
      magicSquareInfoBishop[square].magicNumber = magicBishopNumber[square];

      for (i = 0; i < IMAX_BISHOP; i++)
      {
         magicSquareInfoBishop[square].moves[i] = magicBishopMoves[square][i];
      }
   }

   /*
      for (i = 0; i < 8; i++)
      {
      logDebug("0x%016llx,\n", squaresOfLateralFiles[i]);
      }

      getKeyStroke();
    */

#ifdef GENERATE_MAGIC_NUMBERS
   calculateMagicNumbers();
#endif

   return 0;
}

#ifndef NDEBUG

static int testBitOperations()
{
   Bitboard b = EMPTY_BITBOARD;

   assert(testSquare(lightSquares, A1) == 0);
   assert(testSquare(lightSquares, B1));
   assert(testSquare(lightSquares, G8));
   assert(testSquare(lightSquares, H8) == 0);

   assert(testSquare(darkSquares, A1));
   assert(testSquare(darkSquares, B1) == 0);
   assert(testSquare(darkSquares, G8) == 0);
   assert(testSquare(darkSquares, H8));

   assert(highestBit[0x0f] == 3);
   assert(highestBit[0xffff] == 15);
   assert(numSetBits[0] == 0);
   assert(numSetBits[0x5555] == 8);
   assert(numSetBits[0xaaaa] == 8);
   assert(numSetBits[0xffff] == 16);

   assert(testSquare(b, D4) == 0);
   setSquare(b, D4);
   assert(testSquare(b, D4) != 0);
   clearSquare(b, D4);
   assert(testSquare(b, D4) == 0);

   assert(testSquare(b, H8) == 0);
   setSquare(b, H8);
   assert(testSquare(b, H8) != 0);
   clearSquare(b, H8);
   assert(testSquare(b, H8) == 0);

   assert(getLastSquare(&b) == NO_SQUARE);
   setSquare(b, H8);
   assert(getLastSquare(&b) == H8);
   assert(getLastSquare(&b) == NO_SQUARE);

   assert(testSquare(squaresBehind[D4][C3], E5));
   assert(testSquare(squaresBehind[D4][C3], F6));
   assert(testSquare(squaresBehind[D4][C3], G7));
   assert(testSquare(squaresBehind[D4][C3], H8));
   assert(getNumberOfSetSquares(squaresBehind[D4][C3]) == 4);

   assert(testSquare(squaresBetween[B3][F7], C4));
   assert(testSquare(squaresBetween[B3][F7], D5));
   assert(testSquare(squaresBetween[B3][F7], E6));
   assert(getNumberOfSetSquares(squaresBetween[B3][F7]) == 3);

   assert(rankOverlay[0x8143] == 0xC3);

   b = EMPTY_BITBOARD;
   setSquare(b, A4);
   setSquare(b, E1);
   setSquare(b, E8);
   setSquare(b, H7);
   assert(getRankOverlay(b) == 0x91);

   assert(testSquare(squaresInDistance[1][C3], C4) != FALSE);
   assert(testSquare(squaresInDistance[1][C3], C5) == FALSE);
   assert(testSquare(squaresInDistance[3][E5], E2) != FALSE);
   assert(testSquare(squaresInDistance[3][E5], E1) == FALSE);
   assert(testSquare(squaresInDistance[5][H8], C3) != FALSE);
   assert(testSquare(squaresInDistance[5][H8], B2) == FALSE);

   assert(bitshiftGap[5][0] == 0);
   assert(bitshiftGap[1][129] == 1);
   assert(bitshiftGap[4][129] == 3);

   return 0;
}

static int testGeneralMoves()
{
   assert(testSquare(generalMoves[KING][C3], C4));
   assert(testSquare(generalMoves[KING][C3], D4));
   assert(testSquare(generalMoves[KING][C3], D3));
   assert(testSquare(generalMoves[KING][C3], D2));
   assert(testSquare(generalMoves[KING][C3], C2));
   assert(testSquare(generalMoves[KING][C3], B2));
   assert(testSquare(generalMoves[KING][C3], B3));
   assert(testSquare(generalMoves[KING][C3], B4));
   assert(getNumberOfSetSquares(generalMoves[KING][C3]) == 8);

   assert(testSquare(generalMoves[ROOK][C3], C8));
   assert(testSquare(generalMoves[ROOK][C3], H3));
   assert(testSquare(generalMoves[ROOK][C3], C1));
   assert(testSquare(generalMoves[ROOK][C3], A3));
   assert(getNumberOfSetSquares(generalMoves[ROOK][C3]) == 14);

   assert(testSquare(generalMoves[BISHOP][C3], H8));
   assert(testSquare(generalMoves[BISHOP][C3], E1));
   assert(testSquare(generalMoves[BISHOP][C3], A1));
   assert(testSquare(generalMoves[BISHOP][C3], A5));
   assert(getNumberOfSetSquares(generalMoves[BISHOP][C3]) == 11);

   assert(testSquare(generalMoves[QUEEN][C3], C8));
   assert(testSquare(generalMoves[QUEEN][C3], H8));
   assert(testSquare(generalMoves[QUEEN][C3], H3));
   assert(testSquare(generalMoves[QUEEN][C3], E1));
   assert(testSquare(generalMoves[QUEEN][C3], C1));
   assert(testSquare(generalMoves[QUEEN][C3], A1));
   assert(testSquare(generalMoves[QUEEN][C3], A3));
   assert(testSquare(generalMoves[QUEEN][C3], A5));
   assert(getNumberOfSetSquares(generalMoves[QUEEN][C3]) == 25);

   assert(testSquare(generalMoves[KNIGHT][C3], A2));
   assert(testSquare(generalMoves[KNIGHT][C3], A4));
   assert(testSquare(generalMoves[KNIGHT][C3], B5));
   assert(testSquare(generalMoves[KNIGHT][C3], D5));
   assert(testSquare(generalMoves[KNIGHT][C3], E4));
   assert(testSquare(generalMoves[KNIGHT][C3], E2));
   assert(testSquare(generalMoves[KNIGHT][C3], D1));
   assert(testSquare(generalMoves[KNIGHT][C3], B1));
   assert(getNumberOfSetSquares(generalMoves[KNIGHT][C3]) == 8);
   assert(getNumberOfSetSquares(generalMoves[KNIGHT][A1]) == 2);
   assert(getNumberOfSetSquares(generalMoves[KNIGHT][H8]) == 2);

   assert(testSquare(generalMoves[WHITE_PAWN][A2], B3));
   assert(getNumberOfSetSquares(generalMoves[WHITE_PAWN][A2]) == 1);
   assert(testSquare(generalMoves[WHITE_PAWN][B2], A3));
   assert(testSquare(generalMoves[WHITE_PAWN][B2], C3));
   assert(getNumberOfSetSquares(generalMoves[WHITE_PAWN][B2]) == 2);
   assert(testSquare(generalMoves[WHITE_PAWN][H2], G3));
   assert(getNumberOfSetSquares(generalMoves[WHITE_PAWN][H2]) == 1);

   assert(testSquare(generalMoves[BLACK_PAWN][A7], B6));
   assert(getNumberOfSetSquares(generalMoves[BLACK_PAWN][A7]) == 1);
   assert(testSquare(generalMoves[BLACK_PAWN][B7], A6));
   assert(testSquare(generalMoves[BLACK_PAWN][B7], C6));
   assert(getNumberOfSetSquares(generalMoves[BLACK_PAWN][B7]) == 2);
   assert(testSquare(generalMoves[BLACK_PAWN][H7], G6));
   assert(getNumberOfSetSquares(generalMoves[BLACK_PAWN][H7]) == 1);

   return 0;
}

static int testPieces()
{
   Bitboard b = EMPTY_BITBOARD, moves;
   BYTE obstacles[NUM_LANES];

   setSquare(b, C3);
   setSquare(b, C6);
   setSquare(b, F6);

   calculateObstacles(b, obstacles);

   moves = getMagicRookMoves(C4, b);
   assert(testSquare(moves, C3));
   assert(testSquare(moves, C5));
   assert(testSquare(moves, C6));
   assert(testSquare(moves, A4));
   assert(testSquare(moves, B4));
   assert(testSquare(moves, D4));
   assert(testSquare(moves, E4));
   assert(testSquare(moves, F4));
   assert(testSquare(moves, G4));
   assert(testSquare(moves, H4));
   assert(getNumberOfSetSquares(moves) == 10);

   moves = getMagicBishopMoves(E5, b);
   assert(testSquare(moves, D4));
   assert(testSquare(moves, C3));
   assert(testSquare(moves, D6));
   assert(testSquare(moves, C7));
   assert(testSquare(moves, B8));
   assert(testSquare(moves, F6));
   assert(testSquare(moves, F4));
   assert(testSquare(moves, G3));
   assert(testSquare(moves, H2));
   assert(getNumberOfSetSquares(moves) == 9);

   moves = getMagicQueenMoves(F3, b);
   assert(testSquare(moves, F2));
   assert(testSquare(moves, F1));
   assert(testSquare(moves, F4));
   assert(testSquare(moves, F5));
   assert(testSquare(moves, F6));
   assert(testSquare(moves, E3));
   assert(testSquare(moves, D3));
   assert(testSquare(moves, C3));
   assert(testSquare(moves, G3));
   assert(testSquare(moves, H3));
   assert(testSquare(moves, E4));
   assert(testSquare(moves, D5));
   assert(testSquare(moves, C6));
   assert(testSquare(moves, G4));
   assert(testSquare(moves, H5));
   assert(testSquare(moves, G2));
   assert(testSquare(moves, H1));
   assert(testSquare(moves, E2));
   assert(testSquare(moves, D1));
   assert(getNumberOfSetSquares(moves) == 19);

   moves = getKnightMoves(A8);
   assert(testSquare(moves, B6));
   assert(testSquare(moves, C7));
   assert(getNumberOfSetSquares(moves) == 2);

   moves = getKnightMoves(E5);
   assert(testSquare(moves, F7));
   assert(testSquare(moves, G6));
   assert(testSquare(moves, G4));
   assert(testSquare(moves, F3));
   assert(testSquare(moves, D3));
   assert(testSquare(moves, C4));
   assert(testSquare(moves, C6));
   assert(testSquare(moves, D7));
   assert(getNumberOfSetSquares(moves) == 8);

   return 0;
}

static int testPawns()
{
   Bitboard b = EMPTY_BITBOARD, moves, pawns;
   BYTE obstacles[NUM_LANES];

   setSquare(b, B3);
   setSquare(b, C4);
   setSquare(b, E4);
   setSquare(b, F6);
   setSquare(b, E5);
   setSquare(b, D6);
   setSquare(b, G2);
   setSquare(b, G3);
   setSquare(b, H2);
   setSquare(b, H7);

   calculateObstacles(b, obstacles);

   moves = getPawnAdvances(WHITE, A2, b);
   assert(testSquare(moves, A3));
   assert(testSquare(moves, A4));
   assert(getNumberOfSetSquares(moves) == 2);

   moves = getPawnAdvances(WHITE, B2, b);
   assert(getNumberOfSetSquares(moves) == 0);

   moves = getPawnAdvances(WHITE, C2, b);
   assert(testSquare(moves, C3));
   assert(getNumberOfSetSquares(moves) == 1);

   moves = getPawnAdvances(BLACK, H7, b);
   assert(testSquare(moves, H6));
   assert(testSquare(moves, H5));
   assert(getNumberOfSetSquares(moves) == 2);

   moves = getPawnAdvances(BLACK, F7, b);
   assert(getNumberOfSetSquares(moves) == 0);

   moves = getPawnAdvances(BLACK, E7, b);
   assert(testSquare(moves, E6));
   assert(getNumberOfSetSquares(moves) == 1);

   moves = getPawnCaptures(WHITE_PAWN, A2, b);
   assert(testSquare(moves, B3));
   assert(getNumberOfSetSquares(moves) == 1);

   moves = getPawnCaptures(WHITE_PAWN, D3, b);
   assert(testSquare(moves, C4));
   assert(testSquare(moves, E4));
   assert(getNumberOfSetSquares(moves) == 2);

   moves = getPawnCaptures(BLACK_PAWN, G7, b);
   assert(testSquare(moves, F6));
   assert(getNumberOfSetSquares(moves) == 1);

   moves = getPawnCaptures(BLACK_PAWN, E7, b);
   assert(testSquare(moves, D6));
   assert(testSquare(moves, F6));
   assert(getNumberOfSetSquares(moves) == 2);

   pawns = getInterestedPawns(BLACK, D5, b);
   assert(testSquare(pawns, D6));
   assert(getNumberOfSetSquares(pawns) == 1);

   pawns = getInterestedPawns(BLACK, H5, b);
   assert(testSquare(pawns, H7));
   assert(testSquare(pawns, H6));
   assert(getNumberOfSetSquares(pawns) == 2);

   pawns = getInterestedPawns(WHITE, H4, b);
   assert(testSquare(pawns, H2));
   assert(testSquare(pawns, H3));
   assert(getNumberOfSetSquares(pawns) == 2);

   pawns = getInterestedPawns(WHITE, G4, b);
   assert(testSquare(pawns, G3));
   assert(getNumberOfSetSquares(pawns) == 1);

   return 0;
}

static int testKings()
{
   Bitboard obstacles = EMPTY_BITBOARD, moves;
   BYTE castlingRights = WHITE_00 | WHITE_000 | BLACK_000 | BLACK_00;

   moves = getKingMoves(A1);
   assert(testSquare(moves, B1));
   assert(testSquare(moves, B2));
   assert(testSquare(moves, A2));
   assert(getNumberOfSetSquares(moves) == 3);

   moves = getKingMoves(D5);
   assert(testSquare(moves, D6));
   assert(testSquare(moves, E6));
   assert(testSquare(moves, E5));
   assert(testSquare(moves, E4));
   assert(testSquare(moves, D4));
   assert(testSquare(moves, C4));
   assert(testSquare(moves, C5));
   assert(testSquare(moves, C6));
   assert(getNumberOfSetSquares(moves) == 8);

   moves = getKingMoves(D5);
   assert(testSquare(moves, D6));
   assert(testSquare(moves, E6));
   assert(testSquare(moves, E5));
   assert(testSquare(moves, E4));
   assert(testSquare(moves, D4));
   assert(testSquare(moves, C4));
   assert(testSquare(moves, C5));
   assert(testSquare(moves, C6));
   assert(getNumberOfSetSquares(moves) == 8);

   moves = getCastlingMoves(WHITE, castlingRights, obstacles);
   assert(testSquare(moves, G1));
   assert(testSquare(moves, C1));
   assert(getNumberOfSetSquares(moves) == 2);

   moves = getCastlingMoves(BLACK, castlingRights, obstacles);
   assert(testSquare(moves, G8));
   assert(testSquare(moves, C8));
   assert(getNumberOfSetSquares(moves) == 2);

   castlingRights = WHITE_00 | BLACK_00;

   moves = getCastlingMoves(WHITE, castlingRights, obstacles);
   assert(testSquare(moves, G1));
   assert(getNumberOfSetSquares(moves) == 1);

   moves = getCastlingMoves(BLACK, castlingRights, obstacles);
   assert(testSquare(moves, G8));
   assert(getNumberOfSetSquares(moves) == 1);

   castlingRights = WHITE_000 | BLACK_000;

   moves = getCastlingMoves(WHITE, castlingRights, obstacles);
   assert(testSquare(moves, C1));
   assert(getNumberOfSetSquares(moves) == 1);

   moves = getCastlingMoves(BLACK, castlingRights, obstacles);
   assert(testSquare(moves, C8));
   assert(getNumberOfSetSquares(moves) == 1);

   castlingRights = WHITE_00 | WHITE_000 | BLACK_000 | BLACK_00;
   setSquare(obstacles, D1);
   setSquare(obstacles, D8);
   moves = getCastlingMoves(WHITE, castlingRights, obstacles);
   assert(testSquare(moves, G1));
   assert(getNumberOfSetSquares(moves) == 1);
   moves = getCastlingMoves(BLACK, castlingRights, obstacles);
   assert(testSquare(moves, G8));
   assert(getNumberOfSetSquares(moves) == 1);
   clearSquare(obstacles, D1);
   clearSquare(obstacles, D8);

   moves = getCastlingMoves(WHITE, castlingRights, obstacles);
   assert(testSquare(moves, G1));
   assert(testSquare(moves, C1));
   assert(getNumberOfSetSquares(moves) == 2);
   moves = getCastlingMoves(BLACK, castlingRights, obstacles);
   assert(testSquare(moves, G8));
   assert(testSquare(moves, C8));
   assert(getNumberOfSetSquares(moves) == 2);

   setSquare(obstacles, C1);
   setSquare(obstacles, C8);
   moves = getCastlingMoves(WHITE, castlingRights, obstacles);
   assert(testSquare(moves, G1));
   assert(getNumberOfSetSquares(moves) == 1);
   moves = getCastlingMoves(BLACK, castlingRights, obstacles);
   assert(testSquare(moves, G8));
   assert(getNumberOfSetSquares(moves) == 1);
   clearSquare(obstacles, C1);
   clearSquare(obstacles, C8);

   setSquare(obstacles, F1);
   setSquare(obstacles, F8);
   moves = getCastlingMoves(WHITE, castlingRights, obstacles);
   assert(testSquare(moves, C1));
   assert(getNumberOfSetSquares(moves) == 1);
   moves = getCastlingMoves(BLACK, castlingRights, obstacles);
   assert(testSquare(moves, C8));
   assert(getNumberOfSetSquares(moves) == 1);
   clearSquare(obstacles, F1);
   clearSquare(obstacles, F8);

   setSquare(obstacles, G1);
   setSquare(obstacles, G8);
   moves = getCastlingMoves(WHITE, castlingRights, obstacles);
   assert(testSquare(moves, C1));
   assert(getNumberOfSetSquares(moves) == 1);
   moves = getCastlingMoves(BLACK, castlingRights, obstacles);
   assert(testSquare(moves, C8));
   assert(getNumberOfSetSquares(moves) == 1);
   clearSquare(obstacles, G1);
   clearSquare(obstacles, G8);

   setSquare(obstacles, G1);
   setSquare(obstacles, G8);
   setSquare(obstacles, C1);
   setSquare(obstacles, C8);
   moves = getCastlingMoves(WHITE, castlingRights, obstacles);
   assert(getNumberOfSetSquares(moves) == 0);
   moves = getCastlingMoves(BLACK, castlingRights, obstacles);
   assert(getNumberOfSetSquares(moves) == 0);
   clearSquare(obstacles, G1);
   clearSquare(obstacles, G8);
   clearSquare(obstacles, C1);
   clearSquare(obstacles, C8);

   return 0;
}

static int testFlooding()
{
   Bitboard kingMoves = getKingMoves(D4);
   Bitboard expected;

   floodBoard(&kingMoves);
   expected = squaresBetween[A6][G6] | squaresBetween[A5][G5] |
      squaresBetween[A4][G4] | squaresBetween[A3][G3] |
      squaresBetween[A2][G2];
   assert(kingMoves == expected);

   kingMoves = getKingMoves(H1);
   floodBoard(&kingMoves);
   expected = squaresBehind[E3][A3] | squaresBehind[E2][A2] |
      squaresBehind[E1][A1];
   assert(kingMoves == expected);

   kingMoves = getKingMoves(A8);
   floodBoard(&kingMoves);
   expected = squaresBehind[D8][H8] | squaresBehind[D7][H7] |
      squaresBehind[D6][H6];
   assert(kingMoves == expected);

   return 0;
}

static int testGetSetSquares()
{
   UINT8 moveSquares[_64_];
   Bitboard kingMoves = getKingMoves(D4);
   int numMoves = getSetSquares(kingMoves, moveSquares), i;

   assert(numMoves == 8);

   for (i = 0; i < numMoves; i++)
   {
      const Square square = (Square) moveSquares[i];

      assert(testSquare(kingMoves, square) == TRUE);
   }

   return 0;
}

#endif

int testModuleBitboard()
{
#ifndef NDEBUG
   int result;

   if ((result = testBitOperations()) != 0)
   {
      return result;
   }

   if ((result = testGeneralMoves()) != 0)
   {
      return result;
   }

   if ((result = testPieces()) != 0)
   {
      return result;
   }

   if ((result = testPawns()) != 0)
   {
      return result;
   }

   if ((result = testKings()) != 0)
   {
      return result;
   }

   if ((result = testFlooding()) != 0)
   {
      return result;
   }

   if ((result = testGetSetSquares()) != 0)
   {
      return result;
   }
#endif

   return 0;
}
