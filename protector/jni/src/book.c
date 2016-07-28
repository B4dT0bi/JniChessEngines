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

#include "book.h"
#include "io.h"
#include "pgn.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <time.h>

static const UINT32 ILLEGAL_OFFSET = 0xFFFFFFFF;
Book globalBook;

int openBook(Book * book, const char *name)
{
   char indexfileName[256], movefileName[256];

   strcpy(indexfileName, name);
   strcat(indexfileName, ".bki");
   strcpy(movefileName, name);
   strcat(movefileName, ".bkm");

   book->readonly = FALSE;
   book->indexFile = fopen(indexfileName, "r+");
   book->moveFile = fopen(movefileName, "r+");

   if (book->indexFile == NULL)
   {
      book->indexFile = fopen(indexfileName, "w+");
   }

   if (book->moveFile == NULL)
   {
      book->moveFile = fopen(movefileName, "w+");
   }

   if (book->indexFile == NULL)
   {
      book->indexFile = fopen(indexfileName, "r");
      book->readonly = TRUE;
   }

   if (book->moveFile == NULL)
   {
      book->moveFile = fopen(movefileName, "r");
      book->readonly = TRUE;
   }

   if (book->indexFile != NULL)
   {
      fseek(book->indexFile, 0, SEEK_END);
      book->numberOfPositions =
         (ftell(book->indexFile) + 1) / sizeof(BookPosition);
   }

   if (book->moveFile != NULL)
   {
      fseek(book->moveFile, 0, SEEK_END);
      book->numberOfMoves = (ftell(book->moveFile) + 1) / sizeof(BookMove);
   }

   if (book->indexFile != NULL && book->moveFile != NULL)
   {
      return 0;
   }
   else
   {
      if (book->indexFile != NULL)
      {
         fclose(book->indexFile);
         book->indexFile = NULL;
      }

      if (book->moveFile != NULL)
      {
         fclose(book->moveFile);
         book->moveFile = NULL;
      }

      return -1;
   }
}

void closeBook(Book * book)
{
   if (book->indexFile != NULL)
   {
      fclose(book->indexFile);
   }

   if (book->moveFile != NULL)
   {
      fclose(book->moveFile);
   }
}

static BookPosition loadBookposition(const Book * book, const UINT32 offset)
{
   BookPosition position;

   fseek(book->indexFile, offset, SEEK_SET);

   if (fread(&position, sizeof(BookPosition), 1, book->indexFile) ==
       sizeof(BookPosition))
   {
      return position;
   }
   else
   {
      position.hashKey = 0;
      position.firstMove = NO_MOVE;
      position.nextPosition = 0;

      return position;
   }
}

static void storeBookposition(Book * book, const BookPosition * position,
                              const UINT32 offset)
{
   fseek(book->indexFile, offset, SEEK_SET);
   fwrite(position, sizeof(BookPosition), 1, book->indexFile);
}

static BookMove loadBookmove(const Book * book, const UINT32 offset)
{
   BookMove move;

   fseek(book->moveFile, offset, SEEK_SET);

   if (fread(&move, sizeof(BookMove), 1, book->moveFile) == sizeof(BookMove))
   {
      return move;
   }
   else
   {
      move.move = NO_MOVE;
      move.numberOfGames = 0;
      move.numberOfPersonalGames = 0;
      move.personalScore = 0;
      move.score = 0;
      move.nextAlternative = 0;

      return move;
   }
}

static void storeBookmove(Book * book, const BookMove * move,
                          const UINT32 offset)
{
   fseek(book->moveFile, offset, SEEK_SET);
   fwrite(move, sizeof(BookMove), 1, book->moveFile);
}

void createEmptyBook(Book * book)
{
   BookPosition position;
   int i;

   position.hashKey = 0;
   position.nextPosition = ILLEGAL_OFFSET;
   position.firstMove = ILLEGAL_OFFSET;

   for (i = 0; i < BOOKINDEX_SIZE; i++)
   {
      storeBookposition(book, &position, i * sizeof(BookPosition));
   }

   book->numberOfPositions = BOOKINDEX_SIZE;
   book->numberOfMoves = 0;
}

static UINT32 getBookpositionOffset(const Book * book, const UINT64 hashKey)
{
   UINT32 offset = (UINT32) getBookpositionIndexOffset(hashKey);
   BookPosition position = loadBookposition(book, offset);

   while (position.hashKey != hashKey &&
          position.nextPosition != ILLEGAL_OFFSET)
   {
      offset = position.nextPosition;
      position = loadBookposition(book, offset);
   }

   return (position.hashKey == hashKey &&
           position.firstMove != ILLEGAL_OFFSET ? offset : ILLEGAL_OFFSET);
}

