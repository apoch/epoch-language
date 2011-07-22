#pragma once

#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"

#include "Compiler/Abstract Syntax Tree/Undefined.h"


namespace AST
{

	typedef boost::iterator_range<std::wstring::const_iterator> LiteralStringT;

	typedef boost::variant<Undefined, Integer32, UInteger32, Real32, LiteralStringT, bool> LiteralToken;

}

