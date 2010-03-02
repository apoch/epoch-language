//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Management of thread pools used by the VM
//

#pragma once


// Dependencies
#include "Utility/Threading/ThreadPool.h"


namespace VM
{

	// Forward declarations
	class Program;


	class ThreadPoolTracker
	{
	// Destruction
	public:
		~ThreadPoolTracker();

	// Pool management
	public:
		void CreateNamedPool(const std::wstring& poolname, unsigned numthreads, VM::Program* runningprogram);
		Threads::ThreadPool& GetNamedPool(const std::wstring& poolname);
		bool HasNamedPool(const std::wstring& poolname) const;

	// Internal tracking
	private:
		typedef std::map<std::wstring, Threads::ThreadPool*> NamedThreadPoolMap;
		NamedThreadPoolMap NamedThreadPools;
	};

}

