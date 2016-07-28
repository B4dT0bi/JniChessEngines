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
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */

#include "coordination.h"
#include "protector.h"
#include "search.h"
#include "matesearch.h"
#include "io.h"
#include "hash.h"
#include "test.h"
#include "xboard.h"
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <time.h>

/* #define DEBUG_COORDINATION */

pthread_t searchThread[MAX_THREADS];
pthread_t timer;

static pthread_mutex_t guiSearchMutex = PTHREAD_MUTEX_INITIALIZER;

long searchThreadId[MAX_THREADS];

static int numThreads = 1;
static SearchTask dummyTask;
static SearchTask *currentTask = &dummyTask;
static Variation variations[MAX_THREADS];
static Hashtable sharedHashtable;
static PawnHashInfo pawnHashtable[MAX_THREADS][PAWN_HASHTABLE_SIZE];

Hashtable *getSharedHashtable(void)
{
   return &sharedHashtable;
}

int setNumberOfThreads(int _numThreads)
{
   numThreads = max(1, min(MAX_THREADS, _numThreads));

   return numThreads;
}

int getNumberOfThreads(void)
{
   return numThreads;
}

UINT64 getNodeCount(void)
{
   int threadCount;
   UINT64 sum = 0;

   for (threadCount = 0; threadCount < numThreads; threadCount++)
   {
      sum += variations[threadCount].nodes;
   }

   return sum;
}

Variation *getCurrentVariation(void)
{
   return &variations[0];
}

void getGuiSearchMutex(void)
{
#ifdef DEBUG_COORDINATION
   logDebug("aquiring search lock...\n");
#endif

   pthread_mutex_lock(&guiSearchMutex);

#ifdef DEBUG_COORDINATION
   logDebug("search lock aquired...\n");
#endif
}

void releaseGuiSearchMutex(void)
{
   pthread_mutex_unlock(&guiSearchMutex);

#ifdef DEBUG_COORDINATION
   logDebug("search lock released...\n");
#endif
}

static int startSearch(Variation * currentVariation)
{
   currentVariation->searchStatus = SEARCH_STATUS_RUNNING;

#ifdef DEBUG_COORDINATION
   logDebug("Search with thread #%d started.\n",
            currentVariation->threadNumber);
#endif

   switch (currentTask->type)
   {
   case TASKTYPE_BEST_MOVE:
      currentTask->bestMove = search(currentVariation, NULL);
      break;

   case TASKTYPE_TEST_BEST_MOVE:
      currentTask->bestMove =
         search(currentVariation, &currentTask->solutions);
      break;

   case TASKTYPE_MATE_IN_N:
      searchForMate(currentVariation,
                    &currentTask->calculatedSolutions,
                    currentTask->numberOfMoves);
      break;

   case TASKTYPE_TEST_MATE_IN_N:
      searchForMate(currentVariation,
                    &currentTask->calculatedSolutions,
                    currentTask->numberOfMoves);
      break;

   default:
      break;
   }

   currentTask->nodes = getNodeCount();

   if (currentVariation->threadNumber == 0)
   {
      int threadCount;

      for (threadCount = 1; threadCount < numThreads; threadCount++)
      {
         variations[threadCount].terminate = TRUE;
      }

      pthread_cancel(timer);
   }

#ifdef DEBUG_COORDINATION
   logDebug("Search thread #%d terminated.\n",
            currentVariation->threadNumber);
#endif

   currentVariation->searchStatus = SEARCH_STATUS_FINISHED;

   return 0;
}

long getElapsedTime(void)
{
   return getTimestamp() - variations[0].startTime;
}

static void *executeSearch(void *arg)
{
   Variation *currentVariation = arg;

   startSearch(currentVariation);

   return 0;
}

void handleSearchEvent(int eventId, Variation * variation)
{
   if (variation->handleUciEvents != FALSE)
   {
      handleUciSearchEvent(eventId, variation);
   }
   else
   {
      handleCliSearchEvent(eventId, variation);
   }
}

static void *watchTime(void *arg)
{
   Variation *currentVariation = arg;
   long timeLimit = currentVariation->timeLimit;
   struct timespec requested, remaining;
   int result;

   requested.tv_sec = timeLimit / 1000;
   requested.tv_nsec = 1000000 * (timeLimit - 1000 * requested.tv_sec);

   /* logReport("### Timer thread working sec=%ld nsec=%ld ###\n",
      requested.tv_sec, requested.tv_nsec); */

   result = nanosleep(&requested, &remaining);

   if (result != -1)
   {
      getGuiSearchMutex();
      prepareSearchAbort();
      releaseGuiSearchMutex();
   }

   return 0;
}