static void addBookposition(Book * book, const BookPosition * position)
{
   UINT32 offset = (UINT32) getBookpositionIndexOffset(position->hashKey);
   BookPosition currentPosition = loadBookposition(book, offset);

   if (currentPosition.firstMove == ILLEGAL_OFFSET)
   {
      storeBookposition(book, position, offset);
   }
   else
   {
      UINT32 newOffset = book->numberOfPositions++ * sizeof(BookPosition);

      storeBookposition(book, position, newOffset);

      while (currentPosition.nextPosition != ILLEGAL_OFFSET)
      {
         offset = currentPosition.nextPosition;
         currentPosition = loadBookposition(book, offset);
      }

      currentPosition.nextPosition = newOffset;
      storeBookposition(book, &currentPosition, offset);
   }
}

static UINT32 getBookmoveOffset(const Book * book, const UINT64 hashKey,
                                const UINT16 move)
{
   UINT32 offset = getBookpositionOffset(book, hashKey);
   BookMove currentMove;

   if (offset == ILLEGAL_OFFSET)
   {
      return ILLEGAL_OFFSET;
   }
   else
   {
      BookPosition position = loadBookposition(book, offset);

      if (position.firstMove == ILLEGAL_OFFSET)
      {
         return ILLEGAL_OFFSET;
      }
      else
      {
         offset = position.firstMove;
         currentMove = loadBookmove(book, offset);
      }
   }

   while (currentMove.move != move &&
          currentMove.nextAlternative != ILLEGAL_OFFSET)
   {
      offset = currentMove.nextAlternative;
      currentMove = loadBookmove(book, offset);
   }

   return (currentMove.move == move ? offset : ILLEGAL_OFFSET);
}

static void appendBookmove(Book * book, const UINT64 hashKey,
                           const BookMove * move)
{
   UINT32 moveOffset = book->numberOfMoves++ * sizeof(BookMove);
   UINT32 positionOffset = getBookpositionOffset(book, hashKey);
   BookPosition position;

   storeBookmove(book, move, moveOffset);

   if (positionOffset == ILLEGAL_OFFSET)
   {
      position.hashKey = hashKey;
      position.firstMove = moveOffset;
      position.nextPosition = ILLEGAL_OFFSET;
      addBookposition(book, &position);
   }
   else
   {
      position = loadBookposition(book, positionOffset);

      if (position.firstMove == ILLEGAL_OFFSET)
      {
         position.firstMove = moveOffset;
         storeBookposition(book, &position, positionOffset);
      }
      else
      {
         UINT32 currentMoveOffset = position.firstMove;
         BookMove currentMove = loadBookmove(book, currentMoveOffset);

         while (currentMove.nextAlternative != ILLEGAL_OFFSET)
         {
            currentMoveOffset = currentMove.nextAlternative;
            currentMove = loadBookmove(book, currentMoveOffset);
         }

         currentMove.nextAlternative = moveOffset;
         storeBookmove(book, &currentMove, currentMoveOffset);
      }
   }
}

static void updateBookmove(BookMove * move, const Color activeColor,
                           const GameResult result, const bool personalResult)
{
   if (personalResult != FALSE)
   {
      move->numberOfPersonalGames++;

      if (result == RESULT_WHITE_WINS)
      {
         move->personalScore += (activeColor == WHITE ? 1 : -1);
      }
      else if (result == RESULT_BLACK_WINS)
      {
         move->personalScore += (activeColor == BLACK ? 1 : -1);
      }
   }
   else
   {
      move->numberOfGames++;

      if (result == RESULT_WHITE_WINS)
      {
         move->score += (activeColor == WHITE ? 1 : -1);
      }
      else if (result == RESULT_BLACK_WINS)
      {
         move->score += (activeColor == BLACK ? 1 : -1);
      }
   }
}

