//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper class for source code parsing functionality
//

#pragma once


// Dependencies
#include "Libraries/Library.h"
#include "Compiler/AbstractSyntaxTree.h"


class Parser
{
// Construction
public:
	explicit Parser(const IdentifierTable& identifiers)
		: Identifiers(identifiers)
	{ }

// Parsing operations
public:
	bool Parse(const std::wstring& code, const std::wstring& filename, AST::Program& out) const;

// Internal tracking
private:
	const IdentifierTable& Identifiers;
};

