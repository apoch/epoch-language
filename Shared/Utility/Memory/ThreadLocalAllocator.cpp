//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// A lock-free allocator local to the current thread
//
// This allocator takes advantage of platform-level secondary heap creation.
// A heap is created specifically for the use of this thread by the threading
// control code; memory is then dispensed from this heap. Be careful not to
// pass thread-local memory into other areas of the code that might try to
// free the memory or access it post-release.
//

#include "pch.h"

#include "Utility/Memory/ThreadLocalAllocator.h"

#include "Utility/Threading/Threads.h"


//
// Allocate memory specifically for use by this thread
//
void* ThreadLocalAlloc(size_t size)
{
	return ::HeapAlloc(Threads::GetInfoForThisThread().LocalHeapHandle, HEAP_NO_SERIALIZE, size);
}

//
// Free memory from the thread-local heap
//
void ThreadLocalFree(void* ptr)
{
	::HeapFree(Threads::GetInfoForThisThread().LocalHeapHandle, HEAP_NO_SERIALIZE, ptr);
}
