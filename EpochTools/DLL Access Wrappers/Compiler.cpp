//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper interface for working with the Epoch compiler
//

#include "pch.h"

#include "DLL Access Wrappers/Compiler.h"
#include "DLL Access Wrappers/Exceptions.h"

#include "Utility/DLLPool.h"


using namespace DLLAccess;


//
// Load the DLL, if it is not already loaded, and set up bindings
// between this wrapper class and the actual exported functions.
//
CompilerAccess::CompilerAccess()
{
	HINSTANCE dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochCompiler.DLL");

	DoCompileSource = reinterpret_cast<CompileSourceToByteCodePtr>(::GetProcAddress(dllhandle, "CompileSourceToByteCode"));
	DoGetByteCodeBuffer = reinterpret_cast<GetByteCodeBufferPtr>(::GetProcAddress(dllhandle, "GetByteCodeBuffer"));
	DoGetByteCodeBufferSize = reinterpret_cast<GetByteCodeBufferSizePtr>(::GetProcAddress(dllhandle, "GetByteCodeBufferSize"));
	DoFreeByteCodeBuffer = reinterpret_cast<FreeByteCodeBufferPtr>(::GetProcAddress(dllhandle, "FreeByteCodeBuffer"));

	if(!DoCompileSource || !DoGetByteCodeBuffer || !DoGetByteCodeBufferSize || !DoFreeByteCodeBuffer)
		throw DLLException("Failed to load Epoch compiler");
}


//
// Free all loaded code buffers
//
CompilerAccess::~CompilerAccess()
{
	for(std::set<CompiledByteCodeHandle>::iterator iter = ByteCodeBuffers.begin(); iter != ByteCodeBuffers.end(); ++iter)
		DoFreeByteCodeBuffer(*iter);
}


//
// Compile a block of raw source code into bytecode
//
CompiledByteCodeHandle CompilerAccess::CompileSourceToByteCode(const std::wstring& filename, const std::wstring& source)
{
	CompiledByteCodeHandle handle = DoCompileSource(filename.c_str(), source.c_str(), source.length());
	if(handle)
		ByteCodeBuffers.insert(handle);
	return handle;
}


//
// Given a bytecode buffer handle, return a pointer to the corresponding memory buffer
//
const void* CompilerAccess::GetByteCode(CompiledByteCodeHandle handle) const
{
	if(!handle)
		return NULL;

	return DoGetByteCodeBuffer(handle);
}

//
// Given a bytecode buffer handle, return the size of the corresponding memory buffer
//
size_t CompilerAccess::GetByteCodeSize(CompiledByteCodeHandle handle) const
{
	if(!handle)
		return 0;

	return DoGetByteCodeBufferSize(handle);
}


