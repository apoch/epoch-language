//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Implementation of memory management subsystems.
//

#include "pch.h"
#include "Utility/Memory/MemoryManager.h"

HeapManager* HeapManager::GlobalHeapManager = NULL;
Threads::CriticalSection HeapManager::CritSec;

//
// Construct a heap manager. This should take care of any platform
// specific operations to ensure that subsequent allocation requests
// can be serviced.
//
HeapManager::HeapManager()
{
	SYSTEM_INFO sysinfo;
	::GetSystemInfo(&sysinfo);
	PageSize = sysinfo.dwPageSize;

	HeapHandle = ::HeapCreate(0, PageSize, 0);
	if(!HeapHandle)
		throw MemoryException("Failed to allocate system heap!");
}

//
// Clean up a heap; this should take care of any platform specific
// operations for releasing the heap.
//
HeapManager::~HeapManager()
{
	::HeapDestroy(HeapHandle);
}

