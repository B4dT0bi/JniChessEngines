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
#include <math.h>
#include "movegeneration.h"
#include "evaluation.h"
#include "fen.h"
#include "io.h"
#include "hash.h"
#include "evaluation.h"

#define PAWN_FOR_KNIGHT basicValue[BLACK_PAWN] - basicValue[WHITE_KNIGHT]
#define PAWN_FOR_BISHOP basicValue[BLACK_PAWN] - basicValue[WHITE_BISHOP]

int pieceOrder[16], promotionPieceValue[16];
MovegenerationStage moveGenerationStage[100];
int MG_SCHEME_STANDARD, MG_SCHEME_ESCAPE, MG_SCHEME_CHECKS,
   MG_SCHEME_QUIESCENCE_WITH_CHECKS, MG_SCHEME_QUIESCENCE, MG_SCHEME_CAPTURES;
const Move NO_MOVE = (B4 << 6) | A1;    /* (a1-b4 is always an illegal move) */
const Move NULLMOVE = 0;
const int VALUEOFFSET_PROMOTION_TO_QUEEN = 4000;
const int VALUEOFFSET_HISTORY_MOVE = 24000;
const int VALUEOFFSET_BAD_MOVE = 28000;

/**
 * Register the specified killermove.
 */
void registerKillerMove(PlyInfo * plyInfo, Move killerMove)
{
   if (plyInfo->killerMove1 != killerMove)
   {
      plyInfo->killerMove2 = plyInfo->killerMove1;
      plyInfo->killerMove1 = killerMove;
   }
}

/**
 * Test if the passive king can be captured.
 */
bool passiveKingIsSafe(Position * position)
{
   return (bool)
      (getDirectAttackers(position,
                          position->king[opponent(position->activeColor)],
                          position->activeColor,
                          position->allPieces) == EMPTY_BITBOARD);
}

/**
 * Test if the active king is safe (i.e. not in check).
 */
bool activeKingIsSafe(Position * position)
{
   return (bool)
      (getDirectAttackers(position,
                          position->king[position->activeColor],
                          opponent(position->activeColor),
                          position->allPieces) == EMPTY_BITBOARD);
}

int seeMove(Position * position, const Move move)
{
   const Square to = getToSquare(move);
   const Piece targetPiece = position->piece[to];
   const Bitboard all = position->allPieces;
   const Square enPassantSquare = position->enPassantSquare;
   int result;
   Bitboard attackers[2];

   attackers[WHITE] =
      getDirectAttackers(position, to, WHITE, position->allPieces);
   attackers[BLACK] =
      getDirectAttackers(position, to, BLACK, position->allPieces);

   result = seeMoveRec(position, move, attackers, VALUE_MATED);
   position->enPassantSquare = enPassantSquare;
   position->allPieces = all;
   position->piece[to] = targetPiece;

   return result;
}

/**
 * Compare the value of the two specified moves.
 */
int compareMoves(const void *move1, const void *move2)
{
   return getMoveValue(*((Move *) move2)) - getMoveValue(*((Move *) move1));
}

/**
 * Sort the specified movelist.
 */
void sortMoves(Movelist * movelist)
{
   qsort(&(movelist->moves[0]), movelist->numberOfMoves,
         sizeof(Move), compareMoves);
}

/**
 * Initialize the specified movelist for quiescence move generation.
 */
void initQuiescenceMovelist(Movelist * movelist,
                            Position * position, PlyInfo * plyInfo,
                            UINT16 * historyValue, const Move hashMove,
                            const int restDepth, const bool check)
{
   movelist->position = position;
   movelist->plyInfo = plyInfo;
   movelist->historyValue = historyValue;
   movelist->nextMove = movelist->numberOfPieces = 0;
   movelist->numberOfMoves = movelist->numberOfBadCaptures = 0;
   movelist->hashMove = hashMove;
   movelist->killer1Executed = movelist->killer2Executed = FALSE;
   movelist->killer3Executed = movelist->killer4Executed = FALSE;
   movelist->killer5Executed = movelist->killer6Executed = FALSE;

   if (check)
   {
      movelist->currentStage = MG_SCHEME_ESCAPE;
   }
   else
   {
      movelist->currentStage =
         (restDepth >= 0 ?
          MG_SCHEME_QUIESCENCE_WITH_CHECKS : MG_SCHEME_QUIESCENCE);
   }

   if (hashMove != NO_MOVE)
   {
      movelist->moves[movelist->numberOfMoves++] = hashMove;
   }
}

/**
 * Initialize the specified movelist for capture move generation.
 */
void initCaptureMovelist(Movelist * movelist,
                         Position * position, PlyInfo * plyInfo,
                         UINT16 * historyValue, const Move hashMove,
                         const bool check)
{
   movelist->position = position;
   movelist->plyInfo = plyInfo;
   movelist->historyValue = historyValue;
   movelist->nextMove = movelist->numberOfPieces = 0;
   movelist->numberOfMoves = movelist->numberOfBadCaptures = 0;
   movelist->hashMove = hashMove;
   movelist->killer1Executed = movelist->killer2Executed = FALSE;
   movelist->killer3Executed = movelist->killer4Executed = FALSE;
   movelist->killer5Executed = movelist->killer6Executed = FALSE;

   if (check)
   {
      movelist->currentStage = MG_SCHEME_ESCAPE;
   }
   else
   {
      movelist->currentStage = MGS_GOOD_CAPTURES;
   }

   if (hashMove != NO_MOVE)
   {
      movelist->moves[movelist->numberOfMoves++] = hashMove;
   }
}

/**
 * Initialize the specified movelist for standard move generation.
 */
void initStandardMovelist(Movelist * movelist, Position * position,
                          PlyInfo * plyInfo, UINT16 * historyValue,
                          const Move hashMove, const bool check)
{
   movelist->position = position;
   movelist->plyInfo = plyInfo;
   movelist->historyValue = historyValue;
   movelist->nextMove = movelist->numberOfPieces = 0;
   movelist->numberOfMoves = movelist->numberOfBadCaptures = 0;
   movelist->hashMove = hashMove;

   if (check)
   {
      movelist->currentStage = MG_SCHEME_ESCAPE;
   }
   else
   {
      movelist->currentStage = MG_SCHEME_STANDARD;

      if (hashMove != NO_MOVE)
      {
         movelist->moves[movelist->numberOfMoves++] = hashMove;
      }
   }
}

/**
 * Initialize the specified movelist for check move generation.
 */
void initCheckMovelist(Movelist * movelist, Position * position,
                       UINT16 * historyValue)
{
   movelist->position = position;
   movelist->plyInfo = 0;
   movelist->historyValue = historyValue;
   movelist->nextMove = movelist->numberOfPieces = 0;
   movelist->numberOfMoves = movelist->numberOfBadCaptures = 0;
   movelist->hashMove = NO_MOVE;
   movelist->currentStage = MG_SCHEME_CHECKS;
   movelist->killer1Executed = movelist->killer2Executed = FALSE;
   movelist->killer3Executed = movelist->killer4Executed = FALSE;
   movelist->killer5Executed = movelist->killer6Executed = FALSE;
}

/**
 * Initialize the specified movelist for move insertion.
 */
void initMovelist(Movelist * movelist, Position * position)
{
   movelist->position = position;
   movelist->plyInfo = 0;
   movelist->historyValue = 0;
   movelist->nextMove = movelist->numberOfPieces = 0;
   movelist->numberOfMoves = movelist->numberOfBadCaptures = 0;
   movelist->hashMove = NO_MOVE;
   movelist->currentStage = MG_SCHEME_CHECKS;
   movelist->killer1Executed = movelist->killer2Executed = FALSE;
   movelist->killer3Executed = movelist->killer4Executed = FALSE;
   movelist->killer5Executed = movelist->killer6Executed = FALSE;
}

