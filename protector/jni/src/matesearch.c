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
#include <string.h>
#include <assert.h>
#include "matesearch.h"
#include "io.h"
#include "movegeneration.h"
#include "hash.h"
#include "coordination.h"

static int searchMate(Variation * variation, int alpha, int beta,
                      const int ply, const int restDepth,
                      PrincipalVariation * pv)
{
   const int oldAlpha = alpha;
   Position *position = &variation->singlePosition;
   const bool check = activeKingIsSafe(position) == FALSE;
   int best = VALUE_MATED;
   Movelist movelist;
   Hashentry *tableHit = NULL;
   UINT8 hashentryFlag;
   int i, historyLimit;
   Move hashmove = NO_MOVE;
   Move bestMove = NO_MOVE;
   Move currentMove;
   PrincipalVariation newPv;

   newPv.length = pv->length = 0;

   historyLimit = POSITION_HISTORY_OFFSET + variation->ply -
      variation->singlePosition.halfMoveClock;

   assert(historyLimit >= 0);

   for (i = POSITION_HISTORY_OFFSET + variation->ply - 4;
        i >= historyLimit; i -= 2)
   {
      if (variation->singlePosition.hashKey == variation->positionHistory[i])
      {
         return 0;
      }
   }

   /* Probe the transposition table */
   /* ----------------------------- */
   tableHit = getHashentry(getSharedHashtable(),
                           variation->singlePosition.hashKey);

   if (tableHit != NULL)
   {
      const int importance = getHashentryImportance(tableHit);

      hashmove = (Move) getHashentryMove(tableHit);

      if (hashmove != NO_MOVE)
      {
         if (moveIsPseudoLegal(position, hashmove))
         {
            assert(moveIsLegal(position, hashmove));

            appendMoveToPv(&newPv, pv, hashmove);
         }
         else
         {
            hashmove = NO_MOVE;
         }
      }

      if (restDepth <= importance)
      {                         /* 99% */
         const int value = getHashentryValue(tableHit);
         const int hashValue = calcEffectiveValue(value, ply);
         const int flag = getHashentryFlag(tableHit);

         switch (flag)
         {
         case HASHVALUE_UPPER_LIMIT:
            if (hashValue <= alpha)
            {
               return hashValue;
            }
            break;

         case HASHVALUE_EXACT:
            return hashValue;

         case HASHVALUE_LOWER_LIMIT:
            if (hashValue >= beta)
            {
               return hashValue;
            }
            break;

         default:;
         }
      }
   }

   if (ply >= 2)
   {
      variation->plyInfo[ply].killerMove3 =
         variation->plyInfo[ply - 2].killerMove1;
      variation->plyInfo[ply].killerMove4 =
         variation->plyInfo[ply - 2].killerMove2;
   }
   else
   {
      variation->plyInfo[ply].killerMove3 = NO_MOVE;
      variation->plyInfo[ply].killerMove4 = NO_MOVE;
   }

   if (restDepth == 1)
   {
      initCheckMovelist(&movelist, position, &variation->historyValue[0]);
   }
   else
   {
      movelist.positionalGain = &(variation->positionalGain[0]);
      initStandardMovelist(&movelist, &variation->singlePosition,
                           &variation->plyInfo[ply],
                           &variation->historyValue[0], hashmove, check);
   }

   initializePlyInfo(variation);

   while ((currentMove = getNextMove(&movelist)) != NO_MOVE)
   {
      int value;

      variation->nodes++;

      if (makeMoveFast(variation, currentMove) != 0 ||
          passiveKingIsSafe(&variation->singlePosition) == FALSE ||
          (restDepth == 1 && activeKingIsSafe(&variation->singlePosition)))
      {
         unmakeLastMove(variation);

         continue;
      }

      if (restDepth > 0)
      {
         if (restDepth == 1)
         {
            if (kingCanEscape(position))
            {
               value = 0;
            }
            else
            {
               value = -(VALUE_MATED + ply + 1);
            }
         }
         else
         {
            value = -searchMate(variation, -beta, -alpha, ply + 1,
                                restDepth - 1, &newPv);
         }
      }
      else
      {
         value = 0;
      }

      unmakeLastMove(variation);

      if (value > best)
      {
         best = value;
         appendMoveToPv(&newPv, pv, currentMove);

         if (best > alpha)
         {
            alpha = best;
            bestMove = currentMove;

            if (ply == 0)
            {
               if (variation->threadNumber == 0)
               {
                  getGuiSearchMutex();
                  handleSearchEvent(SEARCHEVENT_NEW_PV, variation);
                  releaseGuiSearchMutex();
               }
            }

            if (best >= beta)
            {
               goto finish;
            }
         }
      }
   }

   if (best == VALUE_MATED)
   {
      if (check)
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

 finish:

   if (bestMove != NO_MOVE)
   {
      if (position->piece[getToSquare(bestMove)] == NO_PIECE &&
          getNewPiece(bestMove) == NO_PIECE &&
          (getToSquare(bestMove) != position->enPassantSquare ||
           pieceType(position->piece[getFromSquare(bestMove)]) != PAWN))
      {
         Move killerMove = bestMove;
         const Piece movingPiece = position->piece[getFromSquare(killerMove)];
         const int index = historyIndex(bestMove, position);

         setMoveValue(&killerMove, movingPiece);
         registerKillerMove(&variation->plyInfo[ply], killerMove);

         variation->historyValue[index] = (UINT16)
            (variation->historyValue[index] + (restDepth * restDepth));

         if (variation->historyValue[index] > HISTORY_MAX)
         {
            shrinkHistoryValues(variation);
         }
      }
   }

   /* Store the value in the transposition table. */
   /* ------------------------------------------- */
   if (best > oldAlpha)
   {
      hashentryFlag =
         (best >= beta ? HASHVALUE_LOWER_LIMIT : HASHVALUE_EXACT);
   }
   else
   {
      hashentryFlag = HASHVALUE_UPPER_LIMIT;
   }

   setHashentry(getSharedHashtable(), position->hashKey,
                calcHashtableValue(best, ply), (INT8) restDepth,
                packedMove(bestMove), hashentryFlag, 0);

   return best;
}

static int searchBaseMoves(Variation * variation, const int alpha,
                           const int beta, const int restDepth,
                           Movelist * solutions)
{
   Position *position = &variation->singlePosition;
   const bool check = activeKingIsSafe(position) == FALSE;
   int best = VALUE_MATED, ply = 0;
   Movelist movelist;
   Move hashmove = NO_MOVE;
   Move currentMove;
   PrincipalVariation pv;

   if (restDepth == 1)
   {
      initCheckMovelist(&movelist, position, &variation->historyValue[0]);
   }
   else
   {
      movelist.positionalGain = &(variation->positionalGain[0]);
      initStandardMovelist(&movelist, &variation->singlePosition,
                           &variation->plyInfo[ply],
                           &variation->historyValue[0], hashmove, check);
   }

   initializePlyInfo(variation);

   while ((currentMove = getNextMove(&movelist)) != NO_MOVE)
   {
      int value;

      variation->nodes++;

      if (makeMoveFast(variation, currentMove) != 0 ||
          passiveKingIsSafe(&variation->singlePosition) == FALSE ||
          (restDepth == 1 && activeKingIsSafe(&variation->singlePosition)))
      {
         unmakeLastMove(variation);

         continue;
      }

      value =
         -searchMate(variation, -beta, -alpha, ply + 1, restDepth - 1, &pv);

      unmakeLastMove(variation);

      if (value >= best)
      {
         best = variation->pv[0].score = value;
         setMoveValue(&currentMove, value);
         appendMoveToPv(&pv, &variation->pv[0], currentMove);

         if (best > alpha)
         {
            variation->bestBaseMove = currentMove;
            solutions->moves[solutions->numberOfMoves++] = currentMove;

            if (variation->threadNumber == 0)
            {
               getGuiSearchMutex();
               handleSearchEvent(SEARCHEVENT_NEW_PV, variation);
               releaseGuiSearchMutex();
            }
         }
      }
   }

   if (best == VALUE_MATED)
   {
      if (check)
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

   return best;
}

void searchForMate(Variation * variation, Movelist * foundSolutions,
                   int numMoves)
{
   int numSolutions = 0, i;

   variation->startTime = getTimestamp();
   variation->startTimeProcess = getProcessTimestamp();
   variation->timestamp = variation->startTime + 1;
   resetHashtable(getSharedHashtable());
   getLegalMoves(variation, foundSolutions);
   variation->numberOfBaseMoves = foundSolutions->numberOfMoves;
   variation->searchStatus = SEARCH_STATUS_RUNNING;

   foundSolutions->numberOfMoves = 0;
   resetHistoryValues(variation);

   for (variation->iteration = 1; variation->iteration <= numMoves;
        variation->iteration++)
   {
      const int restDepth = 2 * variation->iteration - 1;
      const int beta = -VALUE_MATED - restDepth;

      searchBaseMoves(variation, beta - 1, beta, restDepth, foundSolutions);
   }

   variation->finishTime = getTimestamp();
   variation->finishTimeProcess = getProcessTimestamp();
   variation->searchStatus = SEARCH_STATUS_TERMINATE;

   for (i = 0; i < foundSolutions->numberOfMoves; i++)
   {
      if (getMoveValue(foundSolutions->moves[i]) >=
          -VALUE_MATED - 2 * numMoves + 1)
      {
         numSolutions++;
      }
   }

   foundSolutions->numberOfMoves = numSolutions;

   getGuiSearchMutex();

   if (variation->threadNumber == 0 &&
       variation->searchStatus == SEARCH_STATUS_TERMINATE)
   {
      handleSearchEvent(SEARCHEVENT_SEARCH_FINISHED, variation);
   }

   releaseGuiSearchMutex();
}

int initializeModuleMatesearch()
{
   return 0;
}

int testModuleMatesearch()
{
   return 0;
}
