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
class StringPoolManager;


namespace TypeConstructors
{

	void PoolStrings(StringPoolManager& stringpool);

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset);
	void RegisterLibraryFunctions(FunctionCompileHelperTable& table);

	void RegisterLibraryOverloads(OverloadMap& overloadmap);

	void RegisterJITTable(JIT::JITTable& table);

}

