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

#include "tablebase.h"
#include "protector.h"
#include "io.h"
#include <assert.h>

#ifdef INCLUDE_TABLEBASE_ACCESS

#include <pthread.h>

#if defined __linux__
pthread_mutex_t mutex;
#else
pthread_spinlock_t lock;
#endif

volatile int tbAccessCount = 0;

#define MAX_PIECES_PER_SIDE 3
#define NO_EP 127

bool tbAvailable = FALSE;
typedef unsigned long long INDEX;
typedef unsigned int squaret;

char *cache = 0;

#define	TB_FASTCALL             /* __fastcall */

typedef INDEX(*PfnCalcIndex) (squaret *, squaret *, squaret, int fInverse);

extern int IInitializeTb(char *pszPath);
extern int FTbSetCacheSize(void *pv, unsigned long cbSize);
extern void VTbCloseFiles(void);
extern int IDescFindFromCounters(int *piCount);
extern int FRegisteredFun(int iTb, int side);
extern PfnCalcIndex PfnIndCalcFun(int iTb, int side);
extern int TB_FASTCALL L_TbtProbeTable(int iTb, int side, INDEX indOffset);

#define pageL       65536
#define tbbe_ssL    ((pageL-4)/2)
#define bev_broken  (tbbe_ssL+1)        /* illegal or busted */
#define bev_mi1     tbbe_ssL    /* mate in 1 move */
#define bev_mimin   1           /* mate in max moves */
#define bev_draw    0           /* draw */
#define bev_limax   (-1)        /* mated in max moves */
#define bev_li0     (-tbbe_ssL) /* mated in 0 moves */

#define PfnIndCalc PfnIndCalcFun

int setTablebaseCacheSize(unsigned int size)
{
   unsigned long numBytes = (unsigned long) size * 1024 * 1024;

   if (cache != 0)
   {
      free(cache);
   }

   cache = malloc(numBytes);

   if (cache == 0)
   {
      logDebug("### Could not allocate tablebase cache (%lu bytes). ###\n",
               numBytes);

      return -1;
   }

   if (FTbSetCacheSize(cache, numBytes) == 0)
   {
      logDebug("### Could not set tablebase cache size (%lu bytes). ###\n",
               numBytes);

      free(cache);
      cache = 0;

      return -2;
   }

   /* logDebug("table base cache size set to %lu\n", numBytes); */

   return 0;
}

int initializeTablebase(const char *path)
{
   int result = IInitializeTb((char *) path);

   if (result > 0)
   {
      if (setTablebaseCacheSize(4) != 0)
      {
         closeTablebaseFiles();

         return -1;
      }

      /* logDebug("Tablebases found at %s\n", path); */
   }
   else
   {
      logDebug("### Error while looking for tablebases in %s ###\n", path);
   }

   tbAvailable = (bool) (result > 0);

   return (result > 0 ? 0 : -1);
}

void closeTablebaseFiles()
{
   VTbCloseFiles();
}

static void initializePieceData(const Bitboard * pieces, int *pieceCount,
                                unsigned int *pieceLocation)
{
   Bitboard tmp = *pieces;
   Square square;

   *pieceCount = 0;

   ITERATE_BITBOARD(&tmp, square)
   {
      *(pieceLocation++) = square;
      (*pieceCount)++;
   }
}

static int getMateValue(int fullMoves)
{
   if (fullMoves > 0)
   {
      return 1 - VALUE_MATED - 2 * fullMoves;
   }
   else
   {
      return VALUE_MATED - 2 * fullMoves;
   }
}

