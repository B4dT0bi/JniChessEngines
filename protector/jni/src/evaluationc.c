#undef addBonus
#undef addMalus
#undef COLOR
#undef OPPCOLOR

#ifdef PERSPECTIVE_WHITE
#define COLOR WHITE
#define OPPCOLOR BLACK
#define addBonus(base, bonus) (base->balance += bonus);
#define addMalus(base, bonus) (base->balance -= bonus);

void evaluateWhiteKnight(const Position * position,
                         EvaluationBase * base, const Square square);
void evaluateWhiteBishop(const Position * position,
                         EvaluationBase * base, const Square square);
void evaluateWhiteRook(const Position * position,
                       EvaluationBase * base, const Square square);
void evaluateWhiteQueen(const Position * position,
                        EvaluationBase * base, const Square square);
void evaluateWhitePasser(const Position * position,
                         EvaluationBase * base, const Square square);
void evaluateWhitePawns(EvaluationBase * base);
int getPawnSafetyMalusOfWhiteKingFile(const Position * position,
                                      const int file,
                                      const Square kingSquare,
                                      const int fileType);

#else
#define COLOR BLACK
#define OPPCOLOR WHITE
#define addBonus(base, bonus) (base->balance -= bonus);
#define addMalus(base, bonus) (base->balance += bonus);

void evaluateBlackKnight(const Position * position,
                         EvaluationBase * base, const Square square);
void evaluateBlackBishop(const Position * position,
                         EvaluationBase * base, const Square square);
void evaluateBlackRook(const Position * position,
                       EvaluationBase * base, const Square square);
void evaluateBlackQueen(const Position * position,
                        EvaluationBase * base, const Square square);
void evaluateBlackPasser(const Position * position,
                         EvaluationBase * base, const Square square);
void evaluateBlackPawns(EvaluationBase * base);
int getPawnSafetyMalusOfBlackKingFile(const Position * position,
                                      const int file,
                                      const Square kingSquare,
                                      const int fileType);

#endif

#ifdef PERSPECTIVE_WHITE
static void addWhitePieceAttackBonus(const Position * position,
                                     EvaluationBase * base,
                                     const Bitboard moves,
                                     const Bitboard candidateTargets,
                                     const Piece attacker)
#else
static void addBlackPieceAttackBonus(const Position * position,
                                     EvaluationBase * base,
                                     const Bitboard moves,
                                     const Bitboard candidateTargets,
                                     const Piece attacker)
#endif
{
   Bitboard targets = moves & candidateTargets;
   Square targetSquare;

   ITERATE_BITBOARD(&targets, targetSquare)
   {
      const Piece target = position->piece[targetSquare];

      addBonus(base, piecePieceAttackBonus[attacker][target]);
   }
}

#ifdef PERSPECTIVE_WHITE
void evaluateWhiteKnight(const Position * position,
                         EvaluationBase * base, const Square square)
#else
void evaluateBlackKnight(const Position * position,
                         EvaluationBase * base, const Square square)
