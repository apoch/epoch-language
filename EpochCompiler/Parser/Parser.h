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
	Parser(SemanticActionInterface& semantics, const InfixTable& infixtable, const std::set<std::wstring>& customentities)
		: SemanticActions(semantics),
		  InfixIdentifiers(infixtable),
		  CustomEntities(customentities)
	{ }

// Parsing operations
public:
	bool Parse(const std::wstring& code, const std::wstring& filename);

// Internal tracking
private:
	SemanticActionInterface& SemanticActions;
	const InfixTable& InfixIdentifiers;
	const std::set<std::wstring>& CustomEntities;
};

