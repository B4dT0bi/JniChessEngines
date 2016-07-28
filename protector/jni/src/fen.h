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

#ifndef _fen_h_
#define _fen_h_

#define FEN_GAMESTART "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

#include "protector.h"
#include "position.h"

/**
 * Convert a FEN string to a position object.
 *
 * @param fen the FEN string to be converted
 * @param position the position object that is supposed to contain the
 *        position specified by 'fen'
 *
 * @return 0 if the conversion could be accomplished
 */
int readFen(const char *fen, Position * position);

/**
 * Convert a position object to an FEN string.
 *
 * @param position the position object to be converted
 * @param fen the buffer supposed to contain the fen string of the position
 *        specified by 'position'
 */
void getFen(const Position * position, char *fen);

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleFen(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleFen(void);

#endif