#endif
{
   const Square pinningPiece = getPinningPiece(position, base, square, COLOR);
   const Bitboard moves =
      (pinningPiece == NO_SQUARE ? getKnightMoves(square) : EMPTY_BITBOARD);
   const Bitboard ctm = position->piecesOfType[PAWN | OPPCOLOR] |
      position->piecesOfType[BISHOP | OPPCOLOR];
   const Bitboard cth = position->piecesOfType[ROOK | OPPCOLOR] |
      position->piecesOfType[QUEEN | OPPCOLOR];
   const Bitboard candidateTargets = cth |
      (ctm & base->unprotectedPieces[OPPCOLOR]);

#ifdef PERSPECTIVE_WHITE
   const Square relativeSquare = square;
#else
   const Square relativeSquare = getFlippedSquare(square);
#endif

   if (pinningPiece == NO_SQUARE)
   {
      const Bitboard mobSquares = base->countedSquares[COLOR] |
         getNonPawnPieces(position, OPPCOLOR);
      const int mobilityCount = getNumberOfSetSquares(moves & mobSquares);

      assert(mobilityCount <= MAX_MOVES_KNIGHT);

      addBonus(base, KnightMobilityBonus[mobilityCount]);
   }
   else
   {
      setSquare(base->pinnedPieces[COLOR], square);
   }

   base->knightAttackedSquares[COLOR] |= moves;

   if (position->piecesOfType[KNIGHT | OPPCOLOR] == EMPTY_BITBOARD &&
       position->piecesOfType[BISHOP | OPPCOLOR] != EMPTY_BITBOARD &&
       hasAttackingBishop(position, OPPCOLOR, square) == FALSE)
   {
      addBonus(base, V(2, 2));
   }

   if (squareIsPawnSafe(base, COLOR, square) &&
       BONUS_KNIGHT_OUTPOST[relativeSquare] > 0)
   {
      int bonusValue = (28 * BONUS_KNIGHT_OUTPOST[relativeSquare]) / 16;

      if (testSquare(base->pawnProtectedSquares[COLOR], square))
      {
         if (position->piecesOfType[KNIGHT | OPPCOLOR] == EMPTY_BITBOARD &&
             hasAttackingBishop(position, OPPCOLOR, square) == FALSE)
         {
            bonusValue += bonusValue + bonusValue / 2;
         }
         else
         {
            bonusValue += bonusValue;
         }
      }

      addBonus(base, V(bonusValue, bonusValue));
   }

   if (base->evaluateKingSafety[OPPCOLOR] &&
       (moves & base->kingAttackSquares[OPPCOLOR]) != EMPTY_BITBOARD)
   {
      const Bitboard coronaAttacks =
         moves & getKingMoves(position->king[OPPCOLOR]);

      base->kingSquaresAttackCount[COLOR] +=
         getNumberOfSetSquares(coronaAttacks);
      base->attackInfo[COLOR] += V(KNIGHT_BONUS_ATTACK, 1);
   }

#ifdef PERSPECTIVE_WHITE
   addWhitePieceAttackBonus(position, base, moves, candidateTargets,
                            WHITE_KNIGHT);
#else
   addBlackPieceAttackBonus(position, base, moves, candidateTargets,
                            BLACK_KNIGHT);
#endif

   if (testSquare(base->pawnProtectedSquares[OPPCOLOR], square))
   {
      addMalus(base, piecePieceAttackBonus[PAWN | OPPCOLOR][KNIGHT | COLOR]);
   }
}

#ifdef PERSPECTIVE_WHITE
void evaluateWhiteBishop(const Position * position,
                         EvaluationBase * base, const Square square)
#else
void evaluateBlackBishop(const Position * position,
                         EvaluationBase * base, const Square square)
