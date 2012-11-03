//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Memory management subsystems. These classes handle the internal
// memory allocations and services for the virtual machine.
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Threading/Synchronization.h"

#include <vector>


//
// Platform-independent wrapper for managing heaps.
//
class HeapManager
{
// Construction and destruction
public:
	HeapManager();
	~HeapManager();

// Allocation and deallocation interface
public:
	void* Allocate(size_t bytes);
	void Deallocate(void* p);

// Global access to the heap wrapper
public:
	static HeapManager& GetGlobalHeapManager()
	{
		Threads::CriticalSection::Auto lock(CritSec);

		if(!GlobalHeapManager)
			GlobalHeapManager = new HeapManager;
		return *GlobalHeapManager;
	}

// Internal storage (platform specific details)
private:
	HANDLE HeapHandle;
	unsigned PageSize;

// Shared internal storage
private:
	static HeapManager* GlobalHeapManager;
	static Threads::CriticalSection CritSec;
};

HeapManager& GetSingleGlobalHeapManager();



