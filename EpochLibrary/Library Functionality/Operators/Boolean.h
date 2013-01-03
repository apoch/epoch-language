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

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool);
	void RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool);

	void RegisterLibraryOverloads(OverloadMap& overloadmap, StringPoolManager& stringpool);

	void RegisterJITTable(JIT::JITTable& table, StringPoolManager& stringpool);

}

