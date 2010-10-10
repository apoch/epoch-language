//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper container for holding tables of string identifiers used in parsing
//

#pragma once


// Dependencies
#include <string>
#include <set>


typedef std::set<std::wstring> StringSet;

struct IdentifierTable
{
	StringSet InfixOperators;
	StringSet UnaryPrefixes;
	StringSet OpAssignmentIdentifiers;
	StringSet CustomEntities;
	StringSet ChainedEntities;
	StringSet PostfixEntities;
	StringSet PostfixClosers;
};