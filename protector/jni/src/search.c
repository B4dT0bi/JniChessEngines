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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include "search.h"
#include "matesearch.h"
#include "io.h"
#include "movegeneration.h"
#include "hash.h"
#include "evaluation.h"
#include "book.h"
#include "coordination.h"
#include "xboard.h"

#ifdef INCLUDE_TABLEBASE_ACCESS
#include "tablebase.h"
#endif

/* #define DEBUG_THREAD_COORDINATION */

extern bool resetSharedHashtable;
const int HASH_DEPTH_OFFSET = 3;
const UINT64 GUI_NODE_COUNT_MIN = 250000;

#define NUM_FUTILITY_MARGIN_VALUES 13
#define NUM_LOG_VALUES 256
INT32 checkTimeCount = 0;

int quietMoveCountLimit[2][32]; /* number of quiet moves to be examined @ specific restDepth */
int quietPvMoveReduction[2][64][64];    /* [variationType][restDepth][moveCount] */
int quietMoveReduction[2][64][64];      /* [variationType][restDepth][moveCount] */
int futilityMargin[NUM_FUTILITY_MARGIN_VALUES + 1];     /* [restDepth] */
int maxPieceValue[16];          /* the maximal value of a piece type */

/* Prototypes */
static int searchBest(Variation * variation, int alpha, int beta,
                      const int ply, const int restDepth, const bool pvNode,
                      const bool cutNode, Move * bestMove, Move excludeMove,
                      bool tryEarlyPrunings);

static bool moveIsQuiet(const Move move, const Position * position)
{
   return (bool) (position->piece[getToSquare(move)] == NO_PIECE &&
                  getNewPiece(move) == NO_PIECE &&
                  (getToSquare(move) != position->enPassantSquare ||
                   pieceType(position->piece[getFromSquare(move)]) != PAWN));
}

static void checkTerminationConditions(Variation * variation)
{
   if (variation->searchStatus == SEARCH_STATUS_RUNNING)
   {
      if (variation->terminate != FALSE &&
          (variation->iteration > 1 || variation->threadNumber > 0))
      {
         variation->searchStatus = SEARCH_STATUS_TERMINATE;
      }
   }
}

bool checkNodeExclusion(int restDepth)
{
   return restDepth >= 6 * DEPTH_RESOLUTION && getNumberOfThreads() > 1;
}

static bool hasDangerousPawns(const Position * position, const Color color)
{
   if (color == WHITE)
   {
      return (bool)
         ((position->piecesOfType[WHITE_PAWN] & squaresOfRank[RANK_7]) !=
          EMPTY_BITBOARD);
   }
   else
   {
      return (bool)
         ((position->piecesOfType[BLACK_PAWN] & squaresOfRank[RANK_2]) !=
          EMPTY_BITBOARD);
   }
}

int getEvalValue(Variation * variation)
{
   EvaluationBase base;

   base.ownColor = variation->ownColor;
   return
      getValue(&variation->singlePosition, &base, variation->pawnHashtable,
               variation->kingsafetyHashtable);
}

static int getStaticValue(Variation * variation, const int ply)
{
   PlyInfo *pi = &variation->plyInfo[ply];

   if (pi->staticValueAvailable == FALSE)
   {
      EvaluationBase base;

      base.ownColor = variation->ownColor;
      pi->staticValue = pi->refinedStaticValue =
         getValue(&variation->singlePosition, &base, variation->pawnHashtable,
                  variation->kingsafetyHashtable);
      pi->staticValueAvailable = TRUE;
   }
   else
   {
      assert(pi->staticValue == getEvalValue(variation));
   }

   return pi->staticValue;
}

static int getRefinedStaticValue(Variation * variation, const int ply)
{
   PlyInfo *pi = &variation->plyInfo[ply];

   if (pi->staticValueAvailable == FALSE)
   {
      EvaluationBase base;

      base.ownColor = variation->ownColor;
      pi->staticValue = pi->refinedStaticValue =
         getValue(&variation->singlePosition, &base, variation->pawnHashtable,
                  variation->kingsafetyHashtable);
      pi->staticValueAvailable = TRUE;
   }
   else
   {
      assert(pi->staticValue == getEvalValue(variation));
   }

   return pi->refinedStaticValue;
}

static void updateGains(Variation * variation, const int ply)
{
   if (variation->plyInfo[ply].gainsUpdated == FALSE &&
       variation->plyInfo[ply - 1].quietMove != FALSE)
   {
      const int moveIndex = variation->plyInfo[ply - 1].indexCurrentMove;
      INT16 *storedGain = &variation->positionalGain[moveIndex];
      const INT16 currentDiff = (INT16)
         (getStaticValue(variation, ply) +
          variation->plyInfo[ply - 1].staticValue);

      *storedGain =
         (currentDiff >= (*storedGain) ? currentDiff : (*storedGain) - 1);
   }

   variation->plyInfo[ply].gainsUpdated = TRUE;
}

static void updateCounterMoves(Variation * variation, const int ply,
                               Move counterMove)
{
   if (variation->plyInfo[ply - 1].currentMove != NULLMOVE)
   {
      const int moveIndex = variation->plyInfo[ply - 1].indexCurrentMove;

      if (counterMove != variation->counterMove1[moveIndex])
      {
         variation->counterMove2[moveIndex] =
            variation->counterMove1[moveIndex];
         variation->counterMove1[moveIndex] = counterMove;
      }
   }
}

static void updateFollowupMoves(Variation * variation, const int ply,
                                Move followupMove)
{
   if (variation->plyInfo[ply - 2].currentMove != NULLMOVE)
   {
      const int moveIndex = variation->plyInfo[ply - 2].indexCurrentMove;

      if (followupMove != variation->followupMove1[moveIndex])
      {
         variation->followupMove2[moveIndex] =
            variation->followupMove1[moveIndex];
         variation->followupMove1[moveIndex] = followupMove;
      }
   }
}

static bool positionIsWellKnown(Variation * variation,
                                Position * position,
                                const UINT64 hashKey,
                                Hashentry ** bestTableHit,
                                const int ply, const int alpha,
                                const int beta, const int restDepth,
                                const bool pvNode,
                                const bool updateGainValues,
                                Move * hashmove,
                                const Move excludeMove, int *value)
{
   Hashentry *tableHit = getHashentry(getSharedHashtable(),
                                      hashKey);

   if (tableHit != NULL)
   {                            /* 45% */
      const int importance = getHashentryImportance(tableHit);
      const int flag = getHashentryFlag(tableHit);
      const int hashEntryValue =
         calcEffectiveValue(getHashentryValue(tableHit), ply);
      PlyInfo *pi = &variation->plyInfo[ply];

      *bestTableHit = tableHit;
      *value = hashEntryValue;

      /*
         if (getHashentryStaticValue(tableHit) != getStaticValue(variation, ply))
         {
         logDebug("hv=%d sv=%d ev=%d\n", getHashentryStaticValue(tableHit),
         getStaticValue(variation, ply), getEvalValue(variation));
         dumpVariation(variation);
         }
       */

      assert(getHashentryStaticValue(tableHit) ==
             getStaticValue(variation, ply));

      if (pi->staticValueAvailable == FALSE)
      {
         pi->staticValue = pi->refinedStaticValue =
            getHashentryStaticValue(tableHit);
         pi->staticValueAvailable = TRUE;

         if (updateGainValues)
         {
            updateGains(variation, ply);
         }
      }

      if (pvNode == FALSE && excludeMove != NULLMOVE &&
          restDepth <= importance && flag != HASHVALUE_EVAL)
      {                         /* 99% */
         assert(flag == HASHVALUE_UPPER_LIMIT ||
                flag == HASHVALUE_EXACT || flag == HASHVALUE_LOWER_LIMIT);
         assert(hashEntryValue >= VALUE_MATED &&
                hashEntryValue < -VALUE_MATED);

         switch (flag)
         {
         case HASHVALUE_UPPER_LIMIT:
            if (hashEntryValue <= alpha)
            {
               *value = (hashEntryValue <= VALUE_MATED ?
                         alpha : hashEntryValue);

               return TRUE;
            }

            if (restDepth >= HASH_DEPTH_OFFSET &&
                hashEntryValue < getStaticValue(variation, ply))
            {
               variation->plyInfo[ply].refinedStaticValue = hashEntryValue;
            }
            break;

         case HASHVALUE_EXACT:
            *value = hashEntryValue;

            return TRUE;

         case HASHVALUE_LOWER_LIMIT:
            if (hashEntryValue >= beta)
            {
               *value = (hashEntryValue >= -VALUE_MATED ?
                         beta : hashEntryValue);

               return TRUE;
            }

            if (restDepth >= HASH_DEPTH_OFFSET &&
                hashEntryValue > getStaticValue(variation, ply))
            {
               variation->plyInfo[ply].refinedStaticValue = hashEntryValue;
            }
            break;

         default:;
         }
      }
   }

   if (*bestTableHit != 0)
   {
      *hashmove = (Move) getHashentryMove(*bestTableHit);

      if (*hashmove != NO_MOVE)
      {                         /* 81% */
         assert(moveIsPseudoLegal(position, *hashmove));

         if (moveIsPseudoLegal(position, *hashmove))
         {
            assert(moveIsLegal(position, *hashmove));
         }
         else
         {
            *hashmove = NO_MOVE;
         }
      }
   }

   return FALSE;
}

