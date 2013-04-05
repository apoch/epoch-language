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


namespace FunctionTags
{

	void RegisterExternalTag(FunctionSignatureSet& signatureset);

	void RegisterExternalTagHelper(FunctionTagHelperTable& table);

}

