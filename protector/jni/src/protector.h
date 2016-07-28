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

#ifndef _protector_h_
#define _protector_h_

#include "tools.h"

/* #define USE_BOOK 1 */

/*
 * Constants
 */

#define MAX_THREADS 32
#define DEPTH_RESOLUTION 2

#define _64_ 64
#define FALSE 0
#define TRUE 1

#define VALUE_MATED (-30000)
#define VALUE_ALMOST_MATED (-29900)
#define MAX_MOVES_PER_POSITION (250)

/* #define SEND_HASH_ENTRIES 1 */

extern int _distance[_64_][_64_];
extern int _horizontalDistance[_64_][_64_];
extern int _verticalDistance[_64_][_64_];
extern int _taxiDistance[_64_][_64_];
extern const int colorSign[2];
extern int pieceIndex[16];
extern int debugOutput;
extern int numPvs;              /* for multi pv mode; default is 1 */

#define MAX_NUM_PV 8

/**
 * Types
 */
typedef unsigned char BYTE;
typedef char INT8;
typedef unsigned char UINT8;
typedef short INT16;
typedef unsigned short UINT16;
typedef int INT32;
typedef unsigned int UINT32;

#define ULONG_ZERO 0
/* #define ICC */
#define wV(x,w) (((x)*(w))/256)
typedef long long INT64;
typedef unsigned long long UINT64;
typedef unsigned char bool;
extern UINT64 statCount1, statCount2;
extern const char *programVersionNumber;

typedef enum
{
   WHITE, BLACK
}
Color;

typedef enum
{
   DARK, LIGHT, ALL
}
SquareColor;

typedef enum
{
   RESULT_UNKNOWN, RESULT_WHITE_WINS, RESULT_DRAW, RESULT_BLACK_WINS
}
GameResult;

typedef enum
{
   TS_FALSE = 0, TS_UNKNOWN = 1, TS_TRUE = 2
}
TriStateType;

/* Gameresult values */
#define GAMERESULT_UNKNOWN    "*"
#define GAMERESULT_WHITE_WINS "1-0"
#define GAMERESULT_DRAW       "1/2-1/2"
#define GAMERESULT_BLACK_WINS "0-1"

/* Game termination reasons */
#define GAMERESULT_WHITE_MATES           "White mates"
#define GAMERESULT_BLACK_MATES           "Black mates"
#define GAMERESULT_WHITE_RESIGNS         "White resigns"
#define GAMERESULT_BLACK_RESIGNS         "Black resigns"
#define GAMERESULT_WHITE_TIME_FORFEIT    "White forfeits on time"
#define GAMERESULT_BLACK_TIME_FORFEIT    "Black forfeits on time"
#define GAMERESULT_DRAW_AGREED           "Draw agreed"
#define GAMERESULT_STALEMATE             "Stalemate"
#define GAMERESULT_REPETITION            "Draw by repetion"
#define GAMERESULT_50_MOVE_RULE          "Draw by 50 move rule"
#define GAMERESULT_INSUFFICIENT_MATERIAL "Insufficient material"

typedef struct
{
   char result[10];
   char reason[128];
}
Gameresult;

#define PP_BLACK_PIECE   0x01
#define PP_SLIDING_PIECE 0x02
#define PP_ORTHOPIECE    0x04
#define PP_SPECIALPIECE  0x04
#define PP_DIAPIECE      0x08
#define PP_NONKINGPIECE  0x08

typedef enum
{
   NO_PIECETYPE = 0x00,
   KING = PP_SPECIALPIECE,
   QUEEN = PP_SLIDING_PIECE | PP_ORTHOPIECE | PP_DIAPIECE,
   ROOK = PP_SLIDING_PIECE | PP_ORTHOPIECE,
   BISHOP = PP_SLIDING_PIECE | PP_DIAPIECE,
   KNIGHT = PP_NONKINGPIECE,
   PAWN = PP_SPECIALPIECE | PP_NONKINGPIECE
}
PieceType;

