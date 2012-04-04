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
		void ExecuteByteCode(void* buffer, size_t size);
		void EnableVisualDebugger();

	// Internal type definitions for function pointers
	private:
		typedef void (STDCALL *ExecuteByteCodePtr)(void*, size_t);
		typedef void (STDCALL *EnableVisualDebuggerPtr)();

	// Internal function pointers bound to the DLL
	private:
		ExecuteByteCodePtr DoExecByteCode;
		EnableVisualDebuggerPtr DoEnableVisualDebugger;
	};

}

