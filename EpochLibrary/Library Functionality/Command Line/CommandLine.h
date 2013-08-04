//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for command line processing
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"


// Forward declarations
class StringPoolManager;


namespace CommandLineLibrary
{

	void PoolStrings(StringPoolManager& stringpool);

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset);

	void RegisterJITTable(JIT::JITTable& table);

}

