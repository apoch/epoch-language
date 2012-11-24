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

namespace VM
{
	class ExecutionContext;
}


namespace DebugLibrary
{

	void LinkToTestHarness(unsigned* harness);

	void RegisterLibraryFunctions(FunctionSignatureSet& signatureset, StringPoolManager& stringpool);
	void RegisterLibraryFunctions(FunctionInvocationTable& table, StringPoolManager& stringpool);

	void RegisterJITTable(JIT::JITTable& table, StringPoolManager& stringpool);

}

