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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "io.h"
#include "pgn.h"
#include "bitboard.h"
#include "position.h"
#include "movegeneration.h"
#include "fen.h"

#define BUFSIZE 8192
#define INCREMENT 1024

#define STATE_INIT	0
#define STATE_1		1
#define STATE_2		2
#define STATE_IGNORE    3

static char pieceName[16];

/*
 ******************************************************************************
 *
 *   File operations
 *
 ******************************************************************************
 */

static int scanForIndex(PGNFile * pgnfile, int state, char buffer[],
                        size_t bufsize, size_t offset)
{
   size_t i = 0;

   while (i < bufsize)
   {
      switch (buffer[i++])
      {
      case '\n':

         if (state < STATE_IGNORE)
         {
            state++;
         }

         break;

      case '\r':
      case ' ':
         continue;

      case '{':

         if (state < STATE_IGNORE)
         {
            state = STATE_IGNORE;
         }
         else
         {
            state++;
         }

         break;

      case '}':

         if (state > STATE_IGNORE)
         {
            state--;
         }
         else
         {
            state = 0;
         }

         break;

      case '[':

         if (state == 2)
         {
            if (pgnfile->numGames + 1 == pgnfile->indexSize)
            {
               pgnfile->indexSize += INCREMENT;
               pgnfile->index = (long *) realloc(pgnfile->index,
                                                 pgnfile->indexSize *
                                                 sizeof(long));
            }

            pgnfile->index[pgnfile->numGames++] = (long) (i + offset - 1L);
            assert(pgnfile->numGames < pgnfile->indexSize);
         }

         break;

      default:

         if (state != STATE_IGNORE)
         {
            state = 0;
         }
      }
   }

   return state;
}

static void buildIndex(PGNFile * pgnfile)
{
   char buffer[BUFSIZE];
   int state = STATE_2;
   size_t numRead;

   rewind(pgnfile->file);

   do
   {
      long pos = ftell(pgnfile->file);

      numRead = fread(buffer, 1, BUFSIZE, pgnfile->file);
      state = scanForIndex(pgnfile, state, buffer, numRead, pos);
   }
   while (numRead == BUFSIZE);

   fseek(pgnfile->file, 0, SEEK_END);
   pgnfile->index[pgnfile->numGames] = ftell(pgnfile->file) + 1;
}

int openPGNFile(PGNFile * pgnfile, const char *filename)
{
   pgnfile->index = (long *) malloc(INCREMENT * sizeof(long));

   pgnfile->indexSize = INCREMENT;
   pgnfile->numGames = 0;
   pgnfile->file = fopen(filename, "rb");

   if (pgnfile->index != 0 && pgnfile->file != 0)
   {
      buildIndex(pgnfile);
      return 0;
   }
   else
   {
      return -1;
   }
}

void closePGNFile(PGNFile * pgnfile)
{
   if (pgnfile->index != 0)
   {
      free(pgnfile->index);
   }

   pgnfile->indexSize = 0;
   pgnfile->numGames = 0;

   if (pgnfile->file != 0)
   {
      fclose(pgnfile->file);
      pgnfile->file = 0;
   }
}

static char *getGameText(PGNFile * pgnfile, int number)
{
   long start, end, length;
   char *buffer;

   if (number < 1 || number > pgnfile->numGames)
   {
      return 0;
   }

   start = pgnfile->index[number - 1];
   end = pgnfile->index[number] - 1;
   length = (int) (end - start);

   if ((buffer = malloc(length + 1)) == NULL)
   {
      return 0;
   }

   fseek(pgnfile->file, start, SEEK_SET);

   if (fread(buffer, 1, length, pgnfile->file) == length)
   {
      buffer[length] = '\0';
      trim(buffer);
   }
   else
   {
      return 0;
   }

   return buffer;
}

/*
 ******************************************************************************
 *
 *   Object generation and destruction
 *
 ******************************************************************************
 */

static Gamemove *getGamemove(const Position * position,
                             Gamemove * previousMove, PGNGame * game)
{
   Gamemove *new = &(game->moveHeap[game->nextMoveFromHeap++]);

   new->from = NO_SQUARE;
   new->to = NO_SQUARE;
   new->newPiece = NO_PIECE;
   new->position = *position;
   new->previousMove = previousMove;
   new->nextMove = new->alternativeMove = 0;
   new->comment = new->glyphs = 0;

   return new;
}

Move gameMove2Move(const Gamemove * gamemove)
{
   return getPackedMove(gamemove->from, gamemove->to, gamemove->newPiece);
}

