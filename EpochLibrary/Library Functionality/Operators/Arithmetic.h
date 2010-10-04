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
	void RegisterLibraryFunctions(FunctionCompileHelperTable& table);

	void RegisterInfixOperators(InfixTable& infixtable, PrecedenceTable& precedences, StringPoolManager& stringpool);


	void AddIntegers(StringHandle functionname, VM::ExecutionContext& context);
	void SubtractIntegers(StringHandle functionname, VM::ExecutionContext& context);
	void MultiplyIntegers(StringHandle functionname, VM::ExecutionContext& context);
	void DivideIntegers(StringHandle functionname, VM::ExecutionContext& context);

}

