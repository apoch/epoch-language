//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for literal tokens
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"

#include "Compiler/Abstract Syntax Tree/Undefined.h"


namespace AST
{

	//
	// Literal strings are represented by iterator pairs which
	// point into the original code text stream. This prevents
	// a copy of each literal being created every time the AST
	// node is propagated during parsing.
	//
	typedef boost::iterator_range<std::wstring::const_iterator> LiteralStringT;

	//
	// Literal tokens can take one of several base types
	//
	typedef boost::variant<Undefined, Integer32, UInteger32, Real32, LiteralStringT, bool> LiteralToken;

}

