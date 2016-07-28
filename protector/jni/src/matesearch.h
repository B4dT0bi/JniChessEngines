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

#ifndef _matesearch_h_
#define _matesearch_h_

#include "protector.h"
#include "position.h"
#include "movegeneration.h"

/**
 * Solve the mate problem specified by variation.
 *
 * @param movelist contains all solutions found
 * @param numMoves the maximum number of moves until the winner mates
 */
void searchForMate(Variation * variation, Movelist * movelist, int numMoves);

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleMatesearch(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleMatesearch(void);

#endif
