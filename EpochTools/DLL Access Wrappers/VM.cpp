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
	HINSTANCE dllhandle = Marshaling::TheDLLPool.OpenDLL(L"EpochVM.DLL");

	DoExecByteCode = reinterpret_cast<ExecuteByteCodePtr>(::GetProcAddress(dllhandle, "ExecuteByteCode"));
	DoEnableVisualDebugger = reinterpret_cast<EnableVisualDebuggerPtr>(::GetProcAddress(dllhandle, "EnableVisualDebugger"));

	if(!DoExecByteCode || !DoEnableVisualDebugger)
		throw DLLException("Failed to load Epoch Virtual Machine");
}

//
// Execute a block of bytecode that has been mapped into memory.
//
void VMAccess::ExecuteByteCode(const void* buffer, size_t size)
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

