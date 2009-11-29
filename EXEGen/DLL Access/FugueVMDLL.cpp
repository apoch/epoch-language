//
// The Epoch Language Project
// Win32 EXE Generator
//
// Wrapper logic for accessing the Fugue Virtual Machine DLL
//

#include "pch.h"

#include "DLL Access/FugueVMDLL.h"
#include "DLL Access/Exceptions.h"


//
// Construct the access wrapper and initialize the DLL bindings
//
FugueVMDLLAccess::FugueVMDLLAccess()
{
	// Fugue automatically initializes when it is attached to the process
	HMODULE DLLHandle = ::LoadLibrary(L"fuguedll.dll");
	if(!DLLHandle)
		throw DLLAccessException(L"Failed to load Fugue Virtual Machine DLL; ensure that FugueDLL.DLL is present.");

	// Obtain interface into DLL
	ExecSource = reinterpret_cast<ExecuteSourceCodePtr>(::GetProcAddress(DLLHandle, "ExecuteSourceCode"));
	ExecBinary = reinterpret_cast<ExecuteBinaryFilePtr>(::GetProcAddress(DLLHandle, "ExecuteBinaryFile"));
	SerializeSource = reinterpret_cast<SerializeSourceCodePtr>(::GetProcAddress(DLLHandle, "SerializeSourceCode"));

	// Validate interface to be sure
	if(!ExecSource || !ExecBinary || !SerializeSource)
		throw DLLAccessException(L"One or more Epoch service functions could not be loaded from FugueDLL.DLL; please ensure the latest version of Fugue is present.");
}

//
// Destruct the access wrapper and free the library
//
FugueVMDLLAccess::~FugueVMDLLAccess()
{
	::FreeLibrary(DLLHandle);
}

//
// Invoke the DLL function to execute raw Epoch source code
//
bool FugueVMDLLAccess::ExecuteSourceCode(const char* filename)
{
	return ExecSource(filename);
}

//
// Invoke the DLL function to execute a compiled Epoch binary
//
bool FugueVMDLLAccess::ExecuteBinaryFile(const char* filename)
{
	return ExecBinary(filename);
}

//
// Invoke the DLL function to serialize source code into Epoch Assembly code
//
bool FugueVMDLLAccess::SerializeSourceCode(const char* filename, const char* outputfilename)
{
	return SerializeSource(filename, outputfilename);
}

