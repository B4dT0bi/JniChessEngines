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

#ifndef _pgn_h_
#define _pgn_h_

#include <stdio.h>

typedef struct
{
   long *index;
   long indexSize;
   long numGames;
   FILE *file;
}
PGNFile;

#define PGN_ROASTERLINE_SIZE 256

typedef struct
{
   Square from, to;
   Piece newPiece;
   Position position;
   void *previousMove;
   void *nextMove;
   void *alternativeMove;

   char *comment, *glyphs;
}
Gamemove;

typedef struct
{
   char event[PGN_ROASTERLINE_SIZE];
   char site[PGN_ROASTERLINE_SIZE];
   char date[PGN_ROASTERLINE_SIZE];
   char round[PGN_ROASTERLINE_SIZE];
   char white[PGN_ROASTERLINE_SIZE];
   char black[PGN_ROASTERLINE_SIZE];
   char result[PGN_ROASTERLINE_SIZE];
   char setup[PGN_ROASTERLINE_SIZE];
   char fen[PGN_ROASTERLINE_SIZE];
   char whiteTitle[PGN_ROASTERLINE_SIZE];
   char blackTitle[PGN_ROASTERLINE_SIZE];
   char whiteElo[PGN_ROASTERLINE_SIZE];
   char blackElo[PGN_ROASTERLINE_SIZE];
   char eco[PGN_ROASTERLINE_SIZE];
   char nic[PGN_ROASTERLINE_SIZE];
   char timeControl[PGN_ROASTERLINE_SIZE];
   char termination[PGN_ROASTERLINE_SIZE];

   char *moveText;
   Gamemove *firstMove, *lastMove;
   Gamemove moveHeap[1024];
   int nextMoveFromHeap;
}
PGNGame;

/**
 * Open the PGN file specified by 'filename'.
 *
 * @return 0 if the file could be opened without any error
 */
int openPGNFile(PGNFile * pgnfile, const char *filename);

/**
 * Close the PGN file specified by 'pgnfile'.
 */
void closePGNFile(PGNFile * pgnfile);

/**
 * Get the PGNGame specified by 'number'.
 *
 * @param number the number of the game to be loaded [1...pgnfile.numGames]
 * @param pgngame the struct supposed to contain the game data. It is 
 *        important to free the memory allocated for the game moves as
 *        soon as the pgngame is no longer needed.
 *
 * @return 0 if no errors occured
 */
PGNGame *getGame(PGNFile * pgnfile, int number);

/**
 * Initialize the specified PGNGame.
 */
void initializePGNGame(PGNGame * game);

/**
 * Free all memory allocated for the specified pgn game.
 */
void resetPGNGame(PGNGame * game);

/**
 * Free all memory allocated for the specified pgn game 
 * (including the game itself).
 */
void freePgnGame(PGNGame * game);

/**
 * Generate the SAN notation of the specified move.
 */
void generateMoveText(Variation * variation, const Move move, char *pgnMove);

/**
 * Get the current principal variation of the specified variation.
 *
 * @return a newly allocated string
 */
char *getPrincipalVariation(const Variation * variation);

/**
 * Generate the pgn text of the specified game.
 */
char *generatePgn(PGNGame * game);

/**
 * Initialize a variation with the specified game.
 */
void initializeVariationFromGame(Variation * variation, PGNGame * game);

/**
 * Initialize a game with the specified variation.
 */
void initializeGameFromVariation(const Variation * variation, PGNGame * game,
                                 bool copyPv);
/**
 * Convert a gamemove to a move.
 */
Move gameMove2Move(const Gamemove * gamemove);

/**
 * Interpret the given pgn move and return an appropriate Move.
 */
Move interpretPGNMove(const char *moveText, PGNGame * game);

/**
 * Append the specified move to the specified game. If the move
 * is found to be illegal this call has no effect on the game and
 * a nonzero value will be returned.
 *
 * @return 0 if and only if the specified move was legal
 */
int appendMove(PGNGame * game, const Move move);

/**
 * Take back the last move of the specified game.
 */
void takebackLastMove(PGNGame * game);

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModulePgn(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModulePgn(void);

#endif
