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
#include <stdarg.h>
#include <assert.h>
#include "io.h"
#include "pgn.h"
#include "position.h"

char pieceSymbol[16];
char pieceName[16];

static char *logfileName = "protector.log";

int getKeyStroke()
{
   logDebug("\nProgram halted. Hit RETURN to continue.");

   return getchar();
}

void getSquareName(Square square, char name[3])
{
   name[0] = (char) fileName(file(square));
   name[1] = (char) rankName(rank(square));
   name[2] = '\0';
}

void getMoveDump(const Move move, char *buffer)
{
   char from[3], to[3];

   getSquareName(getFromSquare(move), from);
   getSquareName(getToSquare(move), to);

   if (getNewPiece(move) == NO_PIECE)
   {
      sprintf(buffer, "%s-%s", from, to);
   }
   else
   {
      sprintf(buffer, "%s-%s=%c", from, to, pieceSymbol[getNewPiece(move)]);
   }
}

static void getMovelistDump(const Movelist * movelist, char *buffer)
{
   int i;
   char movebuffer[128];

   buffer[0] = '\0';

   sprintf(buffer + strlen(buffer), "\nmoves:\n");

   for (i = 0; i < movelist->numberOfMoves; i++)
   {
      getMoveDump(movelist->moves[i], movebuffer);
      sprintf(buffer + strlen(buffer), "%d. %s (%d)\n", i + 1, movebuffer,
              getMoveValue(movelist->moves[i]));
   }

   sprintf(buffer + strlen(buffer), "bad captures\n");

   for (i = 0; i < movelist->numberOfBadCaptures; i++)
   {
      getMoveDump(movelist->badCaptures[i], movebuffer);
      sprintf(buffer + strlen(buffer), "%d. %s (%d)\n", i + 1, movebuffer,
              getMoveValue(movelist->badCaptures[i]));
   }
}

static void formatTime(long sec, char *buffer)
{
   long seconds = sec % 60;
   long minutes = (sec / 60) % 60;
   long hours = (sec / 3600) % 60;

   sprintf(buffer, "%02ld:%02ld:%02ld", hours, minutes, seconds);
}

void formatLongInteger(UINT64 n, char *buffer)
{
   char tmp[32], *pBuffer, *fmt = "%llu";
   int i, j = 1, ol;

   sprintf(tmp, fmt, n);
   ol = (int) strlen(tmp);
   pBuffer = buffer + ol + (ol - 1) / 3;
   *pBuffer-- = '\0';

   for (i = ol - 1; i >= 0; i--)
   {
      assert(pBuffer >= buffer);

      *pBuffer-- = tmp[i];

      if (j++ % 3 == 0 && i > 0)
      {
         assert(pBuffer >= buffer);
         *pBuffer-- = ',';
      }
   }
}

static void formatCentipawnValue(int centipawnValue, char *buffer)
{
   float value = (float) centipawnValue;

   if (abs(centipawnValue) <= -(VALUE_MATED + 500))
   {
      sprintf(buffer, "%.2f", value / 100.0);
   }
   else
   {
      if (centipawnValue > 0)
      {
         sprintf(buffer, "#%d", (1 - VALUE_MATED - centipawnValue) / 2);
      }
      else
      {
         sprintf(buffer, "-#%d", (centipawnValue - VALUE_MATED) / 2);
      }
   }
}

void formatUciValue(const int centipawnValue, char *buffer)
{
   if (abs(centipawnValue) <= -(VALUE_MATED + 500))
   {
      sprintf(buffer, "cp %d", centipawnValue);
   }
   else
   {
      if (centipawnValue > 0)
      {
         sprintf(buffer, "mate %d", (1 - VALUE_MATED - centipawnValue) / 2);
      }
      else
      {
         sprintf(buffer, "mate -%d", (centipawnValue - VALUE_MATED) / 2);
      }
   }
}

