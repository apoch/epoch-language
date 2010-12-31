//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper for handling AST nodes which might represent Lvalues or Rvalues
//
// We specifically use this mechanism to help resolve member accesses/mutations
// for structures and nested structures. In particular, this hides the complexity
// of resolving nested structure member references behind a uniform interface,
// so that any Lvalue or Rvalue that involves a member access can be handled in
// a single consistent manner.
//

#pragma once


// Dependencies
#include "Utility/Types/IDTypes.h"

#include <vector>


struct LRValue
{
	LRValue()
		: Identifier(0)
	{ }

	explicit LRValue(StringHandle identifier)
		: Identifier(identifier)
	{ }

	StringHandle Identifier;
	std::vector<StringHandle> Members;
};