void initializePGNGame(PGNGame * game)
{
   game->setup[0] = game->fen[0] = '\0';
   game->moveText = 0;
   game->firstMove = game->lastMove = 0;
   game->nextMoveFromHeap = 0;
}

void resetPGNGame(PGNGame * game)
{
   if (game->moveText != 0)
   {
      free(game->moveText);
   }

   initializePGNGame(game);
}

void freePgnGame(PGNGame * game)
{
   resetPGNGame(game);
   free(game);
}

/*
 ******************************************************************************
 *
 *   Parser operations
 *
 ******************************************************************************
 */

static Move parsePawnMove(const Position * position, const char *moveText)
{
   Square from = NO_SQUARE, to = NO_SQUARE;
   Piece newPiece = NO_PIECE;
   int pawnStep = (position->activeColor == WHITE ? 8 : -8);
   size_t textLength = strlen(moveText);

   if (textLength < 2)
   {
      return NO_MOVE;
   }

   assert(textLength <= 6);

   /* 
    * capture or push? 
    */
   if (textLength >= 4 && moveText[1] == 'x')
   {
      to = getSquare(moveText[2] - 'a', moveText[3] - '1');
      from = (Square)
         (getSquare(moveText[0] - 'a', moveText[3] - '1') - pawnStep);
   }
   else
   {
      to = getSquare(moveText[0] - 'a', moveText[1] - '1');
      from = (Square) (to - pawnStep);

      if (position->piece[from] == NO_PIECE)
      {
         from = (Square) (from - pawnStep);
      }
   }

   /* 
    * promotion? 
    */
   if (textLength >= 4 && moveText[textLength - 2] == '=')
   {
      switch (moveText[textLength - 1])
      {
      case 'R':
         newPiece = WHITE_ROOK;
         break;
      case 'B':
         newPiece = WHITE_BISHOP;
         break;
      case 'N':
         newPiece = WHITE_KNIGHT;
         break;
      default:
         newPiece = WHITE_QUEEN;
      }
   }

   return getPackedMove(from, to, newPiece);
}

static Move parseCastlingMove(const Position * position, const char *moveText)
{
   const Rank rank = (position->activeColor == WHITE ? RANK_1 : RANK_8);
   const File file = (strcmp(moveText, "O-O") == 0 ? FILE_G : FILE_C);

   assert(strlen(moveText) >= 3 && strlen(moveText) <= 5);

   return getPackedMove(getSquare(FILE_E, rank), getSquare(file, rank),
                        NO_PIECE);
}

static Move parsePieceMove(const Position * position,
                           const char *moveText, const PieceType pieceType)
{
   Square from = NO_SQUARE, to = NO_SQUARE;
   Piece newPiece = NO_PIECE;
   size_t textLength = strlen(moveText);
   int fromFile = -1, fromRank = -1;
   size_t pointer = 1;
   int currentChar;
   Bitboard candidates;

   if (textLength < 3)
   {
      return NO_MOVE;
   }

   assert(strlen(moveText) <= 6);

   to = getSquare(moveText[textLength - 2] - 'a',
                  moveText[textLength - 1] - '1');

   while (pointer < textLength - 2)
   {
      currentChar = moveText[pointer++];

      if (currentChar >= 'a' && currentChar <= 'h')
      {
         fromFile = currentChar - 'a';
      }

      if (currentChar >= '1' && currentChar <= '8')
      {
         fromRank = currentChar - '1';
      }
   }

   if (fromFile != -1 && fromRank != -1)
   {
      from = getSquare(fromFile, fromRank);
   }
   else
   {
      candidates =
         getDirectAttackers(position, to, position->activeColor,
                            position->allPieces) &
         position->piecesOfType[pieceType | position->activeColor];

      if (fromFile != -1)
      {
         candidates &= getSquaresOfFile((File) fromFile);
      }
      else if (fromRank != -1)
      {
         candidates &= getSquaresOfRank((Rank) fromRank);
      }

      if (candidates == EMPTY_BITBOARD)
      {
         return NO_MOVE;
      }

      from = getLastSquare(&candidates);

      while (candidates != EMPTY_BITBOARD)
      {
         const Move move = getPackedMove(from, to, newPiece);

         if (moveIsLegal(position, move))
         {
            return move;
         }

         from = getLastSquare(&candidates);
      }
   }

   return getPackedMove(from, to, newPiece);
}

