//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper interface for working with the Epoch Virtual Machine
//

#pragma once


namespace DLLAccess
{

	class VMAccess
	{
	// Construction
	public:
		VMAccess();

	// DLL interface
	public:
		void ExecuteByteCode(const void* buffer, size_t size);

	// Internal type definitions for function pointers
	private:
		typedef void (__stdcall *ExecuteByteCodePtr)(const void*, size_t);

	// Internal function pointers bound to the DLL
	private:
		ExecuteByteCodePtr DoExecByteCode;
	};

}