Move getNextMove(Movelist * movelist)
{
   do
   {
      if (movelist->nextMove < movelist->numberOfMoves)
      {
         switch (moveGenerationStage[movelist->currentStage])
         {
         case MGS_GOOD_CAPTURES_AND_PROMOTIONS:
         case MGS_GOOD_CAPTURES_AND_PROMOTIONS_PURE:
         case MGS_GOOD_CAPTURES:
            {
               const Move move = movelist->moves[movelist->nextMove++];

               if (basicValue[movelist->position->piece[getFromSquare(move)]]
                   > basicValue[movelist->position->piece[getToSquare(move)]]
                   && seeMove(movelist->position, move) < 0)
               {
                  movelist->badCaptures[movelist->numberOfBadCaptures++] =
                     move;

                  continue;
               }

               return move;
            }

         case MGS_SAFE_CHECKS:
            {
               const Move move = movelist->moves[movelist->nextMove++];

               if (seeMove(movelist->position, move) < 0)
               {
                  continue;
               }

               return move;
            }

         default:

            return movelist->moves[movelist->nextMove++];
         }
      }
      else
      {
         Position *position;
         Move killer1, killer2, killer3, killer4, killer5, killer6;

         switch (moveGenerationStage[++movelist->currentStage])
         {
         case MGS_GOOD_CAPTURES_AND_PROMOTIONS:
            generateSpecialMoves(movelist);
            break;

         case MGS_GOOD_CAPTURES_AND_PROMOTIONS_PURE:
            generateSpecialMovesPure(movelist);
            break;

         case MGS_GOOD_CAPTURES:
            generateCaptures(movelist);
            break;

         case MGS_KILLER_MOVES:
            movelist->killer1Executed = movelist->killer2Executed = FALSE;
            movelist->killer3Executed = movelist->killer4Executed = FALSE;
            movelist->numberOfMoves = movelist->nextMove = 0;
            position = movelist->position;
            killer1 = movelist->plyInfo->killerMove1;
            killer2 = movelist->plyInfo->killerMove2;
            killer3 = movelist->plyInfo->killerMove3;
            killer4 = movelist->plyInfo->killerMove4;
            killer5 = movelist->plyInfo->killerMove5;
            killer6 = movelist->plyInfo->killerMove6;

            if (moveIsPseudoLegal(position, killer1) &&
                position->piece[getToSquare(killer1)] == NO_PIECE &&
                position->piece[getFromSquare(killer1)] ==
                (Piece) getMoveValue(killer1) &&
                (pieceType(position->piece[getFromSquare(killer1)]) != PAWN ||
                 getToSquare(killer1) != position->enPassantSquare) &&
                movesAreEqual(killer1, movelist->hashMove) == FALSE)
            {
               setMoveValue(&killer1, 6);
               movelist->moves[movelist->numberOfMoves++] = killer1;
               movelist->killer1Executed = TRUE;
            }

            if (moveIsPseudoLegal(position, killer2) &&
                position->piece[getToSquare(killer2)] == NO_PIECE &&
                position->piece[getFromSquare(killer2)] ==
                (Piece) getMoveValue(killer2) &&
                (pieceType(position->piece[getFromSquare(killer2)]) != PAWN ||
                 getToSquare(killer2) != position->enPassantSquare) &&
                movesAreEqual(killer2, movelist->hashMove) == FALSE)
            {
               setMoveValue(&killer2, 5);
               movelist->moves[movelist->numberOfMoves++] = killer2;
               movelist->killer2Executed = TRUE;
            }

            if (moveIsPseudoLegal(position, killer3) &&
                position->piece[getToSquare(killer3)] == NO_PIECE &&
                position->piece[getFromSquare(killer3)] ==
                (Piece) getMoveValue(killer3) &&
                (pieceType(position->piece[getFromSquare(killer3)]) != PAWN ||
                 getToSquare(killer3) != position->enPassantSquare) &&
                movesAreEqual(killer3, movelist->hashMove) == FALSE &&
                movesAreEqual(killer3, killer1) == FALSE &&
                movesAreEqual(killer3, killer2) == FALSE)
            {
               setMoveValue(&killer3, 4);
               movelist->moves[movelist->numberOfMoves++] = killer3;
               movelist->killer3Executed = TRUE;
            }

            if (moveIsPseudoLegal(position, killer4) &&
                position->piece[getToSquare(killer4)] == NO_PIECE &&
                position->piece[getFromSquare(killer4)] ==
                (Piece) getMoveValue(killer4) &&
                (pieceType(position->piece[getFromSquare(killer4)]) != PAWN ||
                 getToSquare(killer4) != position->enPassantSquare) &&
                movesAreEqual(killer4, movelist->hashMove) == FALSE &&
                movesAreEqual(killer4, killer1) == FALSE &&
                movesAreEqual(killer4, killer2) == FALSE)
            {
               setMoveValue(&killer4, 3);
               movelist->moves[movelist->numberOfMoves++] = killer4;
               movelist->killer4Executed = TRUE;
            }

            if (moveIsPseudoLegal(position, killer5) &&
                position->piece[getToSquare(killer5)] == NO_PIECE &&
                position->piece[getFromSquare(killer5)] ==
                (Piece) getMoveValue(killer5) &&
                (pieceType(position->piece[getFromSquare(killer5)]) != PAWN ||
                 getToSquare(killer5) != position->enPassantSquare) &&
                movesAreEqual(killer5, movelist->hashMove) == FALSE &&
                movesAreEqual(killer5, killer1) == FALSE &&
                movesAreEqual(killer5, killer2) == FALSE &&
                movesAreEqual(killer5, killer3) == FALSE &&
                movesAreEqual(killer5, killer4) == FALSE)
            {
               setMoveValue(&killer5, 2);
               movelist->moves[movelist->numberOfMoves++] = killer5;
               movelist->killer5Executed = TRUE;
            }

            if (moveIsPseudoLegal(position, killer6) &&
                position->piece[getToSquare(killer6)] == NO_PIECE &&
                position->piece[getFromSquare(killer6)] ==
                (Piece) getMoveValue(killer6) &&
                (pieceType(position->piece[getFromSquare(killer6)]) != PAWN ||
                 getToSquare(killer6) != position->enPassantSquare) &&
                movesAreEqual(killer6, movelist->hashMove) == FALSE &&
                movesAreEqual(killer6, killer1) == FALSE &&
                movesAreEqual(killer6, killer2) == FALSE &&
                movesAreEqual(killer6, killer3) == FALSE &&
                movesAreEqual(killer6, killer4) == FALSE)
            {
               setMoveValue(&killer6, 1);
               movelist->moves[movelist->numberOfMoves++] = killer6;
               movelist->killer6Executed = TRUE;
            }

            break;

         case MGS_REST:
            generateRestMoves(movelist);
            break;

         case MGS_BAD_CAPTURES:
            movelist->numberOfMoves = movelist->numberOfBadCaptures;
            movelist->nextMove = 0;
            memmove(&movelist->moves[0],
                    &movelist->badCaptures[0],
                    movelist->numberOfMoves * sizeof(Move));
            break;

         case MGS_ESCAPES:
            generateEscapes(movelist);
            break;

         case MGS_SAFE_CHECKS:
            generateChecks(movelist, FALSE);
            break;

         case MGS_CHECKS:
            generateChecks(movelist, TRUE);
            break;

         case MGS_DANGEROUS_PAWN_ADVANCES:
            generateDangerousPawnAdvances(movelist);
            break;

         default:
            break;
         }
      }
   }
   while (moveGenerationStage[movelist->currentStage] != MGS_FINISHED);

   return NO_MOVE;
}

bool moveIsPseudoLegal(const Position * position, const Move move)
{
   const Square from = getFromSquare(move), to = getToSquare(move);
   const Piece newPiece = getNewPiece(move);
   Piece piece;
   Bitboard moves;

   if (squareIsValid(from) == FALSE || squareIsValid(to) == FALSE ||
       (piece = position->piece[from]) == NO_PIECE ||
       pieceColor(piece) != position->activeColor)
   {
      return FALSE;
   }

   if (pieceType(position->piece[from]) == PAWN &&
       colorRank(position->activeColor, from) == RANK_7)
   {
      if (newPiece != WHITE_QUEEN && newPiece != WHITE_ROOK &&
          newPiece != WHITE_BISHOP && newPiece != WHITE_KNIGHT)
      {
         return FALSE;
      }
   }
   else
   {
      if (newPiece != NO_PIECE)
      {
         return FALSE;
      }
   }

   if (pieceType(position->piece[from]) == PAWN &&
       position->enPassantSquare != NO_SQUARE)
   {
      Bitboard allPieces = position->allPieces |
         minValue[position->enPassantSquare];

      moves = getMoves(from, piece, allPieces);
   }
   else
   {
      moves = getMoves(from, piece, position->allPieces);
   }

   excludeSquares(moves, position->piecesOfColor[position->activeColor]);

   if (pieceType(piece) == KING &&
       hasCastlings(position->activeColor, position->castlingRights))
   {
      moves |= getCastlingMoves(position->activeColor,
                                position->castlingRights,
                                position->allPieces);
   }

   return (bool) testSquare(moves, to);
}

bool moveIsLegal(const Position * position, const Move move)
{
   bool result = FALSE;
   Variation variation;

   if (moveIsPseudoLegal(position, move) == FALSE)
   {
      return FALSE;
   }

   setBasePosition(&variation, position);

   if (makeMove(&variation, move) == 0 &&
       passiveKingIsSafe(&variation.singlePosition))
   {
      result = TRUE;
   }

   unmakeLastMove(&variation);

   return result;
}

static Bitboard getPinnedPieces(const Position * position,
                                const Color pinningColor)
{
   const Square kingSquare = position->king[opponent(pinningColor)];
   Bitboard pinnedPieces = EMPTY_BITBOARD, pinningCandidates =
      (generalMoves[ROOK][kingSquare] &
       (position->piecesOfType[QUEEN | pinningColor] |
        position->piecesOfType[ROOK | pinningColor])) |
      (generalMoves[BISHOP][kingSquare] &
       (position->piecesOfType[QUEEN | pinningColor] |
        position->piecesOfType[BISHOP | pinningColor]));
   Square pinningCandidate;

   ITERATE_BITBOARD(&pinningCandidates, pinningCandidate)
   {
      const Bitboard imSquares = squaresBetween[kingSquare][pinningCandidate];

      if ((imSquares & position->piecesOfColor[pinningColor]) ==
          EMPTY_BITBOARD)
      {
         const Bitboard pinnedCandidates = imSquares &
            position->piecesOfColor[opponent(pinningColor)];

         if (getNumberOfSetSquares(pinnedCandidates) == 1)
         {
            pinnedPieces |= pinnedCandidates;
         }
      }
   }

   return pinnedPieces;
}

int getNumberOfPieceMoves(const Position * position, const Color color,
                          const int sufficientNumberOfMoves)
{
   const Color oppColor = opponent(color);
   const Bitboard unpinnedPieces = ~getPinnedPieces(position, oppColor);
   const Bitboard permittedSquares = ~position->piecesOfColor[color];
   int numberOfMoves = 0;
   Square square;
   Bitboard moves, pieces = position->piecesOfColor[color] &
      ~(position->piecesOfType[PAWN | color] |
        minValue[position->king[color]]) & unpinnedPieces;

   ITERATE_BITBOARD(&pieces, square)
   {
      switch (pieceType(position->piece[square]))
      {
      case QUEEN:
         moves =
            getMagicQueenMoves(square,
                               position->allPieces) & permittedSquares;
         break;

      case ROOK:
         moves =
            getMagicRookMoves(square, position->allPieces) & permittedSquares;
         break;

      case BISHOP:
         moves =
            getMagicBishopMoves(square,
                                position->allPieces) & permittedSquares;
         break;

      case KNIGHT:
         moves = getKnightMoves(square) & permittedSquares;
         break;

      default:
         moves = EMPTY_BITBOARD;
      }

      numberOfMoves += getNumberOfSetSquares(moves);

      if (numberOfMoves >= sufficientNumberOfMoves)
      {
         return numberOfMoves;
      }
   }

   return numberOfMoves;
}

