//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for constructing variables of built-in primitive types
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"


// Forward declarations
class ScopeDescription;
class StringPoolManager;

namespace VM
{
	class ExecutionContext;
}


namespace TypeConstructors
{

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool);
	void RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool);
	void RegisterLibraryFunctions(FunctionCompileHelperTable& table);

	void ConstructInteger(StringHandle functionname, VM::ExecutionContext& context);
	void ConstructString(StringHandle functionname, VM::ExecutionContext& context);
	
	void CompileConstructorPrimitive(ScopeDescription& scope, const std::vector<CompileTimeParameter>& compiletimeparams);

}

