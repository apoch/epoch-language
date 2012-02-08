//
// The Epoch Language Project
// Shared Library Code
//
// Thread synchronization objects
//

#include "pch.h"

#include "Utility/Threading/Synchronization.h"


using namespace Threads;


//-------------------------------------------------------------------------------
// Critical section
//-------------------------------------------------------------------------------

//
// Construct and initialize a critical section wrapper
//
CriticalSection::CriticalSection()
{
#ifdef BOOST_WINDOWS
	::InitializeCriticalSection(&CritSec);
#else
    pthread_mutex_init(&CritSec, 0);
#endif
}


//
// Release the system critical section
//
CriticalSection::~CriticalSection()
{
#ifdef BOOST_WINDOWS
	::DeleteCriticalSection(&CritSec);
#else
    pthread_mutex_destroy(&CritSec);
#endif
}

//
// Enter the critical section, blocking until another thread leaves it, if necessary
//
void CriticalSection::Enter() const
{
	// This is an evil cast, but we do it anyways so that critical section holders
	// can lock safely in const member functions.
#ifdef BOOST_WINDOWS
	::EnterCriticalSection(const_cast<LPCRITICAL_SECTION>(&CritSec));
#else
    pthread_mutex_lock(&CritSec);
#endif
}

//
// Leave the critical section, allowing other threads to utilize it
//
void CriticalSection::Exit() const
{
	// This is an evil cast, but we do it anyways so that critical section holders
	// can lock safely in const member functions.
#ifdef BOOST_WINDOWS
	::LeaveCriticalSection(const_cast<LPCRITICAL_SECTION>(&CritSec));
#else
    pthread_mutex_unlock(&CritSec);
#endif
}