static Move parsePGNMove(const char *moveText, Position * position)
{
   char moveTextBuffer[16];
   int i = 0;

   while (i < 15 && moveText[i] != '\0' && moveText[i] != '+' &&
          moveText[i] != '#' && moveText[i] != '!' &&
          moveText[i] != '?' && moveText[i] != ')' &&
          !isspace((int) moveText[i]))
   {
      moveTextBuffer[i] = moveText[i];
      i++;
   }

   moveTextBuffer[i] = '\0';

   switch (moveTextBuffer[0])
   {
   case 'a':
   case 'b':
   case 'c':
   case 'd':
   case 'e':
   case 'f':
   case 'g':
   case 'h':
      return parsePawnMove(position, moveTextBuffer);
      break;
   case 'K':
      return parsePieceMove(position, moveTextBuffer, KING);
      break;
   case 'Q':
      return parsePieceMove(position, moveTextBuffer, QUEEN);
      break;
   case 'R':
      return parsePieceMove(position, moveTextBuffer, ROOK);
      break;
   case 'B':
      return parsePieceMove(position, moveTextBuffer, BISHOP);
      break;
   case 'N':
      return parsePieceMove(position, moveTextBuffer, KNIGHT);
      break;
   case 'O':
      return parseCastlingMove(position, moveTextBuffer);
      break;
   default:

      return NO_MOVE;
   }
}

Move interpretPGNMove(const char *moveText, PGNGame * game)
{
   Variation variation;

   initializeVariationFromGame(&variation, game);

   return parsePGNMove(moveText, &variation.singlePosition);
}

int appendMove(PGNGame * game, const Move move)
{
   Variation variation;

   initializeVariationFromGame(&variation, game);

   if (moveIsLegal(&variation.singlePosition, move))
   {
      Gamemove *newMove =
         getGamemove(&variation.singlePosition, game->lastMove, game);

      newMove->from = getFromSquare(move);
      newMove->to = getToSquare(move);
      newMove->newPiece = getNewPiece(move);

      if (game->lastMove == 0)
      {
         game->firstMove = game->lastMove = newMove;
      }
      else
      {
         game->lastMove->nextMove = newMove;
         game->lastMove = newMove;
      }

      return 0;
   }
   else
   {
      return -1;
   }
}

void takebackLastMove(PGNGame * game)
{
   if (game->lastMove != 0)
   {
      if (game->lastMove == game->firstMove)
      {
         game->lastMove = 0;
      }
      else
      {
         game->lastMove = game->lastMove->previousMove;
      }
   }
}

static void addAlternativeMove(Gamemove * move, Gamemove * alternative)
{
   if (move->alternativeMove != 0)
   {
      addAlternativeMove(move->alternativeMove, alternative);
   }
   else
   {
      move->alternativeMove = alternative;
      alternative->previousMove = move->previousMove;
   }
}