#endif
{
   const Square pinningPiece = getPinningPiece(position, base, square, COLOR);
   const Bitboard xrayPieces = position->piecesOfType[QUEEN | COLOR];
   const Bitboard moves =
      (pinningPiece == NO_SQUARE ?
       getMagicBishopMoves(square, position->allPieces & ~xrayPieces) :
       getMagicBishopMoves(square, position->allPieces & ~xrayPieces) &
       (squaresBetween[pinningPiece][position->king[COLOR]] |
        minValue[pinningPiece]));
   const Bitboard mobSquares = base->countedSquares[COLOR] |
      getNonPawnPieces(position, OPPCOLOR);
   const Bitboard bishopSquares =
      (testSquare(lightSquares, square) ? lightSquares : darkSquares);
   const Bitboard squareColorTargets =
      position->piecesOfType[PAWN | OPPCOLOR] & squaresBelow[COLOR][square] &
      base->unprotectedPieces[OPPCOLOR] & bishopSquares;
   const Bitboard ctm = position->piecesOfType[PAWN | OPPCOLOR] |
      position->piecesOfType[KNIGHT | OPPCOLOR];
   const Bitboard cth = position->piecesOfType[ROOK | OPPCOLOR] |
      position->piecesOfType[QUEEN | OPPCOLOR];
   const Bitboard candidateTargets =
      cth | (ctm & base->unprotectedPieces[OPPCOLOR]);
   const Piece batteryPiece =
      getDiaBatteryPiece(position, moves, square, OPPCOLOR);
   const int mobilityCount = getNumberOfSetSquares(moves & mobSquares);

#ifdef PERSPECTIVE_WHITE
   const int pawnIndex = getWhiteBishopBlockingIndex(position, bishopSquares);
#else
   const int pawnIndex = getBlackBishopBlockingIndex(position, bishopSquares);
#endif

#ifdef PERSPECTIVE_WHITE
   const Square relativeSquare = square;
#else
   const Square relativeSquare = getFlippedSquare(square);
#endif

   assert(mobilityCount <= MAX_MOVES_BISHOP);

   addBonus(base, BishopMobilityBonus[mobilityCount]);

   addMalus(base, V(wV(pawnIndex, 650), wV(pawnIndex, 750)));
   addBonus(base, getNumberOfSetSquares(squareColorTargets) * V(0, 2));

   base->bishopAttackedSquares[COLOR] |= moves;

   if (squareIsPawnSafe(base, COLOR, square) &&
       BONUS_BISHOP_OUTPOST[relativeSquare] > 0)
   {
      int bonusValue = (24 * BONUS_BISHOP_OUTPOST[relativeSquare]) / 16;

      if (testSquare(base->pawnProtectedSquares[COLOR], square))
      {
         if (position->piecesOfType[KNIGHT | OPPCOLOR] == EMPTY_BITBOARD &&
             hasAttackingBishop(position, OPPCOLOR, square) == FALSE)
         {
            bonusValue += bonusValue + bonusValue / 2;
         }
         else
         {
            bonusValue += bonusValue;
         }
      }

      addBonus(base, V(bonusValue, bonusValue));
   }

   if (pinningPiece != NO_SQUARE)
   {
      setSquare(base->pinnedPieces[COLOR], square);
   }

#ifdef PERSPECTIVE_WHITE
   addWhitePieceAttackBonus(position, base, moves, candidateTargets,
                            WHITE_BISHOP);
#else
   addBlackPieceAttackBonus(position, base, moves, candidateTargets,
                            BLACK_BISHOP);
#endif

   if (testSquare(base->pawnProtectedSquares[OPPCOLOR], square))
   {
      addMalus(base, piecePieceAttackBonus[PAWN | OPPCOLOR][BISHOP | COLOR]);
   }

   if (base->evaluateKingSafety[OPPCOLOR])
   {
      if ((moves & base->kingAttackSquares[OPPCOLOR]) != EMPTY_BITBOARD)
      {
         const Bitboard coronaAttacks =
            moves & getKingMoves(position->king[OPPCOLOR]);

         base->kingSquaresAttackCount[COLOR] +=
            getNumberOfSetSquares(coronaAttacks);
         base->attackInfo[COLOR] += V(BISHOP_BONUS_ATTACK, 1);
      }
   }

   if (batteryPiece != NO_PIECE)
   {
      addBonus(base, V(BISHOP_PIN_OP_VAL, BISHOP_PIN_EG_VAL));
   }
}

#ifdef PERSPECTIVE_WHITE
void evaluateWhiteRook(const Position * position,
                       EvaluationBase * base, const Square square)
#else
void evaluateBlackRook(const Position * position,
                       EvaluationBase * base, const Square square)
