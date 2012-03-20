//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper class for source code parsing functionality
//

#pragma once


// Dependencies
#include "Libraries/Library.h"
#include "Compiler/Abstract Syntax Tree/Program.h"


class Parser
{
// Construction
public:
	explicit Parser(const IdentifierTable& identifiers)
		: Identifiers(identifiers)
	{ }

// Non-copyable
private:
	Parser(const Parser& rhs);
	Parser& operator = (const Parser& rhs);

// Parsing operations
public:
	bool Parse(const std::wstring& code, const std::wstring& filename, AST::Program*& out) const;

// Internal tracking
private:
	const IdentifierTable& Identifiers;
};

