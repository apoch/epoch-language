//
// The Epoch Language Project
// Shared Library Code
//
// Code for profiling/timing operations
//

#include "pch.h"

#include "Utility/Profiling.h"


#ifndef BOOST_WINDOWS
#include <sys/time.h>
#endif

using namespace Profiling;


Timer::Timer()
	: Frequency(0),
	  BeginCount(0),
	  EndCount(0),
	  Accumulator(0)
{
#ifdef BOOST_WINDOWS
    LARGE_INTEGER freq;
	::QueryPerformanceFrequency(&freq);

	Frequency = freq.QuadPart;
#else
    Frequency = 1000000;
#endif
}

void Timer::Begin()
{
#ifdef BOOST_WINDOWS
	LARGE_INTEGER counter;
	::QueryPerformanceCounter(&counter);

	BeginCount = counter.QuadPart;
#else
    timeval start;
    gettimeofday(&start, 0);
    BeginCount = start.tv_usec;
#endif
}

void Timer::End()
{
#ifdef BOOST_WINDOWS
	LARGE_INTEGER counter;
	::QueryPerformanceCounter(&counter);

	EndCount = counter.QuadPart;
#else
    timeval stop;
    gettimeofday(&stop, 0);
    EndCount = stop.tv_usec;
#endif
}

void Timer::Accumulate()
{
	Accumulator += (EndCount - BeginCount);
}

Integer64 Timer::GetTimeMs() const
{
	return static_cast<Integer64>(1000.0 * (static_cast<double>(EndCount - BeginCount) / static_cast<double>(Frequency)));
}

Integer64 Timer::GetAccumulatedMs() const
{
	return static_cast<Integer64>(1000.0 * (static_cast<double>(Accumulator) / static_cast<double>(Frequency)));
}