#endif
{
   const Piece piece = position->piece[square];
   const Square pinningPiece = getPinningPiece(position, base, square, COLOR);
   const Bitboard xrayPieces = position->piecesOfType[QUEEN | COLOR] |
      position->piecesOfType[piece];
   const Bitboard moves =
      (pinningPiece == NO_SQUARE ?
       getMagicRookMoves(square, position->allPieces & ~xrayPieces) :
       getMagicRookMoves(square, position->allPieces & ~xrayPieces) &
       (squaresBetween[pinningPiece][position->king[COLOR]] |
        minValue[pinningPiece]));
   const Bitboard mobSquares = base->countedSquares[COLOR];
   const Bitboard fileSquares = squaresOfFile[file(square)];
   const Bitboard ownPawns = position->piecesOfType[PAWN | COLOR];
   const Rank relativeRank = colorRank(COLOR, square);
   const Bitboard ctm = position->piecesOfType[PAWN | OPPCOLOR] |
      position->piecesOfType[KNIGHT | OPPCOLOR] |
      position->piecesOfType[BISHOP | OPPCOLOR];
   const Bitboard cth = position->piecesOfType[QUEEN | OPPCOLOR];
   const Bitboard candidateTargets = cth |
      (ctm & base->unprotectedPieces[OPPCOLOR]);
   const int mobilityCount = getNumberOfSetSquares(moves & mobSquares);

   assert(mobilityCount <= MAX_MOVES_ROOK);

   addBonus(base, RookMobilityBonus[mobilityCount]);
   base->rookDoubleAttackedSquares[COLOR] |=
      base->rookAttackedSquares[COLOR] & moves;
   base->rookAttackedSquares[COLOR] |= moves;

   if (pinningPiece != NO_SQUARE)
   {
      setSquare(base->pinnedPieces[COLOR], square);
   }

#ifdef PERSPECTIVE_WHITE
   addWhitePieceAttackBonus(position, base, moves, candidateTargets,
                            WHITE_ROOK);
#else
   addBlackPieceAttackBonus(position, base, moves, candidateTargets,
                            BLACK_ROOK);
#endif

   if (testSquare(base->pawnProtectedSquares[OPPCOLOR], square))
   {
      addMalus(base, piecePieceAttackBonus[PAWN | OPPCOLOR][ROOK | COLOR]);
   }

   /* Add a bonus if this rook is located on an open file. */
   if ((ownPawns & fileSquares) == EMPTY_BITBOARD)
   {
      const Bitboard oppPawns = position->piecesOfType[PAWN | OPPCOLOR];
      const Bitboard frontSquares = fileSquares & squaresAbove[COLOR][square];
      Bitboard protectedBlockers = frontSquares &
         base->pawnProtectedSquares[OPPCOLOR] &
         (oppPawns | position->piecesOfType[OPPCOLOR | KNIGHT] |
          position->piecesOfType[OPPCOLOR | BISHOP]);

      if (protectedBlockers == EMPTY_BITBOARD)
      {
         if ((frontSquares & oppPawns) == EMPTY_BITBOARD)
         {
            addBonus(base, V(20, 19));
         }
         else                   /* semi-open file */
         {
            addBonus(base, V(9, 11));
         }
      }
      else                      /* file is blocked by a protected pawn, knight of bishop */
      {
         if ((protectedBlockers & oppPawns) != EMPTY_BITBOARD)
         {
            addBonus(base, V(8, 0));
         }
         else
         {
#ifdef PERSPECTIVE_WHITE
            const Square minorSquare = getFirstSquare(&protectedBlockers);
#else
            const Square minorSquare = getLastSquare(&protectedBlockers);
#endif
            if (squareIsPawnSafe(base, OPPCOLOR, minorSquare))
            {
               addBonus(base, V(10, 3));
            }
            else
            {
               addBonus(base, V(15, 5));
            }
         }
      }
   }
   else if (testSquare(kingTrapsRook[COLOR][position->king[COLOR]], square) &&
            (moves & centralFiles) == EMPTY_BITBOARD
            /* && mobilityCount < 4 */ )
   {
      const int basicMalus = max(0, 51 - 9 * mobilityCount);

      addMalus(base, V(basicMalus, 0));
   }

   if (relativeRank >= RANK_5)
   {
      const Bitboard pawnTargets = position->piecesOfType[PAWN | OPPCOLOR] &
         generalMoves[ROOK][square];

      if (relativeRank == RANK_7 &&
          colorRank(COLOR, position->king[OPPCOLOR]) == RANK_8)
      {
         Bitboard companions = position->piecesOfType[ROOK | COLOR] |
            position->piecesOfType[QUEEN | COLOR];

         addBonus(base, V(5, 16));

         if ((moves & companions & squaresOfRank[rank(square)]) !=
             EMPTY_BITBOARD)
         {
            addBonus(base, V(5, 10));
         }
      }

      if (pawnTargets != EMPTY_BITBOARD)
      {
         const int numTargets = getNumberOfSetSquares(pawnTargets);

         addBonus(base, numTargets * V(5, 14));
      }

      if (squareIsPawnSafe(base, COLOR, square) &&
          testSquare(border, square) == FALSE &&
          testSquare(base->pawnProtectedSquares[COLOR], square))
      {
         addBonus(base, V(3, 5));
      }
   }

   if (base->evaluateKingSafety[OPPCOLOR])
   {
      if ((moves & base->kingAttackSquares[OPPCOLOR]) != EMPTY_BITBOARD)
      {
         const Bitboard coronaAttacks =
            moves & getKingMoves(position->king[OPPCOLOR]);

         base->kingSquaresAttackCount[COLOR] +=
            getNumberOfSetSquares(coronaAttacks);
         base->attackInfo[COLOR] += V(ROOK_BONUS_ATTACK, 1);
      }
   }

   /* Give a malus for a rook blocking his own passer on the 7th rank */
   if (numberOfNonPawnPieces(position, COLOR) == 2 &&
       colorRank(COLOR, square) == RANK_8 &&
       testSquare(base->passedPawns[COLOR],
                  (Square) downward(COLOR, square)) &&
       (fileSquares & squaresBelow[COLOR][square] &
        position->piecesOfType[ROOK | OPPCOLOR]) != EMPTY_BITBOARD &&
       (companionFiles[square] &
        (position->piecesOfType[WHITE_PAWN] |
         position->piecesOfType[BLACK_PAWN])) == EMPTY_BITBOARD)
   {
      addMalus(base, V(0, 90));
   }
}

