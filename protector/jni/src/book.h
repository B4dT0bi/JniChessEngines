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

#ifndef _book_h_
#define _book_h_

#include "protector.h"
#include "position.h"
#include <stdio.h>

#define BOOKINDEX_SIZE 10007
#define getBookpositionIndex(hashKey) \
        ((hashKey) % BOOKINDEX_SIZE)
#define getBookpositionIndexOffset(hashKey) \
        (getBookpositionIndex(hashKey) * sizeof(BookPosition))

typedef struct
{
   UINT64 hashKey;
   UINT32 nextPosition;
   UINT32 firstMove;
}
BookPosition;

typedef struct
{
   UINT16 move;
   UINT16 numberOfGames;
   INT16 score;
   UINT16 numberOfPersonalGames;
   INT16 personalScore;
   UINT32 nextAlternative;
}
BookMove;

typedef struct
{
   FILE *indexFile, *moveFile;
   bool readonly;
   UINT32 numberOfPositions, numberOfMoves;
}
Book;

extern Book globalBook;

/**
 * Open the book specified by 'name'.
 *
 * @return 0 if no errors occurred.
 */
int openBook(Book * book, const char *name);

/**
 * Create an empty book.
 */
void createEmptyBook(Book * book);

/**
 * Close the specified book.
 */
void closeBook(Book * book);

/**
 * Get all bookmoves for the position specified by 'hashKey'.
 */
void getBookmoves(const Book * book, const UINT64 hashKey,
                  const Movelist * legalMoves, Movelist * bookMoves);

/**
 * Get a move suggestion for the position specified by 'hashKey'.
 *
 * @return an illegal move if the book doesn't contain the position
 */
Move getBookmove(const Book * book, const UINT64 hashKey,
                 const Movelist * legalMoves);

/**
 * Add the specified move to the opening book.
 */
void addBookmove(Book * book, const Position * position,
                 const Move move, const GameResult result,
                 const bool personalResult);

/**
 * Append the database specified by 'filename' to the book specified by 'book'.
 *
 * @param maximumNumberOfPlies the maximum distance between a book position
 *        and the start position
 */
void appendBookDatabase(Book * book, const char *filename,
                        int maximumNumberOfPlies);

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleBook(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleBook(void);

#endif
