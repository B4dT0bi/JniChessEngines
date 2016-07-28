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

#ifndef _tablebase_h_
#define _tablebase_h_

#include "position.h"

extern bool tbAvailable;

#define TABLEBASE_ERROR 32768

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleTablebase(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleTablebase(void);

/**
 * Initialize tablebase support.
 *
 * @param path the directory containing the tablebase files
 *
 * @return 0 if no errors occurred.
 */
int initializeTablebase(const char *path);

/**
 * Close all tablebase files.
 */
void closeTablebaseFiles(void);

/**
 * Set the size of the tablebase cache.
 *
 * @var size the size of the tablebase cache in MB
 *
 * @return 0 if no errors occurred
 */
int setTablebaseCacheSize(unsigned int size);

/**
 * Probe the tablebase for the specified position.
 */
int probeTablebase(const Position * position);

#endif
