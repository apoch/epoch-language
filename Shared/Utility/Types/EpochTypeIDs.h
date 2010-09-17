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

		EpochType_Void,
		EpochType_Integer,
		EpochType_String,
	};
}

