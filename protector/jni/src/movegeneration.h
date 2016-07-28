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

#ifndef _movegeneration_h_
#define _movegeneration_h_

#define CONTINUE 0
#define ABORT 1

#include "protector.h"
#include "position.h"
#include <stdlib.h>

extern const Move NO_MOVE;
extern const Move NULLMOVE;

typedef enum
{
   MGS_INITIALIZE,
   MGS_FINISHED,
   MGS_GOOD_CAPTURES_AND_PROMOTIONS,
   MGS_GOOD_CAPTURES_AND_PROMOTIONS_PURE,
   MGS_GOOD_CAPTURES,
   MGS_KILLER_MOVES,
   MGS_REST,
   MGS_BAD_CAPTURES,
   MGS_ESCAPES,
   MGS_CHECKS,
   MGS_SAFE_CHECKS,
   MGS_DANGEROUS_PAWN_ADVANCES
}
MovegenerationStage;

extern int MG_SCHEME_STANDARD, MG_SCHEME_ESCAPE, MG_SCHEME_CHECKS,
   MG_SCHEME_QUIESCENCE_WITH_CHECKS, MG_SCHEME_QUIESCENCE, MG_SCHEME_CAPTURES;

extern MovegenerationStage moveGenerationStage[100];

/**
 * Check if the specified move is pseudo-legal.
 */
bool moveIsPseudoLegal(const Position * position, const Move move);

/**
 * Check if the specified move is legal.
 */
bool moveIsLegal(const Position * position, const Move move);

/**
 * Get the number of the available pieces moves.
 */
int getNumberOfPieceMoves(const Position * position, const Color color,
                          const int sufficientNumberOfMoves);

/**
 * Get the static exchange eval of the specified move.
 */
int seeMoveRec(Position * position, const Move,
               Bitboard attackers[2], const int minValue);

/**
 * Check if the king can escape a check situation.
 */
bool kingCanEscape(Position * position);

/**
 * Generate moves leading out of check.
 */
void generateEscapes(Movelist * movelist);

/**
 * Generate only check moves.
 */
void generateChecks(Movelist * movelist, bool allChecks);

/**
 * Generate all moves except captures and promotions and the hashmove.
 */
void generateRestMoves(Movelist * movelist);

/**
 * Generate captures and promotions.
 */
void generateSpecialMoves(Movelist * movelist);

/**
 * Generate captures and promotions.
 */
void generateSpecialMovesPure(Movelist * movelist);

/**
 * Generate captures.
 */
void generateCaptures(Movelist * movelist);

/**
 * Generate pawn advances to the seventh rank.
 */
void generateDangerousPawnAdvances(Movelist * movelist);

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleMovegeneration(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleMovegeneration(void);

/**
 * Get the legal moves of the specified variation.
 */
void getLegalMoves(Variation * variation, Movelist * movelist);

/**
 * Get the result of the current position.
 */
Gameresult getGameresult(Variation * variation);

/**
 * Preset the moves of the specified movelist in order to ensure a stable
 * sort process.
 */
void initializeMoveValues(Movelist * movelist);

/**
 * Check if a movelist contains a specific move.
 */
bool listContainsMove(const Movelist * movelist, const Move move);

/**
 * Check if a movelist contains a specific move.
 */
bool listContainsSimpleMove(Movelist * movelist, Square from, Square to);

/**
 * Add a move at the specified position.
 */
void addMoveAtPosition(Movelist * movelist, const Move move,
                       const int insertPosition);

/**
 * Delete the move at the specified position.
 */
void deleteMoveAtPosition(Movelist * movelist, const int position);

/**
 * Check if the given simple move (no capture, no promotion, no castling)
 * is a check.
 */
bool simpleMoveIsCheck(const Position * position, const Move move);

void registerKillerMove(PlyInfo * plyInfo, Move killerMove);
bool passiveKingIsSafe(Position * position);
bool activeKingIsSafe(Position * position);
int seeMove(Position * position, const Move move);
int compareMoves(const void *move1, const void *move2);
void sortMoves(Movelist * movelist);
void initQuiescenceMovelist(Movelist * movelist,
                            Position * position, PlyInfo * plyInfo,
                            UINT16 * historyValue, const Move hashMove,
                            const int restDepth, const bool check);
void initStandardMovelist(Movelist * movelist, Position * position,
                          PlyInfo * plyInfo, UINT16 * historyValue,
                          const Move hashMove, const bool check);
void initPreQuiescenceMovelist(Movelist * movelist,
                               Position * position,
                               PlyInfo * plyInfo,
                               UINT16 * historyValue,
                               const Move hashMove, const bool check);
void initCaptureMovelist(Movelist * movelist,
                         Position * position, PlyInfo * plyInfo,
                         UINT16 * historyValue, const Move hashMove,
                         const bool check);
void initCheckMovelist(Movelist * movelist, Position * position,
                       UINT16 * historyValue);
void initMovelist(Movelist * movelist, Position * position);
Move getNextMove(Movelist * movelist);
void deferMove(Movelist * movelist, Move move);

#endif