int seeMoveRec(Position * position, const Move move,
               Bitboard attackers[2], const int minValue)
{
   const Square from = getFromSquare(move), to = getToSquare(move);
   const Color activeColor = pieceColor(position->piece[from]);
   const Color passiveColor = opponent(activeColor);
   int valueCaptured = basicValue[position->piece[to]];
   Piece leastValuableAttacker, bestNewPiece;
   Bitboard leastValuableAttackers;
   Move bestCapture;
   int recResult;

   if (pieceType(position->piece[to]) == KING)
   {
      return valueCaptured;
   }

   position->piece[to] = position->piece[from];
   clearSquare(position->allPieces, from);
   clearSquare(attackers[activeColor], from);

   if (horizontalDistance(to, from) == verticalDistance(to, from))
   {
      attackers[WHITE] |=
         getMagicBishopMoves(to, position->allPieces) &
         (position->piecesOfType[WHITE_QUEEN] |
          position->piecesOfType[WHITE_BISHOP]) & position->allPieces;
      attackers[BLACK] |=
         getMagicBishopMoves(to, position->allPieces) &
         (position->piecesOfType[BLACK_QUEEN] |
          position->piecesOfType[BLACK_BISHOP]) & position->allPieces;
   }
   else
   {
      attackers[WHITE] |=
         getMagicRookMoves(to, position->allPieces) &
         (position->piecesOfType[WHITE_QUEEN] |
          position->piecesOfType[WHITE_ROOK]) & position->allPieces;
      attackers[BLACK] |=
         getMagicRookMoves(to, position->allPieces) &
         (position->piecesOfType[BLACK_QUEEN] |
          position->piecesOfType[BLACK_ROOK]) & position->allPieces;
   }

   if (to == position->enPassantSquare &&
       pieceType(position->piece[to]) == PAWN)
   {
      const Square captureSquare = (Square)
         (to + (rank(from) - rank(to)) * 8);

      clearSquare(position->allPieces, captureSquare);
      attackers[WHITE] |= getMagicBishopMoves(to, position->allPieces) &
         (position->piecesOfType[WHITE_QUEEN] |
          position->piecesOfType[WHITE_BISHOP]) & position->allPieces;
      attackers[WHITE] |= getMagicRookMoves(to, position->allPieces) &
         (position->piecesOfType[WHITE_QUEEN] |
          position->piecesOfType[WHITE_ROOK]) & position->allPieces;
      attackers[BLACK] |= getMagicBishopMoves(to, position->allPieces) &
         (position->piecesOfType[BLACK_QUEEN] |
          position->piecesOfType[BLACK_BISHOP]) & position->allPieces;
      attackers[BLACK] |= getMagicRookMoves(to, position->allPieces) &
         (position->piecesOfType[BLACK_QUEEN] |
          position->piecesOfType[BLACK_ROOK]) & position->allPieces;

      valueCaptured += basicValue[position->piece[captureSquare]];
   }

   if (getNewPiece(move) != NO_PIECE)
   {
      valueCaptured += basicValue[getNewPiece(move) | activeColor] -
         basicValue[PAWN | activeColor];
      position->piece[to] = (Piece) (getNewPiece(move) | activeColor);
   }

   if (attackers[passiveColor] == EMPTY_BITBOARD)
   {
      return valueCaptured;
   }

   position->enPassantSquare = NO_SQUARE;

   if (attackers[passiveColor] & position->piecesOfType[PAWN | passiveColor])
   {
      leastValuableAttacker = (Piece) (PAWN | passiveColor);
   }
   else if (attackers[passiveColor] &
            position->piecesOfType[KNIGHT | passiveColor])
   {
      leastValuableAttacker = (Piece) (KNIGHT | passiveColor);
   }
   else if (attackers[passiveColor] &
            position->piecesOfType[BISHOP | passiveColor])
   {
      leastValuableAttacker = (Piece) (BISHOP | passiveColor);
   }
   else if (attackers[passiveColor] &
            position->piecesOfType[ROOK | passiveColor])
   {
      leastValuableAttacker = (Piece) (ROOK | passiveColor);
   }
   else if (attackers[passiveColor] &
            position->piecesOfType[QUEEN | passiveColor])
   {
      leastValuableAttacker = (Piece) (QUEEN | passiveColor);
   }
   else
   {
      leastValuableAttacker = (Piece) (KING | passiveColor);
   }

   leastValuableAttackers =
      attackers[passiveColor] & position->piecesOfType[leastValuableAttacker];

   if ((leastValuableAttacker == WHITE_PAWN && rank(to) == RANK_8) ||
       (leastValuableAttacker == BLACK_PAWN && rank(to) == RANK_1))
   {
      bestNewPiece = WHITE_QUEEN;
   }
   else
   {
      bestNewPiece = NO_PIECE;
   }

   bestCapture =
      getMove(getLastSquare(&leastValuableAttackers), to, bestNewPiece, 0);

   recResult = seeMoveRec(position, bestCapture, attackers, 0);

   /*
      logDebug("valueCaptured = %d\n", valueCaptured);
      logDebug("recResult = %d\n", recResult);
      logDebug("returnValue = %d\n", max(minValue, valueCaptured - recResult));
    */

   return max(minValue, valueCaptured - recResult);
}

void getLegalMoves(Variation * variation, Movelist * movelist)
{
   const int ply = variation->ply;
   Position *position = &variation->singlePosition;
   PlyInfo *plyInfo = &variation->plyInfo[ply];
   Movelist allMoves;
   Move hashmove = NO_MOVE;
   Move currentMove;

   allMoves.positionalGain = &(variation->positionalGain[0]);
   initMovelist(movelist, position);

   plyInfo->killerMove1 = NO_MOVE;
   plyInfo->killerMove2 = NO_MOVE;
   plyInfo->killerMove3 = NO_MOVE;
   plyInfo->killerMove4 = NO_MOVE;
   plyInfo->killerMove5 = NO_MOVE;
   plyInfo->killerMove6 = NO_MOVE;

   initStandardMovelist(&allMoves, position,
                        plyInfo, &variation->historyValue[0],
                        hashmove, FALSE);

   while ((currentMove = getNextMove(&allMoves)) != NO_MOVE)
   {
      if (moveIsLegal(position, currentMove))
      {
         movelist->moves[movelist->numberOfMoves++] = currentMove;
      }
   }

   sortMoves(movelist);
}

Gameresult getGameresult(Variation * variation)
{
   Movelist movelist;
   Gameresult result;
   Position *pos = &variation->singlePosition;
   int i, repetitionCount = 0, historyLimit;

   /* Check for a draw by repetition */

   historyLimit = POSITION_HISTORY_OFFSET - pos->halfMoveClock;

   for (i = POSITION_HISTORY_OFFSET - 4; i >= historyLimit; i -= 2)
   {
      if (pos->hashKey == variation->positionHistory[i])
      {
         repetitionCount++;
      }
   }

   if (repetitionCount >= 2)
   {
      strcpy(result.result, GAMERESULT_DRAW);
      strcpy(result.reason, GAMERESULT_REPETITION);

      return result;
   }

   if (pos->halfMoveClock >= 100)
   {
      strcpy(result.result, GAMERESULT_DRAW);
      strcpy(result.reason, GAMERESULT_50_MOVE_RULE);

      return result;
   }

   if (pos->numberOfPawns[WHITE] == 0 &&
       pos->numberOfPawns[BLACK] == 0 &&
       hasWinningPotential(pos, WHITE) == FALSE &&
       hasWinningPotential(pos, BLACK) == FALSE)
   {
      strcpy(result.result, GAMERESULT_DRAW);
      strcpy(result.reason, GAMERESULT_INSUFFICIENT_MATERIAL);

      return result;
   }

   getLegalMoves(variation, &movelist);

   if (movelist.numberOfMoves > 0)
   {
      strcpy(result.result, GAMERESULT_UNKNOWN);
      strcpy(result.reason, "");
   }
   else
   {
      if (activeKingIsSafe(pos))
      {
         strcpy(result.result, GAMERESULT_DRAW);
         strcpy(result.reason, GAMERESULT_STALEMATE);
      }
      else
      {
         if (pos->activeColor == WHITE)
         {
            strcpy(result.result, GAMERESULT_BLACK_WINS);
            strcpy(result.reason, GAMERESULT_BLACK_MATES);
         }
         else
         {
            strcpy(result.result, GAMERESULT_WHITE_WINS);
            strcpy(result.reason, GAMERESULT_WHITE_MATES);
         }
      }
   }

   return result;
}

bool listContainsMove(const Movelist * movelist, const Move move)
{
   int i;
   const Move shortMove = move & 0xFFFF;

   for (i = 0; i < movelist->numberOfMoves; i++)
   {
      if ((movelist->moves[i] & 0xFFFF) == shortMove)
      {
         return TRUE;
      }
   }

   return FALSE;
}

static void deleteMove(Movelist * movelist, const Move move)
{
   int i;
   const Move shortMove = move & 0xFFFF;

   for (i = 0; i < movelist->numberOfMoves; i++)
   {
      if ((movelist->moves[i] & 0xFFFF) == shortMove)
      {
         movelist->numberOfMoves--;

         if (i < movelist->numberOfMoves)
         {
            memmove(&movelist->moves[i],
                    &movelist->moves[i + 1],
                    (movelist->numberOfMoves - i) * sizeof(Move));
         }

         return;
      }
   }
}

static void setMoveValueInList(Movelist * movelist, const Move move)
{
   int i;
   const Move shortMove = move & 0xFFFF;

   for (i = 0; i < movelist->numberOfMoves; i++)
   {
      if ((movelist->moves[i] & 0xFFFF) == shortMove)
      {
         movelist->moves[i] = move;

         return;
      }
   }
}

bool listContainsSimpleMove(Movelist * movelist, const Square from,
                            const Square to)
{
   return listContainsMove(movelist, getPackedMove(from, to, NO_PIECE));
}

void initializeMoveValues(Movelist * movelist)
{
   int i;

   for (i = 0; i < movelist->numberOfMoves; i++)
   {
      setMoveValue(&movelist->moves[i], VALUE_MATED - 1 - i);
   }
}

static void addMoveByValue(Movelist * movelist, const Move move)
{
   int low = -1, high = movelist->numberOfMoves, insertPosition;
   const int value = getMoveValue(move);

   while (high - low > 1)
   {
      const int avg = (low + high) >> 1;

      assert(avg >= 0);
      assert(avg < movelist->numberOfMoves);

      if (value <= getMoveValue(movelist->moves[avg]))
      {
         low = avg;
      }
      else
      {
         high = avg;
      }
   }

   insertPosition = low + 1;

   assert(insertPosition >= 0);
   assert(insertPosition <= movelist->numberOfMoves);
   assert(insertPosition == 0 ||
          value <= getMoveValue(movelist->moves[insertPosition - 1]));

   if (insertPosition < movelist->numberOfMoves)
   {
      assert(value > getMoveValue(movelist->moves[insertPosition]));

      memmove(&movelist->moves[insertPosition + 1],
              &movelist->moves[insertPosition],
              (movelist->numberOfMoves - insertPosition) * sizeof(Move));
   }

   movelist->moves[insertPosition] = move;
   movelist->numberOfMoves++;
}

void addMoveAtPosition(Movelist * movelist, const Move move,
                       const int insertPosition)
{
   assert(insertPosition >= 0);
   assert(insertPosition <= movelist->numberOfMoves);

   if (insertPosition < movelist->numberOfMoves)
   {
      memmove(&movelist->moves[insertPosition + 1],
              &movelist->moves[insertPosition],
              (movelist->numberOfMoves - insertPosition) * sizeof(Move));
   }

   movelist->moves[insertPosition] = move;
   movelist->numberOfMoves++;
}

void deleteMoveAtPosition(Movelist * movelist, const int position)
{
   assert(position >= 0);
   assert(position < movelist->numberOfMoves);

   movelist->numberOfMoves--;

   if (position < movelist->numberOfMoves)
   {
      memmove(&movelist->moves[position],
              &movelist->moves[position + 1],
              (movelist->numberOfMoves - position) * sizeof(Move));
   }
}

static INT16 captureMoveSortValue(const Position * position,
                                  const Square from, const Square to)
{
   return (INT16) (6 * pieceOrder[position->piece[to]] -
                   pieceOrder[position->piece[from]]);
}

static INT16 promotionMoveSortValue(const Position * position,
                                    const Square to, const Piece newPiece)
{
   return (INT16) (promotionPieceValue[newPiece] +
                   6 * pieceOrder[position->piece[to]]);
}

static INT16 historyMoveSortValue(const Position * position,
                                  const Movelist * movelist, const Move move)
{
   return (INT16) (movelist->historyValue[historyIndex(move, position)] -
                   VALUEOFFSET_HISTORY_MOVE);
}

