//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper interface for working with the Epoch compiler
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"


namespace DLLAccess
{

	// Handy type shortcuts
	typedef HandleType CompiledByteCodeHandle;


	class CompilerAccess
	{
	// Construction and destruction
	public:
		CompilerAccess();
		~CompilerAccess();

	// DLL interface
	public:
		CompiledByteCodeHandle CompileSourceToByteCode(const std::wstring& filename, const std::wstring& source);

		const void* GetByteCode(CompiledByteCodeHandle handle) const;
		size_t GetByteCodeSize(CompiledByteCodeHandle handle) const;

	// Internal type definitions for function pointers
	private:
		typedef CompiledByteCodeHandle (__stdcall *CompileSourceToByteCodePtr)(const wchar_t*, const void*, size_t);
		typedef const void* (__stdcall *GetByteCodeBufferPtr)(CompiledByteCodeHandle);
		typedef size_t (__stdcall *GetByteCodeBufferSizePtr)(CompiledByteCodeHandle);
		typedef void (__stdcall *FreeByteCodeBufferPtr)(CompiledByteCodeHandle);

	// Internal function pointers bound to the DLL
	private:
		CompileSourceToByteCodePtr DoCompileSource;
		GetByteCodeBufferPtr DoGetByteCodeBuffer;
		GetByteCodeBufferSizePtr DoGetByteCodeBufferSize;
		FreeByteCodeBufferPtr DoFreeByteCodeBuffer;

	// Internal state tracking
	private:
		std::set<CompiledByteCodeHandle> ByteCodeBuffers;
	};

}

