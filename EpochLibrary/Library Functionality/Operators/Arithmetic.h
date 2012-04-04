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

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool);
	void RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool);

	void RegisterInfixOperators(StringSet& infixtable, PrecedenceTable& precedences, StringPoolManager& stringpool);
	void RegisterUnaryOperators(StringSet& unaryprefixes, StringSet& preoperators, StringSet& postoperators, StringPoolManager& stringpool);
	void RegisterOpAssignOperators(StringSet& operators, StringPoolManager& stringpool);
	void RegisterLibraryOverloads(OverloadMap& overloadmap, StringPoolManager& stringpool);

	void RegisterJITTable(JITTable& table, StringPoolManager& stringpool);

}

