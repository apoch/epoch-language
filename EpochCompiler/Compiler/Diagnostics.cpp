#include "pch.h"

#include "Compiler/Diagnostics.h"

#include "Compiler/ASTTraversal.h"
#include "Compiler/ASTDump.h"


#include <fstream>


void DumpASTForProgram(AST::Program& program)
{
	std::wofstream outfile(L"c:\\epoch\\foo.txt");

	ASTTraverse::Traverser traverse(program);
	ASTTraverse::DumpToStream dump(outfile, traverse);
	traverse.PerformActions(dump.Entry, dump.Exit);
}
