//
// The Epoch Language Project
// Win32 EXE Generator
//
// Wrapper logic for accessing the Fugue Assembler DLL
//

#pragma once


class FugueASMDLLAccess
{
// Construction and destruction
public:
	FugueASMDLLAccess();
	~FugueASMDLLAccess();

// Assembler DLL interface
public:
	bool Assemble(const char* filename, const char* outputfilename);
	bool Disassemble(const char* filename, const char* outputfilename);

// Internal type definitions for function pointers
private:
	typedef bool (__stdcall *DoAssemblePtr)(const char*, const char*);
	typedef bool (__stdcall *DoDisassemblePtr)(const char*, const char*);

// Internal bindings to the DLL
private:
	HMODULE DLLHandle;

	DoAssemblePtr DoAssemble;
	DoDisassemblePtr DoDisassemble;
};
