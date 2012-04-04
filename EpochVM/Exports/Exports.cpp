//
// The Epoch Language Project
// EPOCHVM Virtual Machine
//
// Exported routines comprising the EpochVM API
//

#include "pch.h"

#include "Virtual Machine/VirtualMachine.h"

#include "Utility/Memory/MemoryManager.h"

#include "Utility/Strings.h"


//
// Wrapper for executing a block of bytecode
//
// This is mainly in place to provide a C-compatible API to outside
// client code, as well as to prevent having to mess with throwing
// exceptions across DLL boundaries.
//
extern "C" void STDCALL ExecuteByteCode(void* bytecodebuffer, size_t size)
{
	try
	{
		VM::VirtualMachine vm;
		vm.InitStandardLibraries();
		vm.ExecuteByteCode(reinterpret_cast<Bytecode::Instruction*>(bytecodebuffer), size);
	}
	catch(const std::exception& e)
	{
		::MessageBox(0, widen(e.what()).c_str(), L"Epoch Execution Exception", MB_ICONSTOP);
	}
	catch(...)
	{
		::MessageBox(0, L"Exception occurred during execution", L"Epoch Execution Exception", MB_ICONSTOP);
	}
}

//
// Permit external access to our heap manager, for shared memory allocation and garbage collection purposes
//
extern "C" HeapManager* STDCALL GetHeapManager()
{
	try
	{
		return &HeapManager::GetGlobalHeapManager();
	}
	catch(...)
	{
		::MessageBox(0, L"Failed to retrieve global heap manager", L"Epoch Memory Exception", MB_ICONSTOP);
		return NULL;
	}
}

//
// Enable the visual debug interface of the VM
//
extern "C" void STDCALL EnableVisualDebugger()
{
	try
	{
		VM::VirtualMachine::EnableVisualDebugger();
	}
	catch(...)
	{
		::MessageBox(0, L"Failed to load visual debugger for Epoch VM", L"Epoch Internal Error", MB_ICONSTOP);
	}
}