static Gamemove *parseMoveText(const char *pgnMoveText,
                               size_t * offset, const Position * position,
                               PGNGame * game)
{
   bool variationTerminated = FALSE;
   Gamemove *baseMove = 0, *lastMove = 0;
   Variation variation;
   char currentChar;
   const char *token;
   const bool debugOutput = FALSE;

   setBasePosition(&variation, position);
   prepareSearch(&variation);

   if (debugOutput)
   {
      logDebug("Starting pgn parsing at: >%s<\n", &pgnMoveText[*offset]);
   }

   while (variationTerminated == FALSE &&
          (currentChar = pgnMoveText[(*offset)++]) != '\0')
   {
      if (strchr(" \r\n0123456789.", currentChar) != 0)
      {
         continue;
      }

      token = &(pgnMoveText[*offset - 1]);

      if (debugOutput)
      {
         logDebug("\nCurrent token: >%s<\n", token);
      }

      if (currentChar == '{')
      {
         char *comment = getToken(token + 1, "}");

         if (debugOutput)
         {
            logDebug("Starting annotation: >%s<\n", comment);
         }

         (*offset) += strlen(comment);

         if (lastMove != 0)
         {
            lastMove->comment = comment;
         }
         else
         {
            free(comment);
         }

         continue;
      }

      if (currentChar == '(')
      {
         if (debugOutput)
         {
            logDebug("Starting subvariation.\n");
         }

         if (lastMove != 0)
         {
            addAlternativeMove(lastMove,
                               parseMoveText(pgnMoveText, offset,
                                             &lastMove->position, game));
         }
      }

      if (currentChar == ')')
      {
         variationTerminated = TRUE;

         if (debugOutput)
         {
            logDebug("Subvariation terminated.\n");
         }
      }

      if (currentChar == '$')
      {
         char *glyph = getToken(token, " )\r\n");

         if (lastMove != 0)
         {
            if (lastMove->glyphs != 0)
            {
               if (debugOutput)
               {
                  logDebug("Existing glyphs: >%s<\n", lastMove->glyphs);
               }

               lastMove->glyphs = realloc(lastMove->glyphs,
                                          strlen(lastMove->glyphs) +
                                          strlen(glyph) + 2);
               strcat(lastMove->glyphs, " ");
               strcat(lastMove->glyphs, glyph);
               free(glyph);
            }
            else
            {
               if (debugOutput)
               {
                  logDebug("Initializing glyphs...\n");
               }

               lastMove->glyphs = glyph;
            }

            if (debugOutput)
            {
               logDebug("glyphs: >%s<\n", lastMove->glyphs);
            }
         }
         else
         {
            free(glyph);
         }
      }

      if (strchr("KQRBNOabcdefgh", currentChar) != 0)
      {
         Move move;

         /*dumpPosition(variation->currentPosition); */

         if (debugOutput)
         {
            logDebug("Move token: >%s<\n", token);
         }

         move = parsePGNMove(token, &variation.singlePosition);

         if (moveIsLegal(&variation.singlePosition, move))
         {
            Gamemove *newMove =
               getGamemove(&variation.singlePosition, lastMove, game);

            newMove->from = getFromSquare(move);
            newMove->to = getToSquare(move);
            newMove->newPiece = getNewPiece(move);

            if (lastMove == 0)
            {
               baseMove = newMove;
            }
            else
            {
               lastMove->nextMove = newMove;
            }

            lastMove = newMove;
            makeMove(&variation, move);
            setBasePosition(&variation, &variation.singlePosition);
         }
         else
         {
            logDebug("### Illegal move: %s\n", token);
            dumpMove(move);
            dumpPosition(&variation.singlePosition);
            exit(-1);

            break;
         }

         while (isspace((int) pgnMoveText[*offset]) == FALSE &&
                pgnMoveText[*offset] != ')')
         {
            (*offset)++;
         }
      }
   }

   return baseMove;
}

static Gamemove *parseMoveSection(const char *pgnMoveText,
                                  const Position * position, PGNGame * game)
{
   size_t offset = 0;

   return parseMoveText(pgnMoveText, &offset, position, game);
}

static void parseRoasterValue(const char *pgn, const char *name,
                              char value[PGN_ROASTERLINE_SIZE])
{
   char *nameBegin, *valueBegin, *valueEnd;
   char buffer[2 * PGN_ROASTERLINE_SIZE];
   size_t valueLength;

   strcpy(value, "");
   strcpy(buffer, "[");
   strcat(buffer, name);

   if ((nameBegin = strstr(pgn, buffer)) == 0)
   {
      return;
   }

   if ((valueBegin = strstr(nameBegin, "\"")) == 0)
   {
      return;
   }

   if ((valueEnd = strstr(valueBegin, "\"]")) == 0)
   {
      return;
   }

   valueLength = min(PGN_ROASTERLINE_SIZE, valueEnd - valueBegin - 1);
   memcpy(value, valueBegin + 1, valueLength);
   value[valueLength] = '\0';
}

static void parseRoasterValues(const char *pgn, PGNGame * pgngame)
{
   char *moves;
   Variation variation;

   parseRoasterValue(pgn, "Event", pgngame->event);
   parseRoasterValue(pgn, "Site", pgngame->site);
   parseRoasterValue(pgn, "Date", pgngame->date);
   parseRoasterValue(pgn, "Round", pgngame->round);
   parseRoasterValue(pgn, "White", pgngame->white);
   parseRoasterValue(pgn, "Black", pgngame->black);
   parseRoasterValue(pgn, "Result", pgngame->result);
   parseRoasterValue(pgn, "SetUp", pgngame->setup);
   parseRoasterValue(pgn, "FEN", pgngame->fen);
   parseRoasterValue(pgn, "WhiteTitle", pgngame->whiteTitle);
   parseRoasterValue(pgn, "BlackTitle", pgngame->blackTitle);
   parseRoasterValue(pgn, "WhiteElo", pgngame->whiteElo);
   parseRoasterValue(pgn, "BlackElo", pgngame->blackElo);
   parseRoasterValue(pgn, "ECO", pgngame->eco);
   parseRoasterValue(pgn, "NIC", pgngame->nic);
   parseRoasterValue(pgn, "TimeControl", pgngame->timeControl);
   parseRoasterValue(pgn, "Termination", pgngame->termination);
   pgngame->moveText = 0;
   pgngame->firstMove = 0;

   initializeVariationFromGame(&variation, pgngame);

   if ((moves = strstr(pgn, "\n\n")) == 0)
   {
      moves = strstr(pgn, "\r\n\r\n");
   }

   if (moves != 0)
   {
      if ((pgngame->moveText = malloc(strlen(moves) + 1)) != 0)
      {
         strcpy(pgngame->moveText, moves);
         trim(pgngame->moveText);

         pgngame->firstMove =
            parseMoveSection(pgngame->moveText, &variation.singlePosition,
                             pgngame);
      }
   }
}

