//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Node classes for constructing abstract syntax trees for Epoch programs
//

#include "pch.h"
#include "Compiler/AbstractSyntaxTree.h"

#include <iostream>

using namespace AST;


Program::Program(const std::vector<Function>& functions)
{
	std::wcout << L"Parsed program" << std::endl;
}
