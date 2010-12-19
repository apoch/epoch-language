//
// The Epoch Language Project
// Shared Library Code
//
// Shared list of variable type identifiers
//

#pragma once


// Dependencies
#include "Utility/Types/IntegerTypes.h"


namespace VM
{
	// Note that we do not use an enum here, because we need to allow custom
	// type values to be created dynamically at runtime, i.e. for structures
	// with their own unique type tags. Therefore we use a generic int32.
	typedef Integer32 EpochTypeID;

	// Built-in variable types
	static const EpochTypeID EpochType_Error = 0;
		
	static const EpochTypeID EpochType_Identifier = 1;
	static const EpochTypeID EpochType_Expression = 2;
	static const EpochTypeID EpochType_Function = 3;

	static const EpochTypeID EpochType_Void = 4;
	static const EpochTypeID EpochType_Integer = 5;
	static const EpochTypeID EpochType_Boolean = 6;
	static const EpochTypeID EpochType_String = 7;
	static const EpochTypeID EpochType_Real = 8;
	static const EpochTypeID EpochType_Integer16 = 9;

	static const EpochTypeID EpochType_Buffer = 10;

	// Custom types (i.e. structures)
	static const EpochTypeID EpochType_CustomBase = 20;			// Must be the highest type ID
}

