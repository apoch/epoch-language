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
	void RegisterLibraryFunctions(FunctionCompileHelperTable& table);

	void RegisterInfixOperators(InfixTable& infixtable, PrecedenceTable& precedences, StringPoolManager& stringpool);


	void IntegerEquality(StringHandle functionname, VM::ExecutionContext& context);
	void IntegerGreaterThan(StringHandle functionname, VM::ExecutionContext& context);

}

