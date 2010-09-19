//
// The Epoch Language Project
// Shared Library Code
//
// Thread synchronization objects
//

#pragma once


namespace Threads
{

	//
	// Wrapper for a system critical section object.
	// Use the helper class for RAII semantics, to
	// ensure that the critical section is always
	// entered and exited correctly.
	//
	class CriticalSection
	{
	// Construction and destruction
	public:
		CriticalSection();
		~CriticalSection();

	// Entry and exit
	public:
		void Enter();
		void Exit();

	// RAII helper
	public:
		struct Auto
		{
			Auto(CriticalSection& critsec)
				: BoundCritSec(critsec)
			{
				BoundCritSec.Enter();
			}

			~Auto()
			{
				BoundCritSec.Exit();
			}

		private:
			CriticalSection& BoundCritSec;
		};

	// Internal tracking
	private:
		CRITICAL_SECTION CritSec;
	};

}

