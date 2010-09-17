//
// The Epoch Language Project
// Shared Library Code
//
// Interface for handling compile-time code execution
//

#pragma once


// Dependencies
#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/IDTypes.h"
#include <string>


struct CompileTimeParameter
{
	// Constructor for convenience
	CompileTimeParameter(const std::wstring& name, VM::EpochTypeID type)
		: Name(name),
		  Type(type)
	{ }

	std::wstring Name;
	VM::EpochTypeID Type;

	union PayloadUnion
	{
		Integer32 IntegerValue;
		StringHandle StringHandleValue;
	} Payload;

	std::wstring StringPayload;
};



