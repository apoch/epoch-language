//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for boolean operators
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"


// Forward declarations
class StringPoolManager;


namespace BooleanLibrary
{

	void PoolStrings(StringPoolManager& stringpool);

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset);

	void RegisterLibraryOverloads(OverloadMap& overloadmap);

	void RegisterJITTable(JIT::JITTable& table);

}

