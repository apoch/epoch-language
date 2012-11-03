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
	// with their own unique type tags. Therefore we use a generic uint32.
	typedef UInteger32 EpochTypeID;

	// The upper 8 bits of a type ID are its "family." Families include
	// built-in primitives, structure types, algebraic types, aliases,
	// and so on, as defined below.
	typedef UInteger32 EpochTypeFamily;

	static const EpochTypeFamily EpochTypeFamily_Magic     = (0x00000000);
	static const EpochTypeFamily EpochTypeFamily_Primitive = (0x01000000);
	static const EpochTypeFamily EpochTypeFamily_GC        = (0x02000000);
	static const EpochTypeFamily EpochTypeFamily_Structure = (0x03000000);
	static const EpochTypeFamily EpochTypeFamily_Alias     = (0x04000000);
	static const EpochTypeFamily EpochTypeFamily_Unit      = (0x05000000);
	static const EpochTypeFamily EpochTypeFamily_Tuple     = (0x06000000);
	static const EpochTypeFamily EpochTypeFamily_SumType   = (0x07000000);
	static const EpochTypeFamily EpochTypeFamily_Template  = (0x08000000);


	// Special types
	static const EpochTypeID EpochType_Error	= 0 | EpochTypeFamily_Magic;
	static const EpochTypeID EpochType_Infer	= 1 | EpochTypeFamily_Magic;
	static const EpochTypeID EpochType_Void		= 2 | EpochTypeFamily_Magic;
	static const EpochTypeID EpochType_Function	= 3 | EpochTypeFamily_Magic;
	static const EpochTypeID EpochType_Nothing	= 4 | EpochTypeFamily_Magic;
	static const EpochTypeID EpochType_RefFlag	= 5 | EpochTypeFamily_Magic;

	// Built-in primitives
	static const EpochTypeID EpochType_Identifier = 0 | EpochTypeFamily_Primitive;
	static const EpochTypeID EpochType_Integer    = 1 | EpochTypeFamily_Primitive;
	static const EpochTypeID EpochType_Integer16  = 2 | EpochTypeFamily_Primitive;
	static const EpochTypeID EpochType_Boolean    = 3 | EpochTypeFamily_Primitive;
	static const EpochTypeID EpochType_Real       = 4 | EpochTypeFamily_Primitive;

	// Special garbage-collected types
	static const EpochTypeID EpochType_String     = 0 | EpochTypeFamily_GC;
	static const EpochTypeID EpochType_Buffer     = 1 | EpochTypeFamily_GC;


	inline EpochTypeFamily GetTypeFamily(EpochTypeID type)
	{
		return type & 0xff000000;
	}
}