#ifdef PERSPECTIVE_WHITE
void evaluateWhiteQueen(const Position * position,
                        EvaluationBase * base, const Square square)
#else
void evaluateBlackQueen(const Position * position,
                        EvaluationBase * base, const Square square)
#endif
{
   const Square pinningPiece = getPinningPiece(position, base, square, COLOR);
   const Bitboard moves =
      (pinningPiece == NO_SQUARE ?
       getMagicQueenMoves(square, position->allPieces) :
       getMagicQueenMoves(square, position->allPieces) &
       (squaresBetween[pinningPiece][position->king[COLOR]] |
        minValue[pinningPiece]));
   const Bitboard xrayOrthoPieces = position->piecesOfType[ROOK | COLOR] |
      position->piecesOfType[QUEEN | COLOR];
   const Bitboard xrayOrthoMoves =
      (pinningPiece == NO_SQUARE ?
       getMagicRookMoves(square, position->allPieces & ~xrayOrthoPieces) :
       getMagicRookMoves(square, position->allPieces & ~xrayOrthoPieces) &
       squaresBetween[pinningPiece][position->king[COLOR]]);
   const Bitboard candidateTargets = base->unprotectedPieces[OPPCOLOR];
   const Bitboard countedQueenSquares = base->countedSquares[COLOR] &
      ~(base->knightAttackedSquares[OPPCOLOR] |
        base->bishopAttackedSquares[OPPCOLOR] |
        base->rookAttackedSquares[OPPCOLOR]);
   const int mobilityCount =
      getNumberOfSetSquares(moves & countedQueenSquares);

   assert(mobilityCount <= MAX_MOVES_QUEEN);

   addBonus(base, QueenMobilityBonus[mobilityCount]);
   base->queenAttackedSquares[COLOR] |= moves;
   base->queenSupportedSquares[COLOR] |= xrayOrthoMoves & ~moves;

   if (colorRank(COLOR, square) >= RANK_5)
   {
      const Bitboard pawnTargets = position->piecesOfType[PAWN | OPPCOLOR] &
         generalMoves[ROOK][square];

      if (colorRank(COLOR, square) == RANK_7 &&
          colorRank(COLOR, position->king[OPPCOLOR]) == RANK_8)
      {
         addBonus(base, V(3, 15));
      }

      if (pawnTargets != EMPTY_BITBOARD)
      {
         const int numTargets = getNumberOfSetSquares(pawnTargets);

         addBonus(base, numTargets * V(2, 10));
      }
   }

   if (base->evaluateKingSafety[OPPCOLOR])
   {
      if ((moves & base->kingAttackSquares[OPPCOLOR]) != EMPTY_BITBOARD)
      {
         const Bitboard coronaAttacks =
            moves & getKingMoves(position->king[OPPCOLOR]);

         base->kingSquaresAttackCount[COLOR] +=
            getNumberOfSetSquares(coronaAttacks);
         base->attackInfo[COLOR] += V(QUEEN_BONUS_ATTACK, 1);
      }
   }

   if (pinningPiece != NO_SQUARE)
   {
      setSquare(base->pinnedPieces[COLOR], square);
   }

#ifdef PERSPECTIVE_WHITE
   addWhitePieceAttackBonus(position, base, moves, candidateTargets,
                            WHITE_QUEEN);
#else
   addBlackPieceAttackBonus(position, base, moves, candidateTargets,
                            BLACK_QUEEN);
#endif

   if (testSquare(base->pawnProtectedSquares[OPPCOLOR], square))
   {
      addMalus(base, piecePieceAttackBonus[PAWN | OPPCOLOR][QUEEN | COLOR]);
   }
}