void addBookmove(Book * book, const Position * position,
                 const Move move, const GameResult result,
                 const bool personalResult)
{
   BookMove bookMove;
   UINT32 bookmoveOffset;

   bookMove.move = packedMove(move);
   bookmoveOffset = getBookmoveOffset(book, position->hashKey, bookMove.move);

   if (bookmoveOffset == ILLEGAL_OFFSET)
   {
      bookMove.numberOfGames = 0;
      bookMove.score = 0;
      bookMove.numberOfPersonalGames = 0;
      bookMove.personalScore = 0;
      bookMove.nextAlternative = ILLEGAL_OFFSET;

      updateBookmove(&bookMove, position->activeColor, result,
                     personalResult);
      appendBookmove(book, position->hashKey, &bookMove);
   }
   else
   {
      bookMove = loadBookmove(book, bookmoveOffset);

      updateBookmove(&bookMove, position->activeColor, result,
                     personalResult);
      storeBookmove(book, &bookMove, bookmoveOffset);
   }
}

static void appendBookGame(Book * book, const PGNGame * game,
                           const int maximumNumberOfPlies)
{
   Gamemove *currentMove = game->firstMove;
   int plycount = 0;
   GameResult result = RESULT_UNKNOWN;

   if (strcmp(game->result, GAMERESULT_WHITE_WINS) == 0)
   {
      result = RESULT_WHITE_WINS;
   }

   if (strcmp(game->result, GAMERESULT_DRAW) == 0)
   {
      result = RESULT_DRAW;
   }

   if (strcmp(game->result, GAMERESULT_BLACK_WINS) == 0)
   {
      result = RESULT_BLACK_WINS;
   }

   if (result == RESULT_UNKNOWN || strcmp(game->setup, "1") == 0)
   {
      logDebug("Skipping book game %s-%s.\n", game->white, game->black);

      return;
   }

   while (plycount++ < maximumNumberOfPlies && currentMove != 0)
   {
      addBookmove(book, &currentMove->position,
                  gameMove2Move(currentMove), result, FALSE);
      currentMove = currentMove->nextMove;
   }
}

void appendBookDatabase(Book * book, const char *filename,
                        int maximumNumberOfPlies)
{
   PGNFile pgnfile;
   PGNGame *game;
   long i;

   if (openPGNFile(&pgnfile, filename) != 0)
   {
      return;
   }

   logReport("\nProcessing book file '%s' [%ld game(s)]\n", filename,
             pgnfile.numGames);

   for (i = 1; i <= pgnfile.numGames; i++)
   {
      game = getGame(&pgnfile, i);
      logReport("Processing book game #%ld.\n", i);
      appendBookGame(book, game, maximumNumberOfPlies);
      freePgnGame(game);
   }

   closePGNFile(&pgnfile);
}

static int getSuccessProbability(UINT16 numGames, INT16 score)
{
   int result;

   if (numGames == 0)
   {
      return 0;
   }

   result = (50 * (long) (numGames + score)) / (long) numGames;

   assert(result >= 0 && result <= 100);

   return result;
}

static int getBookmoveValue(const BookMove * move, const UINT32 numberOfGames)
{
   const int weightGames = 10, weightPersonalGames = 1;
   int successProbability, personalSuccessProbability, moveProbability;

   successProbability =
      getSuccessProbability(move->numberOfGames, move->score);
   personalSuccessProbability =
      getSuccessProbability(move->numberOfPersonalGames, move->personalScore);
   moveProbability = (move->numberOfGames * 100) / numberOfGames;

   successProbability = (successProbability * weightGames +
                         personalSuccessProbability * weightPersonalGames) /
      (weightGames + weightPersonalGames);

   return successProbability * moveProbability;
}

void getBookmoves(const Book * book, const UINT64 hashKey,
                  const Movelist * legalMoves, Movelist * bookMoves)
{
   UINT32 positionOffset = getBookpositionOffset(book, hashKey), moveOffset;
   BookPosition position;
   BookMove bookMove, bookMoveStore[MAX_MOVES_PER_POSITION];
   UINT32 numberOfGames = 0;
   int i;

   bookMoves->numberOfMoves = 0;

   if (positionOffset == ILLEGAL_OFFSET)
   {
      return;
   }

   position = loadBookposition(book, positionOffset);
   moveOffset = position.firstMove;

   while (moveOffset != ILLEGAL_OFFSET)
   {
      Move move;

      bookMove = loadBookmove(book, moveOffset);
      move = (Move) bookMove.move;

      if (listContainsMove(legalMoves, move))
      {
         numberOfGames += bookMove.numberOfGames;
         bookMoveStore[bookMoves->numberOfMoves] = bookMove;
         bookMoves->moves[bookMoves->numberOfMoves++] = move;
      }

      moveOffset = bookMove.nextAlternative;
   }

   for (i = 0; i < bookMoves->numberOfMoves; i++)
   {
      const int value =
         max(1, getBookmoveValue(&bookMoveStore[i], numberOfGames));

      setMoveValue(&bookMoves->moves[i], value);
   }
}