static void getBoardDump(const Position * position, char *buffer)
{
   int file, rank;
   Square square;
   Piece piece;

   for (rank = RANK_8; rank >= RANK_1; rank--)
   {
      for (file = FILE_A; file <= FILE_H; file++)
      {
         square = getSquare(file, rank);
         piece = position->piece[square];
         *buffer++ = pieceName[piece];
      }

      *buffer++ = '\n';
   }

   *buffer = '\0';

   if (position->activeColor == WHITE)
   {
      strcat(buffer, "White to move");
   }
   else
   {
      strcat(buffer, "Black to move");
   }
}

void dumpSquare(const Square square)
{
   char buffer[3];

   getSquareName(square, buffer);

   logDebug("%s\n", buffer);
}

void dumpMove(const Move move)
{
   char buffer[128];

   getMoveDump(move, buffer);

   logDebug("%s (%d)\n", buffer, getMoveValue(move));
}

void logMove(const Move move)
{
   char buffer[128];

   getMoveDump(move, buffer);

   logReport("%s (%d)\n", buffer, getMoveValue(move));
}

void dumpMovelist(const Movelist * movelist)
{
   char buffer[4096];

   getMovelistDump(movelist, buffer);

   logDebug("%s\n", buffer);
}

void dumpPv(int depth, long timestamp,
            const char *moves, int value, UINT64 nodes,
            const Color activeColor)
{
   char ts[32], ns[32], vs[32];

   formatTime(timestamp / 1000, ts);
   formatLongInteger(nodes, ns);

   if (activeColor == BLACK && value > 20000)
   {
      value--;
   }

   formatCentipawnValue((activeColor == WHITE ? value : -value), vs);
   logReport("%d: %s %s (%s) %s\n", depth, ts, moves, vs, ns);
}

void logPosition(const Position * position)
{
   char buffer[1024];

   getBoardDump(position, buffer);

   logReport("%s\n", buffer);
}

void dumpPosition(const Position * position)
{
   logPosition(position);
   getKeyStroke();
}

void dumpVariation(const Variation * variation)
{
   char buffer[1024], moveBuffer[16];
   int ply;

   getBoardDump(&variation->singlePosition, buffer);
   strcat(buffer, "\n");

   for (ply = 0; ply < variation->ply; ply++)
   {
      getMoveDump(variation->plyInfo[ply].currentMove, moveBuffer);
      strcat(buffer, moveBuffer);
      strcat(buffer, " ");
   }

   logReport("%s\nnodeCount: %llu hashKey: %llu", buffer, variation->nodes,
             variation->singlePosition.hashKey);
   getKeyStroke();
}

void reportVariation(const Variation * variation)
{
   char buffer[1024], moveBuffer[16];
   int ply;

   getBoardDump(&variation->startPosition, buffer);
   strcat(buffer, "\n");

   for (ply = 0; ply < variation->ply; ply++)
   {
      getMoveDump(variation->plyInfo[ply].currentMove, moveBuffer);
      strcat(buffer, moveBuffer);
      strcat(buffer, " ");
   }

   logReport("%s\nnodeCount: %llu hashKey: %llu\nbest move=", buffer,
             variation->nodes, variation->singlePosition.hashKey);
   logMove(variation->bestBaseMove);
}

static void bitboard2String(Bitboard bitboard, char *title, char *buffer)
{
   int file, rank;

   for (rank = RANK_8; rank >= RANK_1; rank = (Rank) (rank - 1))
   {
      for (file = FILE_A; file <= FILE_H; file = (File) (file + 1))
      {
         Square square = getSquare(file, rank);

         *buffer++ = (testSquare(bitboard, square) ? '*' : '0');
      }

      *buffer++ = '\n';
   }

   sprintf(buffer, "%s", title);
}

void dumpBitboard(Bitboard bitboard, char *title)
{
   char buffer[128];

   bitboard2String(bitboard, title, buffer);

   logDebug("\n%s\n\n", buffer);
}

void dumpBalance(const INT32 balance)
{
   int opValue = getOpeningValue(balance);
   int egValue = getEndgameValue(balance);

   logDebug("op=%d eg=%d\n", opValue, egValue);
}

