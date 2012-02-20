//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for identifiers which might be missing/omitted
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Identifiers.h"


namespace AST
{

	//
	// An optional identifier is either an identifier or undefined
	//
	typedef boost::variant
		<
			Undefined,
			IdentifierT
		> OptionalIdentifier;

}
