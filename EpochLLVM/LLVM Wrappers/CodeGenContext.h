//
// The Epoch Language Project
// Epoch Development Tools - LLVM wrapper library
//
// CODEGENCONTEXT.H
// Declaration for the code emission context wrapper
//


#pragma once


// Forward declarations
namespace llvm
{
	class Module;
}


namespace CodeGen
{

	typedef size_t (__stdcall *ThunkCallbackT)(const wchar_t* thunkname);
	typedef size_t (__stdcall *StringCallbackT)(size_t stringhandle);


	class Context
	{
	public:		// Construction and destruction
		Context();
		~Context();

	public:		// Object code emission interface
		size_t EmitBinaryObject(char* buffer, size_t maxoutput);

	public:		// Callback configuration interface
		void SetThunkCallback(void* funcptr);
		void SetStringCallback(void* funcptr);

	private:	// Internal state
		llvm::Module* LLVMModule;

		ThunkCallbackT ThunkCallback;
		StringCallbackT StringCallback;
	};

}

