#include <windows.h>
#include <iostream>
using namespace std;
#include <stdlib.h>

#include "Types.h"
#include "Protocols.h"

// здесь запоминаем время
DWORD InitTicks;

// оставшееся у движка время
uint32 EngineTime = 0;

// контроль время на ход
uint32 TimeLimited = 0;

// контроль на определенное число ходов
uint32 MovesPerControl;
uint32 MovesToControl = 0;

// инкремент
uint32 IncrementTime;

// ограничения времени для перебора
uint32 TimeLimitHard;
uint32 TimeLimitSoft;

// запас времени на непредвиденные расходы
const uint32 TimeReserve = 16;

// предполагаем партия еще будет длится ходов не менне
const uint8 MovesToEnd = 20;
//const double BranchingFactor = 4;
// если при игре с контролем на определенное число ходов
// оставшихся ходов до контроля больше, чем MovesToEnd + ControlFactor,
// то считаем, что как-будто контроль до конца игры
// если меньше, то на первые ходы стараемся выделить побольше времени
// тем больше, чем больше ControlFactor
const uint32 ControlFactor = 10;
// на сколько быстрее можем делать ход при завершении уровня
//const double SoftFactor = 0.17;

// включаем таймер
void StartTimer()
{
	// засекаем время
	InitTicks = GetTickCount();

	// режим анализа
	if (AnalyzeMode)
	{
		TimeLimitHard = TimeLimitSoft = 0;
	}
	// время на ход
	else if (TimeLimited)
	{
		TimeLimitHard = TimeLimitSoft = TimeLimited;
	}
	// дано время на определенное число ходов
	else if (MovesToControl)
	{
		double factor; // оставшееся время делится на это число
		if (MovesToControl >= MovesToEnd + ControlFactor) factor = MovesToEnd;
		else factor = 1 + (MovesToControl - 1) * double (MovesToEnd - 1) / (MovesToEnd + ControlFactor - 1);
		int time = EngineTime / factor  + IncrementTime - TimeReserve * __min(MovesToControl, MovesToEnd);
		if (time <= 0) time = 1;
		TimeLimitSoft = time;
		TimeLimitHard = EngineTime - 2 * TimeReserve * __min(MovesToControl, MovesToEnd);
		if (MovesToControl > 1) TimeLimitHard /= 2;
	}

	// часы фишера
	else if (EngineTime)
	{
        // время для завершения итерации
		int time = EngineTime - 300;
		time = time / MovesToEnd + IncrementTime * 0.9;
		if (time > EngineTime - 300) time = EngineTime - 300;
		if (time <= 0) time = 1;
		TimeLimitSoft = time;

		// максимально допустимое время на ход
		time = EngineTime - 300;
		time = time / 2;
		if (time <= 0) time = 1;
		TimeLimitHard = time;
	}
	// без ограничений по времени
	else TimeLimitHard = TimeLimitSoft = 0;
}

// показания таймера
uint32 TimeElapsed()
{
	return GetTickCount() - InitTicks;
}
