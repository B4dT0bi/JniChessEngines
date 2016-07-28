#ifndef TimeManager_H
#define TimeManager_H

#include "Types.h"

void StartTimer();
uint32 TimeElapsed();

extern uint32 EngineTime;
extern uint32 TimeLimited;
extern uint32 MovesPerControl;
extern uint32 MovesToControl;
extern uint32 IncrementTime;

extern uint32 TimeLimitHard;
extern uint32 TimeLimitSoft;


#endif