static void addCaptures(Movelist * movelist, const Position * position,
                        const Square from, Bitboard captures)
{
   Square to;

   ITERATE_BITBOARD(&captures, to)
   {
      const INT16 value = captureMoveSortValue(position, from, to);
      int i = 0;

      movelist->moves[movelist->numberOfMoves] = (value - 1) << 16;

      while (getMoveValue(movelist->moves[i]) >= value)
      {
         i++;
      }

      if (i < movelist->numberOfMoves)
      {
         memmove(&movelist->moves[i + 1], &movelist->moves[i],
                 (movelist->numberOfMoves - i) * sizeof(Move));
      }

      movelist->moves[i] = getMove(from, to, NO_PIECE, value);
      movelist->numberOfMoves++;
   }
}

static void addPromotions(Movelist * movelist,
                          const Square from, Bitboard moves)
{
   Square to;

   ITERATE_BITBOARD(&moves, to)
   {
      INT16 value;

      value = promotionMoveSortValue(movelist->position, to, WHITE_QUEEN);
      addMoveByValue(movelist, getMove(from, to, WHITE_QUEEN, value));

      value = promotionMoveSortValue(movelist->position, to, WHITE_ROOK);
      movelist->badCaptures[movelist->numberOfBadCaptures++] =
         getMove(from, to, WHITE_ROOK, value);

      value = promotionMoveSortValue(movelist->position, to, WHITE_BISHOP);
      movelist->badCaptures[movelist->numberOfBadCaptures++] =
         getMove(from, to, WHITE_BISHOP, value);

      value = promotionMoveSortValue(movelist->position, to, WHITE_KNIGHT);
      movelist->badCaptures[movelist->numberOfBadCaptures++] =
         getMove(from, to, WHITE_KNIGHT, value);
   }
}

void deferMove(Movelist * movelist, Move move)
{
   const int targetSpot = min(movelist->numberOfMoves - 1,
                              movelist->nextMove + 1);

   if (targetSpot > movelist->nextMove &&
       movelist->numberOfMoves < MAX_MOVES_PER_POSITION - 1)
   {
      int i;

      for (i = movelist->numberOfMoves; i > targetSpot; i--)
      {
         movelist->moves[i] = movelist->moves[i - 1];
      }

      movelist->moves[targetSpot] = move;
      movelist->numberOfMoves++;
   }
}

void generateSpecialMoves(Movelist * movelist)
{
   const Position *position = movelist->position;
   const Color activeColor = position->activeColor;
   const Color passiveColor = opponent(activeColor);
   const Bitboard captureTargets = position->piecesOfColor[passiveColor];
   Bitboard moves =
      EMPTY_BITBOARD, pieces, pawnCaptureTargets, promotionPawns;
   const Square hashFrom = getFromSquare(movelist->hashMove);
   Square from;

   movelist->nextMove = 0;
   movelist->numberOfMoves = movelist->numberOfBadCaptures = 0;
   pawnCaptureTargets = captureTargets;

   if (position->enPassantSquare != NO_SQUARE)
   {
      setSquare(pawnCaptureTargets, position->enPassantSquare);
   }

   pieces = position->piecesOfType[PAWN | activeColor];
   promotionPawns = pieces & promotionCandidates[activeColor];
   pieces &= ~promotionPawns;

   ITERATE_BITBOARD(&promotionPawns, from)
   {
      moves = getPawnCaptures((Piece) (PAWN | activeColor), from,
                              pawnCaptureTargets) |
         getPawnAdvances(activeColor, from, position->allPieces);

      if (moves != EMPTY_BITBOARD)
      {
         addPromotions(movelist, from, moves);

         if (hashFrom == from)
         {
            deleteMove(movelist, movelist->hashMove);
         }
      }
   }

   if (position->activeColor == WHITE)
   {
      const Bitboard capturingPawns =
         ((pawnCaptureTargets & nonA) >> 9) |
         ((pawnCaptureTargets & nonH) >> 7);

      pieces &= capturingPawns;
   }
   else
   {
      const Bitboard capturingPawns =
         ((pawnCaptureTargets & nonA) << 7) |
         ((pawnCaptureTargets & nonH) << 9);

      pieces &= capturingPawns;
   }

   ITERATE_BITBOARD(&pieces, from)
   {
      moves = getPawnCaptures((Piece) (PAWN | activeColor), from,
                              pawnCaptureTargets);

      if (moves != EMPTY_BITBOARD)
      {
         if (hashFrom == from)
         {
            clearSquare(moves, getToSquare(movelist->hashMove));
         }

         addCaptures(movelist, position, from, moves);
      }
   }

   pieces = getOrdinaryPieces(position, activeColor);

   ITERATE_BITBOARD(&pieces, from)
   {
      switch (pieceType(position->piece[from]))
      {
      case QUEEN:
         moves = getMagicQueenMoves(from, position->allPieces);
         break;

      case ROOK:
         moves = getMagicRookMoves(from, position->allPieces);
         break;

      case BISHOP:
         moves = getMagicBishopMoves(from, position->allPieces);
         break;

      case KNIGHT:
         moves = getKnightMoves(from);
         break;

      default:
         break;
      }

      if (hashFrom == from)
      {
         clearSquare(moves, getToSquare(movelist->hashMove));
      }

      movelist->movesOfPiece[movelist->numberOfPieces].square = from;
      movelist->movesOfPiece[movelist->numberOfPieces++].moves = moves;
      moves &= captureTargets;

      if (moves != EMPTY_BITBOARD)
      {
         addCaptures(movelist, position, from, moves);
      }
   }

   moves = getKingMoves(position->king[activeColor]);

   if (hasCastlings(position->activeColor, position->castlingRights))
   {
      moves |= getCastlingMoves(position->activeColor,
                                position->castlingRights,
                                position->allPieces);
   }

   if (hashFrom == position->king[activeColor])
   {
      clearSquare(moves, getToSquare(movelist->hashMove));
   }

   movelist->movesOfPiece[movelist->numberOfPieces].square =
      position->king[activeColor];
   movelist->movesOfPiece[movelist->numberOfPieces++].moves = moves;
   moves &= captureTargets;

   if (moves != EMPTY_BITBOARD)
   {
      addCaptures(movelist, position, position->king[activeColor], moves);
   }
}

void generateSpecialMovesPure(Movelist * movelist)
{
   const Position *position = movelist->position;
   const Color activeColor = position->activeColor;
   const Color passiveColor = opponent(activeColor);
   const Bitboard captureTargets = position->piecesOfColor[passiveColor];
   Bitboard moves =
      EMPTY_BITBOARD, pieces, pawnCaptureTargets, promotionPawns;
   const Square hashFrom = getFromSquare(movelist->hashMove);
   Square from;

   movelist->nextMove = 0;
   movelist->numberOfMoves = movelist->numberOfBadCaptures = 0;
   pawnCaptureTargets = captureTargets;

   if (position->enPassantSquare != NO_SQUARE)
   {
      setSquare(pawnCaptureTargets, position->enPassantSquare);
   }

   pieces = position->piecesOfType[(Piece) (PAWN | activeColor)];
   promotionPawns = pieces & promotionCandidates[activeColor];
   pieces &= ~promotionPawns;

   ITERATE_BITBOARD(&promotionPawns, from)
   {
      moves = getPawnCaptures((Piece) (PAWN | activeColor), from,
                              pawnCaptureTargets) |
         getPawnAdvances(activeColor, from, position->allPieces);

      if (moves != EMPTY_BITBOARD)
      {
         addPromotions(movelist, from, moves);

         if (hashFrom == from)
         {
            deleteMove(movelist, movelist->hashMove);
         }
      }
   }

   if (position->activeColor == WHITE)
   {
      const Bitboard capturingPawns =
         ((pawnCaptureTargets & nonA) >> 9) |
         ((pawnCaptureTargets & nonH) >> 7);

      pieces &= capturingPawns;
   }
   else
   {
      const Bitboard capturingPawns =
         ((pawnCaptureTargets & nonA) << 7) |
         ((pawnCaptureTargets & nonH) << 9);

      pieces &= capturingPawns;
   }

   ITERATE_BITBOARD(&pieces, from)
   {
      moves = getPawnCaptures((Piece) (PAWN | activeColor), from,
                              pawnCaptureTargets);

      if (moves != EMPTY_BITBOARD)
      {
         if (hashFrom == from)
         {
            clearSquare(moves, getToSquare(movelist->hashMove));
         }

         addCaptures(movelist, position, from, moves);
      }
   }

   pieces = getOrdinaryPieces(position, activeColor);

   ITERATE_BITBOARD(&pieces, from)
   {
      switch (pieceType(position->piece[from]))
      {
      case QUEEN:
         moves = getMagicQueenMoves(from, position->allPieces);
         break;

      case ROOK:
         moves = getMagicRookMoves(from, position->allPieces);
         break;

      case BISHOP:
         moves = getMagicBishopMoves(from, position->allPieces);
         break;

      case KNIGHT:
         moves = getKnightMoves(from);
         break;

      case KING:
         moves = getKingMoves(position->king[activeColor]);
         break;

      default:
         break;
      }

      moves &= captureTargets;

      if (moves != EMPTY_BITBOARD)
      {
         if (hashFrom == from)
         {
            clearSquare(moves, getToSquare(movelist->hashMove));
         }

         if (moves != EMPTY_BITBOARD)
         {
            addCaptures(movelist, position, from, moves);
         }
      }
   }

   moves = getKingMoves(position->king[activeColor]) & captureTargets;

   if (moves != EMPTY_BITBOARD)
   {
      if (hashFrom == position->king[activeColor])
      {
         clearSquare(moves, getToSquare(movelist->hashMove));
      }

      if (moves != EMPTY_BITBOARD)
      {
         addCaptures(movelist, position, position->king[activeColor], moves);
      }
   }
}

