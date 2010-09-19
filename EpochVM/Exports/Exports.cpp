//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Exported routines comprising the EpochVM API
//

#include "pch.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Utility/Memory/MemoryManager.h"


//
// Wrapper for executing a block of bytecode
//
// This is mainly in place to provide a C-compatible API to outside
// client code, as well as to prevent having to mess with throwing
// exceptions across DLL boundaries.
//
extern "C" void __stdcall ExecuteByteCode(const void* bytecodebuffer, size_t size)
{
	try
	{
		VM::VirtualMachine vm;

		// TODO - we need to request all libraries to pool their strings here so as to avoid ID mismatches later on

		vm.InitStandardLibraries();
		vm.ExecuteByteCode(reinterpret_cast<const Bytecode::Instruction*>(bytecodebuffer), size);
	}
	catch(...)
	{
		::MessageBox(0, L"Exception occurred during execution", L"Epoch Exception", MB_ICONSTOP);
	}
}

//
// Permit external access to our heap manager, for shared memory allocation and garbage collection purposes
//
extern "C" HeapManager* __stdcall GetHeapManager()
{
	return &HeapManager::GetGlobalHeapManager();
}