PGNGame *getGame(PGNFile * pgnfile, int number)
{
   PGNGame *pgngame;
   char *gameText = getGameText(pgnfile, number);

   if (gameText == 0)
   {
      return 0;
   }

   if ((pgngame = (PGNGame *) malloc(sizeof(PGNGame))) == NULL)
   {
      return 0;
   }

   initializePGNGame(pgngame);
   parseRoasterValues(gameText, pgngame);
   free(gameText);

   return pgngame;
}

/*
 ******************************************************************************
 *
 *   Generator operations
 *
 ******************************************************************************
 */

#define SAME_PIECE 1
#define SAME_FILE 2
#define SAME_RANK 4

static int examineAlternativeMoves(Variation * variation, const Move move)
{
   const Square from = getFromSquare(move);
   const Square to = getToSquare(move);
   const Piece newPiece = getNewPiece(move);
   int result = 0;
   Position *position = &variation->singlePosition;
   Piece piece = position->piece[from];
   Square square;
   Bitboard candidates =
      getDirectAttackers(position, to, position->activeColor,
                         position->allPieces) & position->piecesOfType[piece];

   clearSquare(candidates, from);

   ITERATE_BITBOARD(&candidates, square)
   {
      const Move tmp = getPackedMove(square, to, newPiece);

      if (moveIsLegal(position, tmp))
      {
         result |= SAME_PIECE;

         if (file(square) == file(from))
         {
            result |= SAME_FILE;
         }

         if (rank(square) == rank(from))
         {
            result |= SAME_RANK;
         }
      }
   }

   return result;
}

void generateMoveText(Variation * variation, const Move move, char *pgnMove)
{
   Position *position = &variation->singlePosition;
   const char *castlings[] = { "O-O", "O-O-O" };
   const Square from = getFromSquare(move);
   const Square to = getToSquare(move);
   const Piece newPiece = getNewPiece(move);

   Piece movingPiece = position->piece[from];
   char pieceSign[2], origin[3], destination[6], captureSign[2];
   char checkSign[2], promotionSign[3];
   int ambiguityCheckResult;

   getSquareName(to, destination);
   pieceSign[0] = origin[0] = captureSign[0] = checkSign[0] = 0;
   promotionSign[0] = 0;

   if (!moveIsLegal(position, move))
   {
      getSquareName(from, origin);
      sprintf(pgnMove, "(illegal move: %s-%s)", origin, destination);

      return;
   }
   else
   {
      makeMove(variation, move);

      if (activeKingIsSafe(&variation->singlePosition) == FALSE)
      {
         Movelist legalMoves;

         getLegalMoves(variation, &legalMoves);

         if (legalMoves.numberOfMoves == 0)
         {
            strcpy(checkSign, "#");
         }
         else
         {
            strcpy(checkSign, "+");
         }
      }

      unmakeLastMove(variation);
   }

   if (pieceType(movingPiece) == PAWN)
   {
      if (file(from) != file(to))
      {
         strcpy(captureSign, "x");
         sprintf(origin, "%c", fileName(file(from)));
      }

      if (newPiece != NO_PIECE)
      {
         sprintf(promotionSign, "=%c", pieceName[newPiece]);
      }
   }
   else
   {
      if (pieceType(movingPiece) == KING && _distance[from][to] > 1)
      {
         strcpy(destination, castlings[file(to) == FILE_G ? 0 : 1]);
      }
      else
      {
         sprintf(pieceSign, "%c", pieceName[pieceType(movingPiece)]);

         if (position->piece[to] != NO_PIECE)
         {
            strcpy(captureSign, "x");
         }

         ambiguityCheckResult = examineAlternativeMoves(variation, move);

         if (ambiguityCheckResult & SAME_PIECE)
         {
            switch (ambiguityCheckResult)
            {
            case SAME_PIECE | SAME_FILE | SAME_RANK:
               getSquareName(from, origin);
               break;

            case SAME_PIECE | SAME_FILE:
               sprintf(origin, "%c", rankName(rank(from)));
               break;

            default:
               sprintf(origin, "%c", fileName(file(from)));
            }
         }
      }
   }

   sprintf(pgnMove, "%s%s%s%s%s%s", pieceSign, origin, captureSign,
           destination, promotionSign, checkSign);
}

