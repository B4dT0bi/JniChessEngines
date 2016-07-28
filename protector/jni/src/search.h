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

#ifndef _search_h_
#define _search_h_

#include "protector.h"
#include "position.h"
#include "movegeneration.h"

/**
 * Search the best move in the position specified by 'variation'.
 * If acceptable solutions are specified and the search process yields
 * an acceptable solution in two consecutive iterations the search
 * will be terminated and the last acceptable move will be returned 
 * as best move.
 *
 * @param acceptableSolutions the acceptable solution moves (optional)
 *
 * @return the best move found in the conducted search
 */
Move search(Variation * variation, Movelist * acceptableSolutions);

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
bool checkNodeExclusion(int restDepth);
int getEvalValue(Variation * variation);
void copyPvFromHashtable(Variation * variation, const int pvIndex,
                         PrincipalVariation * pv, const Move bestBaseMove);
int getPvScoreType(int value, int alpha, int beta);
int initializeModuleSearch(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleSearch(void);

#endif
