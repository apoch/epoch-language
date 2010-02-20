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
	enum EpochVariableTypeID
	{
		EpochVariableType_Error,
		EpochVariableType_Infer,

		EpochVariableType_Null,

		EpochVariableType_Integer,
		EpochVariableType_Integer16,
		EpochVariableType_Real,
		EpochVariableType_Boolean,
		EpochVariableType_String,

		EpochVariableType_Tuple,
		EpochVariableType_Structure,

		EpochVariableType_Buffer,

		EpochVariableType_Function,

		EpochVariableType_Address,

		EpochVariableType_Array,

		EpochVariableType_TaskHandle,
	};
}