#ifdef PERSPECTIVE_WHITE
void evaluateWhitePasser(const Position * position,
                         EvaluationBase * base, const Square square)
#else
void evaluateBlackPasser(const Position * position,
                         EvaluationBase * base, const Square square)
#endif
{
   const Rank pawnRank = colorRank(COLOR, square);
   const int pawnDirection = (COLOR == WHITE ? 8 : -8);
   const Square stopSquare = (Square) (square + pawnDirection);
   const int numDefenders = position->numberOfPieces[OPPCOLOR] -
      position->numberOfPawns[OPPCOLOR];
   bool unStoppable = FALSE;
   const int rank = pawnRank - RANK_2;
   const int rankFactor = rank * (rank - 1);
   int openingBonus = 17 * rankFactor;
   int endgameBonus = 7 * (rankFactor + rank + 1);
   int opValue, egValue;

   assert(pawnIsPassed(position, square, COLOR));

   if (numDefenders == 1)
   {
      const int egBonus = quad(0, 800, pawnRank);
      const int kingDistance = distance(square, position->king[COLOR]);
      const Square rectangleSquare =
         (COLOR == position->activeColor ?
          square : (Square) (square - pawnDirection));
      const bool kingInRectangle =
         testSquare(passedPawnRectangle[COLOR][rectangleSquare],
                    position->king[OPPCOLOR]);

      if ((kingInRectangle == FALSE &&
           (passedPawnCorridor[COLOR][square] &
            position->piecesOfColor[COLOR]) == EMPTY_BITBOARD))
      {
         addBonus(base, V(0, egBonus));
         unStoppable = TRUE;
      }
      else if (kingDistance == 1)
      {
         const File pawnFile = file(square);
         const File kingFile = file(position->king[COLOR]);
         const Square promotionSquare =
            (COLOR == WHITE ? getSquare(pawnFile, RANK_8) :
             getSquare(pawnFile, RANK_1));
         const bool clearPath = (bool)
            (kingFile != pawnFile ||
             (kingFile != FILE_A && kingFile != FILE_H));

         if (clearPath &&
             distance(promotionSquare, position->king[COLOR]) <= 1)
         {
            addBonus(base, V(0, egBonus));
            unStoppable = TRUE;
         }
      }

      if (unStoppable == FALSE &&
          base->hasPassersOrCandidates[OPPCOLOR] == FALSE &&
          passerWalks(position, square, COLOR))
      {
         addBonus(base, V(0, egBonus));
         unStoppable = TRUE;
      }
   }

   if (rankFactor > 0 && unStoppable == FALSE)
   {
      const Square attKing = position->king[COLOR];
      const Square defKing = position->king[OPPCOLOR];
      const int attackerDistance = distance(attKing, stopSquare);
      const int defenderDistance = distance(defKing, stopSquare);

      endgameBonus -= 3 * rankFactor * attackerDistance;
      endgameBonus += 5 * rankFactor * defenderDistance;

      if (position->piece[stopSquare] == NO_PIECE)
      {
         Bitboard ownAttacks = base->attackedSquares[COLOR] |
            getKingMoves(attKing);
         const Bitboard oppAttacks = base->attackedSquares[OPPCOLOR] |
            getKingMoves(defKing);
         const Bitboard path = passedPawnCorridor[COLOR][square];
         const Bitboard oppBlockers =
            path & position->piecesOfColor[OPPCOLOR];
         const Bitboard blockingSquare = minValue[stopSquare];
         int bonus = 0;
         Bitboard obstacles = path & oppAttacks;

         if (testSquare(base->attackedSquares[OPPCOLOR], square))
         {
            const Bitboard candidates =
               (position->piecesOfType[ROOK | OPPCOLOR] |
                position->piecesOfType[QUEEN | OPPCOLOR]) &
               passedPawnCorridor[OPPCOLOR][square] &
               getMagicRookMoves(square, position->allPieces);

            if (candidates != EMPTY_BITBOARD)
            {
               obstacles = path;
            }
         }

         if (testSquare(base->attackedSquares[COLOR], square))
         {
            const Bitboard candidates =
               (position->piecesOfType[ROOK | COLOR] |
                position->piecesOfType[QUEEN | COLOR]) &
               passedPawnCorridor[OPPCOLOR][square] &
               getMagicRookMoves(square, position->allPieces);

            if (candidates != EMPTY_BITBOARD)
            {
               ownAttacks = path;
            }
         }

         obstacles |= oppBlockers;

         if (obstacles == EMPTY_BITBOARD)
         {
            bonus += 16;
         }
         else if ((obstacles & blockingSquare) == EMPTY_BITBOARD)
         {
            bonus += 9;
         }

         if (ownAttacks == path)
         {
            bonus += 6;
         }
         else if ((ownAttacks & blockingSquare) != EMPTY_BITBOARD)
         {
            bonus += ((obstacles & ~ownAttacks) == EMPTY_BITBOARD ? 4 : 2);
         }

         openingBonus += rankFactor * bonus;
         endgameBonus += rankFactor * bonus;
      }
   }

   if (position->numberOfPawns[COLOR] < position->numberOfPawns[OPPCOLOR])
   {
      endgameBonus += endgameBonus / 4;
   }

   opValue = (openingBonus * PASSED_PAWN_WEIGHT_OP) / 256;
   egValue = (endgameBonus * PASSED_PAWN_WEIGHT_EG) / 256;

   addBonus(base, V(opValue, egValue));
}