static int searchBestQuiescence(Variation * variation, int alpha, int beta,
                                const int ply, const int restDepth,
                                Move * bestMove, const bool pvNode)
{
   const int oldAlpha = alpha;
   UINT8 hashentryFlag;
   UINT8 hashDepth = (restDepth >= 0 ? 2 : 1);
   Position *position = &variation->singlePosition;
   int best = VALUE_MATED, currentValue = VALUE_MATED, historyLimit, i;
   const int VALUE_MATE_HERE = -VALUE_MATED - ply + 1;
   const int VALUE_MATED_HERE = VALUE_MATED + ply;
   Movelist movelist;
   Move currentMove, bestReply, hashmove = NO_MOVE;
   const bool inCheck = variation->plyInfo[ply - 1].currentMoveIsCheck;
   EvaluationBase base;
   Hashentry *bestTableHit = 0;
   int hashValue;

   assert(alpha >= VALUE_MATED && alpha <= -VALUE_MATED);
   assert(beta >= VALUE_MATED && beta <= -VALUE_MATED);
   assert(alpha < beta);
   assert(ply > 0 && ply < MAX_DEPTH);
   assert(restDepth < DEPTH_RESOLUTION);
   assert(passiveKingIsSafe(position));
   assert((inCheck != FALSE) == (activeKingIsSafe(position) == FALSE));

   *bestMove = NO_MOVE;
   variation->plyInfo[ply].quietMove = FALSE;   /* avoid subsequent gain updates */
   variation->plyInfo[ply].pv.length = 0;
   variation->plyInfo[ply].pv.move[0] = NO_MOVE;
   movelist.positionalGain = &(variation->positionalGain[0]);

   variation->nodes++;
   checkTerminationConditions(variation);

   if (variation->searchStatus != SEARCH_STATUS_RUNNING &&
       variation->iteration > 1)
   {
      return 0;
   }

   /* Check for a draw according to the 50-move-rule */
   /* ---------------------------------------------- */
   if (position->halfMoveClock > 100)
   {
      return variation->drawScore[position->activeColor];
   }

   /* Check for a draw by repetition. */
   /* ------------------------------- */
   historyLimit = POSITION_HISTORY_OFFSET + variation->ply -
      position->halfMoveClock;

   assert(historyLimit >= 0);

   for (i = POSITION_HISTORY_OFFSET + variation->ply - 4;
        i >= historyLimit; i -= 2)
   {
      if (position->hashKey == variation->positionHistory[i])
      {
         return variation->drawScore[position->activeColor];
      }
   }

   /* Probe the transposition table */
   /* ----------------------------- */

   if (positionIsWellKnown(variation, position, position->hashKey,
                           &bestTableHit, ply, alpha, beta,
                           hashDepth, pvNode, FALSE,
                           &hashmove, NO_MOVE, &hashValue))
   {
      *bestMove = hashmove;

      return hashValue;
   }

   if (hashmove != NO_MOVE && inCheck == FALSE &&
       moveIsQuiet(hashmove, position))
   {
      hashmove = NO_MOVE;
   }

   if (inCheck == FALSE)
   {
      const bool staticValueAvailable =
         variation->plyInfo[ply].staticValueAvailable;

      assert(flipTest(position,
                      variation->pawnHashtable,
                      variation->kingsafetyHashtable) != FALSE);

      if (staticValueAvailable == FALSE)
      {
         base.ownColor = variation->ownColor;
         best = getValue(position,
                         &base,
                         variation->pawnHashtable,
                         variation->kingsafetyHashtable);
         variation->plyInfo[ply].staticValue =
            variation->plyInfo[ply].refinedStaticValue = best;
         variation->plyInfo[ply].staticValueAvailable = TRUE;
      }
      else
      {
         best = variation->plyInfo[ply].staticValue;
      }

      if (bestTableHit != 0)
      {
         const int flag = getHashentryFlag(bestTableHit);
         const int requiredFlag = (hashValue > best ?
                                   HASHVALUE_LOWER_LIMIT :
                                   HASHVALUE_UPPER_LIMIT);

         if (flag & requiredFlag)
         {
            best = hashValue;
         }
      }

      if (best > alpha)
      {
         alpha = best;

         if (best >= beta)
         {
            if (staticValueAvailable == FALSE)
            {
               UINT8 hashentryFlag = HASHVALUE_EVAL;

               setHashentry(getSharedHashtable(), position->hashKey,
                            calcHashtableValue(best, ply),
                            0, packedMove(NO_MOVE), hashentryFlag,
                            (INT16) getStaticValue(variation, ply));
            }

            return best;
         }
      }

      currentValue = best;
   }

   if (ply >= MAX_DEPTH)
   {
      assert(flipTest(position,
                      variation->pawnHashtable,
                      variation->kingsafetyHashtable) != FALSE);

      return getStaticValue(variation, ply);
   }

   if (alpha < VALUE_MATED_HERE && inCheck == FALSE)
   {
      alpha = VALUE_MATED_HERE;

      if (alpha >= beta)
      {
         return VALUE_MATED_HERE;
      }
   }

   if (beta > VALUE_MATE_HERE)
   {
      beta = VALUE_MATE_HERE;

      if (beta <= alpha)
      {
         return VALUE_MATE_HERE;
      }
   }

   initQuiescenceMovelist(&movelist, &variation->singlePosition,
                          &variation->plyInfo[ply],
                          &variation->historyValue[0],
                          hashmove, restDepth, inCheck);
   initializePlyInfo(variation);

   while ((currentMove = getNextMove(&movelist)) != NO_MOVE)
   {
      int value, newDepth =
         (inCheck ? restDepth : restDepth - DEPTH_RESOLUTION);
      int optValue = currentValue + 43 +
         maxPieceValue[position->piece[getToSquare(currentMove)]];
      const Square toSquare = getToSquare(currentMove);

      if (pvNode == FALSE && inCheck == FALSE && optValue < alpha &&
          optValue > VALUE_ALMOST_MATED &&
          movesAreEqual(currentMove, hashmove) == FALSE &&
          getNewPiece(currentMove) != (Piece) QUEEN &&
          numberOfNonPawnPieces(position, position->activeColor) > 1 &&
          (pieceType(position->piece[getFromSquare(currentMove)]) != PAWN ||
           colorRank(position->activeColor, toSquare) != RANK_7))
      {
         const bool enPassant = (bool)
            (toSquare == position->enPassantSquare &&
             pieceType(position->piece[getFromSquare(currentMove)]) == PAWN);

         if (getNewPiece(currentMove) != NO_PIECE)
         {
            optValue +=
               maxPieceValue[getNewPiece(currentMove)] - basicValue[PAWN];
         }

         if (enPassant)
         {
            optValue += maxPieceValue[PAWN];
         }

         if (optValue < alpha && moveIsCheck(currentMove, position) == FALSE)
         {
            best = max(best, optValue);

            continue;
         }
      }

      assert(moveIsPseudoLegal(position, currentMove));

      if (makeMoveFast(variation, currentMove) != 0 ||
          passiveKingIsSafe(&variation->singlePosition) == FALSE)
      {
         unmakeLastMove(variation);

         continue;
      }

      variation->plyInfo[ply].currentMoveIsCheck =
         activeKingIsSafe(&variation->singlePosition) == FALSE;

      assert(position->piece[getToSquare(currentMove)] != NO_PIECE ||
             (getToSquare(currentMove) == position->enPassantSquare &&
              position->piece[getFromSquare(currentMove)] ==
              (PAWN | position->activeColor)) ||
             getNewPiece(currentMove) != NO_PIECE ||
             inCheck || variation->plyInfo[ply].currentMoveIsCheck);

      assert(inCheck != FALSE ||
             basicValue[position->piece[getFromSquare(currentMove)]] <=
             basicValue[position->piece[getToSquare(currentMove)]] ||
             seeMove(position, currentMove) >= 0);

      value = -searchBestQuiescence(variation, -beta, -alpha, ply + 1,
                                    newDepth, &bestReply, pvNode);

      unmakeLastMove(variation);

      if (variation->searchStatus != SEARCH_STATUS_RUNNING &&
          variation->iteration > 1)
      {
         return 0;
      }

      if (value > best)
      {
         best = value;

         if (best > alpha)
         {
            alpha = best;
            *bestMove = currentMove;

            if (pvNode)
            {
               appendMoveToPv(&(variation->plyInfo[ply].pv),
                              &(variation->plyInfo[ply - 1].pv), currentMove);
            }

            if (best >= beta)
            {
               break;
            }
         }
      }
   }

   if (best == VALUE_MATED)
   {
      /* mate */

      assert(inCheck != FALSE);

      best = VALUE_MATED + ply;
   }

   /* Store the value in the transposition table. */
   /* ------------------------------------------- */
   if (best >= beta)
   {
      hashentryFlag = HASHVALUE_LOWER_LIMIT;
   }
   else
   {
      hashentryFlag = (best > oldAlpha && pvNode ?
                       HASHVALUE_EXACT : HASHVALUE_UPPER_LIMIT);
   }

   setHashentry(getSharedHashtable(), position->hashKey,
                calcHashtableValue(best, ply),
                hashDepth, packedMove(*bestMove), hashentryFlag,
                (INT16) getStaticValue(variation, ply));

   return best;
}

