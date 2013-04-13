//
// The Epoch Language Project
// EPOCHRUNTIME Runtime Library
//
// Exported routines comprising the EpochRuntime API
//

#include "pch.h"

#include "Runtime/Runtime.h"
#include "Runtime/GlobalContext.h"

#include "Utility/Strings.h"


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
		Runtime::ExecutionContext context(reinterpret_cast<Bytecode::Instruction*>(bytecodebuffer), size, TestHarness);
		context.ExecuteByteCode();
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
	//	::MessageBox(0, L"Failed to initialize test harness for Epoch Runtime", L"Epoch Internal Error", MB_ICONSTOP);
	//}
}

extern "C" void* Epoch_GetBuffer(BufferHandle handle)
{
	try
	{
		return Runtime::GetThreadContext()->GetBuffer(handle);
	}
	catch(...)
	{
		return NULL;
	}
}

extern "C" const wchar_t* Epoch_GetString(StringHandle handle)
{
	try
	{
		return Runtime::GetThreadContext()->GetPooledString(handle).c_str();
	}
	catch(...)
	{
		return NULL;
	}
}

extern "C" void* Epoch_AllocStruct(Metadata::EpochTypeID structtype)
{
	try
	{
		StructureHandle handle = Runtime::GetThreadContext()->AllocateStructure(Runtime::GetThreadContext()->GetStructureDefinition(structtype));
		Runtime::GetThreadContext()->TickStructureGarbageCollector();
		return handle;
	}
	catch(...)
	{
		return NULL;
	}
}

extern "C" void* Epoch_CopyStruct(StructureHandle handle)
{
	try
	{
		StructureHandle copyhandle = Runtime::GetThreadContext()->DeepCopy(handle);
		Runtime::GetThreadContext()->TickStructureGarbageCollector();
		return copyhandle;
	}
	catch(...)
	{
		return NULL;
	}
}

extern "C" void Epoch_Halt()
{
	::MessageBox(0, L"Fatal error - program halted", L"Epoch Runtime", MB_ICONSTOP);
	std::terminate();
}

extern "C" void Epoch_Break()
{
	__asm int 3
}

extern "C" BufferHandle Epoch_AllocBuffer(size_t size)
{
	try
	{
		BufferHandle handle = Runtime::GetThreadContext()->AllocateBuffer(size);
		Runtime::GetThreadContext()->TickBufferGarbageCollector();
		return handle;
	}
	catch(...)
	{
		return 0;
	}
}

extern "C" BufferHandle Epoch_CopyBuffer(BufferHandle handle)
{
	try
	{
		BufferHandle clone = Runtime::GetThreadContext()->CloneBuffer(handle);
		Runtime::GetThreadContext()->TickBufferGarbageCollector();
		return clone;
	}
	catch(...)
	{
		return 0;
	}
}