void generateCaptures(Movelist * movelist)
{
   const Position *position = movelist->position;
   const Color activeColor = position->activeColor;
   const Color passiveColor = opponent(activeColor);
   const Bitboard captureTargets = position->piecesOfColor[passiveColor];
   Bitboard moves =
      EMPTY_BITBOARD, pieces, pawnCaptureTargets, promotionPawns;
   const Square hashFrom = getFromSquare(movelist->hashMove);
   Square from;

   movelist->nextMove = 0;
   movelist->numberOfMoves = movelist->numberOfBadCaptures = 0;
   pawnCaptureTargets = captureTargets;

   pieces = position->piecesOfType[PAWN | activeColor];
   promotionPawns = pieces & promotionCandidates[activeColor];
   pieces &= ~promotionPawns;

   ITERATE_BITBOARD(&promotionPawns, from)
   {
      moves = getPawnCaptures((Piece) (PAWN | activeColor), from,
                              pawnCaptureTargets);

      if (moves != EMPTY_BITBOARD)
      {
         addPromotions(movelist, from, moves);

         if (hashFrom == from)
         {
            deleteMove(movelist, movelist->hashMove);
         }
      }
   }

   if (position->activeColor == WHITE)
   {
      const Bitboard capturingPawns =
         ((pawnCaptureTargets & nonA) >> 9) |
         ((pawnCaptureTargets & nonH) >> 7);

      pieces &= capturingPawns;
   }
   else
   {
      const Bitboard capturingPawns =
         ((pawnCaptureTargets & nonA) << 7) |
         ((pawnCaptureTargets & nonH) << 9);

      pieces &= capturingPawns;
   }

   ITERATE_BITBOARD(&pieces, from)
   {
      moves = getPawnCaptures((Piece) (PAWN | activeColor), from,
                              pawnCaptureTargets);

      if (moves != EMPTY_BITBOARD)
      {
         if (hashFrom == from)
         {
            clearSquare(moves, getToSquare(movelist->hashMove));
         }

         addCaptures(movelist, position, from, moves);
      }
   }

   pieces = getOrdinaryPieces(position, activeColor);

   ITERATE_BITBOARD(&pieces, from)
   {
      switch (pieceType(position->piece[from]))
      {
      case QUEEN:
         moves = getMagicQueenMoves(from, position->allPieces);
         break;

      case ROOK:
         moves = getMagicRookMoves(from, position->allPieces);
         break;

      case BISHOP:
         moves = getMagicBishopMoves(from, position->allPieces);
         break;

      case KNIGHT:
         moves = getKnightMoves(from);
         break;

      default:
         break;
      }

      if (hashFrom == from)
      {
         clearSquare(moves, getToSquare(movelist->hashMove));
      }

      movelist->movesOfPiece[movelist->numberOfPieces].square = from;
      movelist->movesOfPiece[movelist->numberOfPieces++].moves = moves;
      moves &= captureTargets;

      if (moves != EMPTY_BITBOARD)
      {
         addCaptures(movelist, position, from, moves);
      }
   }

   moves = getKingMoves(position->king[activeColor]);

   if (hashFrom == position->king[activeColor])
   {
      clearSquare(moves, getToSquare(movelist->hashMove));
   }

   movelist->movesOfPiece[movelist->numberOfPieces].square =
      position->king[activeColor];
   movelist->movesOfPiece[movelist->numberOfPieces++].moves = moves;
   moves &= captureTargets;

   if (moves != EMPTY_BITBOARD)
   {
      addCaptures(movelist, position, position->king[activeColor], moves);
   }
}

void generateRestMoves(Movelist * movelist)
{
   const Position *position = movelist->position;
   const Bitboard permittedSquares = ~position->allPieces;
   const Color activeColor = position->activeColor;
   Bitboard pieces = (activeColor == WHITE ?
                      position->piecesOfType[WHITE_PAWN] &
                      ~((position->allPieces >> 8) | squaresOfRank[RANK_7]) :
                      position->piecesOfType[BLACK_PAWN] &
                      ~((position->allPieces << 8) | squaresOfRank[RANK_2]));
   Square from;
   const Square k1from =
      (movelist->killer1Executed ?
       getFromSquare(movelist->plyInfo->killerMove1) : NO_SQUARE);
   const Square k2from =
      (movelist->killer2Executed ?
       getFromSquare(movelist->plyInfo->killerMove2) : NO_SQUARE);
   const Square k3from =
      (movelist->killer3Executed ?
       getFromSquare(movelist->plyInfo->killerMove3) : NO_SQUARE);
   const Square k4from =
      (movelist->killer4Executed ?
       getFromSquare(movelist->plyInfo->killerMove4) : NO_SQUARE);
   const Square k5from =
      (movelist->killer5Executed ?
       getFromSquare(movelist->plyInfo->killerMove5) : NO_SQUARE);
   const Square k6from =
      (movelist->killer6Executed ?
       getFromSquare(movelist->plyInfo->killerMove6) : NO_SQUARE);
   int i;

   ITERATE_BITBOARD(&pieces, from)
   {
      Bitboard moves =
         getPawnAdvances(activeColor, from, position->allPieces);

      assert(moves != EMPTY_BITBOARD);

      if (getFromSquare(movelist->hashMove) == from)
      {
         clearSquare(moves, getToSquare(movelist->hashMove));
      }

      movelist->movesOfPiece[movelist->numberOfPieces].square = from;
      movelist->movesOfPiece[movelist->numberOfPieces++].moves = moves;
   }

   movelist->nextMove = 0;
   movelist->numberOfMoves = 0;

   for (i = 0; i < movelist->numberOfPieces; i++)
   {
      Bitboard moves = movelist->movesOfPiece[i].moves & permittedSquares;
      UINT8 moveSquares[_64_];
      int numMoves, moveIndex;

      from = movelist->movesOfPiece[i].square;

      if (from == k1from)
      {
         clearSquare(moves, getToSquare(movelist->plyInfo->killerMove1));
      }

      if (from == k2from)
      {
         clearSquare(moves, getToSquare(movelist->plyInfo->killerMove2));
      }

      if (from == k3from)
      {
         clearSquare(moves, getToSquare(movelist->plyInfo->killerMove3));
      }

      if (from == k4from)
      {
         clearSquare(moves, getToSquare(movelist->plyInfo->killerMove4));
      }

      if (from == k5from)
      {
         clearSquare(moves, getToSquare(movelist->plyInfo->killerMove5));
      }

      if (from == k6from)
      {
         clearSquare(moves, getToSquare(movelist->plyInfo->killerMove6));
      }

      numMoves = getSetSquares(moves, moveSquares);

      for (moveIndex = 0; moveIndex < numMoves; moveIndex++)
      {
         Move move = getOrdinaryMove(from, (Square) moveSquares[moveIndex]);

         setMoveValue(&move, historyMoveSortValue(position, movelist, move));
         addMoveByValue(movelist, move);

         assert(movesAreEqual(move, movelist->hashMove) == FALSE);
      }
   }
}

void generateDangerousPawnAdvances(Movelist * movelist)
{
   const Position *position = movelist->position;
   const Color activeColor = position->activeColor;
   const Rank sixthRank = (activeColor == WHITE ? RANK_6 : RANK_3);
   Bitboard moves, pieces = position->piecesOfType[PAWN | activeColor] &
      squaresOfRank[sixthRank];
   Square from;

   ITERATE_BITBOARD(&pieces, from)
   {
      if ((moves = getPawnAdvances(activeColor, from, position->allPieces)) !=
          EMPTY_BITBOARD)
      {
         const Square to = getLastSquare(&moves);
         Move move = getPackedMove(from, to, NO_PIECE);

         if (move == movelist->hashMove)
         {
            continue;
         }

         setMoveValue(&move, historyMoveSortValue(position, movelist, move));
         addMoveByValue(movelist, move);
      }
   }
}

void generateChecks(Movelist * movelist, bool allChecks)
{
   Position *position = movelist->position;
   const Color activeColor = position->activeColor;
   const Color passiveColor = opponent(activeColor);
   Bitboard permittedSquares;
   Bitboard moves, orthoChecks, diaChecks, pawnChecks;
   Bitboard pieces = position->piecesOfType[PAWN | activeColor] &
      ~promotionCandidates[activeColor];
   const Square opponentKing = position->king[passiveColor];
   Square from;
   int i;

   ITERATE_BITBOARD(&pieces, from)
   {
      if ((moves = getPawnAdvances(activeColor, from, position->allPieces)) !=
          EMPTY_BITBOARD)
      {
         if (getFromSquare(movelist->hashMove) == from)
         {
            clearSquare(moves, getToSquare(movelist->hashMove));
         }

         movelist->movesOfPiece[movelist->numberOfPieces].square = from;
         movelist->movesOfPiece[movelist->numberOfPieces++].moves = moves;
      }
   }

   movelist->nextMove = 0;
   movelist->numberOfMoves = movelist->numberOfBadCaptures = 0;
   orthoChecks = getMagicRookMoves(opponentKing, position->allPieces);
   diaChecks = getMagicBishopMoves(opponentKing, position->allPieces);
   pawnChecks = generalMoves[PAWN | passiveColor][opponentKing];
   permittedSquares = (allChecks ? ~position->piecesOfColor[activeColor] :
                       ~position->allPieces);

   for (i = 0; i < movelist->numberOfPieces; i++)
   {
      Bitboard checks = EMPTY_BITBOARD;
      const Square from = movelist->movesOfPiece[i].square;
      const PieceType piece = pieceType(position->piece[from]);
      Square batteryPieceSquare = NO_SQUARE, to;

      moves = movelist->movesOfPiece[i].moves & permittedSquares;

      if (testSquare(orthoChecks, from))
      {
         static const int pieceProperties = PP_SLIDING_PIECE | PP_ORTHOPIECE;
         Bitboard batteryPiece = position->piecesOfColor[activeColor] &
            squaresBehind[from][opponentKing] &
            getMagicRookMoves(from, position->allPieces);
         batteryPieceSquare = getLastSquare(&batteryPiece);

         if (batteryPieceSquare != NO_SQUARE &&
             (int) (position->piece[batteryPieceSquare] & pieceProperties) !=
             pieceProperties)
         {
            batteryPieceSquare = NO_SQUARE;
         }
      }
      else if (testSquare(diaChecks, from))
      {
         static const int pieceProperties = PP_SLIDING_PIECE | PP_DIAPIECE;
         Bitboard batteryPiece = position->piecesOfColor[activeColor] &
            squaresBehind[from][opponentKing] &
            getMagicBishopMoves(from, position->allPieces);
         batteryPieceSquare = getLastSquare(&batteryPiece);

         if (batteryPieceSquare != NO_SQUARE &&
             (int) (position->piece[batteryPieceSquare] & pieceProperties) !=
             pieceProperties)
         {
            batteryPieceSquare = NO_SQUARE;
         }
      }

      if (batteryPieceSquare != NO_SQUARE)
      {
         checks = moves & ~squaresBetween[opponentKing][batteryPieceSquare];
      }
      else
      {
         switch (piece)
         {
         case QUEEN:
            checks = moves & (orthoChecks | diaChecks);
            break;

         case ROOK:
            checks = moves & orthoChecks;
            break;

         case BISHOP:
            checks = moves & diaChecks;
            break;

         case KNIGHT:
            checks = moves & getKnightMoves(opponentKing);
            break;

         case PAWN:
            checks = moves & pawnChecks;
            break;

         default:
            break;
         }
      }

      if (piece == KING)
      {
         Bitboard castlingMoves = moves &
            ~getKingMoves(position->king[activeColor]);
         Square kingSquare;

         ITERATE_BITBOARD(&castlingMoves, kingSquare)
         {
            const Bitboard obstacles = position->allPieces &
               maxValue[position->king[activeColor]];
            Bitboard rookMoves;
            const Square rookSquare = (Square)
               ((kingSquare + position->king[activeColor]) / 2);

            rookMoves = getMagicRookMoves(rookSquare, obstacles);

            if (testSquare(rookMoves, opponentKing))
            {
               setSquare(checks, kingSquare);
            }
         }
      }

      if (checks != EMPTY_BITBOARD)
      {
         if (from == getFromSquare(movelist->hashMove))
         {
            clearSquare(checks, getToSquare(movelist->hashMove));
         }

         if (movelist->killer1Executed &&
             from == getFromSquare(movelist->plyInfo->killerMove1))
         {
            clearSquare(checks, getToSquare(movelist->plyInfo->killerMove1));
         }

         if (movelist->killer2Executed &&
             from == getFromSquare(movelist->plyInfo->killerMove2))
         {
            clearSquare(checks, getToSquare(movelist->plyInfo->killerMove2));
         }

         if (movelist->killer3Executed &&
             from == getFromSquare(movelist->plyInfo->killerMove3))
         {
            clearSquare(checks, getToSquare(movelist->plyInfo->killerMove3));
         }

         if (movelist->killer4Executed &&
             from == getFromSquare(movelist->plyInfo->killerMove4))
         {
            clearSquare(checks, getToSquare(movelist->plyInfo->killerMove4));
         }

         if (movelist->killer5Executed &&
             from == getFromSquare(movelist->plyInfo->killerMove5))
         {
            clearSquare(checks, getToSquare(movelist->plyInfo->killerMove5));
         }

         if (movelist->killer6Executed &&
             from == getFromSquare(movelist->plyInfo->killerMove6))
         {
            clearSquare(checks, getToSquare(movelist->plyInfo->killerMove6));
         }

         movelist->movesOfPiece[i].moves &= ~checks;

         ITERATE_BITBOARD(&checks, to)
         {
            Move move = getPackedMove(from, to, NO_PIECE);

            setMoveValue(&move,
                         historyMoveSortValue(position, movelist, move));
            addMoveByValue(movelist, move);
         }
      }
   }
}