static bool moveIsCastling(const Move move, const Position * position)
{
   return (bool) (pieceType(position->piece[getFromSquare(move)]) == KING &&
                  distance(getFromSquare(move), getToSquare(move)) == 2);
}

static bool isPassedPawnMove(const Square pawnSquare,
                             const Square targetSquare,
                             const Position * position)
{
   const Piece piece = position->piece[pawnSquare];

   if (pieceType(piece) == PAWN)
   {
      return pawnIsPassed(position, targetSquare, pieceColor(piece));
   }
   else
   {
      return FALSE;
   }
}

static int getSingleMoveExtensionDepth(const bool pvNode)
{
   return (pvNode ? 6 : 8) * DEPTH_RESOLUTION;
}

static int searchBest(Variation * variation, int alpha, int beta,
                      const int ply, const int restDepth, const bool pvNode,
                      const bool cutNode, Move * bestMove, Move excludeMove,
                      const bool tryEarlyPrunings)
{
   Position *position = &variation->singlePosition;
   int best = VALUE_MATED;
   const int VALUE_MATE_HERE = -VALUE_MATED - ply + 1;
   const int VALUE_MATED_HERE = VALUE_MATED + ply;
   const int numPieces =
      numberOfNonPawnPieces(position, position->activeColor);
   Movelist movelist;
   UINT8 hashentryFlag;
   int i, historyLimit, numMovesPlayed = 0;
   Move hashmove = NO_MOVE;
   Hashentry *bestTableHit = 0;
   Move currentMove, bestReply;
   const bool inCheck = variation->plyInfo[ply - 1].currentMoveIsCheck;
   const int rdBasic = restDepth / DEPTH_RESOLUTION;
   bool singularExtensionNode = FALSE;
   int hashEntryValue = 0;
   const UINT64 hashKey = position->hashKey;
   const bool cutsAreAllowed = (bool) (abs(beta) < -(VALUE_ALMOST_MATED));
   int variationType = (ply < 2 ? 1 : 0);
   int quietMoveIndex[MAX_MOVES_PER_POSITION], quietMoveCount = 0;
   int deferCount = 0;

   *bestMove = NO_MOVE;
   variation->plyInfo[ply].quietMove = FALSE;   /* avoid subsequent gain updates */
   variation->plyInfo[ply].isHashMove = FALSE;
   variation->plyInfo[ply].pv.length = 0;
   variation->plyInfo[ply].pv.move[0] = NO_MOVE;
   movelist.positionalGain = &(variation->positionalGain[0]);

   if (ply + 1 > variation->selDepth)
   {
      variation->selDepth = ply + 1;
   }

   assert(alpha >= VALUE_MATED && alpha <= -VALUE_MATED);
   assert(beta >= VALUE_MATED && beta <= -VALUE_MATED);
   assert(alpha < beta);
   assert(ply > 0 && ply < MAX_DEPTH);
   assert(passiveKingIsSafe(position));
   assert((inCheck != FALSE) == (activeKingIsSafe(position) == FALSE));

   /* Check for a draw according to the 50-move-rule */
   /* ---------------------------------------------- */
   if (position->halfMoveClock > 100)
   {
      return variation->drawScore[position->activeColor];
   }

   /* Check for a draw by repetition. */
   /* ------------------------------- */
   historyLimit = POSITION_HISTORY_OFFSET + variation->ply -
      position->halfMoveClock;

   assert(historyLimit >= 0);

   for (i = POSITION_HISTORY_OFFSET + variation->ply - 4;
        i >= historyLimit; i -= 2)
   {
      if (position->hashKey == variation->positionHistory[i])
      {
         return variation->drawScore[position->activeColor];
      }
   }

   if (restDepth < DEPTH_RESOLUTION)
   {                            /* 63% */
      int qsValue;

      qsValue =
         searchBestQuiescence(variation, alpha, beta, ply, 0, bestMove,
                              pvNode);

      if (inCheck == FALSE &&
          variation->plyInfo[ply].staticValueAvailable != FALSE)
      {
         updateGains(variation, ply);
      }

      return qsValue;
   }

   variation->nodes++;
   checkTerminationConditions(variation);

   if (variation->searchStatus != SEARCH_STATUS_RUNNING &&
       variation->iteration > 1)
   {
      return 0;
   }

#ifdef INCLUDE_TABLEBASE_ACCESS
   /* Probe the tablebases in case of reduced material */
   /* ------------------------------------------------ */
   if (ply >= 2 && (pvNode || restDepth >= 10 * DEPTH_RESOLUTION) &&
       position->numberOfPieces[WHITE] +
       position->numberOfPieces[BLACK] <= 6 &&
       excludeMove == NO_MOVE && tbAvailable != FALSE)
   {
      int tbValue;

      tbValue = probeTablebase(position);

      if (tbValue != TABLEBASE_ERROR)
      {
         variation->tbHits++;

         if (tbValue == 0)
         {
            return variation->drawScore[position->activeColor];
         }

         return (tbValue > 0 ? tbValue - ply : tbValue + ply);
      }
   }
#endif

   /* Probe the transposition table */
   /* ----------------------------- */
   if (excludeMove == NO_MOVE)
   {
      int hashValue;

      if (positionIsWellKnown(variation, position, hashKey,
                              &bestTableHit, ply, alpha, beta,
                              restDepth + HASH_DEPTH_OFFSET, pvNode,
                              inCheck == FALSE, &hashmove, excludeMove,
                              &hashValue))
      {
         *bestMove = hashmove;

         if (hashValue >= beta && *bestMove != NO_MOVE &&
             moveIsQuiet(*bestMove, position))
         {
            Move killerMove = *bestMove;
            const Piece movingPiece =
               position->piece[getFromSquare(killerMove)];

            setMoveValue(&killerMove, movingPiece);
            registerKillerMove(&variation->plyInfo[ply], killerMove);
         }

         return hashValue;
      }
   }

   if (inCheck == FALSE)
   {
      updateGains(variation, ply);
   }

   if (ply >= 2)
   {
      if (variation->plyInfo[ply].staticValue >=
          variation->plyInfo[ply - 2].staticValue ||
          variation->plyInfo[ply].staticValueAvailable == FALSE ||
          variation->plyInfo[ply - 2].staticValueAvailable == FALSE)
      {
         variationType = 1;
      }
   }

   if (ply >= MAX_DEPTH)
   {
      return getStaticValue(variation, ply);
   }

   if (alpha < VALUE_MATED_HERE && inCheck == FALSE)
   {
      alpha = VALUE_MATED_HERE;

      if (alpha >= beta)
      {
         return VALUE_MATED_HERE;
      }
   }

   if (beta > VALUE_MATE_HERE)
   {
      beta = VALUE_MATE_HERE;

      if (beta <= alpha)
      {
         return VALUE_MATE_HERE;
      }
   }

   initializePlyInfo(variation);

   if (tryEarlyPrunings == FALSE)
   {
      goto checkAvailableMoves;
   }

   /* Razoring */
   if (pvNode == FALSE && restDepth < 4 * DEPTH_RESOLUTION &&
       inCheck == FALSE && hashmove == NO_MOVE && cutsAreAllowed &&
       hasDangerousPawns(position, position->activeColor) == FALSE)
   {
      const int limit = alpha - (204 + 22 * restDepth);

      if (getRefinedStaticValue(variation, ply) < limit)
      {
         const int qsValue =
            searchBestQuiescence(variation, limit - 1, limit, ply, 0,
                                 bestMove, pvNode);

         if (qsValue < limit)
         {
            return qsValue;
         }
      }
   }

   /* Static nullmove pruning */
   if (pvNode == FALSE && restDepth < 4 * DEPTH_RESOLUTION &&
       inCheck == FALSE && cutsAreAllowed && excludeMove == NO_MOVE &&
       numPieces >= 2 &&
       !hasDangerousPawns(position, opponent(position->activeColor)))
   {
      const int referenceValue =
         getRefinedStaticValue(variation, ply) - (40 + 41 * restDepth);

      if (referenceValue >= beta)
      {
         return referenceValue;
      }
   }

   /* Nullmove pruning with verification */
   if (restDepth >= 2 * DEPTH_RESOLUTION && inCheck == FALSE &&
       pvNode == FALSE && cutsAreAllowed && excludeMove == NO_MOVE &&
       numPieces >= 2 && getRefinedStaticValue(variation, ply) >= beta)
   {                            /* 16-32% */
      const int diff = getRefinedStaticValue(variation, ply) - beta;
      const int additionalReduction = min(diff / 110, 3) * DEPTH_RESOLUTION;
      const int newDepth =
         restDepth - 3 * DEPTH_RESOLUTION - restDepth / 4 -
         additionalReduction;
      const int verificationDepth =
         (numPieces >= 3 ? 12 : 6) * DEPTH_RESOLUTION;
      int nullValue;

      assert(flipTest(position,
                      variation->pawnHashtable,
                      variation->kingsafetyHashtable) != FALSE);

      makeMoveFast(variation, NULLMOVE);
      variation->plyInfo[ply].currentMoveIsCheck = FALSE;
      nullValue = -searchBest(variation, -beta, -beta + 1, ply + 1,
                              newDepth, pvNode, !cutNode, &bestReply,
                              NO_MOVE, FALSE);
      unmakeLastMove(variation);

      if (nullValue >= VALUE_MATE_HERE)
      {
         nullValue = beta;
      }

      if (nullValue >= beta && newDepth >= DEPTH_RESOLUTION &&
          restDepth >= verificationDepth)
      {                         /* 2% */
         const int noNullValue = searchBest(variation, alpha, beta, ply,
                                            newDepth,
                                            FALSE, FALSE, &bestReply,
                                            NULLMOVE, FALSE);

         if (noNullValue >= beta)
         {                      /* 99% */
            best = nullValue;

            goto storeResult;
         }
      }
      else if (nullValue >= beta)
      {                         /* 70% */
         if (numPieces >= 3)
         {
            best = nullValue;

            goto storeResult;
         }
         else
         {
            return nullValue;
         }
      }
   }

   /* Try to find a quick refutation of the opponent's previous move */
   if (pvNode == FALSE && cutsAreAllowed &&
       restDepth >= 5 * DEPTH_RESOLUTION && excludeMove == NO_MOVE &&
       inCheck == FALSE)
   {
      const int limit = beta + 192;
      const int staticValue = getRefinedStaticValue(variation, ply);
      const Move qrHashmove = (hashmove != NO_MOVE &&
                               position->piece[getToSquare(hashmove)] !=
                               NO_PIECE ? hashmove : NO_MOVE);

      initCaptureMovelist(&movelist, &variation->singlePosition,
                          &variation->plyInfo[ply],
                          &variation->historyValue[0], qrHashmove, inCheck);

      while ((currentMove = getNextMove(&movelist)) != NO_MOVE)
      {
         const Square toSquare = getToSquare(currentMove);
         const Piece capturedPiece = position->piece[toSquare];
         int moveValue;

         if (staticValue + maxPieceValue[capturedPiece] < limit - 38)
         {
            continue;
         }

         /* Execute the current move and check if it is legal. */
         /* -------------------------------------------------- */
         if (makeMoveFast(variation, currentMove) != 0 ||
             passiveKingIsSafe(&variation->singlePosition) == FALSE)
         {
            unmakeLastMove(variation);

            continue;
         }

         variation->plyInfo[ply].currentMoveIsCheck =
            activeKingIsSafe(&variation->singlePosition) == FALSE;
         variation->plyInfo[ply].indexCurrentMove =
            historyIndex(currentMove, position);
         variation->plyInfo[ply].quietMove = FALSE;
         variation->plyInfo[ply].isHashMove = FALSE;

         moveValue = -searchBest(variation, -limit, -limit + 1, ply + 1,
                                 restDepth - 4 * DEPTH_RESOLUTION, FALSE,
                                 !cutNode, &bestReply, NO_MOVE, TRUE);

         unmakeLastMove(variation);

         if (moveValue >= limit)
         {
            best = moveValue;
            *bestMove = currentMove;

            goto storeResult;
         }
      }
   }

 checkAvailableMoves:

   /* Internal iterative deepening. */
   /* ----------------------------- */
   if (hashmove == NO_MOVE &&
       restDepth >= (pvNode ? 3 : 7) * DEPTH_RESOLUTION &&
       (pvNode || (inCheck == FALSE &&
                   getRefinedStaticValue(variation, ply) >= beta - 100)))
   {
      const Move excludeHere =
         (excludeMove != NO_MOVE ? excludeMove : NULLMOVE);
      const int searchDepth =
         (pvNode ? restDepth - 2 * DEPTH_RESOLUTION : restDepth / 2);

      searchBest(variation, alpha, beta, ply, searchDepth, pvNode,
                 TRUE, &bestReply, excludeHere, FALSE);

      if (moveIsPseudoLegal(position, bestReply))
      {
         hashmove = bestReply;
      }

      if (hashmove != NO_MOVE && excludeMove == NO_MOVE &&
          restDepth >= getSingleMoveExtensionDepth(pvNode))
      {
         Hashentry *tableHit = getHashentry(getSharedHashtable(),
                                            variation->singlePosition.
                                            hashKey);

         if (tableHit != 0)
         {
            bestTableHit = tableHit;
         }
      }
   }

   /* Check if the conditions for a singular extension are met */
   if (bestTableHit != 0 && excludeMove == NO_MOVE && hashmove != NO_MOVE)
   {
      const int singleMoveExtensionDepth =
         getSingleMoveExtensionDepth(pvNode);
      /* const int singleMoveExtensionDepth = 8 * DEPTH_RESOLUTION; */
      const int importance =
         getHashentryImportance(bestTableHit) - HASH_DEPTH_OFFSET;
      const int flag = getHashentryFlag(bestTableHit);

      if (restDepth >= singleMoveExtensionDepth &&
          importance >= restDepth - 3 * DEPTH_RESOLUTION &&
          flag != HASHVALUE_UPPER_LIMIT)
      {
         singularExtensionNode = TRUE;
         hashEntryValue =
            calcEffectiveValue(getHashentryValue(bestTableHit), ply);
      }
   }

   if (ply >= 1)
   {
      const int moveIndex = variation->plyInfo[ply - 1].indexCurrentMove;

      variation->plyInfo[ply].killerMove3 =
         variation->counterMove1[moveIndex];
      variation->plyInfo[ply].killerMove4 =
         variation->counterMove2[moveIndex];
   }
   else
   {
      variation->plyInfo[ply].killerMove3 = NO_MOVE;
      variation->plyInfo[ply].killerMove4 = NO_MOVE;
   }

   if (ply >= 2)
   {
      const int moveIndex = variation->plyInfo[ply - 2].indexCurrentMove;

      variation->plyInfo[ply].killerMove5 =
         variation->followupMove1[moveIndex];
      variation->plyInfo[ply].killerMove6 =
         variation->followupMove2[moveIndex];
   }
   else
   {
      variation->plyInfo[ply].killerMove5 = NO_MOVE;
      variation->plyInfo[ply].killerMove6 = NO_MOVE;
   }

   initStandardMovelist(&movelist, &variation->singlePosition,
                        &variation->plyInfo[ply],
                        &variation->historyValue[0], hashmove, inCheck);

   /* Ensure that a static value for this ply is available. */
   getStaticValue(variation, ply);

   /* Loop through all moves in this node. */
   /* ------------------------------------ */
   while ((currentMove = getNextMove(&movelist)) != NO_MOVE)
   {
      const int stage = moveGenerationStage[movelist.currentStage];
      const int moveIndex = min(63, numMovesPlayed);
      const int depthIndex = min(63, rdBasic);
      const int gain =
         variation->positionalGain[historyIndex(currentMove, position)];
      const int reduction =
         (pvNode ? quietPvMoveReduction[variationType][depthIndex][moveIndex]
          : quietMoveReduction[variationType][depthIndex][moveIndex] +
          (cutNode /* || gain < 0 */ ? DEPTH_RESOLUTION : 0));
      const bool quietMove = moveIsQuiet(currentMove, position);
      const Square toSquare = getToSquare(currentMove);
      bool reduce = FALSE, nodeWasBlocked = FALSE, check;
      int value = 0, extension = 0, newDepth;

      if (variation->searchStatus != SEARCH_STATUS_RUNNING &&
          variation->iteration > 1)
      {
         return 0;
      }

      if (excludeMove != NO_MOVE && movesAreEqual(currentMove, excludeMove))
      {
         assert(excludeMove != NULLMOVE);

         continue;              /* exclude excludeMove */
      }

      variation->plyInfo[ply].indexCurrentMove =
         historyIndex(currentMove, position);
      variation->plyInfo[ply].quietMove = quietMove;
      variation->plyInfo[ply].isHashMove =
         movesAreEqual(currentMove, hashmove);

      assert(moveIsPseudoLegal(position, currentMove));
      assert(hashmove == NO_MOVE || numMovesPlayed > 0 ||
             movesAreEqual(currentMove, hashmove));

      /* Optimistic futility cuts */
      if (pvNode == FALSE && inCheck == FALSE && quietMove != FALSE &&
          !isPassedPawnMove(getFromSquare(currentMove), toSquare, position) &&
          !moveIsCastling(currentMove, position) &&
          best > VALUE_ALMOST_MATED && cutsAreAllowed && restDepth < 32)
      {
         bool moveIsRelevant = FALSE;
         const int predictedDepth = restDepth - reduction;

         if (numMovesPlayed >= quietMoveCountLimit[variationType][restDepth])
         {
            if (simpleMoveIsCheck(position, currentMove))
            {
               moveIsRelevant = TRUE;
            }
            else
            {
               continue;
            }
         }

         if (moveIsRelevant == FALSE &&
             predictedDepth <= NUM_FUTILITY_MARGIN_VALUES)
         {
            const int futLimit =
               getRefinedStaticValue(variation, ply) + gain +
               futilityMargin[predictedDepth];

            if (futLimit < beta)
            {
               if (simpleMoveIsCheck(position, currentMove))
               {
                  moveIsRelevant = TRUE;
               }
               else
               {
                  if (futLimit > best)
                  {
                     best = futLimit;
                  }

                  continue;
               }
            }
         }

         if (moveIsRelevant == FALSE &&
             predictedDepth < 4 * DEPTH_RESOLUTION &&
             seeMove(position, currentMove) < 0 &&
             simpleMoveIsCheck(position, currentMove) == FALSE)
         {
            continue;
         }
      }

      /* Execute the current move and check if it is legal. */
      /* -------------------------------------------------- */
      if (makeMoveFast(variation, currentMove) != 0 ||
          passiveKingIsSafe(&variation->singlePosition) == FALSE)
      {
         unmakeLastMove(variation);

         continue;
      }

      if (numMovesPlayed > 0 && inCheck == FALSE &&
          stage == MGS_REST && deferCount < 10 &&
          checkNodeExclusion(restDepth))
      {
         if (nodeIsInUse(position->hashKey, restDepth))
         {
            deferMove(&movelist, currentMove);
            deferCount++;
            unmakeLastMove(variation);
            continue;
         }
         else
         {
            nodeWasBlocked = setNodeUsage(position->hashKey, restDepth);
         }
      }

      /* Check the conditions for search extensions and finally */
      /* calculate the rest depth for the next ply.             */
      /* ------------------------------------------------------ */
      variation->plyInfo[ply].currentMoveIsCheck = check =
         activeKingIsSafe(&variation->singlePosition) == FALSE;

      if (check)
      {
         unmakeLastMove(variation);

         if (seeMove(position, currentMove) >= 0)
         {
            extension = DEPTH_RESOLUTION;
         }

         makeMoveFast(variation, currentMove);
      }

      if (singularExtensionNode != FALSE &&
          extension < DEPTH_RESOLUTION &&
          movesAreEqual(currentMove, hashmove))
      {
         const int limitValue = hashEntryValue - (266 * restDepth) / 256;

         assert(excludeMove == NO_MOVE);

         if (limitValue > VALUE_ALMOST_MATED &&
             limitValue < -VALUE_ALMOST_MATED)
         {
            int excludeValue;
            PlyInfo pi = variation->plyInfo[ply];

            unmakeLastMove(variation);
            excludeValue =
               searchBest(variation, limitValue - 1, limitValue, ply,
                          restDepth / 2, FALSE, cutNode, &bestReply,
                          hashmove, FALSE);
            makeMoveFast(variation, currentMove);
            variation->plyInfo[ply] = pi;

            if (excludeValue < limitValue)
            {
               extension = DEPTH_RESOLUTION;
            }
         }
      }

      newDepth = restDepth - DEPTH_RESOLUTION + extension;

      /* History pruning */
      /* --------------- */
      if (inCheck == FALSE && extension == 0 &&
          restDepth >= 3 * DEPTH_RESOLUTION && quietMove != FALSE &&
          stage != MGS_GOOD_CAPTURES_AND_PROMOTIONS &&
          movesAreEqual(currentMove,
                        variation->plyInfo[ply].killerMove1) == FALSE &&
          movesAreEqual(currentMove,
                        variation->plyInfo[ply].killerMove2) == FALSE &&
          isPassedPawnMove(toSquare, toSquare, position) == FALSE)
      {
         reduce = TRUE;
      }

      /* Recursive search */
      /* ---------------- */
      if (pvNode && numMovesPlayed == 0)
      {
         value = -searchBest(variation, -beta, -alpha, ply + 1,
                             newDepth, pvNode, FALSE, &bestReply, NO_MOVE,
                             TRUE);
      }
      else
      {
         const Move cm1 = variation->plyInfo[ply].killerMove3;
         const Move cm2 = variation->plyInfo[ply].killerMove4;
         const bool counterMove = movesAreEqual(currentMove, cm1) ||
            movesAreEqual(currentMove, cm2);
         const int effectiveReduction =
            max(0, reduction - (counterMove ? DEPTH_RESOLUTION : 0));
         const int minRestDepth = (pvNode ? DEPTH_RESOLUTION : 0);
         const int reducedRestDepth =
            max(minRestDepth, newDepth - effectiveReduction);

         bool fullDepthSearch = TRUE;

         if (reduce && effectiveReduction > 0)
         {
            value = -searchBest(variation, -alpha - 1, -alpha, ply + 1,
                                reducedRestDepth, FALSE, TRUE,
                                &bestReply, NO_MOVE, TRUE);

            if (value > alpha && effectiveReduction >= 4 * DEPTH_RESOLUTION &&
                stage != MGS_KILLER_MOVES)
            {
               const int limitedRestDepth =
                  max(DEPTH_RESOLUTION, newDepth - 2 * DEPTH_RESOLUTION);

               value = -searchBest(variation, -alpha - 1, -alpha, ply + 1,
                                   limitedRestDepth, FALSE,
                                   TRUE, &bestReply, NO_MOVE, TRUE);
            }

            fullDepthSearch = (bool) (value > alpha);
         }

         if (fullDepthSearch)
         {
            value = -searchBest(variation, -alpha - 1, -alpha, ply + 1,
                                newDepth, FALSE, !cutNode, &bestReply,
                                NO_MOVE, TRUE);

            if (pvNode && value > alpha && value < beta)
            {                   /* 2% */
               value = -searchBest(variation, -beta, -alpha, ply + 1,
                                   newDepth, TRUE, FALSE, &bestReply,
                                   NO_MOVE, TRUE);
            }
         }
      }

      assert(value >= VALUE_MATED && value <= -VALUE_MATED);

      if (nodeWasBlocked)
      {
         resetNodeUsage(position->hashKey, restDepth);
      }

      unmakeLastMove(variation);
      numMovesPlayed++;

      if (quietMove && inCheck == FALSE)
      {
         quietMoveIndex[quietMoveCount++] =
            historyIndex(currentMove, position);
      }

      /* Calculate the parameters controlling the search tree. */
      /* ----------------------------------------------------- */
      if (value > best)
      {                         /* 32% */
         best = value;

         if (best > alpha)
         {                      /* 63% */
            alpha = best;
            *bestMove = currentMove;

            if (pvNode)
            {
               appendMoveToPv(&(variation->plyInfo[ply].pv),
                              &(variation->plyInfo[ply - 1].pv), currentMove);
            }

            if (best >= beta)
            {                   /* 99% */
               break;
            }
         }
      }
   }

   /* No legal move was found. Check if it's mate or stalemate. */
   /* --------------------------------------------------------- */
   if (best == VALUE_MATED)
   {
      if (excludeMove != NO_MOVE && excludeMove != NULLMOVE)
      {
         return beta - 1;
      }

      if (inCheck)
      {
         /* mate */

         best = VALUE_MATED + ply;
      }
      else
      {
         /* stalemate */

         best = variation->drawScore[position->activeColor];
      }
   }

   /* Calculate the parameters controlling the move ordering. */
   /* ------------------------------------------------------- */
   if (*bestMove != NO_MOVE && moveIsQuiet(*bestMove, position) &&
       inCheck == FALSE &&
       (excludeMove == NO_MOVE || excludeMove == NULLMOVE))
   {
      Move killerMove = *bestMove;
      const Piece movingPiece = position->piece[getFromSquare(killerMove)];
      const int index = historyIndex(*bestMove, position);
      const UINT16 bestMoveValue = (UINT16)
         (variation->historyValue[index] +
          ((HISTORY_MAX - variation->historyValue[index]) * restDepth) / 256);
      int i;

      for (i = 0; i < quietMoveCount; i++)
      {
         const int historyIdx = quietMoveIndex[i];

         variation->historyValue[historyIdx] = (UINT16)
            (variation->historyValue[historyIdx] -
             (variation->historyValue[historyIdx] * restDepth) / 128);
         assert(variation->historyValue[historyIdx] <= HISTORY_MAX);
      }

      variation->historyValue[index] = bestMoveValue;
      assert(variation->historyValue[index] <= HISTORY_MAX);

      setMoveValue(&killerMove, movingPiece);
      registerKillerMove(&variation->plyInfo[ply], killerMove);
      updateCounterMoves(variation, ply, killerMove);

      if (ply >= 2 && variation->plyInfo[ply - 1].isHashMove)
      {
         updateFollowupMoves(variation, ply, killerMove);
      }
   }

 storeResult:

   /* Store the value in the transposition table. */
   /* ------------------------------------------- */
   if ((excludeMove == NO_MOVE || excludeMove == NULLMOVE) &&
       variation->searchStatus == SEARCH_STATUS_RUNNING)
   {
      if (best >= beta)
      {
         hashentryFlag = HASHVALUE_LOWER_LIMIT;
      }
      else
      {
         hashentryFlag = (pvNode && *bestMove != NO_MOVE ?
                          HASHVALUE_EXACT : HASHVALUE_UPPER_LIMIT);
      }

      setHashentry(getSharedHashtable(), hashKey,
                   calcHashtableValue(best, ply),
                   (UINT8) (restDepth + HASH_DEPTH_OFFSET),
                   packedMove(*bestMove), hashentryFlag,
                   (INT16) getStaticValue(variation, ply));

#ifdef SEND_HASH_ENTRIES
      if (hashentryFlag == HASHVALUE_EXACT &&
          restDepth >= minPvHashEntrySendDepth &&
          ply <= maxPvHashEntrySendHeight)
      {
         const long timestamp = getTimestamp();
         const long elapsedTime = timestamp - variation->startTime;
         const long intervalTime = timestamp - variation->hashSendTimestamp;

         if (elapsedTime >= minPvHashEntrySendTime &&
             intervalTime >= pvHashEntriesSendInterval)
         {
            Hashentry entry =
               constructHashEntry(hashKey, calcHashtableValue(best, ply),
                                  (INT16) getStaticValue(variation, ply),
                                  (UINT8) (restDepth + HASH_DEPTH_OFFSET),
                                  packedMove(*bestMove), 0, hashentryFlag);

            sendHashentry(&entry);
            variation->hashSendTimestamp = timestamp;
         }
      }
#endif
   }

   return best;
}

