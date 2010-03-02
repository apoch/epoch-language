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


// Forward declarations
namespace VM { class Program; }


namespace Extensions
{

	class ExtensionDLLAccess
	{
	// Construction and destruction
	public:
		ExtensionDLLAccess(const std::wstring& dllname, VM::Program& program);
		~ExtensionDLLAccess();

	// Extension interface
	public:
		void RegisterExtensionKeywords(ExtensionLibraryHandle token);
		CodeBlockHandle LoadSourceBlock(OriginalCodeHandle handle);
		void ExecuteSourceBlock(CodeBlockHandle handle, HandleType activatedscopehandle);
		void PrepareForExecution();

		CompileSessionHandle GetCompileSession() const
		{ return SessionHandle; }

		const std::wstring& GetDLLFileName() const
		{ return DLLName; }

	// Internal type definitions for function pointers
	private:
		typedef void (__stdcall *RegistrationPtr)(const ExtensionInterface* extensioninterface, ExtensionLibraryHandle token);
		typedef CodeBlockHandle (__stdcall *LoadSourceBlockPtr)(CompileSessionHandle sessionid, OriginalCodeHandle handle);
		typedef void (__stdcall *ExecuteSourceBlockPtr)(CodeBlockHandle handle, HandleType activatedscopehandle);

		typedef CompileSessionHandle (__stdcall *StartCompileSessionPtr)(HandleType programhandle);
		typedef void (__stdcall *PreparePtr)(CompileSessionHandle sessionid);

	// Internal bindings to the DLL
	private:
		std::wstring DLLName;

		HMODULE DLLHandle;

		RegistrationPtr DoRegistration;
		LoadSourceBlockPtr DoLoadSource;
		ExecuteSourceBlockPtr DoExecuteSource;

		StartCompileSessionPtr DoStartSession;
		PreparePtr DoPrepare;

		CompileSessionHandle SessionHandle;
	};

}

