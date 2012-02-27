//
// The Epoch Language Project
// EPOCHCOMPILER Compiler Toolchain
//
// AST node representing any valid statement
//

#pragma once


// Dependencies
#include "Compiler/Abstract Syntax Tree/Undefined.h"
#include "Compiler/Abstract Syntax Tree/RefCounting.h"


namespace AST
{

	//
	// Statements can come in one of three varieties:
	//   - Pre-operator statements, such as ++i
	//   - Post-operator statements, such as i++
	//   - Invocations, such as foo(i)
	//
	// This variant allows all three forms to be represented by a
	// single AST node type.
	//
	typedef boost::variant
		<
			Undefined,
			DeferredPreOperatorStatement,
			DeferredPostOperatorStatement,
			DeferredStatement,
			DeferredInitialization
		> AnyStatement;

}
