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

#ifndef _evaluation_h_
#define _evaluation_h_

#include "position.h"
#include "bitboard.h"
#include "keytable.h"
#include "io.h"

#ifndef NDEBUG
extern bool debugEval;
#endif

#define MATERIALINFO_TABLE_SIZE ( 648 * 648 )
extern MaterialInfo materialInfo[MATERIALINFO_TABLE_SIZE];
extern Bitboard companionFiles[_64_];
extern Bitboard troitzkyArea[2];
extern Bitboard pawnOpponents[2][_64_];
extern Bitboard krprkDrawFiles;
extern Bitboard A1C1, F1H1, A1B1, G1H1;

#define VALUE_TEMPO_OPENING 20
#define VALUE_TEMPO_ENDGAME 10
#define MIN_PIECE_WEIGHT_FOR_KING_ATTACK 14

void addEvalBonusForColor(EvaluationBase * base, const Color color,
                          const INT32 bonus);
void addEvalMalusForColor(EvaluationBase * base, const Color color,
                          const INT32 bonus);
Color getWinningColor(const Position * position, const int value);
int getWhiteBishopBlockingIndex(const Position * position,
                                const Bitboard bishopSquares);
int getBlackBishopBlockingIndex(const Position * position,
                                const Bitboard bishopSquares);
Bitboard getPromotablePawns(const Position * position, const Color color);
bool oppositeColoredBishops(const Position * position);
int getKnnkpChances(const Position * position, const Color color);
bool passiveKingStopsPawn(const Square kingSquare,
                          const Square pawnSquare, const Color pawnColor);
bool passiveKingOnFileStopsPawn(const Square kingSquare,
                                const Square pawnSquare,
                                const Color pawnColor);
int getKppxKxChances(const Position * position, const Color color);
int getKpxKpxChances(const Position * position, const EvaluationBase * base,
                     const Color color);
int getKrppkrChances(const Position * position, const Color color);
int getKrpkrChances(const Position * position, const Color color);
int getKqppkqChances(const Position * position, const Color color);
int getKqpkqChances(const Position * position, const Color color);
int getKpkChances(const Position * position, const Color color);
int getKbpkChances(const Position * position, const Color color);
int specialPositionChances(const Position * position,
                           const EvaluationBase * base,
                           const SpecialEvalType type, const Color color);
int getChances(const Position * position, const EvaluationBase * base,
               const Color winningColor);
bool hasBishopPair(const Position * position, const Color color);
int phaseValue(const INT32 value, const Position * position,
               EvaluationBase * base);
INT32 materialBalance(const Position * position);
INT32 positionalBalance(const Position * position, EvaluationBase * base);
int basicPositionalBalance(Position * position);
int getValue(const Position * position,
             EvaluationBase * base,
             PawnHashInfo * pawnHashtable,
             KingSafetyHashInfo * kingsafetyHashtable);
bool hasWinningPotential(Position * position, Color color);
Bitboard calculateKingPawnSafetyHashKey(const Position * position,
                                        const Color color);
int getPawnWidth(const Position * position, const Color color);
int getPassedPawnWidth(const Position * position,
                       const EvaluationBase * base, const Color color);
int getMaterialUpPawnCountWeight(int numPawns);

int getLogarithmicValue(const double minValue, const double maxValue,
                        const double numValues, const double valueCount);
double wmbv(const double baseValue);

/**
 * Calculate the value of the specified position.
 *
 * @return the value of the specified position
 */
int getValue(const Position * position,
             EvaluationBase * base,
             PawnHashInfo * pawnHashtable,
             KingSafetyHashInfo * kingsafetyHashtable);

/**
 * Check if the pawn at the specified square is a passed pawn.
 */
bool pawnIsPassed(const Position * position, const Square pawnSquare,
                  const Color pawnColor);

/**
 * Check if a pawn capture creates at least one passer.
 */
bool captureCreatesPasser(Position * position, const Square captureSquare,
                          const Piece capturingPiece);

/**
 * Reset the pawn hashtable.
 */
void resetPawnHashtable(void);

/**
 * Flip the given position and check if it yields the same result.
 *
 * @return FALSE if the flipped position yields a diffent result
 */
bool flipTest(Position * position, PawnHashInfo * pawnHashtable,
              KingSafetyHashInfo * kingsafetyHashtable);

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleEvaluation(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleEvaluation(void);

#endif
