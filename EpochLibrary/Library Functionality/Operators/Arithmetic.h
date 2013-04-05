//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for arithmetic operators
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"


// Forward declarations
class StringPoolManager;


namespace ArithmeticLibrary
{

	void PoolStrings(StringPoolManager& stringpool);

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset);

	void RegisterInfixOperators(StringSet& infixtable, PrecedenceTable& precedences);
	void RegisterUnaryOperators(StringSet& unaryprefixes, StringSet& preoperators, StringSet& postoperators);
	void RegisterOpAssignOperators(StringSet& operators);
	void RegisterLibraryOverloads(OverloadMap& overloadmap);

	void RegisterJITTable(JIT::JITTable& table);

}