void copyPvFromHashtable(Variation * variation, const int pvIndex,
                         PrincipalVariation * pv, const Move bestBaseMove)
{
   Move bestMove = NO_MOVE;

   if (pvIndex == 0)
   {
      bestMove = bestBaseMove;
   }
   else
   {
      Hashentry *tableHit = getHashentry(getSharedHashtable(),
                                         variation->singlePosition.hashKey);

      if (tableHit != NULL)
      {
         Move currentMove = (Move) getHashentryMove(tableHit);

         if (moveIsLegal(&variation->singlePosition, currentMove))
         {
            bestMove = (Move) getHashentryMove(tableHit);
         }
      }
   }

   if (bestMove != NO_MOVE && pvIndex < MAX_DEPTH)
   {
      pv->move[pvIndex] = (UINT16) bestMove;
      pv->move[pvIndex + 1] = (UINT16) NO_MOVE;
      pv->length = pvIndex + 1;
      makeMove(variation, bestMove);
      copyPvFromHashtable(variation, pvIndex + 1, pv, bestBaseMove);
      unmakeLastMove(variation);
   }
   else
   {
      pv->move[pvIndex] = (UINT16) NO_MOVE;
      pv->length = pvIndex;
   }
}

static void copyPvToHashtable(Variation * variation,
                              PrincipalVariation * pv, const int pvIndex)
{
   Move move = (Move) pv->move[pvIndex];

   if (pvIndex < pv->length && moveIsLegal(&variation->singlePosition, move))
   {
      UINT8 importance = (UINT8) HASH_DEPTH_OFFSET;
      bool entryExists = FALSE;
      Move bestMove = NO_MOVE;
      Hashentry *tableHit = getHashentry(getSharedHashtable(),
                                         variation->singlePosition.hashKey);

      if (tableHit != 0)
      {
         bestMove = (Move) getHashentryMove(tableHit);
         importance = max(importance, getHashentryImportance(tableHit));

         if (bestMove != NO_MOVE && movesAreEqual(bestMove, move))
         {
            entryExists = TRUE;
         }
      }

      if (entryExists == FALSE)
      {
         UINT8 hashentryFlag = HASHVALUE_LOWER_LIMIT;

         /* Store the move in the transposition table. */
         /* ------------------------------------------- */

         setHashentry(getSharedHashtable(),
                      variation->singlePosition.hashKey, VALUE_MATED,
                      importance, packedMove(move),
                      hashentryFlag, getEvalValue(variation));
      }

      makeMove(variation, move);
      copyPvToHashtable(variation, pv, pvIndex + 1);
      unmakeLastMove(variation);
   }
}