static void boardValues2String(const int value[64], char *buffer)
{
   int file, rank;
   char valueBuffer[64];

   for (rank = RANK_8; rank >= RANK_1; rank = (Rank) (rank - 1))
   {
      for (file = FILE_A; file <= FILE_H; file = (File) (file + 1))
      {
         Square square = getSquare(file, rank);

         sprintf(valueBuffer, "%i ", value[square]);
         sprintf(buffer, "%s", valueBuffer);
         buffer += strlen(valueBuffer);
      }

      *buffer++ = '\n';
   }

   *buffer++ = '\0';
}

void dumpBoardValues(const int value[64])
{
   char buffer[1024];

   boardValues2String(value, buffer);

   logDebug("\n%s\n\n", buffer);
}

void logDebug(const char *fmt, ...)
{
   va_list args;

   va_start(args, fmt);

   if (commandlineOptions.xboardMode == FALSE)
   {
      vprintf(fmt, args);
   }
   else
   {
      FILE *logfile = fopen(logfileName, "a");

      fprintf(logfile, "%lu: ", getTimestamp());
      vfprintf(logfile, fmt, args);
      fflush(logfile);

      if (fclose(logfile) != 0)
      {
         printf("Could not close file '%s'.", logfileName);
      }
   }

   va_end(args);
}

void logReport(const char *fmt, ...)
{
   char buffer[1024];
   FILE *logfile = fopen(logfileName, "a");
   va_list args;

   va_start(args, fmt);
   vsprintf(buffer, fmt, args);
   va_end(args);

   if (commandlineOptions.xboardMode == FALSE)
   {
      printf("%s", buffer);
   }

   if (logfile != NULL)
   {
      fprintf(logfile, "%s", buffer);

      if (fclose(logfile) != 0)
      {
         printf("Could not close file '%s'.", logfileName);
      }
   }
   else
   {
      printf("Could not open file '%s'.", logfileName);
   }
}

void writeTableToFile(UINT64 * table, const int tablesize,
                      const char *fileName, const char *tableName)
{
   FILE *file = fopen(fileName, "w");
   int i;
   const char *format = "%llullu, ";

   fprintf(file, "#include \"protector.h\"\n\n");
   fprintf(file, "UINT64 %s[%d] = {\n", tableName, tablesize);

   for (i = 0; i < tablesize; i++)
   {
      fprintf(file, format, table[i]);

      if ((i % 4) == 3)
      {
         fprintf(file, "\n");
      }
   }

   fclose(file);
}

int initializeModuleIo()
{
   pieceSymbol[KING] = 'K';
   pieceSymbol[QUEEN] = 'Q';
   pieceSymbol[ROOK] = 'R';
   pieceSymbol[BISHOP] = 'B';
   pieceSymbol[KNIGHT] = 'N';
   pieceSymbol[PAWN] = 'P';

   pieceName[NO_PIECE] = '*';
   pieceName[WHITE_KING] = 'K';
   pieceName[WHITE_QUEEN] = 'Q';
   pieceName[WHITE_ROOK] = 'R';
   pieceName[WHITE_BISHOP] = 'B';
   pieceName[WHITE_KNIGHT] = 'N';
   pieceName[WHITE_PAWN] = 'P';
   pieceName[BLACK_KING] = 'k';
   pieceName[BLACK_QUEEN] = 'q';
   pieceName[BLACK_ROOK] = 'r';
   pieceName[BLACK_BISHOP] = 'b';
   pieceName[BLACK_KNIGHT] = 'n';
   pieceName[BLACK_PAWN] = 'p';

   return 0;
}

int testModuleIo()
{
   char buffer[32];

   formatLongInteger(123, buffer);
   assert(strcmp(buffer, "123") == 0);
   formatLongInteger(1234, buffer);
   assert(strcmp(buffer, "1,234") == 0);
   formatLongInteger(1234567, buffer);
   assert(strcmp(buffer, "1,234,567") == 0);

   return 0;
}
