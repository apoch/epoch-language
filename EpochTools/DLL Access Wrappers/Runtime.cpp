//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper interface for working with the Epoch runtime
//

#include "pch.h"

#include "DLL Access Wrappers/Runtime.h"
#include "DLL Access Wrappers/Exceptions.h"

#include "Utility/DLLPool.h"


using namespace DLLAccess;

static void* Persisted = NULL;


//
// Load the DLL, if it is not already loaded, and set up bindings
// between this wrapper class and the actual exported functions.
//
RuntimeAccess::RuntimeAccess()
{
	Marshaling::DLLPool::DLLPoolHandle dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochRuntime.DLL");

	DoExecByteCode = Marshaling::DLLPool::GetFunction<ExecuteByteCodePtr>(dllhandle, "ExecuteByteCode");
	DoExecByteCodePersistent = Marshaling::DLLPool::GetFunction<ExecuteByteCodePersistentPtr>(dllhandle, "ExecuteByteCodePersistent");
	DoLinkTestHarness = Marshaling::DLLPool::GetFunction<LinkTestHarnessPtr>(dllhandle, "LinkTestHarness");
	DoFreePersisted = Marshaling::DLLPool::GetFunction<FreePersistedPtr>(dllhandle, "FreePersistedByteCode");
	DoFreeNativeCode = Marshaling::DLLPool::GetFunction<FreeNativeCodePtr>(dllhandle, "FreeNativeCode");

	if(!DoExecByteCode || !DoExecByteCodePersistent || !DoLinkTestHarness || !DoFreePersisted || !DoFreeNativeCode)
		throw DLLException("Failed to load Epoch Runtime");
}

RuntimeAccess::~RuntimeAccess()
{
	Marshaling::TheDLLPool.CloseDLL(L"EpochRuntime.DLL");
}

//
// Execute a block of bytecode that has been mapped into memory.
//
void RuntimeAccess::ExecuteByteCode(void* buffer, size_t size)
{
	DoExecByteCode(buffer, size);
}

void RuntimeAccess::LinkTestHarness(unsigned* harness)
{
	DoLinkTestHarness(harness);
}

void RuntimeAccess::ExecuteByteCodeAndPersist(void* buffer, size_t size)
{
	FreePersisted();

	Persisted = DoExecByteCodePersistent(buffer, size);
}

void RuntimeAccess::FreePersisted()
{
	if(Persisted)
	{
		DoFreePersisted(Persisted);
		Persisted = NULL;
	}
}

void RuntimeAccess::FreeNativeCode()
{
	DoFreeNativeCode();
}
