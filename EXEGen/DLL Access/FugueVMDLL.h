//
// The Epoch Language Project
// Win32 EXE Generator
//
// Wrapper logic for accessing the Fugue Virtual Machine DLL
//

#pragma once


class FugueVMDLLAccess
{
// Construction and destruction
public:
	FugueVMDLLAccess();
	~FugueVMDLLAccess();

// Virtual machine interface
public:
	bool ExecuteSourceCode(const char* filename);
	bool ExecuteBinaryFile(const char* filename);
	bool SerializeSourceCode(const char* filename, const char* outputfilename);

// Internal type definitions for function pointers
private:
	typedef bool (__stdcall *ExecuteSourceCodePtr)(const char*);
	typedef bool (__stdcall *ExecuteBinaryFilePtr)(const char*);
	typedef bool (__stdcall *SerializeSourceCodePtr)(const char*, const char*);

// Internal bindings to the DLL
private:
	HMODULE DLLHandle;

	ExecuteSourceCodePtr ExecSource;
	ExecuteBinaryFilePtr ExecBinary;
	SerializeSourceCodePtr SerializeSource;
};
