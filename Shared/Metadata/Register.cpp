//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for a virtual machine register
//

#include "pch.h"

#include "Metadata/Register.h"

#include "Utility/Memory/Stack.h"


//
// Construct and initialize the register wrapper
//
Register::Register()
	: Type(VM::EpochType_Error)
{
}


//
// Set the value of the register to a 32-bit integer
//
void Register::Set(Integer32 value)
{
	Value_Integer32 = value;
	Type = VM::EpochType_Integer;
}

//
// Set the value of the register to a string handle
//
void Register::Set(StringHandle value)
{
	Value_StringHandle = value;
	Type = VM::EpochType_String;
}

//
// Set the value of the register to a boolean
//
void Register::Set(bool value)
{
	Value_Boolean = value;
	Type = VM::EpochType_Boolean;
}

//
// Set the value of the register to a real
//
void Register::Set(Real32 value)
{
	Value_Real = value;
	Type = VM::EpochType_Real;
}


//
// Push the contents of the register onto the given stack
//
void Register::PushOntoStack(StackSpace& stack) const
{
	switch(Type)
	{
	case VM::EpochType_Error:
		throw FatalException("Register is empty; cannot push its value onto the stack");

	case VM::EpochType_Integer:
		stack.PushValue(Value_Integer32);
		break;

	case VM::EpochType_String:
		stack.PushValue(Value_StringHandle);
		break;

	case VM::EpochType_Boolean:
		stack.PushValue(Value_Boolean);
		break;

	case VM::EpochType_Real:
		stack.PushValue(Value_Real);
		break;

	default:
		throw NotImplementedException("Unsupported data type in Register::PushOntoStack");
	}
}

