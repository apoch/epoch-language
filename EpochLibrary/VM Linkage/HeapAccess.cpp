//
// The Epoch Language Project
// Epoch Standard Library
//
// Helper wrapper for accessing the VM DLL's global heap manager
//

#include "pch.h"

#include "Utility/Memory/MemoryManager.h"

#include "Utility/DLLPool.h"

#include "Utility/Threading/Synchronization.h"


// Internal tracking
namespace
{
	HeapManager* SharedHeapManager = NULL;
	Threads::CriticalSection SharedCriticalSection;
}


//
// Access the heap manager created by the virtual machine
//
// This interface permits us to share heap space with the VM, allowing for
// easy interaction of the library's data management with the core garbage
// collector and so on.
//
HeapManager& GetSingleGlobalHeapManager()
{
	Threads::CriticalSection::Auto lock(SharedCriticalSection);
	
	if(!SharedHeapManager)
	{
		Marshaling::DLLPool::DLLPoolHandle dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochRuntime.DLL");

		typedef HeapManager* (STDCALL *getheapmanagerptr)();
		getheapmanagerptr getheapmanager = Marshaling::DLLPool::GetFunction<getheapmanagerptr>(dllhandle, "GetHeapManager");

		if(!getheapmanager)
			throw FatalException("Failed to load Epoch virtual machine");

		SharedHeapManager = getheapmanager();
	}

	return *SharedHeapManager;
}

