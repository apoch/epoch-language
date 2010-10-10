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
	void Set(Integer32 value);
	void Set(StringHandle value);
	void Set(bool value);
	void Set(Real32 value);

// Stack interaction
public:
	void PushOntoStack(StackSpace& stack) const;

// Internal tracking
private:
	union
	{
		Integer32 Value_Integer32;
		StringHandle Value_StringHandle;
		bool Value_Boolean;
		Real32 Value_Real;
	};

	VM::EpochTypeID Type;
};




