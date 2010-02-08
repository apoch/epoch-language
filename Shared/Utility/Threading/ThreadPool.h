//
// The Epoch Language Project
// Shared Library Code
//
// Wrappers for creating thread pools and feeding them work
//

#pragma once


// Dependencies
#include "Utility/Threading/Synchronization.h"
#include "Utility/Threading/Threads.h"
#include <map>


// Forward declarations
namespace VM { class Program; }


namespace Threads
{

	//
	// Wrapper interface for work items that can be fed to a thread pool
	//
	struct PoolWorkItem
	{
		virtual ~PoolWorkItem() { }
		virtual void PerformWork() = 0;
	};


	//
	// Wrapper for managing a pool of threads
	//
	class ThreadPool
	{
	// Construction and destruction
	public:
		ThreadPool(unsigned threadcount, VM::Program* runningprogram);
		~ThreadPool();

	// Make pools uncopyable (since a deep copy doesn't make sense)
	private:
		ThreadPool(const ThreadPool& other);
		ThreadPool& operator=(const ThreadPool& other);

	// Interface for supplying work to the pool
	public:
		void AddWorkItem(const std::wstring& taskname, PoolWorkItem* item);

	// Interface for dispatching work items to the pool threads
	public:
		PoolWorkItem* ClaimWorkItem(HANDLE threadhandle);

	// Thread tracking helpers
	public:
		struct ThreadDetails
		{
			ThreadPool* OwningPool;
			HANDLE ThreadHandle;
			HANDLE ThreadWakeEvent;
			Threads::ThreadInfo Info;
		};

		HANDLE GetShutdownEvent() const
		{ return ShutdownEvent; }

	// Internal helpers
	private:
		void Clean();

		void ResumeAllThreads();

	// Internal tracking
	private:
		typedef std::map<HANDLE, ThreadDetails*> ThreadDetailMap;
		ThreadDetailMap Details;

		typedef std::map<std::wstring, PoolWorkItem*> WorkItemMap;
		WorkItemMap UnassignedWorkItems;
		std::map<HANDLE, WorkItemMap> AssignedWorkItems;

		HANDLE ShutdownEvent;

		CriticalSection WorkItemCritSec;
	};

}

