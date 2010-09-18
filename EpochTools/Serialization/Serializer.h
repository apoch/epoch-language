//
// The Epoch Language Project
// EPOCHTOOLS Command Line Toolkit
//
// Declarations for bytecode serialization subsystem
//

#pragma once


// Dependencies
#include "DLL Access Wrappers/Compiler.h"


namespace Serialization
{

	class Serializer
	{
	// Construction
	public:
		Serializer(const DLLAccess::CompilerAccess& compileraccess, DLLAccess::CompiledByteCodeHandle bytecodehandle);

	// Writer interface
	public:
		void Write(const std::wstring& filename) const;

	// Internal tracking
	private:
		const DLLAccess::CompilerAccess& CompilerAccess;
		DLLAccess::CompiledByteCodeHandle ByteCodeHandle;
	};

}

