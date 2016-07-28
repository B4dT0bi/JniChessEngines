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

#ifndef _coordination_h_
#define _coordination_h_

#include "position.h"
#include "movegeneration.h"

typedef enum
{
   TASKTYPE_BEST_MOVE,
   TASKTYPE_TEST_BEST_MOVE,
   TASKTYPE_MATE_IN_N,
   TASKTYPE_TEST_MATE_IN_N
} TaskType;

typedef struct
{
   TaskType type;               /* the type of task to be performed */
   Variation *variation;        /* the variation to be examined */
   int numberOfMoves;           /* mateproblems: the number of moves */
   Movelist solutions;          /* mateproblems: the known solutions */

   Movelist calculatedSolutions;        /* the calculated solutions */
   Move bestMove;               /* the calculated best move */

   UINT64 nodes;                /* the number of nodes calculated */
} SearchTask;

/**
 * Set the number of threads to be used.
 *
 * @var numThreads the number of threads to be used
 * @return the effective number of threads that will be used
 */
int setNumberOfThreads(int numThreads);
int getNumberOfThreads(void);

/**
 * Set the size of the hashtable.
 *
 * @var size the size of the hashtable in MB
 */
void setHashtableSizeInMb(unsigned int size);

/**
 * Lock out either the gui or the search thread.
 */
void getGuiSearchMutex(void);

/**
 * Release the gui/search thread lock.
 */
void releaseGuiSearchMutex(void);

/**
 * Schedule the specified task as the next task to be calculated.
 */
void scheduleTask(SearchTask * task);

/**
 * Start the timer of the specified task.
 */
void startTimerThread(SearchTask * task);

/**
 * Get the elapsed time of the current search.
 */
long getElapsedTime(void);

/**
 * Get the hashtable shared by the search threads.
 */
Hashtable *getSharedHashtable(void);

/**
 * Signal an abortion of the current search and copy 
 * the current variation to 'variation'.
 */
void prepareSearchAbort(void);

/**
 * Unset the ponder mode for the current search.
 */
void unsetPonderMode(void);

/**
 * Wait for the current search to terminate.
 */
void waitForSearchTermination(void);

/**
 * Set the timelimits for the current search task.
 */
void setTimeLimit(unsigned long timeTarget, unsigned long timeLimit);

/**
 * Schedule the specified task as the next task to be calculated.
 * Then wait until the task is completed.
 */
void completeTask(SearchTask * task);

/**
 * Get the variation object of the current search task.
 */
Variation *getCurrentVariation(void);

/**
 * Handle a search event defined by eventId.
 */
void handleSearchEvent(int eventId, Variation * variation);

/**
 * Get the number of nodes calculated by all active threads.
 */
UINT64 getNodeCount(void);

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleCoordination(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleCoordination(void);

#endif
