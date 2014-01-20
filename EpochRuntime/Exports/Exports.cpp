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
#include "User Interface/Output.h"

#include "../../EpochTools/Serialization/Serializer.h"


#include <iostream>

#ifdef _MSC_VER
#define NORETURN __declspec(noreturn)
#else
#define NORETURN
#endif

#pragma warning(disable: 4702)			// unreachable code (we don't care)



extern "C" void STDCALL ExecuteByteCode(void* bytecodebuffer, size_t size);


namespace
{
	unsigned* TestHarness = NULL;


	typedef std::vector<char> DeferredExecBuffer;

	DWORD WINAPI ForkBytecodeExecThreadProc(void* param)
	{
		DeferredExecBuffer* buffer = reinterpret_cast<DeferredExecBuffer*>(param);

		ExecuteByteCode(&((*buffer)[0]), buffer->size());

		delete buffer;

		return 0;
	}
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
		UI::OutputStream out;
		out << UI::lightred << widen(e.what()).c_str() << UI::resetcolor << std::endl;
	}
	catch(...)
	{
		UI::OutputStream out;
		out << UI::lightred << L"Unknown exception" << UI::resetcolor << std::endl;
	}
}

extern "C" void STDCALL ExecuteByteCodeDeferred(void* bytecodebuffer, size_t size)
{
	{
		Serialization::Serializer serializer(bytecodebuffer, size);
		serializer.Write(L"D:\\epoch\\selfhost.txt");
	}

	DeferredExecBuffer* buffer = new DeferredExecBuffer(reinterpret_cast<char*>(bytecodebuffer), reinterpret_cast<char*>(bytecodebuffer) + size);

	HANDLE thread = ::CreateThread(NULL, 0, ForkBytecodeExecThreadProc, buffer, 0, NULL);
	::WaitForSingleObject(thread, INFINITE);
}

extern "C" void* STDCALL ExecuteByteCodePersistent(void* bytecodebuffer, size_t size)
{
	Runtime::ExecutionContext* context = new Runtime::ExecutionContext(reinterpret_cast<Bytecode::Instruction*>(bytecodebuffer), size, TestHarness);

	try
	{
		context->ExecuteByteCode();
	}
	catch(const std::exception& e)
	{
		UI::OutputStream out;
		out << UI::lightred << widen(e.what()).c_str() << UI::resetcolor << std::endl;
	}
	catch(...)
	{
		UI::OutputStream out;
		out << UI::lightred << "Unknown exception" << UI::resetcolor << std::endl;
	}

	return context;
}

extern "C" void STDCALL FreePersistedByteCode(Runtime::ExecutionContext* contextptr)
{
	delete contextptr;
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

NORETURN extern "C" void Epoch_Halt()
{
	::MessageBox(0, L"Fatal error - program halted", L"Epoch Runtime", MB_ICONSTOP);
	std::terminate();
}

NORETURN extern "C" void Epoch_HaltExt(unsigned stringhandle)
{
	const wchar_t* title = Runtime::GetThreadContext()->GetPooledString(stringhandle).c_str();
	::MessageBox(0, L"Fatal error - program halted", title, MB_ICONSTOP);
	std::terminate();
}


extern "C" void Epoch_ProfileEnter(unsigned stringhandle)
{
	try
	{
		Runtime::GetThreadContext()->ProfileEnter(stringhandle);
	}
	catch(...)
	{
		Epoch_Halt();
	}
}

extern "C" void Epoch_ProfileExit(unsigned stringhandle)
{
	try
	{
		Runtime::GetThreadContext()->ProfileExit(stringhandle);
	}
	catch(...)
	{
		Epoch_Halt();
	}
}

extern "C" void Epoch_ProfileDump()
{
	try
	{
		Runtime::GetThreadContext()->ProfileDump();
	}
	catch(...)
	{
		Epoch_Halt();
	}
}



extern "C" void* Epoch_GetBuffer(BufferHandle handle)
{
	try
	{
		return Runtime::GetThreadContext()->GetBuffer(handle);
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}

extern "C" const wchar_t* Epoch_GetString(StringHandle handle)
{
	try
	{
		return Runtime::GetThreadContext()->GetPooledString(handle).c_str();
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}

extern "C" StringHandle Epoch_PoolString(const wchar_t* str)
{
	return Runtime::GetThreadContext()->PoolString(str);
}

/*
#pragma optimize( "", off )

static void* AllocStructWorker(unsigned type, void* retaddr)
{
	try
	{
		std::map<void*, std::pair<void*, StringHandle> >::const_iterator iter = Runtime::GetThreadContext()->GeneratedJITFunctionCode.lower_bound(retaddr);
		--iter;
		if(iter->second.second != 0)
		{
			std::wcout << Runtime::GetThreadContext()->GetPooledString(iter->second.second) << std::endl;
		}
	}
	catch(...)
	{
	}

	try
	{
		Runtime::ExecutionContext* context = Runtime::GetThreadContext();
		StructureHandle handle = context->AllocateStructure(context->GetStructureDefinition(type));
		context->TickStructureGarbageCollector();
		return handle;
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}

#pragma optimize( "", on ) 

extern "C" __declspec(naked) void* Epoch_AllocStruct(unsigned)
{
	__asm
	{
		push [esp]
		push [esp+8]
		call AllocStructWorker
		add  esp,8
		ret
	}
}
*/

extern "C" void* Epoch_AllocStruct(Metadata::EpochTypeID structtype)
{
	try
	{
		Runtime::ExecutionContext* context = Runtime::GetThreadContext();
		StructureHandle handle = context->AllocateStructure(context->GetStructureDefinition(structtype));
		context->TickStructureGarbageCollector();
		return handle;
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}


/*
#pragma optimize( "", off )

static void* CopyStructWorker(StructureHandle handle, void* retaddr)
{
	try
	{
		std::map<void*, std::pair<void*, StringHandle> >::const_iterator iter = Runtime::GetThreadContext()->GeneratedJITFunctionCode.lower_bound(retaddr);
		--iter;
		if(iter->second.second != 0)
		{
			UI::OutputStream out;
			out << Runtime::GetThreadContext()->GetPooledString(iter->second.second) << std::endl;
		}
	}
	catch(...)
	{
	}

	try
	{
		StructureHandle copyhandle = Runtime::GetThreadContext()->DeepCopy(handle);
		Runtime::GetThreadContext()->TickStructureGarbageCollector();
		return copyhandle;
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}

#pragma optimize( "", on ) 

extern "C" __declspec(naked) void* Epoch_CopyStruct(StructureHandle handle)
{
	__asm
	{
		push [esp]
		push [esp+8]
		call CopyStructWorker
		add  esp,8
		ret
	}
}
*/

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
		Epoch_Halt();
	}

	return 0;
}


extern "C" void Epoch_Break()
{
	__debugbreak();
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
		Epoch_Halt();
	}

	return 0;
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
		Epoch_Halt();
	}

	return 0;
}

extern "C" BufferHandle Epoch_GetBufferByPtr(const char* bufferptr)
{
	try
	{
		return Runtime::GetThreadContext()->FindBuffer(bufferptr);
	}
	catch(...)
	{
		Epoch_Halt();
	}

	return 0;
}

extern "C" void FreeNativeCode()
{
	JIT::DestructLLVMNativeCode();
}

