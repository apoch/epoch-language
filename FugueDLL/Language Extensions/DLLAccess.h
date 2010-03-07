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
	// Construction
	public:
		ExtensionDLLAccess(const std::wstring& dllname, VM::Program& program, bool startsession);

	// Extension interface
	public:
		void RegisterExtensionKeywords(ExtensionLibraryHandle token);
		CodeBlockHandle LoadSourceBlock(const std::wstring& keyword, OriginalCodeHandle handle);
		void ExecuteSourceBlock(CodeBlockHandle handle, HandleType activatedscopehandle);
		void ExecuteSourceBlock(CodeBlockHandle handle, HandleType activatedscopehandle, const std::vector<Traverser::Payload>& payloads);
		void PrepareForExecution();
		void PrepareCodeBlock(CodeBlockHandle handle);

		void FillSerializationBuffer(wchar_t*& buffer, size_t& buffersize);
		void FreeSerializationBuffer(wchar_t* buffer);
		void LoadDataBuffer(const std::string& buffer);

		void ClearEverything();

		CompileSessionHandle GetCompileSession() const
		{ return SessionHandle; }

		const std::wstring& GetDLLFileName() const
		{ return DLLName; }

		bool IsAvailableForExecution() const
		{ return ExtensionValid; }

	// Internal type definitions for function pointers
	private:
		typedef bool (__stdcall *InitializePtr)();
		typedef void (__stdcall *RegistrationPtr)(const ExtensionInterface* extensioninterface, ExtensionLibraryHandle token);
		typedef CodeBlockHandle (__stdcall *LoadSourceBlockPtr)(CompileSessionHandle sessionid, OriginalCodeHandle handle, const wchar_t* keyword);
		typedef void (__stdcall *ExecuteSourceBlockPtr)(CodeBlockHandle handle, HandleType activatedscopehandle);
		typedef void (__stdcall *ExecuteControlPtr)(CodeBlockHandle handle, HandleType activatedscopehandle, size_t numparams, const Traverser::Payload* params);
		typedef void (__stdcall *LoadDataBufferPtr)(const char* buffer, size_t buffersize);

		typedef CompileSessionHandle (__stdcall *StartCompileSessionPtr)(HandleType programhandle);
		typedef void (__stdcall *PreparePtr)(CompileSessionHandle sessionid);
		typedef void (__stdcall *PrepareBlockPtr)(CodeBlockHandle codehandle);

		typedef void (__stdcall *FillSerializationBufferPtr)(wchar_t** buffer, size_t* buffersize);
		typedef void (__stdcall *FreeSerializationBufferPtr)(wchar_t* buffer);

		typedef void (__stdcall *ClearEverythingPtr)();

	// Internal bindings to the DLL
	private:
		std::wstring DLLName;

		HMODULE DLLHandle;

		InitializePtr DoInitialize;
		RegistrationPtr DoRegistration;
		LoadSourceBlockPtr DoLoadSource;
		ExecuteSourceBlockPtr DoExecuteSource;
		ExecuteControlPtr DoExecuteControl;

		StartCompileSessionPtr DoStartSession;
		PreparePtr DoPrepare;
		PrepareBlockPtr DoPrepareBlock;

		FillSerializationBufferPtr DoFillSerializationBuffer;
		FreeSerializationBufferPtr DoFreeSerializationBuffer;
		LoadDataBufferPtr DoLoadDataBuffer;

		ClearEverythingPtr DoClearEverything;

		CompileSessionHandle SessionHandle;

		bool ExtensionValid;
	};

}

