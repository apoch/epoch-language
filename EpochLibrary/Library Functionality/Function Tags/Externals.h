//
// The Epoch Language Project
// Epoch Standard Library
//
// Function tag handlers for external (DLL-based) functions
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


namespace FunctionTags
{

	void RegisterExternalTag(FunctionSignatureSet& signatureset, StringPoolManager& stringpool);
	void RegisterExternalTag(EpochFunctionPtr marshalfunction, FunctionInvocationTable& table, StringPoolManager& stringpool);

	void RegisterExternalTagHelper(FunctionTagHelperTable& table);

}

