//
// The Epoch Language Project
// Epoch Standard Library
//
// Function tag handlers for overloaded custom constructors
//

#pragma once


// Dependencies
#include "Metadata/FunctionSignature.h"
#include "Libraries/Library.h"


// Forward declarations
class StringPoolManager;


namespace FunctionTags
{

	void PoolStrings(StringPoolManager& stringpool);		// TODO - move to a better file

	void RegisterConstructorTagHelper(FunctionTagHelperTable& table);

}