void generateEscapes(Movelist * movelist)
{
   Move move;
   Square from, to;
   Piece newPiece;
   INT16 value;
   PieceType attackerType;
   Position *position = movelist->position;
   const Color activeColor = position->activeColor;
   const Color passiveColor = opponent(activeColor);
   Square kingsquare = position->king[activeColor], attackerSquare;
   Bitboard attackers = getDirectAttackers(position, kingsquare, passiveColor,
                                           position->allPieces), defenders;
   Bitboard kingmoves =
      getKingMoves(kingsquare) & ~position->piecesOfColor[activeColor];
   Bitboard corridor;
   const Bitboard obstacles = position->allPieces & maxValue[kingsquare];

   movelist->nextMove = 0;
   movelist->numberOfMoves = movelist->numberOfBadCaptures = 0;
   from = kingsquare;
   newPiece = NO_PIECE;

   ITERATE_BITBOARD(&kingmoves, to)
   {
      if (getDirectAttackers(position, to, passiveColor, obstacles) ==
          EMPTY_BITBOARD)
      {
         move = getPackedMove(from, to, newPiece);

         if (position->piece[to] == NO_PIECE)
         {
            value = historyMoveSortValue(position, movelist, move);
         }
         else
         {
            value = captureMoveSortValue(position, from, to);
         }

         setMoveValue(&move, value);
         movelist->moves[movelist->numberOfMoves++] = move;
      }
   }

   if (getNumberOfSetSquares(attackers) > 1)
   {
      goto sortMoves;
   }

   attackerSquare = getLastSquare(&attackers);
   attackerType = pieceType(position->piece[attackerSquare]);
   defenders = getDirectAttackers(position, attackerSquare, activeColor,
                                  position->allPieces);
   clearSquare(defenders, kingsquare);
   to = attackerSquare;

   ITERATE_BITBOARD(&defenders, from)
   {
      if (pieceType(position->piece[from]) == PAWN &&
          testSquare(promotionCandidates[activeColor], from))
      {
         INT16 value;

         value = promotionMoveSortValue(movelist->position, to, WHITE_QUEEN);
         movelist->moves[movelist->numberOfMoves++] =
            getMove(from, to, WHITE_QUEEN, value);

         value = promotionMoveSortValue(movelist->position, to, WHITE_ROOK);
         movelist->moves[movelist->numberOfMoves++] =
            getMove(from, to, WHITE_ROOK, value);

         value = promotionMoveSortValue(movelist->position, to, WHITE_BISHOP);
         movelist->moves[movelist->numberOfMoves++] =
            getMove(from, to, WHITE_BISHOP, value);

         value = promotionMoveSortValue(movelist->position, to, WHITE_KNIGHT);
         movelist->moves[movelist->numberOfMoves++] =
            getMove(from, to, WHITE_KNIGHT, value);
      }
      else
      {
         value = captureMoveSortValue(position, from, to);
         move = getMove(from, to, NO_PIECE, value);

         if (basicValue[position->piece[from]] >
             basicValue[position->piece[to]] && seeMove(position, move) < 0)
         {
            setMoveValue(&move, value - VALUEOFFSET_BAD_MOVE);
         }

         movelist->moves[movelist->numberOfMoves++] = move;
      }
   }

   if (position->enPassantSquare != NO_SQUARE && attackerType == PAWN)
   {
      defenders = getPawnCaptures((Piece) (PAWN | opponent(activeColor)),
                                  position->enPassantSquare,
                                  position->piecesOfType[PAWN | activeColor]);

      to = position->enPassantSquare;
      newPiece = NO_PIECE;
      value = (INT16) basicValue[PAWN | opponent(activeColor)];

      ITERATE_BITBOARD(&defenders, from)
      {
         movelist->moves[movelist->numberOfMoves++] =
            getMove(from, to, newPiece, value);
      }

      goto sortMoves;
   }

   if ((attackerType & PP_SLIDING_PIECE) == 0)
   {
      goto sortMoves;
   }

   corridor = squaresBetween[attackerSquare][kingsquare];

   ITERATE_BITBOARD(&corridor, to)
   {
      defenders = getInterestedPieces(position, to, activeColor);
      clearSquare(defenders, kingsquare);

      ITERATE_BITBOARD(&defenders, from)
      {
         if (pieceType(position->piece[from]) == PAWN &&
             testSquare(promotionCandidates[activeColor], from))
         {
            INT16 value;

            value = promotionMoveSortValue(movelist->position,
                                           to, WHITE_QUEEN);

            if (seeMove(position, getPackedMove(from, to, WHITE_QUEEN)) < 0)
            {
               value = (INT16) (value - (VALUEOFFSET_BAD_MOVE +
                                         VALUEOFFSET_PROMOTION_TO_QUEEN));
            }

            movelist->moves[movelist->numberOfMoves++] =
               getMove(from, to, WHITE_QUEEN, value);

            value = promotionMoveSortValue(movelist->position,
                                           to, WHITE_ROOK);
            movelist->moves[movelist->numberOfMoves++] =
               getMove(from, to, WHITE_ROOK, value);

            value = promotionMoveSortValue(movelist->position,
                                           to, WHITE_BISHOP);
            movelist->moves[movelist->numberOfMoves++] =
               getMove(from, to, WHITE_BISHOP, value);

            value = promotionMoveSortValue(movelist->position,
                                           to, WHITE_KNIGHT);
            movelist->moves[movelist->numberOfMoves++] =
               getMove(from, to, WHITE_KNIGHT, value);
         }
         else
         {
            move = getPackedMove(from, to, NO_PIECE);

            if (seeMove(position, move) >= 0)
            {
               value = historyMoveSortValue(position, movelist, move);
            }
            else
            {
               value = (INT16) (captureMoveSortValue(position, from, to) -
                                VALUEOFFSET_BAD_MOVE);
            }

            setMoveValue(&move, value);
            movelist->moves[movelist->numberOfMoves++] = move;
         }
      }
   }

 sortMoves:

   if (movelist->hashMove != NO_MOVE && movelist->numberOfMoves > 1)
   {
      move = movelist->hashMove;
      setMoveValue(&move, 32000);
      setMoveValueInList(movelist, move);
   }

   sortMoves(movelist);

   assert(movelist->hashMove == NO_MOVE ||
          movesAreEqual(movelist->moves[0], movelist->hashMove));
}

bool simpleMoveIsCheck(const Position * position, const Move move)
{
   const Color attackingColor = position->activeColor;
   const Color defendingColor = opponent(attackingColor);
   const Square from = getFromSquare(move);
   const Square to = getToSquare(move);
   const Piece movingPiece = position->piece[from];
   const PieceType movingPieceType = pieceType(movingPiece);
   const Square target = position->king[defendingColor];
   const Bitboard moves =
      getMoves(to, position->piece[from], position->allPieces);

   if (testSquare(moves, target) && movingPieceType != KING)
   {
      return TRUE;
   }

   if (testSquare(getMagicRookMoves(target, position->allPieces), from))
   {
      Bitboard batteryPieces =
         (position->piecesOfType[ROOK | attackingColor] |
          position->piecesOfType[QUEEN | attackingColor]) &
         squaresBehind[from][target];
      Square square;

      ITERATE_BITBOARD(&batteryPieces, square)
      {
         Bitboard blockingPieces = (position->allPieces | minValue[to]) &
            squaresBetween[target][square] & maxValue[from];

         if (blockingPieces == EMPTY_BITBOARD)
         {
            return TRUE;
         }
      }
   }

   if (testSquare(getMagicBishopMoves(target, position->allPieces), from))
   {
      Bitboard batteryPieces =
         (position->piecesOfType[BISHOP | attackingColor] |
          position->piecesOfType[QUEEN | attackingColor]) &
         squaresBehind[from][target];
      Square square;

      ITERATE_BITBOARD(&batteryPieces, square)
      {
         Bitboard blockingPieces = (position->allPieces | minValue[to]) &
            squaresBetween[target][square] & maxValue[from];

         if (blockingPieces == EMPTY_BITBOARD)
         {
            return TRUE;
         }
      }
   }

   return FALSE;
}

