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

//
// Construct and initialize a critical section wrapper
//
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

