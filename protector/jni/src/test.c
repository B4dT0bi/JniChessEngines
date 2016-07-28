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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "io.h"
#include "pgn.h"
#include "coordination.h"
#include "evaluation.h"

extern bool resetSharedHashtable;

void handleCliSearchEvent(int eventId, Variation * variation)
{
   long time;
   char *pvMoves;

   switch (eventId)
   {
   case SEARCHEVENT_SEARCH_FINISHED:
      time = variation->finishTimeProcess - variation->startTimeProcess;

      logReport("%lld nodes in %ld msec\n", getNodeCount(), time);

      if (time > 1000)
      {
         logReport("%ld knps\n", getNodeCount() / max(1, time));
      }

      break;

   case SEARCHEVENT_PLY_FINISHED:
   case SEARCHEVENT_NEW_PV:
      pvMoves = getPrincipalVariation(variation);
      dumpPv(variation->iteration,
             (getTimestamp() - variation->startTime), pvMoves,
             variation->pv[0].score, variation->nodes,
             variation->startPosition.activeColor);
      free(pvMoves);
      break;

   default:
      break;
   }
}

static bool solveMateProblem(SearchTask * entry)
{
   bool result = TRUE;
   int i;

   completeTask(entry);

   if (entry->solutions.numberOfMoves !=
       entry->calculatedSolutions.numberOfMoves)
   {
      result = FALSE;
   }

   for (i = 0; i < entry->solutions.numberOfMoves; i++)
   {
      if (listContainsMove
          (&entry->calculatedSolutions, entry->solutions.moves[i]) == FALSE)
      {
         result = FALSE;
      }
   }

   return result;
}

static bool solveBestMoveProblem(SearchTask * entry)
{
   completeTask(entry);

   return listContainsMove(&entry->solutions, entry->bestMove);
}

static bool dumpEvaluation(SearchTask * entry)
{
   EvaluationBase base;

   prepareSearch(entry->variation);
   getValue(&entry->variation->startPosition,
            &base,
            entry->variation->pawnHashtable,
            entry->variation->kingsafetyHashtable);

   return TRUE;
}

int processTestsuite(const char *filename)
{
   PGNFile pgnfile;
   PGNGame *game;
   SearchTask entry;
   Gamemove *gamemove;
   long i;
   UINT64 overallNodes = 0;
   const char *fmt = "\nTestsuite '%s': %d/%d solved, %s nodes\n";
   char ons[32];
   int solved = 0;
   String notSolved = getEmptyString();
   Variation variation;

   if (openPGNFile(&pgnfile, filename) != 0)
   {
      return -1;
   }

   logReport("\nProcessing file '%s' [%ld game(s)]\n", filename,
             pgnfile.numGames);

   statCount1 = statCount2 = 0;
   variation.timeTarget = 60 * 1000;
   variation.timeLimit = 60 * 1000;
   variation.ponderMode = FALSE;
   entry.variation = &variation;

   for (i = 1; i <= pgnfile.numGames; i++)
   {
      game = getGame(&pgnfile, i);

      if (game == 0)
      {
         continue;
      }

      logReport("\n%ld (%ld): %s-%s\n", i, pgnfile.numGames,
                game->white, game->black);
      logPosition(&game->firstMove->position);

      entry.solutions.numberOfMoves = 0;
      gamemove = game->firstMove;

      while (gamemove != 0)
      {
         entry.solutions.moves
            [entry.solutions.numberOfMoves++] =
            getPackedMove(gamemove->from, gamemove->to, gamemove->newPiece);

         gamemove = gamemove->alternativeMove;
      }

      initializeVariation(&variation, game->fen);
      resetSharedHashtable = TRUE;
      variation.handleUciEvents = FALSE;

      if (strstr(game->white, "[#") != NULL)
      {
         char *tmp = strstr(game->white, "[#");

         entry.type = TASKTYPE_TEST_MATE_IN_N;
         entry.numberOfMoves = atoi(tmp + 2);

         if (solveMateProblem(&entry) != FALSE)
         {
            solved++;
         }
         else
         {
            assert(0);
         }

         overallNodes += entry.nodes;
      }
      else
      {
         entry.type = TASKTYPE_TEST_BEST_MOVE;
         entry.numberOfMoves = 0;

         if (commandlineOptions.dumpEvaluation)
         {
            dumpEvaluation(&entry);
         }
         else
         {
            if (solveBestMoveProblem(&entry) != FALSE)
            {
               solved++;
            }
            else
            {
               appendToString(&notSolved, "%d ", i);
            }

            overallNodes += entry.nodes;
         }
      }

      freePgnGame(game);
   }

   formatLongInteger(overallNodes, ons);
   logReport(fmt, filename, solved, pgnfile.numGames, ons);
   logReport("Not solved: %s\n", notSolved.buffer);
   free(notSolved.buffer);
   closePGNFile(&pgnfile);

   return 0;
}

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleTest(void)
{
   return 0;
}

int testModuleTest()
{
   int result;

   if ((result = processTestsuite("moduletest.pgn")) != 0)
   {
      return result;
   }

   return 0;
}
