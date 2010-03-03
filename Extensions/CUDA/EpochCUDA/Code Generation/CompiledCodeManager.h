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

#include <set>


namespace Compiler
{

	Extensions::CompileSessionHandle StartNewCompilation(HandleType programhandle);
	void CommitCompile(Extensions::CompileSessionHandle sessionid);
	const std::wstring& GetGeneratedPTXFileName(Extensions::CompileSessionHandle sessionid);
	Extensions::CompileSessionHandle GetAssociatedSession(Extensions::CodeBlockHandle codehandle);

	Extensions::CodeBlockHandle GetCompiledBlock(Extensions::CompileSessionHandle sessionid, Extensions::OriginalCodeHandle handle, const std::wstring& keyword);
	Extensions::OriginalCodeHandle GetOriginalCodeHandle(Extensions::CodeBlockHandle handle);
	
	const std::list<Traverser::ScopeContents>& GetRegisteredVariables(Extensions::CodeBlockHandle handle);

	void RecordInvokedFunction(Extensions::CompileSessionHandle session, const std::wstring& functionname);
	void TraverseInvokedFunctions(Extensions::CompileSessionHandle session);

	void DestroyTempFiles();

	const std::wstring& GetCodeControlKeyword(Extensions::CodeBlockHandle handle);

}