#ifdef PERSPECTIVE_WHITE
int getPawnSafetyMalusOfWhiteKingFile(const Position * position,
                                      const int file,
                                      const Square kingSquare,
                                      const int fileType)
#else
int getPawnSafetyMalusOfBlackKingFile(const Position * position,
                                      const int file,
                                      const Square kingSquare,
                                      const int fileType)
#endif
{
   const int ATTACK_WEIGHT = 306;
   const Square baseSquare = getSquare(file, rank(kingSquare));
   const Bitboard pawnRealm = squaresOfRank[rank(kingSquare)] |
      squaresAbove[COLOR][kingSquare];
   const Bitboard fileRealm = squaresOfFile[file] & pawnRealm;
   const Bitboard diagRealm = pawnRealm &
      generalMoves[WHITE_BISHOP][baseSquare] &
      (file(kingSquare) <= FILE_D ?
       squaresRightOf[baseSquare] : squaresLeftOf[baseSquare]);
   Bitboard fileDefenders = fileRealm & position->piecesOfType[PAWN | COLOR];
   Bitboard diagDefenders = diagRealm & position->piecesOfType[PAWN | COLOR];
   Bitboard attackers = fileRealm & position->piecesOfType[PAWN | OPPCOLOR];
   int fileDefenderIndex = 0, fileAttackerIndex = 0;
   int fileAttackerDivisor = 256;
   int diagonalDefenderMalus = 0;

#ifdef PERSPECTIVE_WHITE
   const Square fileDefenderSquare = getFirstSquare(&fileDefenders);
   const Square fileAttackerSquare = getFirstSquare(&attackers);
#else
   const Square fileDefenderSquare = getLastSquare(&fileDefenders);
   const Square fileAttackerSquare = getLastSquare(&attackers);
#endif

   if (fileType == 1)
   {
#ifdef PERSPECTIVE_WHITE
      const Square diagDefenderSquare = getFirstSquare(&diagDefenders);
#else
      const Square diagDefenderSquare = getLastSquare(&diagDefenders);
#endif

      const File fileIndex = (File) (file <= FILE_D ? file : FILE_H - file);
      const Rank rankIndex = (diagDefenderSquare != NO_SQUARE ?
                              colorRank(COLOR, diagDefenderSquare) : RANK_1);

      assert(fileIndex >= FILE_A);
      assert(fileIndex <= FILE_D);
      assert(rankIndex >= RANK_1);
      assert(rankIndex <= RANK_8);

      diagonalDefenderMalus =
         KINGSAFETY_PAWN_BONUS_DEFENDER_DIAG[fileIndex][rankIndex];
   }

   if (fileDefenderSquare != NO_SQUARE)
   {
      fileDefenderIndex = colorRank(COLOR, fileDefenderSquare);
   }
   else
   {
      fileAttackerDivisor = 192;
   }

   if (fileAttackerSquare != NO_SQUARE)
   {
      fileAttackerIndex = colorRank(COLOR, fileAttackerSquare);

      if (fileAttackerIndex == fileDefenderIndex + 1)
      {
         fileAttackerDivisor = (fileAttackerIndex == RANK_3 ? 160 : 390);
      }
   }

   assert(fileType >= 0);
   assert(fileType <= 2);
   assert(fileDefenderIndex >= RANK_1);
   assert(fileDefenderIndex <= RANK_8);
   assert(fileAttackerIndex >= RANK_1);
   assert(fileAttackerIndex <= RANK_8);

   return KINGSAFETY_PAWN_MALUS_DEFENDER[fileType][fileDefenderIndex] +
      (KINGSAFETY_PAWN_BONUS_ATTACKER[fileType][fileAttackerIndex] *
       ATTACK_WEIGHT) / fileAttackerDivisor + diagonalDefenderMalus;
}

#ifdef PERSPECTIVE_WHITE
void evaluateWhitePawns(EvaluationBase * base)
#else
void evaluateBlackPawns(EvaluationBase * base)
#endif
{
   static const INT32 doubledMalusPerFile[8] = {
      V(5, 13), V(7, 15), V(8, 15), V(8, 15),
      V(8, 15), V(8, 15), V(7, 15), V(5, 13)
   };

   Bitboard chainPawns = base->chainPawns[COLOR];
   Bitboard doubledPawns = base->doubledPawns[COLOR];
   Square square;

   ITERATE_BITBOARD(&chainPawns, square)
   {
#ifdef PERSPECTIVE_WHITE
      addBonus(base, PAWN_CHAIN_BONUS[square]);
#else
      addBonus(base, PAWN_CHAIN_BONUS[getFlippedSquare(square)]);
#endif
   }

   ITERATE_BITBOARD(&doubledPawns, square)
   {
      addMalus(base, doubledMalusPerFile[file(square)]);
   }
}
