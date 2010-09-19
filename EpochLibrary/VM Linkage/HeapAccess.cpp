//
// The Epoch Language Project
// Epoch Standard Library
//
// Helper wrapper for accessing the VM DLL's global heap manager
//

#include "pch.h"

#include "Utility/Memory/MemoryManager.h"

#include "Utility/DLLPool.h"


// Internal tracking
namespace
{
	HeapManager* SharedHeapManager = NULL;
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
	// TODO - thread safety
	if(!SharedHeapManager)
	{
		HINSTANCE dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochVM.DLL");

		typedef HeapManager* (__stdcall *getheapmanagerptr)();
		getheapmanagerptr getheapmanager = reinterpret_cast<getheapmanagerptr>(::GetProcAddress(dllhandle, "GetHeapManager"));

		if(!getheapmanager)
			throw FatalException("Failed to load Epoch virtual machine");

		SharedHeapManager = getheapmanager();
	}

	return *SharedHeapManager;
}

