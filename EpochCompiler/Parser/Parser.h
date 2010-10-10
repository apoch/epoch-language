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
	Parser(SemanticActionInterface& semantics, const InfixTable& infixtable, const std::set<std::wstring>& unaryprefixes, const std::set<std::wstring>& customentities, const std::set<std::wstring>& chainedentities, const std::set<std::wstring>& postfixentities, const std::set<std::wstring>& postfixclosers)
		: SemanticActions(semantics),
		  InfixIdentifiers(infixtable),
		  UnaryPrefixes(unaryprefixes),
		  CustomEntities(customentities),
		  ChainedEntities(chainedentities),
		  PostfixEntities(postfixentities),
		  PostfixClosers(postfixclosers)
	{ }

// Parsing operations
public:
	bool Parse(const std::wstring& code, const std::wstring& filename);

// Internal tracking
private:
	SemanticActionInterface& SemanticActions;
	const InfixTable& InfixIdentifiers;
	const std::set<std::wstring>& UnaryPrefixes;
	const std::set<std::wstring>& CustomEntities;
	const std::set<std::wstring>& ChainedEntities;
	const std::set<std::wstring>& PostfixEntities;
	const std::set<std::wstring>& PostfixClosers;
};

