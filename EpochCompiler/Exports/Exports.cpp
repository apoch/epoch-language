//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Exported routines comprising the EpochCompiler API
//

#include "pch.h"

#include "Compiler/Session.h"


//
// Wrapper for converting an in-memory source code buffer to bytecode
//
// This is mainly in place to provide a C-compatible API to outside
// client code, as well as to prevent having to mess with throwing
// exceptions across DLL boundaries.
//
extern "C" void* __stdcall CompileSourceToByteCode(const wchar_t* filename, const void* sourcecodebuffer, size_t sourcesize)
{
	try
	{
		std::wstring source(reinterpret_cast<std::wstring::const_pointer>(sourcecodebuffer), sourcesize);
		std::auto_ptr<CompileSession> session(new CompileSession);
		session->AddCompileBlock(source, filename);
		session->EmitByteCode();
		return session.release();
	}
	catch(...)
	{
		::MessageBox(0, L"Exception occurred during compilation", L"Epoch Exception", MB_ICONSTOP);
		return NULL;
	}
}


//
// Free the bytecode associated with the given handle
//
extern "C" void __stdcall FreeByteCodeBuffer(void* handle)
{
	delete reinterpret_cast<CompileSession*>(handle);
}

//
// Retrieve the memory buffer associated with a given handle
//
extern "C" const void* __stdcall GetByteCodeBuffer(void* handle)
{
	return reinterpret_cast<CompileSession*>(handle)->GetEmittedBuffer();
}

//
// Retrieve the size of the memory buffer associated with a given handle
//
extern "C" size_t __stdcall GetByteCodeBufferSize(void* handle)
{
	return reinterpret_cast<CompileSession*>(handle)->GetEmittedBufferSize();
}
