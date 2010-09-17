//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Helper wrapper for accessing the global heap manager
//

#include "pch.h"

#include "Utility/Memory/MemoryManager.h"


HeapManager& GetSingleGlobalHeapManager()
{
	return HeapManager::GetGlobalHeapManager();
}

