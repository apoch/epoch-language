//
// The Epoch Language Project
// Shared Library Code
//
// Code for profiling/timing operations
//

#include "pch.h"

#include "Utility/Profiling.h"


using namespace Profiling;


Timer::Timer()
	: Frequency(0),
	  BeginCount(0),
	  EndCount(0)
{
	LARGE_INTEGER freq;
	::QueryPerformanceFrequency(&freq);

	Frequency = freq.QuadPart;
}

void Timer::Begin()
{
	LARGE_INTEGER counter;
	::QueryPerformanceCounter(&counter);

	BeginCount = counter.QuadPart;
}

void Timer::End()
{
	LARGE_INTEGER counter;
	::QueryPerformanceCounter(&counter);

	EndCount = counter.QuadPart;
}

Integer64 Timer::GetTimeMs() const
{
	return static_cast<Integer64>(1000.0 * (static_cast<double>(EndCount - BeginCount) / static_cast<double>(Frequency)));
}

