//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for string-handling operators
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"


// Forward declarations
class StringPoolManager;


namespace StringLibrary
{

	void PoolStrings(StringPoolManager& stringpool);

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset);

	void RegisterInfixOperators(StringSet& infixtable, PrecedenceTable& precedences);

	void RegisterJITTable(JIT::JITTable& table);

}

