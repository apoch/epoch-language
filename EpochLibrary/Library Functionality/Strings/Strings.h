//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for string manipulation and processing
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"


// Forward declarations
class StringPoolManager;

namespace VM
{
	class ExecutionContext;
}


namespace StringFunctionLibrary
{

	void PoolStrings(StringPoolManager& stringpool);

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset);

	void RegisterLibraryOverloads(OverloadMap& overloadmap);

}

