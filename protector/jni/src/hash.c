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
#include <assert.h>
#include <math.h>
#include "hash.h"
#include "protector.h"
#include "io.h"
#include "keytable.h"

#define NODE_TABLE_SIZE (MAX_THREADS * MAX_DEPTH * 32)

const unsigned int NUM_DATES = 16;
const unsigned int CLUSTER_SIZE = 4;
const UINT8 DEPTH_NONE = 0;
Nodeentry nodeUsageTable[NODE_TABLE_SIZE];
long numNodeTableEntries;

INT16 getHashentryValue(const Hashentry * entry)
{
   return (INT16) (entry->data & 0xFFFF);
}

INT16 getHashentryStaticValue(const Hashentry * entry)
{
   return (INT16) ((entry->data >> 16) & 0xFFFF);
}

UINT8 getHashentryImportance(const Hashentry * entry)
{
   return (UINT8) ((entry->data >> 32) & 0xFF);
}

UINT16 getHashentryMove(const Hashentry * entry)
{
   return (UINT16) ((entry->data >> 40) & 0xFFFF);
}

UINT8 getHashentryDate(const Hashentry * entry)
{
   return (UINT8) ((entry->data >> 56) & 0x0F);
}

UINT8 getHashentryFlag(const Hashentry * entry)
{
   return (UINT8) ((entry->data >> 60) & 0x03);
}

UINT64 getHashentryKey(const Hashentry * entry)
{
   return entry->key ^ entry->data;
}

static int getAge(const Hashtable * hashtable, const UINT8 date)
{
   assert(date < NUM_DATES);
   assert(hashtable->date < NUM_DATES);

   return (hashtable->date + NUM_DATES - date) & (NUM_DATES - 1);
}

void incrementDate(Hashtable * hashtable)
{
   assert(hashtable->date < NUM_DATES);

   hashtable->date = (UINT8) ((hashtable->date + 1) % NUM_DATES);

   assert(hashtable->date < NUM_DATES);
}

static void deleteTables(Hashtable * hashtable)
{
   if (hashtable->table != 0)
   {
      free(hashtable->table);
      hashtable->table = 0;
   }
}

static UINT64 _getHashData(INT16 value, INT16 staticValue, UINT8 importance,
                           UINT16 bestMove, UINT8 date, UINT8 flag)
{
   return ((UINT64) (value & 0xFFFF)) |
      ((UINT64) (staticValue & 0xFFFF)) << 16 |
      ((UINT64) importance) << 32 |
      ((UINT64) bestMove) << 40 |
      ((UINT64) date) << 56 | ((UINT64) flag) << 60;
}

Hashentry constructHashEntry(UINT64 key, INT16 value, INT16 staticValue,
                             UINT8 importance, UINT16 bestMove, UINT8 date,
                             UINT8 flag)
{
   Hashentry entry;

   entry.key = key;
   entry.data = _getHashData(value, staticValue, importance,
                             bestMove, date, flag);

   return entry;
}

void resetHashtable(Hashtable * hashtable)
{
   UINT64 l;
   Hashentry emptyEntry;

   emptyEntry.key = ULONG_ZERO;
   emptyEntry.data = _getHashData(-VALUE_MATED, 0, DEPTH_NONE,
                                  (UINT16) NO_MOVE, 0, HASHVALUE_UPPER_LIMIT);

   for (l = 0; l < hashtable->tableSize + CLUSTER_SIZE; l++)
   {
      hashtable->table[l] = emptyEntry;
   }

   hashtable->date = 0;
   hashtable->entriesUsed = 0;

   /* logDebug("hashtable reset done.\n"); */
}

void resetNodetable(void)
{
   int i;

   for (i = 0; i < NODE_TABLE_SIZE; i++)
   {
      nodeUsageTable[i].key = ULONG_ZERO;
   }
}

void initializeHashtable(Hashtable * hashtable)
{
   hashtable->table = 0;
   hashtable->tableSize = 0;
   hashtable->entriesUsed = 0;
}

bool isPrimeNumber(UINT64 n)
{
   UINT64 limit, d;

   if (n == 2)
   {
      return TRUE;
   }

   if (n < 2 || n % 2 == 0)
   {
      return FALSE;
   }

   limit = (UINT64) (sqrt((double) n) + 1.0);

   for (d = 3; d <= limit; d += 2)
   {
      if (n % d == 0)
      {
         return FALSE;
      }
   }

   return TRUE;
}

UINT64 getNextPrime(UINT64 n)
{
   while (isPrimeNumber(n) == FALSE)
   {
      n++;
   }

   return n;
}

UINT64 getPreviousPrime(UINT64 n)
{
   n--;

   while (isPrimeNumber(n) == FALSE)
   {
      n--;
   }

   return n;
}

void setHashtableSize(Hashtable * hashtable, UINT64 size)
{
   const UINT64 ENTRY_SIZE = sizeof(Hashentry);

   deleteTables(hashtable);
   hashtable->tableSize = getNextPrime(size / ENTRY_SIZE);
   hashtable->table =
      malloc((hashtable->tableSize + CLUSTER_SIZE) * ENTRY_SIZE);

   /* logDebug("Hashtable size: %ld entries\n",
      hashtable->tableSize); */
}

UINT64 getHashIndex(Hashtable * hashtable, UINT64 key)
{
   return key % ((hashtable)->tableSize);
}

