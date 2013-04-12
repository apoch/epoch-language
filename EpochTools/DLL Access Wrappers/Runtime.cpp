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


//
// Load the DLL, if it is not already loaded, and set up bindings
// between this wrapper class and the actual exported functions.
//
RuntimeAccess::RuntimeAccess()
{
	Marshaling::DLLPool::DLLPoolHandle dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochRuntime.DLL");

	DoExecByteCode = Marshaling::DLLPool::GetFunction<ExecuteByteCodePtr>(dllhandle, "ExecuteByteCode");
	DoLinkTestHarness = Marshaling::DLLPool::GetFunction<LinkTestHarnessPtr>(dllhandle, "LinkTestHarness");

	if(!DoExecByteCode || !DoLinkTestHarness)
		throw DLLException("Failed to load Epoch Runtime");
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