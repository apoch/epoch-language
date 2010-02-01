//
// The Epoch Language Project
// Shared Library Code
//
// Thread synchronization objects
//

#include "pch.h"

#include "Utility/Threading/Synchronization.h"
#include "Utility/Threading/Lockless.h"


using namespace Threads;


//-------------------------------------------------------------------------------
// Critical section
//-------------------------------------------------------------------------------

CriticalSection::CriticalSection()
{
	::InitializeCriticalSection(&CritSec);
}


//
// Release the system critical section
//
CriticalSection::~CriticalSection()
{
	::DeleteCriticalSection(&CritSec);
}

//
// Enter the critical section, blocking until another thread leaves it, if necessary
//
void CriticalSection::Enter()
{
	::EnterCriticalSection(&CritSec);
}

//
// Leave the critical section, allowing other threads to utilize it
//
void CriticalSection::Exit()
{
	::LeaveCriticalSection(&CritSec);
}



//-------------------------------------------------------------------------------
// Synchronization counter
//-------------------------------------------------------------------------------

//
// Construct the counter wrapper and increment the counter
//
SyncCounter::SyncCounter(unsigned* pcounter, HANDLE tripevent)
	: PointerToCounter(pcounter), TripEvent(tripevent)
{
	::ResetEvent(tripevent);

	while(true)
	{
		unsigned oldval = *pcounter;
		bool success = CompareAndSwap(pcounter, oldval, oldval + 1);
		if(success)
			return;
	}
}

//
// Destruct the counter wrapper and decrement the counter
//
SyncCounter::~SyncCounter()
{
	while(true)
	{
		unsigned oldval = *PointerToCounter;
		bool success = CompareAndSwap(PointerToCounter, oldval, oldval - 1);
		if(success)
		{
			if(oldval - 1 == 0)
				::SetEvent(TripEvent);

			return;
		}
	}
}