static void registerBestMove(Variation * variation, Move * move,
                             const int value)
{
   if (variation->searchStatus == SEARCH_STATUS_RUNNING)
   {
      setMoveValue(move, value);
      variation->bestBaseMove = *move;

      if (variation->iteration > 4 && variation->numberOfCurrentBaseMove > 1)
      {
         variation->bestMoveChangeCount += 256;
      }
   }
}

static int getBaseMoveValue(Variation * variation, const Move move,
                            const int alpha, const int beta,
                            const bool fullWindow)
{
   int depth = DEPTH_RESOLUTION * variation->iteration;
   int value;
   Move bestReply;

   assert(alpha >= VALUE_MATED);
   assert(alpha <= -VALUE_MATED);
   assert(beta >= VALUE_MATED);
   assert(beta <= -VALUE_MATED);
   assert(alpha < beta);

   makeMoveFast(variation, move);

   if (variation->nodes > GUI_NODE_COUNT_MIN && variation->threadNumber == 0)
   {
      getGuiSearchMutex();
      handleSearchEvent(SEARCHEVENT_NEW_BASEMOVE, variation);
      releaseGuiSearchMutex();
   }

   if (activeKingIsSafe(&variation->singlePosition) == FALSE)
   {
      variation->plyInfo[0].currentMoveIsCheck = TRUE;
      depth += DEPTH_RESOLUTION;
   }
   else
   {
      variation->plyInfo[0].currentMoveIsCheck = FALSE;
   }

   if (fullWindow)
   {
      value = -searchBest(variation, -beta, -alpha, 1,
                          depth - DEPTH_RESOLUTION, TRUE, FALSE,
                          &bestReply, NO_MOVE, TRUE);
   }
   else
   {
      value = -searchBest(variation, -alpha - 1, -alpha, 1,
                          depth - DEPTH_RESOLUTION, FALSE, TRUE,
                          &bestReply, NO_MOVE, TRUE);

      if (value > alpha)
      {
         value = -searchBest(variation, -beta, -alpha, 1,
                             depth - DEPTH_RESOLUTION, TRUE,
                             FALSE, &bestReply, NO_MOVE, TRUE);
      }
   }

   unmakeLastMove(variation);

   return value;
}

