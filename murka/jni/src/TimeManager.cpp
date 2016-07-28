#include <windows.h>
#include <iostream>
using namespace std;
#include <stdlib.h>

#include "Types.h"
#include "Protocols.h"

// ����� ���������� �����
DWORD InitTicks;

// ���������� � ������ �����
uint32 EngineTime = 0;

// �������� ����� �� ���
uint32 TimeLimited = 0;

// �������� �� ������������ ����� �����
uint32 MovesPerControl;
uint32 MovesToControl = 0;

// ���������
uint32 IncrementTime;

// ����������� ������� ��� ��������
uint32 TimeLimitHard;
uint32 TimeLimitSoft;

// ����� ������� �� �������������� �������
const uint32 TimeReserve = 16;

// ������������ ������ ��� ����� ������ ����� �� �����
const uint8 MovesToEnd = 20;
//const double BranchingFactor = 4;
// ���� ��� ���� � ��������� �� ������������ ����� �����
// ���������� ����� �� �������� ������, ��� MovesToEnd + ControlFactor,
// �� �������, ��� ���-����� �������� �� ����� ����
// ���� ������, �� �� ������ ���� ��������� �������� �������� �������
// ��� ������, ��� ������ ControlFactor
const uint32 ControlFactor = 10;
// �� ������� ������� ����� ������ ��� ��� ���������� ������
//const double SoftFactor = 0.17;

// �������� ������
void StartTimer()
{
	// �������� �����
	InitTicks = GetTickCount();

	// ����� �������
	if (AnalyzeMode)
	{
		TimeLimitHard = TimeLimitSoft = 0;
	}
	// ����� �� ���
	else if (TimeLimited)
	{
		TimeLimitHard = TimeLimitSoft = TimeLimited;
	}
	// ���� ����� �� ������������ ����� �����
	else if (MovesToControl)
	{
		double factor; // ���������� ����� ������� �� ��� �����
		if (MovesToControl >= MovesToEnd + ControlFactor) factor = MovesToEnd;
		else factor = 1 + (MovesToControl - 1) * double (MovesToEnd - 1) / (MovesToEnd + ControlFactor - 1);
		int time = EngineTime / factor  + IncrementTime - TimeReserve * __min(MovesToControl, MovesToEnd);
		if (time <= 0) time = 1;
		TimeLimitSoft = time;
		TimeLimitHard = EngineTime - 2 * TimeReserve * __min(MovesToControl, MovesToEnd);
		if (MovesToControl > 1) TimeLimitHard /= 2;
	}

	// ���� ������
	else if (EngineTime)
	{
        // ����� ��� ���������� ��������
		int time = EngineTime - 300;
		time = time / MovesToEnd + IncrementTime * 0.9;
		if (time > EngineTime - 300) time = EngineTime - 300;
		if (time <= 0) time = 1;
		TimeLimitSoft = time;

		// ����������� ���������� ����� �� ���
		time = EngineTime - 300;
		time = time / 2;
		if (time <= 0) time = 1;
		TimeLimitHard = time;
	}
	// ��� ����������� �� �������
	else TimeLimitHard = TimeLimitSoft = 0;
}

// ��������� �������
uint32 TimeElapsed()
{
	return GetTickCount() - InitTicks;
}
