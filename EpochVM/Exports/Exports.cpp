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


namespace
{
	unsigned* TestHarness = NULL;

	VM::ExecutionContext* GlobalContext = NULL;
}


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
		vm.ExecuteByteCode(reinterpret_cast<Bytecode::Instruction*>(bytecodebuffer), size, TestHarness);
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

//
// Link to a test harness for running unit tests
//
extern "C" void STDCALL LinkTestHarness(unsigned* harness)
{
	//try
	//{
		TestHarness = harness;
	//}
	//catch(...)
	//{
	//	::MessageBox(0, L"Failed to initialize test harness for Epoch VM", L"Epoch Internal Error", MB_ICONSTOP);
	//}
}

extern "C" void* VMGetBuffer(BufferHandle handle)
{
	try
	{
		return GlobalContext->OwnerVM.GetBuffer(handle);
	}
	catch(...)
	{
		return NULL;
	}
}

extern "C" void* VMAllocStruct(Metadata::EpochTypeID structtype)
{
	try
	{
		StructureHandle handle = GlobalContext->OwnerVM.AllocateStructure(GlobalContext->OwnerVM.GetStructureDefinition(structtype));
		GlobalContext->TickStructureGarbageCollector();
		return handle;
	}
	catch(...)
	{
		return NULL;
	}
}

extern "C" void* VMCopyStruct(StructureHandle handle)
{
	try
	{
		StructureHandle copyhandle = GlobalContext->OwnerVM.DeepCopy(handle);
		GlobalContext->TickStructureGarbageCollector();
		return copyhandle;
	}
	catch(...)
	{
		return NULL;
	}
}

extern "C" void VMHalt()
{
	::MessageBox(0, L"Fatal error - program halted", L"Epoch Virtual Machine", MB_ICONSTOP);
	std::terminate();
}

extern "C" void VMBreak()
{
	__asm int 3
}


void SetGlobalExecutionContext(VM::ExecutionContext* context)
{
	GlobalContext = context;
}

