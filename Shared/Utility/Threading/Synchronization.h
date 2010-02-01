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


	//
	// RAII wrapper of a special synchronization counter.
	// This counter is effectively an inverse semaphore;
	// when its count is 0, it is "unlocked", and when
	// the count is greater than 0, is is "locked."
	//
	class SyncCounter
	{
	// Construction and destruction
	public:
		explicit SyncCounter(unsigned* pcounter, HANDLE tripevent);
		~SyncCounter();

	// Internal tracking
	private:
		unsigned* PointerToCounter;
		HANDLE TripEvent;
	};

}

