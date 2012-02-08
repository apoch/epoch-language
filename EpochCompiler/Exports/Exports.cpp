//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Exported routines comprising the EpochCompiler API
//

#include "pch.h"

#include "Compiler/Session.h"

#include "Utility/Strings.h"


//
// Wrapper for converting an in-memory source code buffer to bytecode
//
// This is mainly in place to provide a C-compatible API to outside
// client code, as well as to prevent having to mess with throwing
// exceptions across DLL boundaries.
//
extern "C" void* STDCALL CompileSourceToByteCode(const wchar_t* filename, const void* sourcecodebuffer, size_t sourcesize)
{
	try
	{
		std::wstring source(reinterpret_cast<std::wstring::const_pointer>(sourcecodebuffer), sourcesize);
		std::auto_ptr<CompileSession> session(new CompileSession);
		session->AddCompileBlock(source, filename);
		session->EmitByteCode();
		return session.release();
	}
	catch(const std::exception& e)
	{
#ifdef BOOST_WINDOWS
		::MessageBox(0, widen(e.what()).c_str(), L"Epoch Compilation Exception", MB_ICONSTOP);
#else
        std::wcerr<<e.what()<<std::endl;
#endif
        return NULL;
	}
	catch(...)
	{
#ifdef BOOST_WINDOWS
		::MessageBox(0, L"Exception occurred during compilation", L"Epoch Compilation Exception", MB_ICONSTOP);
#else
        std::wcerr<<L"Exception occurred during compilation"<<std::endl;
#endif
        return NULL;
	}
}


//
// Free the bytecode associated with the given handle
//
extern "C" void STDCALL FreeByteCodeBuffer(void* handle)
{
	try
	{
		delete reinterpret_cast<CompileSession*>(handle);
	}
	catch(...)
	{
#ifdef BOOST_WINDOWS
		::MessageBox(0, L"Exception occurred while freeing code buffer", L"Epoch Compilation Exception", MB_ICONSTOP);
#else
        std::wcerr<<L"Exception occurred while freeing code buffer"<<std::endl;
#endif
	}
}

//
// Retrieve the memory buffer associated with a given handle
//
extern "C" const void* STDCALL GetByteCodeBuffer(void* handle)
{
	try
	{
		return reinterpret_cast<CompileSession*>(handle)->GetEmittedBuffer();
	}
	catch(...)
	{
		return NULL;
	}
}

//
// Retrieve the size of the memory buffer associated with a given handle
//
extern "C" size_t STDCALL GetByteCodeBufferSize(void* handle)
{
	try
	{
		return reinterpret_cast<CompileSession*>(handle)->GetEmittedBufferSize();
	}
	catch(...)
	{
		return 0;
	}
}
