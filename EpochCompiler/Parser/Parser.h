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
	Parser(SemanticActionInterface& semantics, const InfixTable& infixtable)
		: SemanticActions(semantics),
		  InfixIdentifiers(infixtable)
	{ }

// Parsing operations
public:
	bool Parse(const std::wstring& code);

// Internal tracking
private:
	SemanticActionInterface& SemanticActions;
	const InfixTable& InfixIdentifiers;
};

