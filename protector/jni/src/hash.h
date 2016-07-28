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

#ifndef _hash_h_
#define _hash_h_

#include "protector.h"
#include "position.h"

/**
 * Define the size of the specified hashtable.
 *
 * @param size the size of the hashtable in bytes
 */
void setHashtableSize(Hashtable * hashtable, UINT64 size);

/**
 * Initialize the specified hashtable. Call this function exactly once
 * for every hashtable before using it.
 */

void resetNodetable(void);
void initializeHashtable(Hashtable * hashtable);
bool isPrimeNumber(UINT64 n);
UINT64 getNextPrime(UINT64 n);
UINT64 getPreviousPrime(UINT64 n);
UINT64 getHashIndex(Hashtable * hashtable, UINT64 key);

/**
 * Reset the specified hashtable. Call this function in order to
 * erase all stored data.
 */
void resetHashtable(Hashtable * hashtable);

/**
 * Increment the date of the specified hashtable.
 */
void incrementDate(Hashtable * hashtable);

/**
 * Construct a hashentry from the given values.
 */
Hashentry constructHashEntry(UINT64 key, INT16 value, INT16 staticValue,
                             UINT8 importance, UINT16 bestMove, UINT8 date,
                             UINT8 flag);

/**
 * Put the specified entry into the hashtable.
 */
void setHashentry(Hashtable * hashtable, UINT64 key, INT16 value,
                  UINT8 importance, UINT16 bestMove, UINT8 flag,
                  INT16 staticValue);

/**
 * Get the entry specified by key.
 */
Hashentry *getHashentry(Hashtable * hashtable, UINT64 key);
UINT64 getNodeIndex(UINT64 key);

INT16 getHashentryValue(const Hashentry * entry);

UINT8 getHashentryImportance(const Hashentry * entry);
UINT16 getHashentryMove(const Hashentry * entry);
UINT8 getHashentryDate(const Hashentry * entry);
UINT8 getHashentryFlag(const Hashentry * entry);
UINT64 getHashentryKey(const Hashentry * entry);
INT16 getHashentryStaticValue(const Hashentry * entry);
bool nodeIsInUse(UINT64 key, UINT8 depth);
bool setNodeUsage(UINT64 key, UINT8 depth);
void resetNodeUsage(UINT64 key, UINT8 depth);

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleHash(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleHash(void);

#endif
