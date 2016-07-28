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

#ifndef _io_h_
#define _io_h_

#include "protector.h"
#include "bitboard.h"
#include "position.h"
#include "movegeneration.h"

extern char pieceSymbol[16];

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleIo(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleIo(void);

/**
 * Stop program execution until the user strikes a key.
 */
int getKeyStroke(void);

/**
 * Send a string representation of the specified bitboard to stdout.
 */
void dumpBitboard(Bitboard bitboard, char *title);

/**
 * Send a string representation of the specified balance value to stdout.
 */
void dumpBalance(const INT32 balance);

/**
 * Send a string representation of the specified value array to stdout.
 */
void dumpBoardValues(const int value[64]);

/**
 * Send a string representation of the specified square to stdout.
 */
void dumpSquare(const Square square);

/**
 * Send a string representation of the specified move to stdout.
 */
void dumpMove(const Move move);
void logMove(const Move move);

/**
 * Send a string representation of the specified movelist to stdout.
 */
void dumpMovelist(const Movelist * movelist);

/**
 * Send a string representation of the specified pv to stdout.
 */
void dumpPv(int depth, long timestamp,
            const char *moves, int value, UINT64 nodes,
            const Color activeColor);

/**
 * Send a string representation of the specified position to stdout.
 */
void logPosition(const Position * position);

/**
 * Send a string representation of the specified position to stdout
 * and wait for a keystroke.
 */
void dumpPosition(const Position * position);

/**
 * Send a string representation of the specified variation to stdout
 * and wait for a keystroke.
 */
void dumpVariation(const Variation * variation);

/**
 * Send a string representation of the specified variation to stdout.
 */
void reportVariation(const Variation * variation);

/**
 * Get the name of the specified square.
 */
void getSquareName(Square square, char name[3]);

/**
 * Get a dump of the specified move.
 */
void getMoveDump(const Move move, char *buffer);

/**
 * Close the logfile.
 */
void closeLogfile(void);

/**
 * Write the specified message to the logfile.
 */
void logDebug(const char *fmt, ...);

/**
 * Write the specified message to the logfile.
 */
void logReport(const char *fmt, ...);

/**
 * Format the given integer value by adding thousands separators.
 */
void formatLongInteger(UINT64 n, char *buffer);

/**
 * Format the given centipawn value according to the uci protocol.
 */
void formatUciValue(const int centipawnValue, char *buffer);

/**
 * Write the specified table to a source code file.
 */
void writeTableToFile(UINT64 * table, const int tablesize,
                      const char *fileName, const char *tableName);

#endif
