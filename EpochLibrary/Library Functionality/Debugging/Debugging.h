//
// The Epoch Language Project
// Epoch Standard Library
//
// Library routines for debugging purposes
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"


// Forward declarations
class StringPoolManager;

namespace Runtime
{
	class ExecutionContext;
}


namespace DebugLibrary
{

	void PoolStrings(StringPoolManager& stringpool);

	void LinkToTestHarness(unsigned* harness);

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset);

	void RegisterJITTable(JIT::JITTable& table);

}

