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

		void* GetByteCode(CompiledByteCodeHandle handle) const;
		size_t GetByteCodeSize(CompiledByteCodeHandle handle) const;

		void FreePlugins();

	// Internal type definitions for function pointers
	private:
		typedef CompiledByteCodeHandle (STDCALL *CompileSourceToByteCodePtr)(const wchar_t*, const void*, size_t);
		typedef void* (STDCALL *GetByteCodeBufferPtr)(CompiledByteCodeHandle);
		typedef size_t (STDCALL *GetByteCodeBufferSizePtr)(CompiledByteCodeHandle);
		typedef void (STDCALL *FreeByteCodeBufferPtr)(CompiledByteCodeHandle);
		typedef void (STDCALL *ReleasePluginsPtr)();

	// Internal function pointers bound to the DLL
	private:
		CompileSourceToByteCodePtr DoCompileSource;
		GetByteCodeBufferPtr DoGetByteCodeBuffer;
		GetByteCodeBufferSizePtr DoGetByteCodeBufferSize;
		FreeByteCodeBufferPtr DoFreeByteCodeBuffer;
		ReleasePluginsPtr DoReleasePlugins;

	// Internal state tracking
	private:
		std::set<CompiledByteCodeHandle> ByteCodeBuffers;
	};

}