int probeTablebase(const Position * position)
{
   int pieceCount[10];
   unsigned int whitePieces[MAX_PIECES_PER_SIDE * 5 + 1];
   unsigned int blackPieces[MAX_PIECES_PER_SIDE * 5 + 1];
   unsigned int *pwhite, *pblack;
   int tableNr, fInvert, tableValue, fullMoves;
   Color color;
   INDEX index;
   Square enPassantSquare = (Square) NO_EP;

   initializePieceData(&position->piecesOfType[WHITE_PAWN], &pieceCount[0],
                       &whitePieces[0 * MAX_PIECES_PER_SIDE]);
   initializePieceData(&position->piecesOfType[WHITE_KNIGHT], &pieceCount[1],
                       &whitePieces[1 * MAX_PIECES_PER_SIDE]);
   initializePieceData(&position->piecesOfType[WHITE_BISHOP], &pieceCount[2],
                       &whitePieces[2 * MAX_PIECES_PER_SIDE]);
   initializePieceData(&position->piecesOfType[WHITE_ROOK], &pieceCount[3],
                       &whitePieces[3 * MAX_PIECES_PER_SIDE]);
   initializePieceData(&position->piecesOfType[WHITE_QUEEN], &pieceCount[4],
                       &whitePieces[4 * MAX_PIECES_PER_SIDE]);

   initializePieceData(&position->piecesOfType[BLACK_PAWN], &pieceCount[5],
                       &blackPieces[0 * MAX_PIECES_PER_SIDE]);
   initializePieceData(&position->piecesOfType[BLACK_KNIGHT], &pieceCount[6],
                       &blackPieces[1 * MAX_PIECES_PER_SIDE]);
   initializePieceData(&position->piecesOfType[BLACK_BISHOP], &pieceCount[7],
                       &blackPieces[2 * MAX_PIECES_PER_SIDE]);
   initializePieceData(&position->piecesOfType[BLACK_ROOK], &pieceCount[8],
                       &blackPieces[3 * MAX_PIECES_PER_SIDE]);
   initializePieceData(&position->piecesOfType[BLACK_QUEEN], &pieceCount[9],
                       &blackPieces[4 * MAX_PIECES_PER_SIDE]);

   tableNr = IDescFindFromCounters(pieceCount);

   if (tableNr == 0)
   {
      return TABLEBASE_ERROR;
   }

   whitePieces[5 * MAX_PIECES_PER_SIDE] = position->king[WHITE];
   blackPieces[5 * MAX_PIECES_PER_SIDE] = position->king[BLACK];

   if (tableNr > 0)
   {
      color = position->activeColor;
      fInvert = 0;
      pwhite = whitePieces;
      pblack = blackPieces;
   }
   else
   {
      color = opponent(position->activeColor);
      fInvert = 1;
      pwhite = blackPieces;
      pblack = whitePieces;
      tableNr = -tableNr;
   }

#if defined __APPLE__
   pthread_mutex_lock((pthread_mutex_t *) & lock);
#else
#if defined __linux__
   pthread_mutex_lock(&mutex);
#else
   pthread_spin_lock(&lock);
#endif
#endif

   if (FRegisteredFun(tableNr, color) == FALSE)
   {
#if defined __APPLE__
      pthread_mutex_unlock((pthread_mutex_t *) & lock);
#else
#if defined __linux__
      pthread_mutex_unlock(&mutex);
#else
      pthread_spin_unlock(&lock);
#endif
#endif

      return TABLEBASE_ERROR;
   }

   if (position->enPassantSquare != NO_SQUARE)
   {
      const Bitboard attackers =
         position->piecesOfType[PAWN | position->activeColor] &
         generalMoves[PAWN | opponent(position->activeColor)]
         [position->enPassantSquare];

      if (attackers != EMPTY_BITBOARD)
      {
         /*
            dumpSquare(position->enPassantSquare);
            dumpPosition(position);
          */

         enPassantSquare = position->enPassantSquare;
      }
   }

   index = PfnIndCalcFun(tableNr, color)
      (pwhite, pblack, enPassantSquare, fInvert);

   tableValue = L_TbtProbeTable(tableNr, color, index);

#if defined __APPLE__
   pthread_mutex_unlock((pthread_mutex_t *) & lock);
#else
#if defined __linux__
   pthread_mutex_unlock(&mutex);
#else
   pthread_spin_unlock(&lock);
#endif
#endif

   if (tableValue == bev_broken)
   {
      return TABLEBASE_ERROR;
   }

   if (tableValue == 0)
   {
      return 0;
   }

   fullMoves = (tableValue > 0 ?
                1 + tbbe_ssL - tableValue : bev_li0 - tableValue);

   return getMateValue(fullMoves);
}

int initializeModuleTablebase()
{
#if defined __APPLE__
   pthread_mutex_init((pthread_mutex_t *) & lock, tbAccessCount);
#else
#if defined __linux__
   pthread_mutexattr_t mattr;

   pthread_mutex_init(&mutex, &mattr);
#else
   pthread_spin_init(&lock, tbAccessCount);
#endif
#endif
   if (commandlineOptions.tablebasePath != 0)
   {
      return initializeTablebase(commandlineOptions.tablebasePath);
   }
   else
   {
      return 0;
   }
}

static int testTableFinder()
{
#ifndef NDEBUG
   Variation variation;
   int tableValue;

   initializeVariation(&variation, "B6k/8/8/8/8/8/8/K6N w - - 0 1");
   tableValue = probeTablebase(&variation.singlePosition);
   assert(tableValue == 29941);

   initializeVariation(&variation, "B6k/8/8/8/8/8/8/K6N b - - 0 1");
   tableValue = probeTablebase(&variation.singlePosition);
   assert(tableValue == -29940);
#endif

   return 0;
}

static int testMateValues()
{
#ifndef NDEBUG
   Variation variation;
   int tableValue;

   initializeVariation(&variation, "R6k/8/6K1/8/8/8/8/8 b - - 0 1");
   tableValue = probeTablebase(&variation.singlePosition);
   assert(tableValue == VALUE_MATED);

   initializeVariation(&variation, "7k/8/6K1/8/8/8/8/R7 w - - 0 1");
   tableValue = probeTablebase(&variation.singlePosition);
   assert(tableValue == -VALUE_MATED - 1);
#endif

   return 0;
}

int testModuleTablebase()
{
   int result;

   if (tbAvailable == FALSE)
   {
      return 0;
   }

   if (commandlineOptions.tablebasePath == 0)
   {
      return 0;
   }

   if ((result = testTableFinder()) != 0)
   {
      return result;
   }

   if ((result = testMateValues()) != 0)
   {
      return result;
   }

   return 0;
}
#endif
