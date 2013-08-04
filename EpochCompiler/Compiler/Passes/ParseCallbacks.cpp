#include "pch.h"

#include "Compiler/Passes/ParseCallbacks.h"

#include "Compiler/Abstract Syntax Tree/ASTTraversal.h"
#include "Compiler/Abstract Syntax Tree/Structures.h"



void CompilerPasses::ParseWithCallbacks(AST::Program& program, const CompileSession::ParseCallbackTable& callbacks)
{
	ASTTraverse::CompilePassParseWithCallbacks pass(callbacks);
	ASTTraverse::DoTraversal(pass, program);
}


using namespace ASTTraverse;

CompilePassParseWithCallbacks::CompilePassParseWithCallbacks(const CompileSession::ParseCallbackTable& callbacks)
	: Callbacks(callbacks)
{
	Entry.self = this;
}

void CompilePassParseWithCallbacks::EntryHelper::operator () (AST::Structure& structure)
{
	std::wstring identifier(structure.Identifier.begin(), structure.Identifier.end());
	self->Callbacks.ParseStructure(identifier.c_str());
}

