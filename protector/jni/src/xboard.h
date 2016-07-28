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

#ifndef _xboard_h_
#define _xboard_h_

#include "protector.h"
#include "position.h"

extern int minPvHashEntrySendDepth;
extern int minPvHashEntrySendTime;
extern int maxPvHashEntrySendHeight;
extern int pvHashEntriesSendInterval;

typedef struct
{
   int movesToGo;
   int incrementTime;           /* unit: milliseconds */

   int numberOfMovesPlayed;
   int restTime;                /* unit: milliseconds */
}
TimecontrolData;

#define XBOARD_OPERATIONMODE_ANALYSIS 2
#define XBOARD_OPERATIONMODE_USERGAME 4

typedef struct
{
   TimecontrolData timecontrolData[2];
   int operationMode;           /* see above */
   Color engineColor;
   bool pondering;
   bool engineIsActive, engineIsPondering, bestMoveWasSent;
   Move ponderingMove;
   Move ponderResultMove;
   int maxPlies;
}
XboardStatus;

/**
 * Wait for the next Xboard/Winboard command and process it.
 */
void acceptGuiCommands(void);

/**
 * Handle a search event defined by eventId.
 */
void handleUciSearchEvent(int eventId, Variation * variation);

/*
 * Send a hash entry via UCI.
 */
void sendHashentry(Hashentry * entry);

/**
 * Initialize this module.
 *
 * @return 0 if no errors occurred.
 */
int initializeModuleXboard(void);

/**
 * Test this module.
 *
 * @return 0 if all tests succeed.
 */
int testModuleXboard(void);
void addHashentry(Hashentry * entry);

#endif