bool kingCanEscape(Position * position)
{
   Square square;
   PieceType attackerType;
   const Color activeColor = position->activeColor;
   const Color passiveColor = opponent(activeColor);
   Square kingsquare = position->king[activeColor], attackerSquare;
   Bitboard attackers = getDirectAttackers(position, kingsquare, passiveColor,
                                           position->allPieces), defenders;
   Bitboard kingmoves =
      getKingMoves(kingsquare) & ~position->piecesOfColor[activeColor];
   Bitboard corridor;
   const Bitboard unpinnedPieces = ~getPinnedPieces(position, passiveColor);
   const Bitboard obstacles = position->allPieces & maxValue[kingsquare];

   ITERATE_BITBOARD(&kingmoves, square)
   {
      if (getDirectAttackers(position, square, passiveColor, obstacles) ==
          EMPTY_BITBOARD)
      {
         return TRUE;
      }
   }

   if (getNumberOfSetSquares(attackers) > 1)
   {
      return FALSE;
   }

   attackerSquare = getLastSquare(&attackers);
   attackerType = pieceType(position->piece[attackerSquare]);
   defenders = getDirectAttackers(position, attackerSquare, activeColor,
                                  position->allPieces);
   clearSquare(defenders, kingsquare);

   if ((defenders & unpinnedPieces) != EMPTY_BITBOARD)
   {
      return TRUE;
   }

   if (position->enPassantSquare != NO_SQUARE && attackerType == PAWN)
   {
      defenders = getPawnCaptures((Piece) (PAWN | opponent(activeColor)),
                                  position->enPassantSquare,
                                  position->piecesOfType[PAWN | activeColor]);

      if ((defenders & unpinnedPieces) != EMPTY_BITBOARD)
      {
         return TRUE;
      }
   }

   if ((attackerType & PP_SLIDING_PIECE) == 0)
   {
      return FALSE;
   }

   corridor = squaresBetween[attackerSquare][kingsquare];

   ITERATE_BITBOARD(&corridor, square)
   {
      defenders = getInterestedPieces(position, square, activeColor);
      clearSquare(defenders, kingsquare);

      if ((defenders & unpinnedPieces) != EMPTY_BITBOARD)
      {
         return TRUE;
      }
   }

   return FALSE;
}

int initializeModuleMovegeneration()
{
   int i = 0;

   MG_SCHEME_STANDARD = i;
   moveGenerationStage[i++] = MGS_INITIALIZE;
   moveGenerationStage[i++] = MGS_GOOD_CAPTURES_AND_PROMOTIONS;
   moveGenerationStage[i++] = MGS_KILLER_MOVES;
   moveGenerationStage[i++] = MGS_REST;
   moveGenerationStage[i++] = MGS_BAD_CAPTURES;
   moveGenerationStage[i++] = MGS_FINISHED;

   MG_SCHEME_ESCAPE = i;
   moveGenerationStage[i++] = MGS_INITIALIZE;
   moveGenerationStage[i++] = MGS_ESCAPES;
   moveGenerationStage[i++] = MGS_FINISHED;

   MG_SCHEME_QUIESCENCE_WITH_CHECKS = i;
   moveGenerationStage[i++] = MGS_INITIALIZE;
   moveGenerationStage[i++] = MGS_GOOD_CAPTURES_AND_PROMOTIONS;
   moveGenerationStage[i++] = MGS_SAFE_CHECKS;
   moveGenerationStage[i++] = MGS_FINISHED;

   MG_SCHEME_QUIESCENCE = i;
   moveGenerationStage[i++] = MGS_INITIALIZE;
   moveGenerationStage[i++] = MGS_GOOD_CAPTURES_AND_PROMOTIONS_PURE;
   moveGenerationStage[i++] = MGS_FINISHED;

   MG_SCHEME_CAPTURES = i;
   moveGenerationStage[i++] = MGS_INITIALIZE;
   moveGenerationStage[i++] = MGS_GOOD_CAPTURES;
   moveGenerationStage[i++] = MGS_FINISHED;

   MG_SCHEME_CHECKS = i;
   moveGenerationStage[i++] = MGS_INITIALIZE;
   moveGenerationStage[i++] = MGS_GOOD_CAPTURES_AND_PROMOTIONS;
   moveGenerationStage[i++] = MGS_CHECKS;
   moveGenerationStage[i++] = MGS_FINISHED;

   pieceOrder[NO_PIECE] = 0;
   pieceOrder[WHITE_PAWN] = pieceOrder[BLACK_PAWN] = 1;
   pieceOrder[WHITE_KNIGHT] = pieceOrder[BLACK_KNIGHT] = 2;
   pieceOrder[WHITE_BISHOP] = pieceOrder[BLACK_BISHOP] = 3;
   pieceOrder[WHITE_ROOK] = pieceOrder[BLACK_ROOK] = 4;
   pieceOrder[WHITE_QUEEN] = pieceOrder[BLACK_QUEEN] = 5;
   pieceOrder[WHITE_KING] = pieceOrder[BLACK_KING] = 6;

   promotionPieceValue[WHITE_KNIGHT] = promotionPieceValue[BLACK_KNIGHT] =
      2 - VALUEOFFSET_BAD_MOVE;
   promotionPieceValue[WHITE_BISHOP] = promotionPieceValue[BLACK_BISHOP] =
      3 - VALUEOFFSET_BAD_MOVE;
   promotionPieceValue[WHITE_ROOK] = promotionPieceValue[BLACK_ROOK] =
      4 - VALUEOFFSET_BAD_MOVE;
   promotionPieceValue[WHITE_QUEEN] = promotionPieceValue[BLACK_QUEEN] =
      VALUEOFFSET_PROMOTION_TO_QUEEN;

   return 0;
}

#ifndef NDEBUG

static int testPseudoLegalMoves()
{
   Variation variation;
   Bitboard attackers, interested;

   initializeVariation(&variation, FEN_GAMESTART);
   assert(moveIsPseudoLegal
          (&variation.singlePosition, getPackedMove(E2, E4, NO_PIECE)));
   assert(moveIsPseudoLegal
          (&variation.singlePosition, getPackedMove(G1, F3, NO_PIECE)));
   assert(moveIsPseudoLegal
          (&variation.singlePosition,
           getPackedMove(G1, E2, NO_PIECE)) == FALSE);
   assert(moveIsPseudoLegal
          (&variation.singlePosition,
           getPackedMove(G8, F6, NO_PIECE)) == FALSE);
   assert(moveIsPseudoLegal
          (&variation.singlePosition,
           getPackedMove(E4, E5, NO_PIECE)) == FALSE);

   attackers =
      getDirectAttackers(&variation.singlePosition, F3, WHITE,
                         variation.singlePosition.allPieces);
   assert(testSquare(attackers, G1));
   assert(testSquare(attackers, E2));
   assert(testSquare(attackers, G2));
   assert(getNumberOfSetSquares(attackers) == 3);

   interested = getInterestedPieces(&variation.singlePosition, F3, WHITE);
   assert(testSquare(interested, G1));
   assert(testSquare(interested, F2));
   assert(getNumberOfSetSquares(interested) == 2);

   interested = getInterestedPieces(&variation.singlePosition, F4, WHITE);
   assert(testSquare(interested, F2));
   assert(getNumberOfSetSquares(interested) == 1);

   attackers =
      getDirectAttackers(&variation.singlePosition, C6, BLACK,
                         variation.singlePosition.allPieces);
   assert(testSquare(attackers, B8));
   assert(testSquare(attackers, B7));
   assert(testSquare(attackers, D7));
   assert(getNumberOfSetSquares(attackers) == 3);

   makeMove(&variation, getPackedMove(E2, E4, NO_PIECE));

   assert(moveIsPseudoLegal
          (&variation.singlePosition,
           getPackedMove(E2, E4, NO_PIECE)) == FALSE);
   assert(moveIsPseudoLegal
          (&variation.singlePosition,
           getPackedMove(G1, F3, NO_PIECE)) == FALSE);
   assert(moveIsPseudoLegal
          (&variation.singlePosition,
           getPackedMove(G1, E2, NO_PIECE)) == FALSE);
   assert(moveIsPseudoLegal
          (&variation.singlePosition, getPackedMove(G8, F6, NO_PIECE)));
   assert(moveIsPseudoLegal
          (&variation.singlePosition,
           getPackedMove(E4, E5, NO_PIECE)) == FALSE);

   attackers =
      getDirectAttackers(&variation.singlePosition, F3, WHITE,
                         variation.singlePosition.allPieces);
   assert(testSquare(attackers, G1));
   assert(testSquare(attackers, D1));
   assert(testSquare(attackers, G2));
   assert(getNumberOfSetSquares(attackers) == 3);

   attackers =
      getDirectAttackers(&variation.singlePosition, D2, WHITE,
                         variation.singlePosition.allPieces);
   assert(testSquare(attackers, B1));
   assert(testSquare(attackers, C1));
   assert(testSquare(attackers, D1));
   assert(testSquare(attackers, E1));
   assert(getNumberOfSetSquares(attackers) == 4);

   makeMove(&variation, getPackedMove(G8, F6, NO_PIECE));

   assert(moveIsPseudoLegal
          (&variation.singlePosition, getPackedMove(E4, E5, NO_PIECE)));

   makeMove(&variation, getPackedMove(E4, E5, NO_PIECE));

   assert(moveIsPseudoLegal
          (&variation.singlePosition, getPackedMove(D7, D5, NO_PIECE)));

   makeMove(&variation, getPackedMove(D7, D5, NO_PIECE));

   assert(moveIsPseudoLegal
          (&variation.singlePosition, getPackedMove(E5, D6, NO_PIECE)));
   assert(moveIsPseudoLegal
          (&variation.singlePosition, getPackedMove(G1, F3, NO_PIECE)));

   makeMove(&variation, getPackedMove(G1, F3, NO_PIECE));

   assert(moveIsPseudoLegal
          (&variation.singlePosition, getPackedMove(E7, E6, NO_PIECE)));

   makeMove(&variation, getPackedMove(E7, E6, NO_PIECE));

   assert(moveIsPseudoLegal
          (&variation.singlePosition, getPackedMove(F1, C4, NO_PIECE)));
   assert(moveIsPseudoLegal
          (&variation.singlePosition,
           getPackedMove(E1, G1, NO_PIECE)) == FALSE);

   makeMove(&variation, getPackedMove(F1, C4, NO_PIECE));

   assert(moveIsPseudoLegal
          (&variation.singlePosition, getPackedMove(F8, C5, NO_PIECE)));
   assert(moveIsPseudoLegal
          (&variation.singlePosition,
           getPackedMove(E8, G8, NO_PIECE)) == FALSE);

   makeMove(&variation, getPackedMove(F8, C5, NO_PIECE));

   assert(moveIsPseudoLegal
          (&variation.singlePosition, getPackedMove(E1, G1, NO_PIECE)));

   makeMove(&variation, getPackedMove(E1, G1, NO_PIECE));

   assert(moveIsPseudoLegal
          (&variation.singlePosition, getPackedMove(E8, G8, NO_PIECE)));

   return 0;
}

static int testLegalMoves()
{
   Variation variation;

   initializeVariation(&variation, FEN_GAMESTART);
   assert(moveIsLegal
          (&variation.singlePosition, getPackedMove(E2, E4, NO_PIECE)));
   assert(moveIsLegal
          (&variation.singlePosition, getPackedMove(G1, F3, NO_PIECE)));

   makeMove(&variation, getPackedMove(E2, E4, NO_PIECE));

   assert(moveIsLegal
          (&variation.singlePosition, getPackedMove(G8, F6, NO_PIECE)));

   makeMove(&variation, getPackedMove(D7, D5, NO_PIECE));
   makeMove(&variation, getPackedMove(F1, B5, NO_PIECE));

   assert(moveIsLegal
          (&variation.singlePosition, getPackedMove(B8, C6, NO_PIECE)));
   assert(moveIsLegal
          (&variation.singlePosition, getPackedMove(B8, D7, NO_PIECE)));
   assert(moveIsLegal
          (&variation.singlePosition,
           getPackedMove(B8, A6, NO_PIECE)) == FALSE);
   assert(moveIsLegal
          (&variation.singlePosition,
           getPackedMove(G8, F6, NO_PIECE)) == FALSE);

   return 0;
}

