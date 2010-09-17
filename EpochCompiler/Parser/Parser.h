//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// Wrapper class for source code parsing functionality
//

#pragma once


// Forward declarations
class SemanticActionInterface;


class Parser
{
// Construction
public:
	Parser(SemanticActionInterface& semantics)
		: SemanticActions(semantics)
	{ }

// Parsing operations
public:
	bool Parse(const std::wstring& code);

// Internal tracking
private:
	SemanticActionInterface& SemanticActions;
};

