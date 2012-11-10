//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Wrapper interface for working with the Epoch Virtual Machine
//

#include "pch.h"

#include "DLL Access Wrappers/VM.h"
#include "DLL Access Wrappers/Exceptions.h"

#include "Utility/DLLPool.h"


using namespace DLLAccess;


//
// Load the DLL, if it is not already loaded, and set up bindings
// between this wrapper class and the actual exported functions.
//
VMAccess::VMAccess()
{
	Marshaling::DLLPool::DLLPoolHandle dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochVM.DLL");

	DoExecByteCode = Marshaling::DLLPool::GetFunction<ExecuteByteCodePtr>(dllhandle, "ExecuteByteCode");
	DoEnableVisualDebugger = Marshaling::DLLPool::GetFunction<EnableVisualDebuggerPtr>(dllhandle, "EnableVisualDebugger");
	DoLinkTestHarness = Marshaling::DLLPool::GetFunction<LinkTestHarnessPtr>(dllhandle, "LinkTestHarness");

	if(!DoExecByteCode || !DoEnableVisualDebugger || !DoLinkTestHarness)
		throw DLLException("Failed to load Epoch Virtual Machine");
}

//
// Execute a block of bytecode that has been mapped into memory.
//
void VMAccess::ExecuteByteCode(void* buffer, size_t size)
{
	DoExecByteCode(buffer, size);
}

//
// Enable the visual debug facility of the virtual machine
//
void VMAccess::EnableVisualDebugger()
{
	DoEnableVisualDebugger();
}

void VMAccess::LinkTestHarness(unsigned* harness)
{
	DoLinkTestHarness(harness);
}