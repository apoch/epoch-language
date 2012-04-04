//
// The Epoch Language Project
// Epoch Standard Library
//
// Function tag handlers for precompiling functions to native code
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"


// Forward declarations
class StringPoolManager;


namespace FunctionTags
{

	void RegisterNativeTag(FunctionSignatureSet& signatureset, StringPoolManager& stringpool);
	void RegisterNativeTag(EpochFunctionPtr marshalfunction, FunctionInvocationTable& table, StringPoolManager& stringpool);

	void RegisterNativeTagHelper(FunctionTagHelperTable& table);

}

