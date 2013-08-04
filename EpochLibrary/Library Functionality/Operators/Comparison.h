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

	void PoolStrings(StringPoolManager& stringpool);

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset);

	void RegisterInfixOperators(StringSet& infixtable, PrecedenceTable& precedences);
	void RegisterLibraryOverloads(OverloadMap& overloadmap);

	void RegisterJITTable(JIT::JITTable& table);

}

