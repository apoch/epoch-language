//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for describing an inline code entity
//

#pragma once


// Dependencies
#include "Bytecode/EntityTags.h"

#include "Metadata/CompileTimeParams.h"


struct EntityDescription
{
	StringHandle StringName;
	CompileTimeParameterVector Parameters;
};

