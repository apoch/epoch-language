//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// Exported routines comprising the EpochRuntime API
//

#include "pch.h"

#include "Runtime/Runtime.h"

#include "Utility/Strings.h"


Runtime::ExecutionContext* GlobalContext = NULL;

namespace
{
	unsigned* TestHarness = NULL;
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
		Runtime::VirtualMachine vm;
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

extern "C" const wchar_t* VMGetString(StringHandle handle)
{
	try
	{
		return GlobalContext->OwnerVM.GetPooledString(handle).c_str();
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
	::MessageBox(0, L"Fatal error - program halted", L"Epoch Runtime", MB_ICONSTOP);
	std::terminate();
}

extern "C" void VMBreak()
{
	__asm int 3
}

extern "C" BufferHandle VMAllocBuffer(size_t size)
{
	try
	{
		BufferHandle handle = GlobalContext->OwnerVM.AllocateBuffer(size);
		GlobalContext->TickBufferGarbageCollector();
		return handle;
	}
	catch(...)
	{
		return 0;
	}
}

extern "C" BufferHandle VMCopyBuffer(BufferHandle handle)
{
	try
	{
		BufferHandle clone = GlobalContext->OwnerVM.CloneBuffer(handle);
		GlobalContext->TickBufferGarbageCollector();
		return clone;
	}
	catch(...)
	{
		return 0;
	}
}


void SetGlobalExecutionContext(Runtime::ExecutionContext* context)
{
	GlobalContext = context;
}

