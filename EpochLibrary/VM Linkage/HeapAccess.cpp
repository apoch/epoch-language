//
// The Epoch Language Project
// Epoch Standard Library
//
// Helper wrapper for accessing the VM DLL's global heap manager
//

#include "pch.h"

#include "Utility/Memory/MemoryManager.h"

#include "Utility/DLLPool.h"


namespace
{
	HeapManager* SharedHeapManager = NULL;
}


HeapManager& GetSingleGlobalHeapManager()
{
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