int getPvScoreType(int value, int alpha, int beta)
{
   if (value <= alpha)
   {
      return HASHVALUE_UPPER_LIMIT;
   }
   else if (value >= beta)
   {
      return HASHVALUE_LOWER_LIMIT;
   }
   else
   {
      return HASHVALUE_EXACT;
   }
}

static void sendPvInfo(Variation * variation, const int eventType)
{
   if ((variation->nodes > GUI_NODE_COUNT_MIN ||
        eventType == SEARCHEVENT_PLY_FINISHED) &&
       variation->threadNumber == 0)
   {
      int i;

      getGuiSearchMutex();

      for (i = 0; i < numPvs && variation->pv[i].length > 0; i++)
      {
         variation->pvId = i;
         handleSearchEvent(eventType, variation);
      }

      releaseGuiSearchMutex();
   }
}

static void exploreBaseMoves(Variation * variation, Movelist * basemoves,
                             const int aspirationWindow)
{
   const int previousBest = variation->previousBest;
   const int ply = 0;
   Position *position = &variation->singlePosition;
   const bool fullWindow = (bool) (variation->iteration <= 3);
   int window = aspirationWindow, best;
   bool exactValueFound = FALSE;
   const int staticValue = getEvalValue(variation);
   int alpha =
      (fullWindow ? VALUE_MATED : max(VALUE_MATED, previousBest - window));
   int beta =
      (fullWindow ? -VALUE_MATED : min(-VALUE_MATED, previousBest + window));

   variation->failingLow = FALSE;
   variation->selDepth = variation->iteration;
   initializePvsOfVariation(variation);

   do
   {
      int pvCount = 0, worstValue = VALUE_MATED;
      const int numPvLimit = min(basemoves->numberOfMoves, numPvs);

      initializeMoveValues(basemoves);
      resetPvsOfVariation(variation);
      best = VALUE_MATED;

      for (variation->numberOfCurrentBaseMove = 1;
           variation->numberOfCurrentBaseMove <= basemoves->numberOfMoves;
           variation->numberOfCurrentBaseMove++)
      {
         int value;
         const int icm = variation->numberOfCurrentBaseMove - 1;
         const bool searchBelowBest = fullWindow || numPvs > 1;
         const bool pvNode = (bool) (icm == 0 || searchBelowBest);
         const int searchAlpha = (searchBelowBest ? alpha : max(alpha, best));

         resetPlyInfo(variation);
         variation->currentBaseMove = basemoves->moves[icm];
         variation->plyInfo[ply].indexCurrentMove =
            historyIndex(variation->currentBaseMove, position);

         value = getBaseMoveValue(variation, basemoves->moves[icm],
                                  searchAlpha, beta, pvNode);

         if (variation->searchStatus != SEARCH_STATUS_RUNNING &&
             variation->iteration > 1)
         {
            break;
         }

         if (icm == 0 || value > searchAlpha)
         {
            PrincipalVariation pv;

            setMoveValue(&basemoves->moves[icm], value);
            pv.score = value;
            pv.scoreType = getPvScoreType(value, searchAlpha, beta);
            appendMoveToPv(&(variation->plyInfo[0].pv), &pv,
                           basemoves->moves[icm]);
            addPvByScore(variation, &pv);

            if (++pvCount >= numPvLimit)
            {
               sendPvInfo(variation, SEARCHEVENT_NEW_PV);
            }

            if (icm == 0 || value > best)
            {
               registerBestMove(variation, &basemoves->moves[icm], value);

               if (value > best && value < beta)
               {
                  variation->completePv = pv;
               }
            }
         }

         if (value > best)
         {
            best = value;

            if (value >= beta && numPvs == 1)
            {
               break;
            }
         }
      }

      /* Store the value in the transposition table. */
      /* ------------------------------------------- */
      if (variation->searchStatus == SEARCH_STATUS_RUNNING &&
          best > alpha && best < beta)
      {
         UINT8 hashentryFlag;
         const int depth = DEPTH_RESOLUTION * variation->iteration;
         const Move bestMove = variation->bestBaseMove;

         if (best > alpha)
         {
            hashentryFlag =
               (best >= beta ? HASHVALUE_LOWER_LIMIT : HASHVALUE_EXACT);
         }
         else
         {
            hashentryFlag = HASHVALUE_UPPER_LIMIT;
         }

         setHashentry(getSharedHashtable(), position->hashKey,
                      calcHashtableValue(best, ply),
                      (UINT8) (depth + HASH_DEPTH_OFFSET),
                      packedMove(bestMove), hashentryFlag,
                      (INT16) staticValue);
      }

      worstValue = (numPvs == 1 ? best :
                    max(VALUE_MATED + 3, variation->pv[numPvs - 1].score));

      if (best >= beta)
      {
         beta = min(-VALUE_MATED, best + window);
      }
      else if (worstValue <= alpha && worstValue > VALUE_MATED + 2)
      {
         alpha = max(VALUE_MATED, alpha - window);
         variation->failingLow = TRUE;
      }
      else
      {
         exactValueFound = TRUE;        /* exact value found */
      }

      window = window + window / 2;

      sortMoves(basemoves);

      assert(fullWindow == TRUE ||
             movesAreEqual(basemoves->moves[0], variation->bestBaseMove));

      if (variation->threadNumber == 0)
      {
         copyPvToHashtable(variation, &variation->completePv, 0);
      }
   }
   while (variation->searchStatus == SEARCH_STATUS_RUNNING &&
          exactValueFound == FALSE);

   variation->pv[0].score = getMoveValue(variation->bestBaseMove);

   if (variation->threadNumber == 0 && variation->iteration > 1 &&
       variation->completePv.length <= 1)
   {
      PrincipalVariation tmpPv;

      copyPvFromHashtable(variation, 0, &tmpPv, variation->bestBaseMove);

      if (tmpPv.length > 1)
      {
         variation->completePv = tmpPv;
      }
   }

   sendPvInfo(variation, SEARCHEVENT_PLY_FINISHED);
}

