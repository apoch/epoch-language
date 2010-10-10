//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for comparison operators
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"


// Forward declarations
class StringPoolManager;


namespace ComparisonLibrary
{

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool);
	void RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool);

	void RegisterInfixOperators(StringSet& infixtable, PrecedenceTable& precedences, StringPoolManager& stringpool);

}

