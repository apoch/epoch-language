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

	class Context
	{
	public:		// Construction and destruction
		Context();
		~Context();

	public:		// Object code emission interface
		size_t EmitBinaryObject(char* buffer, size_t maxoutput);

	private:	// Internal state
		llvm::Module* LLVMModule;
	};

}