static void initializePawnHashtable(PawnHashInfo * pawnHashtable)
{
   int i;

   for (i = 0; i < PAWN_HASHTABLE_SIZE; i++)
   {
      pawnHashtable[i].hashKey = 0;
   }
}

static void initializeKingsafetyHashtable(KingSafetyHashInfo *
                                          kingsafetyHashtable)
{
   int i;

   for (i = 0; i < KINGSAFETY_HASHTABLE_SIZE; i++)
   {
      kingsafetyHashtable[i].hashKey = 0;
   }
}

static void updatePieceValues()
{
   maxPieceValue[WHITE_QUEEN] = maxPieceValue[BLACK_QUEEN] =
      max(getOpeningValue(basicValue[WHITE_QUEEN]),
          getEndgameValue(basicValue[WHITE_QUEEN])) - 42;
   maxPieceValue[WHITE_ROOK] = maxPieceValue[BLACK_ROOK] =
      max(getOpeningValue(basicValue[WHITE_ROOK]),
          getEndgameValue(basicValue[WHITE_ROOK]));
   maxPieceValue[WHITE_BISHOP] = maxPieceValue[BLACK_BISHOP] =
      max(getOpeningValue(basicValue[WHITE_BISHOP]),
          getEndgameValue(basicValue[WHITE_BISHOP]));
   maxPieceValue[WHITE_KNIGHT] = maxPieceValue[BLACK_KNIGHT] =
      max(getOpeningValue(basicValue[WHITE_KNIGHT]),
          getEndgameValue(basicValue[WHITE_KNIGHT]));
   maxPieceValue[WHITE_PAWN] = maxPieceValue[BLACK_PAWN] =
      max(getOpeningValue(basicValue[WHITE_PAWN]),
          getEndgameValue(basicValue[WHITE_PAWN]));
}

