//
// The Epoch Language Project
// CUDA Interoperability Library
//
// Routines for invoking the compiler and managing compiled blocks of code
//

#pragma once


// Dependencies
#include "Language Extensions/HandleTypes.h"
#include "Traverser/TraversalInterface.h"


namespace Compiler
{

	Extensions::CompileSessionHandle StartNewCompilation();
	void CommitCompile(Extensions::CompileSessionHandle sessionid);
	const std::wstring& GetGeneratedPTXFileName(Extensions::CompileSessionHandle sessionid);
	Extensions::CompileSessionHandle GetAssociatedSession(Extensions::CodeBlockHandle codehandle);

	Extensions::CodeBlockHandle GetCompiledBlock(Extensions::CompileSessionHandle sessionid, Extensions::OriginalCodeHandle handle);
	Extensions::OriginalCodeHandle GetOriginalCodeHandle(Extensions::CodeBlockHandle handle);

	const std::list<Traverser::ScopeContents>& GetRegisteredVariables(Extensions::CodeBlockHandle handle);

	void DestroyTempFiles();

}