void startTimerThread(SearchTask * task)
{
   if (task->variation->timeLimit > 0 && task->variation->ponderMode == FALSE)
   {
      if (pthread_create(&timer, NULL, &watchTime, task->variation) == 0)
      {
#ifdef DEBUG_COORDINATION
         logDebug("Timer thread started.\n");
#endif
      }
      else
      {
         logDebug("### Timer thread could not be started. ###\n");

         exit(EXIT_FAILURE);
      }
   }
}

void scheduleTask(SearchTask * task)
{
   const unsigned long startTime = getTimestamp();
   int threadCount;

   sharedHashtable.entriesUsed = 0;

   startTimerThread(task);

   for (threadCount = 0; threadCount < numThreads; threadCount++)
   {
      Variation *currentVariation = &variations[threadCount];

      currentTask = task;
      *currentVariation = *(currentTask->variation);
      currentVariation->searchStatus = SEARCH_STATUS_TERMINATE;
      currentVariation->bestBaseMove = NO_MOVE;
      currentVariation->terminate = FALSE;
      currentVariation->pawnHashtable = &(pawnHashtable[threadCount][0]);
      currentVariation->kingsafetyHashtable =
         &(kingSafetyHashtable[threadCount][0]);
      currentVariation->threadNumber = threadCount;
      currentVariation->startTime = startTime;

      if (pthread_create(&searchThread[threadCount], NULL,
                         &executeSearch, currentVariation) == 0)
      {
#ifdef DEBUG_COORDINATION
         logDebug("Search thread #%d created.\n", threadCount);
#endif
      }
      else
      {
         logDebug("### Search thread #%d could not be started. ###\n",
                  threadCount);

         exit(EXIT_FAILURE);
      }
   }
}

void waitForSearchTermination(void)
{
   int threadCount;
   bool finished;
   int count = 0;

   do
   {
      finished = TRUE;

      if (count > 1000)
      {
         logDebug("waiting for search termination.\n");
         count = 0;
      }

      for (threadCount = 0; threadCount < numThreads; threadCount++)
      {
         Variation *currentVariation = &variations[threadCount];

         if (currentVariation->searchStatus != SEARCH_STATUS_FINISHED)
         {
            if (searchThread[threadCount] != 0)
            {
               const int result = pthread_join(searchThread[threadCount], 0);

               if (result == 0)
               {
                  searchThread[threadCount] = 0;
               }
               else
               {
                  finished = FALSE;
               }
            }
         }
         else
         {
            searchThread[threadCount] = 0;
         }

#ifdef DEBUG_COORDINATION
         logDebug("Task %d finished.\n", threadCount);
#endif
      }

      count++;
   }
   while (finished == FALSE);
}

void completeTask(SearchTask * task)
{
   scheduleTask(task);

#ifdef DEBUG_COORDINATION
   logDebug("Task scheduled. Waiting for completion.\n");
#endif

   waitForSearchTermination();
}

void prepareSearchAbort(void)
{
   int threadCount;

   for (threadCount = 0; threadCount < numThreads; threadCount++)
   {
      variations[threadCount].terminate = TRUE;
   }
}

void unsetPonderMode(void)
{
   int threadCount;

   for (threadCount = 0; threadCount < numThreads; threadCount++)
   {
      Variation *currentVariation = &variations[threadCount];

      currentVariation->ponderMode = FALSE;
   }
}

void setTimeLimit(unsigned long timeTarget, unsigned long timeLimit)
{
   int threadCount;

   for (threadCount = 0; threadCount < numThreads; threadCount++)
   {
      Variation *currentVariation = &variations[threadCount];

      currentVariation->timeTarget = timeTarget;
      currentVariation->timeLimit = timeLimit;
   }
}

void setHashtableSizeInMb(unsigned int size)
{
   UINT64 tablesize = 1024 * 1024 * (UINT64) size;

   setHashtableSize(&sharedHashtable, tablesize);
   resetHashtable(&sharedHashtable);
}

int initializeModuleCoordination(void)
{
   int threadCount;

   initializeHashtable(&sharedHashtable);
   setHashtableSize(&sharedHashtable, 16 * 1024 * 1024);
   resetHashtable(&sharedHashtable);

   for (threadCount = 0; threadCount < MAX_THREADS; threadCount++)
   {
      Variation *currentVariation = &variations[threadCount];

      currentVariation->searchStatus = SEARCH_STATUS_FINISHED;
   }

   return 0;
}

int testModuleCoordination(void)
{
   return 0;
}
