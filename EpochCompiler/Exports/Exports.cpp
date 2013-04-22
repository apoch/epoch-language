//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Exported routines comprising the EpochCompiler API
//

#include "pch.h"

#include "Compiler/Session.h"
#include "Compiler/Self Hosting Plugins/Plugin.h"

#include "User Interface/Output.h"

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
	UI::OutputStream output;

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
        output << UI::lightred << e.what() << UI::white << std::endl;
        return NULL;
	}
	catch(...)
	{
        output << UI::lightred << L"Exception occurred during compilation" << UI::white << std::endl;
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
        std::wcerr << L"Exception occurred while freeing code buffer" << std::endl;
	}
}

//
// Retrieve the memory buffer associated with a given handle
//
extern "C" void* STDCALL GetByteCodeBuffer(void* handle)
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



extern "C" void STDCALL ParseSource(const char* source, void* callbacktable)
{
	try
	{
		std::string source(reinterpret_cast<std::string::const_pointer>(source));
		std::wstring wsource = widen(source);

		CompileSession session;
		session.Parse(wsource, L"?", *reinterpret_cast<const CompileSession::ParseCallbackTable*>(callbacktable));
	}
	catch(...)
	{
	}
}

extern "C" void STDCALL RegisterPluginFunction(const wchar_t* functionname, void* codeptr)
{
	Plugins.RegisterPluginFunction(functionname, codeptr);
}

extern "C" void STDCALL ReleasePluginFunctions()
{
	Plugins.Clear();
}

