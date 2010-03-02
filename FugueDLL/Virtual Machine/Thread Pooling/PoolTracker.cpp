//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Management of thread pools used by the VM
//

#include "pch.h"

#include "Virtual Machine/Thread Pooling/PoolTracker.h"
#include "Virtual Machine/VMExceptions.h"

#include "Utility/Strings.h"


using namespace VM;


//
// Destruct the thread pool tracker and all associated thread pool objects
//
ThreadPoolTracker::~ThreadPoolTracker()
{
	for(NamedThreadPoolMap::iterator iter = NamedThreadPools.begin(); iter != NamedThreadPools.end(); ++iter)
		delete iter->second;
}


//
// Create a new thread pool with the given name and number of worker threads
//
void ThreadPoolTracker::CreateNamedPool(const std::wstring& poolname, unsigned numthreads, VM::Program* runningprogram)
{
	if(NamedThreadPools.find(poolname) != NamedThreadPools.end())
		throw ExecutionException("A thread pool by the name of \"" + narrow(poolname) + "\" already exists, cannot create another pool by this name");

	NamedThreadPools[poolname] = new Threads::ThreadPool(numthreads, runningprogram);
}


//
// Retrieve a thread pool given its name
//
Threads::ThreadPool& ThreadPoolTracker::GetNamedPool(const std::wstring& poolname)
{
	NamedThreadPoolMap::iterator iter = NamedThreadPools.find(poolname);
	if(iter == NamedThreadPools.end())
		throw ExecutionException("Cannot retrieve thread pool \"" + narrow(poolname) + "\" - no pool by that name exists");

	return *iter->second;
}


bool ThreadPoolTracker::HasNamedPool(const std::wstring& poolname) const
{
	return (NamedThreadPools.find(poolname) != NamedThreadPools.end());
}