void setHashentry(Hashtable * hashtable, UINT64 key, INT16 value,
                  UINT8 importance, UINT16 bestMove, UINT8 flag,
                  INT16 staticValue)
{
   const UINT64 index = getHashIndex(hashtable, key);
   UINT64 data, i, bestEntry = 0;
   int bestEntryScore = -1024;
   Hashentry *entryToBeReplaced;

   for (i = 0; i < CLUSTER_SIZE; i++)
   {
      Hashentry copy = hashtable->table[index + i];
      const UINT8 copyDate = getHashentryDate(&copy);

      if (getHashentryKey(&copy) == key || copy.key == ULONG_ZERO)
      {
         if (copyDate != hashtable->date || copy.key == ULONG_ZERO)
         {
            hashtable->entriesUsed++;
         }

         if (bestMove == (UINT16) NO_MOVE)
         {
            bestMove = getHashentryMove(&copy);
         }

         data = _getHashData(value, staticValue, importance, bestMove,
                             hashtable->date, flag);
         hashtable->table[index + i].key = key ^ data;
         hashtable->table[index + i].data = data;

         return;
      }
      else
      {
         const int score =
            getAge(hashtable, copyDate) * 2 -
            getHashentryImportance(&copy) -
            (HASHVALUE_EXACT == getHashentryFlag(&copy) ? 2 : 0);

         if (score > bestEntryScore)
         {
            bestEntry = i;
            bestEntryScore = score;
         }
      }
   }

   if (getHashentryDate(&hashtable->table[index + bestEntry]) !=
       hashtable->date)
   {
      hashtable->entriesUsed++;
   }

   entryToBeReplaced = &hashtable->table[index + bestEntry];
   data = _getHashData(value, staticValue, importance, bestMove,
                       hashtable->date, flag);
   entryToBeReplaced->key = key ^ data;
   entryToBeReplaced->data = data;
}

Hashentry *getHashentry(Hashtable * hashtable, UINT64 key)
{
   const UINT64 index = getHashIndex(hashtable, key);
   UINT64 i;

   for (i = 0; i < CLUSTER_SIZE; i++)
   {
      Hashentry *tableEntry = &hashtable->table[index + i];

      if (getHashentryKey(tableEntry) == key)
      {
         if (getHashentryDate(tableEntry) != hashtable->date)
         {
#ifndef NDEBUG
            const Hashentry originalEntry = *tableEntry;
#endif
            const UINT64 mask = (((UINT64) 0x0F) << 56);
            const UINT64 newData = (tableEntry->data & ~mask) |
               (((UINT64) hashtable->date) << 56);

            tableEntry->key = key ^ newData;
            tableEntry->data = newData;
            hashtable->entriesUsed++;

            assert(getHashentryValue(&originalEntry) ==
                   getHashentryValue(tableEntry));
            assert(getHashentryStaticValue(&originalEntry) ==
                   getHashentryStaticValue(tableEntry));
            assert(getHashentryImportance(&originalEntry) ==
                   getHashentryImportance(tableEntry));
            assert(getHashentryMove(&originalEntry) ==
                   getHashentryMove(tableEntry));
            assert(getHashentryFlag(&originalEntry) ==
                   getHashentryFlag(tableEntry));
            assert(hashtable->date == getHashentryDate(tableEntry));
         }

         return tableEntry;
      }
   }

   return 0;
}

UINT64 getNodeIndex(UINT64 key)
{
   return key % numNodeTableEntries;
}

bool nodeIsInUse(UINT64 key, UINT8 depth)
{
   UINT64 nodeIndex = getNodeIndex(key);

   return nodeUsageTable[nodeIndex].key == key &&
      nodeUsageTable[nodeIndex].depth >= depth && key != ULONG_ZERO;
}

bool setNodeUsage(UINT64 key, UINT8 depth)
{
   UINT64 nodeIndex = getNodeIndex(key);

   if (nodeUsageTable[nodeIndex].key == ULONG_ZERO)
   {
      nodeUsageTable[nodeIndex].key = key;
      nodeUsageTable[nodeIndex].depth = depth;

      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

void resetNodeUsage(UINT64 key, UINT8 depth)
{
   UINT64 nodeIndex = getNodeIndex(key);

   nodeUsageTable[nodeIndex].key = ULONG_ZERO;
}

int initializeModuleHash(void)
{
   resetNodetable();
   numNodeTableEntries = getPreviousPrime(NODE_TABLE_SIZE);

   return 0;
}

static int testAgeCalculation(void)
{
   Hashtable hashtable;

   hashtable.date = 0;
   assert(getAge(&hashtable, (UINT8) (NUM_DATES - 1)) == 1);
   assert(getAge(&hashtable, 0) == 0);
   assert(getAge(&hashtable, 1) == (int) (NUM_DATES - 1));

   hashtable.date = 2;
   assert(getAge(&hashtable, 1) == 1);
   assert(getAge(&hashtable, 2) == 0);
   assert(getAge(&hashtable, 3) == (int) (NUM_DATES - 1));

   hashtable.date = (UINT8) (NUM_DATES - 1);
   assert(getAge(&hashtable, (UINT8) (NUM_DATES - 2)) == 1);
   assert(getAge(&hashtable, (UINT8) (NUM_DATES - 1)) == 0);
   assert(getAge(&hashtable, 0) == (int) (NUM_DATES - 1));

   return (hashtable.date == (UINT8) (NUM_DATES - 1) ? 0 : 1);
}

int testModuleHash(void)
{
   int result = 0;

   if ((result = testAgeCalculation()) != 0)
   {
      return result;
   }

   return 0;
}