Move search(Variation * variation, Movelist * acceptableSolutions)
{
   Movelist movelist;
   long timeTarget;
   int stableIterationCount = 0;
   int stableBestMoveCount = 0;
   Move bestMove = NO_MOVE;
   UINT64 nodeCount = 0;
   int iv1 = 0, iv2 = 0, iv3 = 0;

   if (resetSharedHashtable)
   {
      resetHashtable(getSharedHashtable());
      initializePawnHashtable(variation->pawnHashtable);
      initializeKingsafetyHashtable(variation->kingsafetyHashtable);
      resetSharedHashtable = FALSE;
   }

   resetHistoryValues(variation);
   resetGainValues(variation);

   variation->ply = 0;
   variation->ownColor = variation->singlePosition.activeColor;
   variation->nodes = variation->nodesAtTimeCheck = 0;
   variation->startTimeProcess = getProcessTimestamp();
   variation->timestamp = variation->startTime + 1;
   variation->hashSendTimestamp = variation->startTime;
   variation->tbHits = 0;
   variation->numPvUpdates = 0;
   variation->terminateSearchOnPonderhit = FALSE;
   variation->previousBest = getStaticValue(variation, 0);
   variation->bestBaseMove = NO_MOVE;
   variation->failingLow = FALSE;
   movelist.positionalGain = &(variation->positionalGain[0]);
   initializePlyInfo(variation);
   getLegalMoves(variation, &movelist);

#ifdef TRACE_EVAL
   getValue(&variation->singlePosition, VALUE_MATED, -VALUE_MATED,
            variation->pawnHashtable, variation->kingsafetyHashtable);
#endif

#ifdef USE_BOOK
   if (globalBook.indexFile != NULL && globalBook.moveFile != NULL &&
       &variation->singlePosition->moveNumber <= 12)
   {
      Move bookMove = getBookmove(&globalBook,
                                  &variation->singlePosition->hashKey,
                                  &movelist);

      if (bookMove != NO_MOVE)
      {
         variation->bestBaseMove = bookMove;
         variation->searchStatus = SEARCH_STATUS_TERMINATE;
         variation->finishTime = getTimestamp();

         if (variation->handleSearchEvent != 0)
         {
            getGuiSearchMutex();
            variation->handleSearchEvent(SEARCHEVENT_SEARCH_FINISHED,
                                         variation);
            releaseGuiSearchMutex();
         }

         variation->nodes = 0;

         return variation->bestBaseMove;
      }
   }
#endif

   variation->numberOfBaseMoves = movelist.numberOfMoves;
   setMoveValue(&variation->bestBaseMove, VALUE_MATED);

   for (variation->iteration = 1; variation->iteration <= MAX_DEPTH;
        variation->iteration++)
   {
      long calculationTime;
      int iterationValue, aspirationWindow;

      variation->ply = 0;

      aspirationWindow =
         min(12, max(8, (abs(iv1 - iv2) + abs(iv2 - iv3)) / 2));
      exploreBaseMoves(variation, &movelist, aspirationWindow);
      calculationTime =
         (unsigned long) (getTimestamp() - variation->startTime);

      if (movesAreEqual(variation->bestBaseMove, bestMove))
      {
         stableBestMoveCount++;
      }
      else
      {
         stableBestMoveCount = 0;
      }

      bestMove = variation->bestBaseMove;
      iv3 = iv2;
      iv2 = iv1;
      iv1 = iterationValue = getMoveValue(variation->bestBaseMove);

      variation->previousBest = iterationValue;

      assert(calculationTime >= 0);

      if (acceptableSolutions != 0 &&
          listContainsMove(acceptableSolutions, variation->bestBaseMove))
      {
         stableIterationCount++;

         if (stableIterationCount == 1)
         {
            nodeCount = variation->nodes;
         }
      }
      else
      {
         stableIterationCount = 0;
         nodeCount = variation->nodes;
      }

      /* Check for a fail low. */
      /* --------------------- */

      if (variation->numberOfBaseMoves == 1)
      {
         timeTarget = (19 * variation->timeTarget) / 256;
      }
      else
      {
         const int timeWeight = 160 +
            (223 * variation->bestMoveChangeCount) / 256;

         timeTarget = (timeWeight * variation->timeTarget) / 256;
      }

      variation->bestMoveChangeCount =
         (17 * variation->bestMoveChangeCount) / 32;

      getGuiSearchMutex();

      if (variation->threadNumber == 0 &&
          variation->searchStatus == SEARCH_STATUS_RUNNING &&
          variation->iteration > 8 && variation->timeLimit != 0 &&
          calculationTime >= timeTarget)
      {
#ifdef DEBUG_THREAD_COORDINATION
         logDebug
            ("Time target reached (%lu/%lu ms, %lu%%)).\n",
             calculationTime, variation->timeTarget,
             (calculationTime * 100) / variation->timeTarget);
#endif

         if (variation->ponderMode)
         {
            variation->terminateSearchOnPonderhit = TRUE;

#ifdef DEBUG_THREAD_COORDINATION
            logDebug("Setting ponder termination flag.\n");
#endif
         }
         else
         {
            variation->searchStatus = SEARCH_STATUS_TERMINATE;

#ifdef DEBUG_THREAD_COORDINATION
            logDebug("Terminating search.\n");
#endif
         }
      }

      if (variation->searchStatus == SEARCH_STATUS_RUNNING &&
          (getMoveValue(variation->bestBaseMove) <=
           VALUE_MATED + variation->iteration ||
           getMoveValue(variation->bestBaseMove) >=
           -VALUE_MATED - variation->iteration))
      {
#ifdef DEBUG_THREAD_COORDINATION
         logDebug("Best value out of bounds (%d). Terminating search.\n",
                  getMoveValue(variation->bestBaseMove));
#endif
         variation->searchStatus = SEARCH_STATUS_TERMINATE;
      }

      if (variation->searchStatus == SEARCH_STATUS_RUNNING &&
          variation->iteration == MAX_DEPTH)
      {
#ifdef DEBUG_THREAD_COORDINATION
         logDebug("Max depth reached. Terminating search.\n");
#endif
         variation->searchStatus = SEARCH_STATUS_TERMINATE;
      }

      if (acceptableSolutions != 0 && stableIterationCount >= 1 &&
          (getMoveValue(variation->bestBaseMove) > 20000 ||
           (stableIterationCount >= 2 &&
            (getMoveValue(variation->bestBaseMove) >= 25 ||
             (getTimestamp() - variation->startTime) >= 3000))))
      {
#ifdef DEBUG_THREAD_COORDINATION
         logDebug("Solution found (value=%d). Terminating search.\n",
                  getMoveValue(variation->bestBaseMove));
#endif
         variation->searchStatus = SEARCH_STATUS_TERMINATE;
      }

      if (variation->searchStatus != SEARCH_STATUS_RUNNING)
      {
         variation->terminateSearchOnPonderhit = TRUE;
         variation->searchStatus = SEARCH_STATUS_TERMINATE;

         variation->finishTime = getTimestamp();
         variation->finishTimeProcess = getProcessTimestamp();

         if (variation->threadNumber == 0)
         {
            incrementDate(getSharedHashtable());
            handleSearchEvent(SEARCHEVENT_SEARCH_FINISHED, variation);
         }
      }

      if (variation->searchStatus != SEARCH_STATUS_RUNNING)
      {
#ifdef DEBUG_THREAD_COORDINATION
         logReport
            ("search status != SEARCH_STATUS_RUNNING -> exiting search.\n",
             getMoveValue(variation->bestBaseMove));
#endif

         releaseGuiSearchMutex();
         break;
      }

      releaseGuiSearchMutex();
   }

   variation->nodes = nodeCount;

   if (statCount1 != 0 || statCount2 != 0)
   {
      logReport("statCount1=%lld statCount2=%lld (%lld%%) \n",
                statCount1, statCount2,
                (statCount2 * 100) / max(1, statCount1));
   }

   return variation->bestBaseMove;
}

/* #define DEBUG_FUT_VALUES */

static void initializeArrays(void)
{
   int i, j;

   for (i = 0; i < 64; i++)
   {
      for (j = 0; j < 64; j++)
      {
         if (i == 0 || j == 0)
         {
            quietPvMoveReduction[0][i][j] = quietPvMoveReduction[1][i][j] =
               quietMoveReduction[0][i][j] = quietMoveReduction[1][i][j] = 0;
         }
         else
         {
            const double baseFactor = log((double) (i)) * log((double) (j));
            const double pvReductionNonImproving = baseFactor / 1.00;
            const double pvReductionImproving = baseFactor / 1.55;
            const double nonPvReductionNonImproving = baseFactor / 0.65;
            const double nonPvReductionImproving = baseFactor / 0.68;

            quietPvMoveReduction[0][i][j] = (int) pvReductionNonImproving;
            quietPvMoveReduction[1][i][j] = (int) pvReductionImproving;
            quietMoveReduction[0][i][j] = (int) nonPvReductionNonImproving;
            quietMoveReduction[1][i][j] = (int) nonPvReductionImproving;

            /*logDebug("qmr[%d][%d]=%d\n", i,j, quietPvMoveReduction[i][j]); */
         }
      }
   }

   /*getKeyStroke(); */

   for (i = 0; i < 32; i++)
   {
      quietMoveCountLimit[0][i] = (48 * i * i) / 512 + (250 * i) / 512 + 2;
      quietMoveCountLimit[1][i] = (75 * i * i) / 512 + (640 * i) / 512 + 3;

#ifdef DEBUG_FUT_VALUES
      logDebug("mcl[%d]=%d\n", i, quietMoveCountLimit[i]);
#endif
   }

   for (i = 0; i <= NUM_FUTILITY_MARGIN_VALUES; i++)
   {
      futilityMargin[i] = (3420 * i) / 64 - 12800 / 256;

#ifdef DEBUG_FUT_VALUES
      if (j <= 2)
      {
         logDebug("fm[%d][%d]=%d\n", i, j, futilityMargin[i][j]);
      }
#endif
   }

   updatePieceValues();

#ifdef DEBUG_FUT_VALUES
   getKeyStroke();
#endif
}

int initializeModuleSearch(void)
{
   initializeArrays();

   return 0;
}

int testModuleSearch(void)
{
   return 0;
}
