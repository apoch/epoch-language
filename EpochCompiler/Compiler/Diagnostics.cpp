#include "pch.h"

#include "Compiler/Diagnostics.h"

#include "Compiler/Abstract Syntax Tree/ASTTraversal.h"
#include "Compiler/Abstract Syntax Tree/ASTDump.h"

#include "Compiler/Abstract Syntax Tree/Identifiers.h"
#include "Compiler/Abstract Syntax Tree/Expression.h"
#include "Compiler/Abstract Syntax Tree/Statement.h"
#include "Compiler/Abstract Syntax Tree/Assignment.h"
#include "Compiler/Abstract Syntax Tree/CodeBlock.h"
#include "Compiler/Abstract Syntax Tree/Entities.h"
#include "Compiler/Abstract Syntax Tree/FunctionParameter.h"


#include <fstream>


void DumpASTForProgram(AST::Program& program)
{
	std::wofstream outfile(L"c:\\epoch\\foo-ast.txt");
	ASTTraverse::DoTraversal(ASTTraverse::DumpToStream(outfile), program);
}
