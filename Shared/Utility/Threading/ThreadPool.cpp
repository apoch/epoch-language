//
// The Epoch Language Project
// Shared Library Code
//
// Wrappers for creating thread pools and feeding them work
//

#include "pch.h"

#include "Utility/Threading/ThreadPool.h"
#include "Utility/Threading/ThreadExceptions.h"


using namespace Threads;


namespace
{

	// Entry point stub for launching worker threads for a thread pool
	DWORD __stdcall WorkerThreadProc(void* detailptr)
	{
		ThreadPool::ThreadDetails* mydetails = reinterpret_cast<ThreadPool::ThreadDetails*>(detailptr);
		ThreadPool& mypool = *mydetails->OwningPool;
		HANDLE shutdownevent = mypool.GetShutdownEvent();

		Threads::Enter(&mydetails->Info);

		while(true)
		{
			// Exit the thread if the program is being shut down
			if(::WaitForSingleObject(shutdownevent, 0) == WAIT_OBJECT_0)
				break;

			// Wait until the pool wakes us up, indicating that there is work to do
			::WaitForSingleObject(mydetails->ThreadWakeEvent, INFINITE);

			// Extract work items 'til there ain't no more
			std::auto_ptr<PoolWorkItem> workitem(mypool.ClaimWorkItem(mydetails->ThreadHandle));
			while(workitem.get() != NULL)
			{
				// Don't do a work item if the program is trying to exit
				if(::WaitForSingleObject(shutdownevent, 0) == WAIT_OBJECT_0)
					break;

				// Go do something. Hopefully something interesting.
				try
				{
					workitem->PerformWork();
				}
				catch(std::exception& ex)
				{
					::MessageBoxA(0, ex.what(), Strings::WindowTitle, MB_ICONERROR);
				}
				catch(...)
				{
					::MessageBoxA(0, "An unexpected error has occurred while executing an Epoch task in a thread pool.", Strings::WindowTitle, MB_ICONERROR);
				}

				workitem.reset(mypool.ClaimWorkItem(mydetails->ThreadHandle));
			}

			::ResetEvent(mydetails->ThreadWakeEvent);
		}

		Threads::Exit();

		return 0;
	}

}


//
// Create a thread pool, allocating the requested number of threads
//
ThreadPool::ThreadPool(unsigned threadcount, VM::Program* runningprogram)
{
	ShutdownEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
	if(!ShutdownEvent)
		throw ThreadException("Failed to create synchronization event for pool shutdown procedure!");

	try
	{
		for(unsigned i = 0; i < threadcount; ++i)
		{
			ThreadDetails* details = new ThreadDetails;
			details->OwningPool = this;
			details->ThreadHandle = INVALID_HANDLE_VALUE;
			details->ThreadWakeEvent = INVALID_HANDLE_VALUE;

			details->Info.CodeBlock = NULL;
			details->Info.BoundFuture = NULL;
			details->Info.RunningProgram = runningprogram;
			details->Info.TaskOrigin = reinterpret_cast<Threads::ThreadInfo*>(::TlsGetValue(Threads::GetTLSIndex()))->HandleToSelf;
			details->Info.LocalHeapHandle = NULL;
			details->Info.MessageEvent = NULL;
			details->Info.Mailbox = NULL;

			details->ThreadHandle = ::CreateThread(NULL, 0, WorkerThreadProc, details, CREATE_SUSPENDED, &details->Info.HandleToSelf);
			if(!details->ThreadHandle)
				throw ThreadException("Failed to create a worker thread for a thread pool!");

			Details[details->ThreadHandle] = details;
			ThreadDetails* storeddetails = Details[details->ThreadHandle];

			storeddetails->ThreadWakeEvent = ::CreateEvent(NULL, TRUE, FALSE, NULL);
			if(!storeddetails->ThreadWakeEvent)
				throw ThreadException("Failed to create synchronization event for worker thread in a thread pool!");
		}
	}
	catch(...)
	{
		::SetEvent(ShutdownEvent);
		ResumeAllThreads();
		Clean();
		throw;
	}

	ResumeAllThreads();
}


