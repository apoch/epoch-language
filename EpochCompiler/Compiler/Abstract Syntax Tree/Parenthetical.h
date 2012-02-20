//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST nodes for parenthetical expressions
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Forwards.h"


namespace AST
{

	//
	// A parenthetical can consist of a pre-operation statement,
	// post-operation statement, or full expression.
	//
	typedef boost::variant
		<
			Undefined,
			DeferredPreOperatorStatement,
			DeferredPostOperatorStatement,
			DeferredExpression
		> Parenthetical;

}

