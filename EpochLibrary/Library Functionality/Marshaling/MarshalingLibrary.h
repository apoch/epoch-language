//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for data/code marshaling
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"


// Forward declarations
class StringPoolManager;


namespace MarshalingLibrary
{

	void PoolStrings(StringPoolManager& stringpool);

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset);

	void RegisterLibraryOverloads(OverloadMap& overloadmap);

	void RegisterJITTable(JIT::JITTable& table);

}

