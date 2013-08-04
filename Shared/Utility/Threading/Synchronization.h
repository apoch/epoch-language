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
		void Enter() const;
		void Exit() const;

	// RAII helper
	public:
		struct Auto
		{
			Auto(const CriticalSection& critsec)
				: BoundCritSec(critsec)
			{
				BoundCritSec.Enter();
			}

			~Auto()
			{
				BoundCritSec.Exit();
			}

		// Non-copyable
		private:
			Auto(const Auto& rhs);
			Auto& operator = (const Auto& rhs);

		private:
			const CriticalSection& BoundCritSec;
		};

	// Internal tracking
	private:
#ifdef BOOST_WINDOWS
		CRITICAL_SECTION CritSec;
#else
        mutable pthread_mutex_t CritSec;
#endif
	};

}

