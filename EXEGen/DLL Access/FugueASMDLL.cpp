//
// The Epoch Language Project
// Win32 EXE Generator
//
// Wrapper logic for accessing the Fugue Assembler DLL
//

#include "pch.h"

#include "DLL Access/FugueASMDLL.h"
#include "DLL Access/Exceptions.h"


//
// Construct the access wrapper and initialize the DLL bindings
//
FugueASMDLLAccess::FugueASMDLLAccess()
{
	// Load the Fugue assembler utility DLL
	HINSTANCE DLLHandle = ::LoadLibrary(L"fugueasm.dll");
	if(!DLLHandle)
		throw DLLAccessException(L"Failed to load Fugue Assembler DLL; ensure that FugueASM.DLL is present.");

	// Obtain interface into DLL
	DoAssemble = reinterpret_cast<DoAssemblePtr>(::GetProcAddress(DLLHandle, "DoAssemble"));
	DoDisassemble = reinterpret_cast<DoDisassemblePtr>(::GetProcAddress(DLLHandle, "DoDisassemble"));

	// Validate interface to be sure
	if(!DoAssemble || !DoDisassemble)
		throw DLLAccessException(L"One or more Epoch service functions could not be loaded from FugueASM.DLL; please ensure the latest version of Fugue is present.");
}

//
// Destruct the access wrapper and free the library
//
FugueASMDLLAccess::~FugueASMDLLAccess()
{
	::FreeLibrary(DLLHandle);
}

//
// Invoke the DLL function to assemble Epoch ASM code into Epoch Binary format
//
bool FugueASMDLLAccess::Assemble(const char* filename, const char* outputfilename)
{
	return DoAssemble(filename, outputfilename);
}

//
// Invoke the DLL function to disassemble Epoch Binary code back to Epoch ASM format
//
bool FugueASMDLLAccess::Disassemble(const char* filename, const char* outputfilename)
{
	return DoDisassemble(filename, outputfilename);
}

