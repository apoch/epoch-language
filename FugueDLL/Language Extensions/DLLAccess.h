//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Wrapper logic for accessing language extension libraries
//

#pragma once


// Dependencies
#include "Language Extensions/HandleTypes.h"
#include "Language Extensions/FunctionPointerTypes.h"


namespace Extensions
{

	class ExtensionDLLAccess
	{
	// Construction and destruction
	public:
		ExtensionDLLAccess(const std::wstring& dllname);
		~ExtensionDLLAccess();

	// Extension interface
	public:
		void RegisterExtensionKeywords(ExtensionLibraryHandle token);
		CodeBlockHandle LoadSourceBlock(OriginalCodeHandle handle);
		void ExecuteSourceBlock(CodeBlockHandle handle, HandleType activatedscopehandle);
		void PrepareForExecution();

		CompileSessionHandle GetCompileSession() const
		{ return SessionHandle; }

	// Internal type definitions for function pointers
	private:
		typedef void (__stdcall *RegistrationPtr)(const ExtensionInterface* extensioninterface, ExtensionLibraryHandle token);
		typedef CodeBlockHandle (__stdcall *LoadSourceBlockPtr)(CompileSessionHandle sessionid, OriginalCodeHandle handle);
		typedef void (__stdcall *ExecuteSourceBlockPtr)(CodeBlockHandle handle, HandleType activatedscopehandle);

		typedef CompileSessionHandle (__stdcall *StartCompileSessionPtr)();
		typedef void (__stdcall *PreparePtr)(CompileSessionHandle sessionid);

	// Internal bindings to the DLL
	private:
		HMODULE DLLHandle;

		RegistrationPtr DoRegistration;
		LoadSourceBlockPtr DoLoadSource;
		ExecuteSourceBlockPtr DoExecuteSource;

		StartCompileSessionPtr DoStartSession;
		PreparePtr DoPrepare;

		CompileSessionHandle SessionHandle;
	};

}

