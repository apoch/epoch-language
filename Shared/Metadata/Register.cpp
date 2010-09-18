//
// The Epoch Language Project
// Shared Library Code
//
// Wrapper class for a virtual machine register
//

#include "pch.h"

#include "Metadata/Register.h"

#include "Utility/Memory/Stack.h"


Register::Register()
	: Type(VM::EpochType_Error)
{
}


void Register::Set(Integer32 value)
{
	Value_Integer32 = value;
	Type = VM::EpochType_Integer;
}

void Register::Set(StringHandle value)
{
	Value_StringHandle = value;
	Type = VM::EpochType_String;
}


void Register::PushOntoStack(StackSpace& stack) const
{
	switch(Type)
	{
	case VM::EpochType_Integer:
		stack.PushValue(Value_Integer32);
		break;

	case VM::EpochType_String:
		stack.PushValue(Value_StringHandle);
		break;

	default:
		throw std::exception("Not implemented.");
	}
}