static Move chooseBookmove(const Movelist * bookMoves)
{
   int i;
   UINT32 randomNumber, sum = 0;

   for (i = 0; i < bookMoves->numberOfMoves; i++)
   {
      sum += getMoveValue(bookMoves->moves[i]);
   }

   srand((unsigned int) (getTimestamp() + time(0)));
   randomNumber = ((UINT32) rand() * (UINT32) rand() + (UINT32) rand()) % sum;

   logDebug("dumping bookmove list...\n");
   dumpMovelist(bookMoves);
   logDebug("sum: %lu random: %lu\n", sum, randomNumber);

   sum = 0;

   for (i = 0; i < bookMoves->numberOfMoves; i++)
   {
      sum += getMoveValue(bookMoves->moves[i]);

      if (sum > randomNumber)
      {
         return bookMoves->moves[i];
      }
   }

   return bookMoves->moves[bookMoves->numberOfMoves - 1];
}

Move getBookmove(const Book * book, const UINT64 hashKey,
                 const Movelist * legalMoves)
{
   Movelist bookMoves;
   Move move = NO_MOVE;

   if (book->indexFile != 0 && book->moveFile != 0 &&
       book->numberOfPositions > 0 && book->numberOfMoves > 0)
   {
      initMovelist(&bookMoves, 0);
      getBookmoves(book, hashKey, legalMoves, &bookMoves);

      if (bookMoves.numberOfMoves > 0)
      {
         logDebug("%d bookmoves available. choosing bookmove...\n",
                  bookMoves.numberOfMoves);

         move = chooseBookmove(&bookMoves);

         logDebug("chose bookmove %d-%d\n", getFromSquare(move),
                  getToSquare(move));
      }
   }

   return move;
}

int initializeModuleBook()
{
   if (openBook(&globalBook, "book") < 0)
   {
      logDebug("No opening book available.\n");
   }
   else
   {
      logDebug("Opening book found. %ld positions, %ld moves\n",
               globalBook.numberOfPositions, globalBook.numberOfMoves);
   }

   return 0;
}

static int testPositionOperations()
{
   Book book;
   UINT64 hashKey = BOOKINDEX_SIZE + 100;
   BookPosition position;

   position.hashKey = hashKey;
   position.nextPosition = ILLEGAL_OFFSET;
   position.firstMove = 0;

   openBook(&book, "moduletest");
   createEmptyBook(&book);

   assert(getBookpositionOffset(&book, hashKey) == ILLEGAL_OFFSET);
   addBookposition(&book, &position);
   assert(getBookpositionOffset(&book, hashKey) ==
          getBookpositionIndexOffset(hashKey));
   position.hashKey += BOOKINDEX_SIZE;
   addBookposition(&book, &position);
   assert(getBookpositionOffset(&book, position.hashKey) ==
          BOOKINDEX_SIZE * sizeof(BookPosition));

   closeBook(&book);
   assert(remove("moduletest.bki") == 0);
   assert(remove("moduletest.bkm") == 0);

   return 0;
}

static int testMoveOperations()
{
   Book book;
   UINT64 hashKey = 4711;
   BookMove move;

   openBook(&book, "moduletest");
   createEmptyBook(&book);

   move.move = 7;
   assert(getBookmoveOffset(&book, hashKey, move.move) == ILLEGAL_OFFSET);
   move.move = 17;
   move.nextAlternative = ILLEGAL_OFFSET;
   appendBookmove(&book, hashKey, &move);
   assert(getBookmoveOffset(&book, hashKey, move.move) ==
          0 * sizeof(BookMove));
   move.move = 19;
   appendBookmove(&book, hashKey, &move);
   assert(getBookmoveOffset(&book, hashKey, move.move) ==
          1 * sizeof(BookMove));

   hashKey += 160939;
   assert(getBookmoveOffset(&book, hashKey, move.move) == ILLEGAL_OFFSET);
   appendBookmove(&book, hashKey, &move);
   assert(getBookmoveOffset(&book, hashKey, move.move) ==
          2 * sizeof(BookMove));

   closeBook(&book);
   assert(remove("moduletest.bki") == 0);
   assert(remove("moduletest.bkm") == 0);

   return 0;
}

int testModuleBook()
{
   int result;

   if ((result = testPositionOperations()) != 0)
   {
      return result;
   }

   if ((result = testMoveOperations()) != 0)
   {
      return result;
   }

   return 0;
}
