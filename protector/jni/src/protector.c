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
#include "tools.h"
#include "io.h"
#include "protector.h"
#include "bitboard.h"
#include "position.h"
#include "fen.h"
#include "movegeneration.h"
#include "matesearch.h"
#include "search.h"
#include "hash.h"
#include "test.h"
#include "pgn.h"
#include "evaluation.h"
#include "coordination.h"
#include "xboard.h"
#include "book.h"
#ifdef INCLUDE_TABLEBASE_ACCESS
#include "tablebase.h"
#endif

const char *programVersionNumber = "1.9.0";

int _distance[_64_][_64_];
int _horizontalDistance[_64_][_64_];
int _verticalDistance[_64_][_64_];
int _taxiDistance[_64_][_64_];
int castlingsOfColor[2];
const int colorSign[2] = { 1, -1 };

CommandlineOptions commandlineOptions;
UINT64 statCount1, statCount2;
int debugOutput = FALSE;

static void initializeModuleProctector()
{
   int sq1, sq2;

   ITERATE(sq1)
   {
      ITERATE(sq2)
      {
         _horizontalDistance[sq1][sq2] = abs(file(sq1) - file(sq2));
         _verticalDistance[sq1][sq2] = abs(rank(sq1) - rank(sq2));
         _distance[sq1][sq2] =
            max(_horizontalDistance[sq1][sq2], _verticalDistance[sq1][sq2]);
         _taxiDistance[sq1][sq2] =
            _horizontalDistance[sq1][sq2] + _verticalDistance[sq1][sq2];
      }
   }

   castlingsOfColor[WHITE] = WHITE_00 | WHITE_000;
   castlingsOfColor[BLACK] = BLACK_00 | BLACK_000;

   initializeModuleIo();
   initializeModuleTools();
   initializeModuleCoordination();
   initializeModuleBitboard();
   initializeModulePosition();
   initializeModuleFen();
   initializeModuleMovegeneration();
   initializeModuleMatesearch();
   initializeModuleSearch();
   initializeModuleHash();
   initializeModuleTest();
   initializeModulePgn();
#ifdef INCLUDE_TABLEBASE_ACCESS
   initializeModuleTablebase();
#endif
   initializeModuleEvaluation();
   initializeModuleXboard();
#ifdef USE_BOOK
   initializeModuleBook();
#endif
}

static void reportSuccess(const char *moduleName)
{
   logDebug("Module %s tested successfully.\n", moduleName);
}

static int testModuleProtector()
{
   assert(_horizontalDistance[C2][E6] == 2);
   assert(_verticalDistance[C2][E6] == 4);
   assert(_distance[C3][H8] == 5);
   assert(_taxiDistance[B2][F7] == 9);

   if (testModuleIo() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Io");
   }

   if (testModuleTools() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Tools");
   }

   if (testModuleCoordination() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Coordination");
   }

   if (testModuleBitboard() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Bitboard");
   }

   if (testModulePosition() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Position");
   }

   if (testModuleFen() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Fen");
   }

   if (testModuleMovegeneration() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Movegeneration");
   }

   if (testModuleHash() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Hash");
   }

   if (testModuleMatesearch() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Matesearch");
   }

   if (testModuleSearch() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Search");
   }

   if (testModulePgn() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Pgn");
   }

   if (testModuleEvaluation() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Evaluation");
   }

   if (testModuleXboard() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Xboard");
   }

#ifdef USE_BOOK
   if (testModuleBook() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Book");
   }
#endif

#ifdef INCLUDE_TABLEBASE_ACCESS
   if (testModuleTablebase() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Tablebase");
   }
#endif

   if (testModuleTest() != 0)
   {
      return -1;
   }
   else
   {
      reportSuccess("Test");
   }

   logDebug("\nModuletest finished successfully.\n");

   return 0;
}

static void parseOptions(int argc, char **argv, CommandlineOptions * options)
{
   int i;

   options->processModuleTest = FALSE;
   options->xboardMode = TRUE;
   options->dumpEvaluation = FALSE;
   options->testfile = 0;
   options->bookfile = 0;
   options->tablebasePath = 0;

   for (i = 0; i < argc; i++)
   {
      const char *currentArg = argv[i];

      if (strcmp(currentArg, "-m") == 0)
      {
         options->processModuleTest = TRUE;
         options->xboardMode = FALSE;
      }

      if (strcmp(currentArg, "-d") == 0)
      {
         options->dumpEvaluation = TRUE;
      }

      if (strcmp(currentArg, "-t") == 0 && i < argc - 1)
      {
         options->testfile = argv[++i];
         options->xboardMode = FALSE;
      }

      if (strcmp(currentArg, "-e") == 0 && i < argc - 1)
      {
         options->tablebasePath = argv[++i];
      }

      if (strcmp(currentArg, "-b") == 0 && i < argc - 1)
      {
         options->bookfile = argv[++i];
      }

      if (strcmp(currentArg, "-v") == 0)
      {
         printf("Protector %s", programVersionNumber);
      }
   }
}

int main(int argc, char **argv)
{

   parseOptions(argc, argv, &commandlineOptions);
   initializeModuleProctector();
   /* logDebug("protector initialized\n"); */

   if (commandlineOptions.xboardMode)
   {
      acceptGuiCommands();

      return 0;
   }

   if (commandlineOptions.processModuleTest)
   {
      if (testModuleProtector() != 0)
      {
         logDebug("\n##### Moduletest failed! #####\n");
         getKeyStroke();

         return -1;
      }
   }

   if (commandlineOptions.testfile != 0)
   {
      if (processTestsuite(commandlineOptions.testfile) != 0)
      {
         return -1;
      }
   }

   if (commandlineOptions.bookfile != 0)
   {
      createEmptyBook(&globalBook);
      appendBookDatabase(&globalBook, commandlineOptions.bookfile, 50);
      closeBook(&globalBook);
   }

   logDebug("Main thread terminated.\n");

   return 0;
}