//
// Clean up and destruct the pool wrapper.
//
ThreadPool::~ThreadPool()
{
	Clean();
}


//
// Wait until all worker threads are finished, then release resources used by the thread pool
//
void ThreadPool::Clean()
{
	// Notify all threads to shut down, and wait until they exit
	::SetEvent(ShutdownEvent);
	for(ThreadDetailMap::iterator iter = Details.begin(); iter != Details.end(); ++iter)
	{
		// Wake the thread up, just in case it isn't already running
		if(iter->second->ThreadWakeEvent != INVALID_HANDLE_VALUE)
			::SetEvent(iter->second->ThreadWakeEvent);

		::WaitForSingleObject(iter->second->ThreadHandle, INFINITE);
		delete iter->second;
	}

	// Now clean up our tracking resources
	{
		CriticalSection::Auto mutex(WorkItemCritSec);

		Details.clear();

		for(WorkItemMap::iterator iter = UnassignedWorkItems.begin(); iter != UnassignedWorkItems.end(); ++iter)
			delete iter->second;
		UnassignedWorkItems.clear();

		for(std::map<HANDLE, WorkItemMap>::iterator iter = AssignedWorkItems.begin(); iter != AssignedWorkItems.end(); ++iter)
		{
			for(WorkItemMap::iterator inneriter = iter->second.begin(); inneriter != iter->second.end(); ++inneriter)
				delete inneriter->second;
			iter->second.clear();
		}
		AssignedWorkItems.clear();
	}
}


//
// Resume all of the threads in the pool.
//
// WARNING: assumes that the threads ARE suspended, with a suspend count of one.
//
void ThreadPool::ResumeAllThreads()
{
	for(ThreadDetailMap::iterator iter = Details.begin(); iter != Details.end(); ++iter)
		::ResumeThread(iter->second->ThreadHandle);
}


//
// Add a work item to the thread pool.
//
// This function also assigns the work item to a specific thread, waking that
// thread up to perform the work item if necessary. The work item will be
// deleted by the thread pool, so the caller does not need to free it manually.
//
void ThreadPool::AddWorkItem(const std::wstring& taskname, PoolWorkItem* item)
{
	// TODO - assertion safety here (e.g. make sure nothing else is using the given taskname, etc.)

	for(ThreadDetailMap::iterator iter = Details.begin(); iter != Details.end(); ++iter)
	{
		// If the thread's wake event is not signalled, it must be idle
		if(::WaitForSingleObject(iter->second->ThreadWakeEvent, 0) == WAIT_TIMEOUT)
		{
			{
				CriticalSection::Auto mutex(WorkItemCritSec);
				AssignedWorkItems[iter->first][taskname] = item;
			}

			// Wake the thread up and bail
			::SetEvent(iter->second->ThreadWakeEvent);
			return;
		}
	}

	// We didn't find any idle threads, so shove this item on the back burner
	{
		CriticalSection::Auto mutex(WorkItemCritSec);
		UnassignedWorkItems[taskname] = item;
	}
}


//
// Request a work item from the pool.
//
PoolWorkItem* ThreadPool::ClaimWorkItem(HANDLE threadhandle)
{
	CriticalSection::Auto mutex(WorkItemCritSec);

	WorkItemMap& assigneditems = AssignedWorkItems[threadhandle];
	if(assigneditems.empty())
	{
		if(UnassignedWorkItems.empty())
			return NULL;

		// If there are any unassigned work items, grab the first one and do it
		std::auto_ptr<PoolWorkItem> item(UnassignedWorkItems.begin()->second);
		std::wstring taskname = UnassignedWorkItems.begin()->first;
		UnassignedWorkItems.erase(UnassignedWorkItems.begin());
		assigneditems[taskname] = item.release();
	}

	std::auto_ptr<PoolWorkItem> item(assigneditems.begin()->second);
	assigneditems.erase(assigneditems.begin());

	return item.release();
}

