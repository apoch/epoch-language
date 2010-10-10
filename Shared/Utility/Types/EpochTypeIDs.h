//
// The Epoch Language Project
// Shared Library Code
//
// Shared list of variable type identifiers
//

#pragma once


namespace VM
{
	// Built-in variable types
	enum EpochTypeID
	{
		EpochType_Error,
		
		EpochType_Identifier,
		EpochType_Expression,

		EpochType_Void,
		EpochType_Integer,
		EpochType_Boolean,
		EpochType_String,
		EpochType_Real
	};
}