static char *generateMoveSection(Gamemove * gamemove, const char *result)
{
   String moveText = getEmptyString();
   char moveBuffer[64], *temp;
   Variation variation;
   bool restart = TRUE;

   initializeVariation(&variation, FEN_GAMESTART);

   while (gamemove != 0)
   {
      if (restart == TRUE && gamemove->position.activeColor == BLACK)
      {
         appendToString(&moveText, " %d...", gamemove->position.moveNumber);
      }

      restart = FALSE;

      setBasePosition(&variation, &gamemove->position);
      generateMoveText(&variation,
                       getPackedMove(gamemove->from, gamemove->to,
                                     gamemove->newPiece), moveBuffer);

      if (gamemove->position.activeColor == WHITE)
      {
         appendToString(&moveText, " %d. %s", gamemove->position.moveNumber,
                        moveBuffer);
      }
      else
      {
         appendToString(&moveText, " %s", moveBuffer);
      }

      if (gamemove->glyphs != 0)
      {
         appendToString(&moveText, " %s", gamemove->glyphs);
      }

      if (gamemove->comment != 0)
      {
         appendToString(&moveText, " {%s}", gamemove->comment);
      }

      if (gamemove->alternativeMove != 0)
      {
         temp = generateMoveSection(gamemove->alternativeMove, "");
         appendToString(&moveText, " (%s)", temp);
         free(temp);

         restart = TRUE;
      }

      gamemove = gamemove->nextMove;
   }

   if (result[0] != '\0')
   {
      appendToString(&moveText, " %s", result);
   }

   trim(moveText.buffer);
   breakLines(moveText.buffer, 79);

   return moveText.buffer;
}

static char *generateRoasterLine(const char *tag, const char *tagValue,
                                 char buffer[PGN_ROASTERLINE_SIZE])
{
   if (tagValue[0] != '\0')
   {
      sprintf(buffer, "[%s \"%s\"]\n", tag, tagValue);
   }
   else
   {
      buffer[0] = '\0';
   }

   return buffer;
}

static char *generateRoaster(const PGNGame * pgngame)
{
   char tagbuffer[PGN_ROASTERLINE_SIZE];
   String string = getEmptyString();

   appendToString(&string,
                  generateRoasterLine("Event", pgngame->event, tagbuffer));
   appendToString(&string,
                  generateRoasterLine("Site", pgngame->site, tagbuffer));
   appendToString(&string,
                  generateRoasterLine("Date", pgngame->date, tagbuffer));
   appendToString(&string,
                  generateRoasterLine("Round", pgngame->round, tagbuffer));
   appendToString(&string,
                  generateRoasterLine("White", pgngame->white, tagbuffer));
   appendToString(&string,
                  generateRoasterLine("Black", pgngame->black, tagbuffer));
   appendToString(&string,
                  generateRoasterLine("Result", pgngame->result, tagbuffer));
   appendToString(&string,
                  generateRoasterLine("SetUp", pgngame->setup, tagbuffer));
   appendToString(&string,
                  generateRoasterLine("FEN", pgngame->fen, tagbuffer));
   appendToString(&string,
                  generateRoasterLine("WhiteTitle", pgngame->whiteTitle,
                                      tagbuffer));
   appendToString(&string,
                  generateRoasterLine("BlackTitle", pgngame->blackTitle,
                                      tagbuffer));
   appendToString(&string,
                  generateRoasterLine("WhiteElo", pgngame->whiteElo,
                                      tagbuffer));
   appendToString(&string,
                  generateRoasterLine("BlackElo", pgngame->blackElo,
                                      tagbuffer));
   appendToString(&string,
                  generateRoasterLine("ECO", pgngame->eco, tagbuffer));
   appendToString(&string,
                  generateRoasterLine("NIC", pgngame->nic, tagbuffer));
   appendToString(&string,
                  generateRoasterLine("TimeControl", pgngame->timeControl,
                                      tagbuffer));
   appendToString(&string,
                  generateRoasterLine("Termination", pgngame->termination,
                                      tagbuffer));

   return string.buffer;
}

