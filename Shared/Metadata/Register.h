//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for a virtual machine register
//

#pragma once


// Dependencies
#include "Utility/Types/EpochTypeIDs.h"
#include "Utility/Types/IntegerTypes.h"
#include "Utility/Types/RealTypes.h"
#include "Utility/Types/IDTypes.h"


// Forward declarations
class StackSpace;


class Register
{
// Construction
public:
	Register();

// Value assignment
public:
	void Set(Integer16 value);
	void Set(Integer32 value);
	void Set(bool value);
	void Set(Real32 value);

	void SetString(StringHandle value);
	void SetBuffer(BufferHandle value);

	void SetStructure(StructureHandle value, VM::EpochTypeID typetag);

	void SetFunction(StringHandle value);

// Stack interaction
public:
	void PushOntoStack(StackSpace& stack) const;

// Value tracking
public:
	union
	{
		Integer16 Value_Integer16;
		Integer32 Value_Integer32;
		StringHandle Value_StringHandle;
		BufferHandle Value_BufferHandle;
		StructureHandle Value_StructureHandle;
		bool Value_Boolean;
		Real32 Value_Real;
	};

	VM::EpochTypeID Type;
	bool SumType;
};




