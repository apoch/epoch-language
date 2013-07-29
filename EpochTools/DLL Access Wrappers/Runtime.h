//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper interface for working with the Epoch runtime
//

#pragma once


namespace DLLAccess
{

	class RuntimeAccess
	{
	// Construction and destruction
	public:
		RuntimeAccess();
		~RuntimeAccess();

	// DLL interface
	public:
		void ExecuteByteCode(void* buffer, size_t size);
		void ExecuteByteCodeAndPersist(void* buffer, size_t size);
		void LinkTestHarness(unsigned* harness);
		void FreePersisted();
		void FreeNativeCode();

	// Internal type definitions for function pointers
	private:
		typedef void (STDCALL *ExecuteByteCodePtr)(void*, size_t);
		typedef void* (STDCALL *ExecuteByteCodePersistentPtr)(void*, size_t);
		typedef void (STDCALL *LinkTestHarnessPtr)(unsigned*);
		typedef void (STDCALL *FreePersistedPtr)(void*);
		typedef void (STDCALL *FreeNativeCodePtr)();

	// Internal function pointers bound to the DLL
	private:
		ExecuteByteCodePtr DoExecByteCode;
		ExecuteByteCodePersistentPtr DoExecByteCodePersistent;
		LinkTestHarnessPtr DoLinkTestHarness;
		FreePersistedPtr DoFreePersisted;
		FreeNativeCodePtr DoFreeNativeCode;
	};

}