char *generatePgn(PGNGame * game)
{
   String string = getEmptyString();
   char *roaster = generateRoaster(game);
   char *moveText = generateMoveSection(game->firstMove, game->result);

   appendToString(&string, "%s\n%s", roaster, moveText);
   free(roaster);
   free(moveText);

   return string.buffer;
}

void initializeVariationFromGame(Variation * variation, PGNGame * game)
{
   if (game->lastMove != 0)
   {
      Gamemove *currentMove = game->lastMove;
      int i = POSITION_HISTORY_OFFSET - 1;

      setBasePosition(variation, &(game->lastMove->position));
      makeMove(variation, gameMove2Move(game->lastMove));

      if (variation->singlePosition.activeColor == WHITE)
      {
         variation->singlePosition.moveNumber++;
      }

      setBasePosition(variation, &variation->singlePosition);
      prepareSearch(variation);

      while (i >= 0 && currentMove != 0)
      {
         variation->positionHistory[i--] = currentMove->position.hashKey;
         currentMove = currentMove->previousMove;
      }
   }
   else
   {
      if (strcmp(game->setup, "") != 0)
      {
         initializeVariation(variation, game->fen);
      }
      else
      {
         initializeVariation(variation, FEN_GAMESTART);
      }
   }
}

void initializeGameFromVariation(const Variation * variation, PGNGame * game,
                                 bool copyPv)
{
   const PrincipalVariation *pv = &variation->pv[0];

   resetPGNGame(game);
   getFen(&variation->startPosition, game->fen);
   strcpy(game->setup, "1");

   if (copyPv != FALSE)
   {
      int i = 0;

      for (i = 0; i < min(8, pv->length); i++)
      {
         Move move = (Move) pv->move[i];

         if (appendMove(game, move) != 0)
         {
            break;
         }
      }
   }
}

char *getPrincipalVariation(const Variation * variation)
{
   PGNGame game;
   char *pv;

   initializePGNGame(&game);
   initializeGameFromVariation(variation, &game, TRUE);
   pv = generateMoveSection(game.firstMove, "");
   trim(pv);
   resetPGNGame(&game);

   return pv;
}

/*
 ******************************************************************************
 *
 *   Module initialization and testing
 *
 ******************************************************************************
 */

int initializeModulePgn()
{
   pieceName[WHITE_KING] = 'K';
   pieceName[WHITE_QUEEN] = 'Q';
   pieceName[WHITE_ROOK] = 'R';
   pieceName[WHITE_BISHOP] = 'B';
   pieceName[WHITE_KNIGHT] = 'N';
   pieceName[WHITE_PAWN] = 'P';
   pieceName[BLACK_KING] = 'K';
   pieceName[BLACK_QUEEN] = 'Q';
   pieceName[BLACK_ROOK] = 'R';
   pieceName[BLACK_BISHOP] = 'B';
   pieceName[BLACK_KNIGHT] = 'N';
   pieceName[BLACK_PAWN] = 'P';

   return 0;
}

