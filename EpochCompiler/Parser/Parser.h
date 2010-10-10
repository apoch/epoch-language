//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper class for source code parsing functionality
//

#pragma once


// Dependencies
#include "Libraries/Library.h"


// Forward declarations
class SemanticActionInterface;


class Parser
{
// Construction
public:
	Parser(SemanticActionInterface& semantics, const IdentifierTable& identifiers)
		: SemanticActions(semantics),
		  Identifiers(identifiers)
	{ }

// Parsing operations
public:
	bool Parse(const std::wstring& code, const std::wstring& filename);

// Internal tracking
private:
	SemanticActionInterface& SemanticActions;
	const IdentifierTable& Identifiers;
};

