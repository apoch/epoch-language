//
// The Epoch Language Project
// Shared Library Code
//
// Code for profiling/timing operations
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"


namespace Profiling
{

	class Timer
	{
	// Construction
	public:
		Timer();

	// Timing interface
	public:
		void Begin();
		void End();

		Integer64 GetTimeMs() const;

	// Internal state
	private:
		Integer64 Frequency;
		Integer64 BeginCount;
		Integer64 EndCount;
	};

}