static int testGeneration()
{
   char *p1 = "r2qkb1r/ppp2ppp/3p4/5b2/3PN3/n4N2/PPP1QPPP/R3K2R w KQkq - 0 1";
   char *p2 = "r3k2r/n3nppp/3n2P1/8/1q1q4/2B5/1q1pRPPP/5RK1 b kq - 0 1";
   Variation variation;
   char pgnMove[64];

   initializeVariation(&variation, p1);

   generateMoveText(&variation, getPackedMove(D4, D5, NO_PIECE), pgnMove);
   assert(strcmp("d5", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(E1, G1, NO_PIECE), pgnMove);
   assert(strcmp("O-O", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(E1, C1, NO_PIECE), pgnMove);
   assert(strcmp("O-O-O", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(E4, D6, NO_PIECE), pgnMove);
   assert(strcmp("Nxd6+", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(E4, F6, NO_PIECE), pgnMove);
   assert(strcmp("Nf6#", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(F3, G5, NO_PIECE), pgnMove);
   assert(strcmp("Nfg5", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(E4, G5, NO_PIECE), pgnMove);
   assert(strcmp("Neg5+", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(B2, A3, NO_PIECE), pgnMove);
   assert(strcmp("bxa3", pgnMove) == 0);

   initializeVariation(&variation, p2);

   generateMoveText(&variation, getPackedMove(E7, F5, NO_PIECE), pgnMove);
   assert(strcmp("(illegal move: e7-f5)", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(D2, D1, WHITE_BISHOP), pgnMove);
   assert(strcmp("d1=B", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(B4, C3, NO_PIECE), pgnMove);
   assert(strcmp("Qb4xc3", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(B2, C3, NO_PIECE), pgnMove);
   assert(strcmp("Q2xc3", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(D4, C3, NO_PIECE), pgnMove);
   assert(strcmp("Qdxc3", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(D6, F5, NO_PIECE), pgnMove);
   assert(strcmp("Nf5", pgnMove) == 0);

   generateMoveText(&variation, getPackedMove(D6, C8, NO_PIECE), pgnMove);
   assert(strcmp("Ndc8", pgnMove) == 0);

   return 0;
}

static int testParsing()
{
   char *p1 = "r2qkb1r/ppp2ppp/3p4/5b2/3PN3/n4N2/PPP1QPPP/R3K2R w KQkq - 0 1";
   char *p2 = "r3k2r/n3nppp/3n2P1/8/1q1q4/2B5/1q1pRPPP/5RK1 b kq - 0 1";
   Variation variation;

   initializeVariation(&variation, p1);

   assert(parsePGNMove("d5", &variation.singlePosition) ==
          getPackedMove(D4, D5, NO_PIECE));

   assert(parsePGNMove("b4", &variation.singlePosition) ==
          getPackedMove(B2, B4, NO_PIECE));

   assert(parsePGNMove("bxa3", &variation.singlePosition) ==
          getPackedMove(B2, A3, NO_PIECE));

   assert(parsePGNMove("O-O", &variation.singlePosition) ==
          getPackedMove(E1, G1, NO_PIECE));

   assert(parsePGNMove("O-O-O", &variation.singlePosition) ==
          getPackedMove(E1, C1, NO_PIECE));

   assert(parsePGNMove("Ng3+", &variation.singlePosition) ==
          getPackedMove(E4, G3, NO_PIECE));

   assert(parsePGNMove("Nf6#", &variation.singlePosition) ==
          getPackedMove(E4, F6, NO_PIECE));

   assert(parsePGNMove("Nfg5", &variation.singlePosition) ==
          getPackedMove(F3, G5, NO_PIECE));

   initializeVariation(&variation, p2);

   assert(parsePGNMove("h6", &variation.singlePosition) ==
          getPackedMove(H7, H6, NO_PIECE));

   assert(parsePGNMove("f5", &variation.singlePosition) ==
          getPackedMove(F7, F5, NO_PIECE));

   assert(parsePGNMove("fxg6", &variation.singlePosition) ==
          getPackedMove(F7, G6, NO_PIECE));

   assert(parsePGNMove("d1=B", &variation.singlePosition) ==
          getPackedMove(D2, D1, BISHOP));

   assert(parsePGNMove("O-O", &variation.singlePosition) ==
          getPackedMove(E8, G8, NO_PIECE));

   assert(parsePGNMove("O-O-O", &variation.singlePosition) ==
          getPackedMove(E8, C8, NO_PIECE));

   assert(parsePGNMove("Nf5", &variation.singlePosition) ==
          getPackedMove(D6, F5, NO_PIECE));

   assert(parsePGNMove("Q2xc3", &variation.singlePosition) ==
          getPackedMove(B2, C3, NO_PIECE));

   assert(parsePGNMove("Qdxc3", &variation.singlePosition) ==
          getPackedMove(D4, C3, NO_PIECE));

   assert(parsePGNMove("Qb4xc3", &variation.singlePosition) ==
          getPackedMove(B4, C3, NO_PIECE));

   return 0;
}

static int testLoading()
{
   PGNFile pgnfile;
   PGNGame *game, game2;
   char *gameText, *generatedGameText;

   pgnfile.numGames = 0;
   pgnfile.index = 0;
   pgnfile.file = 0;

   assert(openPGNFile(&pgnfile, "test.pgn") == 0);
   game = getGame(&pgnfile, 1);
   assert(game != 0);

   gameText = generatePgn(game);
   initializePGNGame(&game2);
   parseRoasterValues(gameText, &game2);
   generatedGameText = generatePgn(&game2);
   /* logDebug("\n>%s<\n", generatedGameText); */
   assert(strcmp(gameText, generatedGameText) == 0);

   free(generatedGameText);
   freePgnGame(game);
   resetPGNGame(&game2);
   free(gameText);
   closePGNFile(&pgnfile);

   return 0;
}

int testModulePgn()
{
   int result;

   if ((result = testLoading()) != 0)
   {
      return result;
   }

   if ((result = testGeneration()) != 0)
   {
      return result;
   }

   if ((result = testParsing()) != 0)
   {
      return result;
   }

   return 0;
}
