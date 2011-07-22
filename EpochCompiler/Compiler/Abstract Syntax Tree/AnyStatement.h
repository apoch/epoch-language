#pragma once

#include "Compiler/Abstract Syntax Tree/Undefined.h"
#include "Compiler/Abstract Syntax Tree/RefCounting.h"

namespace AST
{

	typedef boost::variant
		<
			Undefined,
			DeferredPreOperatorStatement,
			DeferredPostOperatorStatement,
			DeferredStatement
		> AnyStatement;

}
