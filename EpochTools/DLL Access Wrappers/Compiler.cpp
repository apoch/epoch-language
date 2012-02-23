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
	Marshaling::DLLPool::DLLPoolHandle dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochCompiler.DLL");

	DoCompileSource = Marshaling::DLLPool::GetFunction<CompileSourceToByteCodePtr>(dllhandle, "CompileSourceToByteCode");
	DoGetByteCodeBuffer = Marshaling::DLLPool::GetFunction<GetByteCodeBufferPtr>(dllhandle, "GetByteCodeBuffer");
	DoGetByteCodeBufferSize = Marshaling::DLLPool::GetFunction<GetByteCodeBufferSizePtr>(dllhandle, "GetByteCodeBufferSize");
	DoFreeByteCodeBuffer = Marshaling::DLLPool::GetFunction<FreeByteCodeBufferPtr>(dllhandle, "FreeByteCodeBuffer");

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


