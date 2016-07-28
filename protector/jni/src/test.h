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

#ifndef _test_h_
#define _test_h_

#include "position.h"

/**
 * Process the testsuite specified by 'filename'.
 *
 * @return 0 if no errors occurred.
 */
int processTestsuite(const char *filename);

/**
 * Handle a search event defined by eventId.
 */
void handleCliSearchEvent(int eventId, Variation * variation);

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleTest(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleTest(void);

#endif