static int testStaticExchangeEvaluation()
{
   Variation variation;
   Move move;

   initializeVariation(&variation, FEN_GAMESTART);

   makeMove(&variation, getPackedMove(E2, E4, NO_PIECE));
   makeMove(&variation, getPackedMove(E7, E5, NO_PIECE));
   makeMove(&variation, getPackedMove(G1, F3, NO_PIECE));
   makeMove(&variation, getPackedMove(B8, C6, NO_PIECE));

   move = getPackedMove(F3, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == PAWN_FOR_KNIGHT);

   makeMove(&variation, getPackedMove(B1, A3, NO_PIECE));
   makeMove(&variation, getPackedMove(G8, H6, NO_PIECE));
   makeMove(&variation, getPackedMove(A3, C4, NO_PIECE));

   move = getPackedMove(F3, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == basicValue[BLACK_PAWN]);
   move = getPackedMove(C4, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == basicValue[BLACK_PAWN]);

   makeMove(&variation, getPackedMove(H6, G4, NO_PIECE));

   move = getPackedMove(F3, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == PAWN_FOR_KNIGHT);

   makeMove(&variation, getPackedMove(B2, B3, NO_PIECE));
   makeMove(&variation, getPackedMove(G7, G6, NO_PIECE));
   makeMove(&variation, getPackedMove(C1, B2, NO_PIECE));

   move = getPackedMove(F3, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == basicValue[BLACK_PAWN]);
   move = getPackedMove(C4, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == basicValue[BLACK_PAWN]);
   move = getPackedMove(B2, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) ==
          basicValue[BLACK_PAWN] - basicValue[WHITE_BISHOP] +
          basicValue[BLACK_KNIGHT]);

   makeMove(&variation, getPackedMove(F8, G7, NO_PIECE));

   move = getPackedMove(F3, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == PAWN_FOR_KNIGHT);
   move = getPackedMove(C4, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == PAWN_FOR_KNIGHT);
   move = getPackedMove(B2, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == PAWN_FOR_BISHOP);

   makeMove(&variation, getPackedMove(A2, A4, NO_PIECE));
   makeMove(&variation, getPackedMove(H7, H5, NO_PIECE));
   makeMove(&variation, getPackedMove(A4, A5, NO_PIECE));
   makeMove(&variation, getPackedMove(H5, H4, NO_PIECE));
   makeMove(&variation, getPackedMove(A5, A6, NO_PIECE));
   makeMove(&variation, getPackedMove(H4, H3, NO_PIECE));
   makeMove(&variation, getPackedMove(A1, A5, NO_PIECE));

   move = getPackedMove(F3, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) > 0);
   move = getPackedMove(C4, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) > 0);
   move = getPackedMove(B2, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) > 0);
   move = getPackedMove(A5, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) ==
          basicValue[BLACK_PAWN] + basicValue[BLACK_KNIGHT] -
          basicValue[WHITE_ROOK]);

   makeMove(&variation, getPackedMove(H8, H5, NO_PIECE));

   move = getPackedMove(F3, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) < 0);
   move = getPackedMove(C4, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) < 0);
   move = getPackedMove(B2, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) < 0);
   move = getPackedMove(A5, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) ==
          basicValue[BLACK_PAWN] - basicValue[WHITE_ROOK]);

   makeMove(&variation, getPackedMove(D1, A1, NO_PIECE));

   move = getPackedMove(F3, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) > 0);
   move = getPackedMove(C4, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) > 0);
   move = getPackedMove(B2, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) > 0);
   move = getPackedMove(A5, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) ==
          basicValue[BLACK_PAWN] + basicValue[BLACK_KNIGHT] -
          basicValue[WHITE_ROOK]);

   makeMove(&variation, getPackedMove(D8, F6, NO_PIECE));

   move = getPackedMove(F3, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == PAWN_FOR_KNIGHT);
   move = getPackedMove(C4, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == PAWN_FOR_KNIGHT);
   move = getPackedMove(B2, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == PAWN_FOR_BISHOP);
   move = getPackedMove(A5, E5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) ==
          basicValue[BLACK_PAWN] - basicValue[WHITE_KNIGHT] ||
          seeMove(&variation.singlePosition, move) ==
          basicValue[BLACK_PAWN] - basicValue[WHITE_BISHOP]);
   move = getPackedMove(F6, F3, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) ==
          basicValue[WHITE_KNIGHT] - basicValue[BLACK_QUEEN]);
   move = getPackedMove(A6, B7, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == 0);

   makeMove(&variation, getPackedMove(A6, B7, NO_PIECE));
   makeMove(&variation, getPackedMove(C8, B7, NO_PIECE));

   move = getPackedMove(A5, A7, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) ==
          basicValue[BLACK_PAWN] - basicValue[WHITE_ROOK]);
   move = getPackedMove(C6, A5, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) ==
          basicValue[WHITE_ROOK] - basicValue[BLACK_KNIGHT]);

   return 0;
}

static int testStaticExchangeEvaluationWithKing()
{
   Variation variation;
   Move move;

   initializeVariation(&variation, FEN_GAMESTART);

   makeMove(&variation, getPackedMove(E2, E4, NO_PIECE));
   makeMove(&variation, getPackedMove(E7, E5, NO_PIECE));
   makeMove(&variation, getPackedMove(D1, H5, NO_PIECE));
   makeMove(&variation, getPackedMove(D7, D6, NO_PIECE));

   move = getPackedMove(H5, F7, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) ==
          basicValue[BLACK_PAWN] - basicValue[WHITE_QUEEN]);

   makeMove(&variation, getPackedMove(F1, C4, NO_PIECE));

   move = getPackedMove(H5, F7, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == basicValue[BLACK_PAWN]);

   makeMove(&variation, getPackedMove(G8, H6, NO_PIECE));

   move = getPackedMove(C4, F7, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) ==
          basicValue[BLACK_PAWN] - basicValue[WHITE_BISHOP]);

   return 0;
}

static int testStaticExchangeEvaluationWithEnPassantCapture()
{
   Variation variation;
   Move move;

   initializeVariation(&variation, "6k1/8/7r/pP6/8/8/8/R5K1 w - a6 0 1");

   move = getPackedMove(B5, A6, NO_PIECE);
   assert(seeMove(&variation.singlePosition, move) == basicValue[BLACK_PAWN]);

   makeMove(&variation, getPackedMove(B5, A6, NO_PIECE));
   makeMove(&variation, getPackedMove(G8, G7, NO_PIECE));
   makeMove(&variation, getPackedMove(A6, A7, NO_PIECE));
   makeMove(&variation, getPackedMove(H6, H8, NO_PIECE));

   move = getPackedMove(A7, A8, WHITE_QUEEN);
   assert(seeMove(&variation.singlePosition, move) ==
          basicValue[BLACK_ROOK] - basicValue[WHITE_PAWN]);

   return 0;
}

#define P1 "2b3k1/1p1n1pP1/5P2/2r2q2/4Q3/3N1p2/5Pp1/1R1B2K1 w - - 0 1"

static int testAttackCalculations()
{
   Variation variation;

   initializeVariation(&variation, FEN_GAMESTART);
   assert(passiveKingIsSafe(&variation.singlePosition));

   initializeVariation(&variation, "R5k1/8/6K1/8/8/8/1r6/8 w - - 0 1");
   assert(passiveKingIsSafe(&variation.singlePosition) == FALSE);

   initializeVariation(&variation, "6k1/R7/6K1/8/8/8/6r1/8 b - - 0 1");
   assert(passiveKingIsSafe(&variation.singlePosition) == FALSE);

   initializeVariation(&variation, "6k1/R7/6K1/6P1/8/8/6r1/8 b - - 0 1");
   assert(passiveKingIsSafe(&variation.singlePosition));

   initializeVariation(&variation, "R5k1/8/6K1/8/8/8/1r6/8 b - - 0 1");
   assert(activeKingIsSafe(&variation.singlePosition) == FALSE);

   initializeVariation(&variation, "6k1/R7/6K1/8/8/8/6r1/8 w - - 0 1");
   assert(activeKingIsSafe(&variation.singlePosition) == FALSE);

   initializeVariation(&variation, "6k1/R7/6K1/6P1/8/8/6r1/8 w - - 0 1");
   assert(activeKingIsSafe(&variation.singlePosition));

   return 0;
}

static int testLegalMoveGeneration()
{
   Variation variation;
   Movelist movelist;

   initializeVariation(&variation, "R5k1/8/5K2/8/8/8/1r6/8 b - - 0 1");
   getLegalMoves(&variation, &movelist);

   assert(movelist.numberOfMoves == 2);
   assert(listContainsSimpleMove(&movelist, G8, H7));
   assert(listContainsSimpleMove(&movelist, B2, B8));

   return 0;
}

static int testPinCheck()
{
   static const char *fen1 =
      "r1b1k2r/ppp2ppp/2n2n2/1B6/1b2qp2/2NP1N2/PPP1QPPP/R3K2R w KQkq - 0 9";
   Variation variation;
   Bitboard pinnedPieces;

   initializeVariation(&variation, fen1);
   pinnedPieces = getPinnedPieces(&variation.singlePosition, WHITE);
   assert(testSquare(pinnedPieces, E4));
   assert(testSquare(pinnedPieces, C6));
   assert(getNumberOfSetSquares(pinnedPieces) == 2);
   pinnedPieces = getPinnedPieces(&variation.singlePosition, BLACK);
   assert(testSquare(pinnedPieces, E2));
   assert(testSquare(pinnedPieces, C3));
   assert(getNumberOfSetSquares(pinnedPieces) == 2);

   return 0;
}

#endif

int testModuleMovegeneration()
{

#ifndef NDEBUG

   int result;

   if ((result = testPseudoLegalMoves()) != 0)
   {
      return result;
   }

   if ((result = testLegalMoves()) != 0)
   {
      return result;
   }

   if ((result = testStaticExchangeEvaluation()) != 0)
   {
      return result;
   }

   if ((result = testStaticExchangeEvaluationWithKing()) != 0)
   {
      return result;
   }

   if ((result = testStaticExchangeEvaluationWithEnPassantCapture()) != 0)
   {
      return result;
   }

   if ((result = testAttackCalculations()) != 0)
   {
      return result;
   }

   if ((result = testLegalMoveGeneration()) != 0)
   {
      return result;
   }

   if ((result = testPinCheck()) != 0)
   {
      return result;
   }

#endif

   return 0;
}