typedef enum
{
   NO_PIECE = 0x00,
   WHITE_KING = KING,
   WHITE_QUEEN = QUEEN,
   WHITE_ROOK = ROOK,
   WHITE_BISHOP = BISHOP,
   WHITE_KNIGHT = KNIGHT,
   WHITE_PAWN = PAWN,
   BLACK_KING = BLACK | KING,
   BLACK_QUEEN = BLACK | QUEEN,
   BLACK_ROOK = BLACK | ROOK,
   BLACK_BISHOP = BLACK | BISHOP,
   BLACK_KNIGHT = BLACK | KNIGHT,
   BLACK_PAWN = BLACK | PAWN
}
Piece;

typedef enum
{
   WHITE_BISHOP_DARK = 2,
   WHITE_BISHOP_LIGHT = WHITE_BISHOP,
   BLACK_BISHOP_DARK = 3,
   BLACK_BISHOP_LIGHT = BLACK_BISHOP
}
BishopPiece;

typedef enum
{
   NO_SQUARE = -1,
   A1, B1, C1, D1, E1, F1, G1, H1,
   A2, B2, C2, D2, E2, F2, G2, H2,
   A3, B3, C3, D3, E3, F3, G3, H3,
   A4, B4, C4, D4, E4, F4, G4, H4,
   A5, B5, C5, D5, E5, F5, G5, H5,
   A6, B6, C6, D6, E6, F6, G6, H6,
   A7, B7, C7, D7, E7, F7, G7, H7,
   A8, B8, C8, D8, E8, F8, G8, H8
}
Square;

typedef enum
{
   FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H
}
File;

typedef enum
{
   RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8
}
Rank;

typedef enum
{
   NO_CASTLINGS = 0,
   WHITE_00 = 1, WHITE_000 = 2,
   BLACK_00 = 4, BLACK_000 = 8
}
Castlings;

/**
 * Functions
 */

#define pieceColor( piece ) ((Color) ( (piece) & 0x01 ))
#define pieceType( piece ) ((PieceType) ( (piece) & 0x0E ))
#define opponents( piece1, piece2 ) ( ((piece1)^(piece2)) & 0x01 )
#define squareColor( square ) ((SquareColor)( (file(square) + rank(square)) % 2 ))

#define file( square ) ((File) ( (square) & 0x07 ))
#define rank( square ) ((Rank) ( (square) >> 3 ))
#define colorRank( color, square ) (Rank) \
        ( (color) == WHITE ? rank(square) : 7-rank(square) )
#define squareIsValid( square ) ( (square) >= A1 && (square) <= H8  )
#define getSquare( file, rank ) ((Square) ( (file) + ((rank) << 3) ))
#define getFlippedSquare( square ) \
        ( getSquare ( file(square), (RANK_8-rank(square)) ) )
#define getHflippedSquare( square ) \
        ( getSquare ( (FILE_H-file(square)), rank(square) ) )
#define fileName( file ) ( 'a' + (file) )
#define rankName( rank ) ( '1' + (rank) )
#define distance(sq1,sq2) (_distance[(sq1)][(sq2)])
#define horizontalDistance(sq1,sq2) (_horizontalDistance[(sq1)][(sq2)])
#define verticalDistance(sq1,sq2) (_verticalDistance[(sq1)][(sq2)])
#define taxiDistance(sq1,sq2) (_taxiDistance[(sq1)][(sq2)])
#define hasCastlings(color, castlings)(castlingsOfColor[(color)]&(castlings))
#define upward(color, square)((square)+((color)==WHITE?8:-8))
#define downward(color, square)((square)+((color)==WHITE?-8:8))

/**
 * Iteration shortcuts
 */
#define ITERATE(sq) for ( sq = A1; sq <= H8; sq++ )

typedef struct
{
   bool processModuleTest;
   bool xboardMode;
   bool dumpEvaluation;
   char *testfile, *bookfile;
   char *tablebasePath;
}
CommandlineOptions;

extern CommandlineOptions commandlineOptions;

#endif
