//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for converting data between types
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


namespace TypeCasts
{

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool);
	void RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool);
	void RegisterLibraryFunctions(FunctionCompileHelperTable& table);

	void Cast(StringHandle functionname, VM::ExecutionContext& context);

}
