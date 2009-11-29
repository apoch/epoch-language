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

	void StartNewCompilation();
	void CommitCompile();
	const std::wstring& GetGeneratedPTXFileName();

	Extensions::CodeBlockHandle GetCompiledBlock(Extensions::OriginalCodeHandle handle);
	Extensions::OriginalCodeHandle GetOriginalCodeHandle(Extensions::CodeBlockHandle handle);

	const std::list<Traverser::ScopeContents>& GetRegisteredVariables(Extensions::CodeBlockHandle handle);

	void DestroyTempFiles();

}

