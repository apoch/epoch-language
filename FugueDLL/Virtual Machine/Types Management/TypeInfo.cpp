//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Central interface for retrieving information about types
//

#include "pch.h"
#include "Virtual Machine/Types Management/TypeInfo.h"

using namespace VM;


//
// Given a type ID, determine the amount of stack storage space needed
//
size_t TypeInfo::GetStorageSize(EpochVariableTypeID type)
{
	switch(type)
	{
	case EpochVariableType_Null:		return NullVariable::GetStorageSize();
	case EpochVariableType_Integer:		return IntegerVariable::GetStorageSize();
	case EpochVariableType_Integer16:	return Integer16Variable::GetStorageSize();
	case EpochVariableType_Real:		return RealVariable::GetStorageSize();
	case EpochVariableType_Boolean:		return BooleanVariable::GetStorageSize();
	case EpochVariableType_String:		return StringVariable::GetStorageSize();
	case EpochVariableType_Function:	return FunctionBinding::GetStorageSize();
	case EpochVariableType_Address:		return AddressVariable::GetStorageSize();
	case EpochVariableType_Buffer:		return BufferVariable::GetStorageSize();
	case EpochVariableType_TaskHandle:	return TaskHandleVariable::GetStorageSize();

	case EpochVariableType_Tuple:
	case EpochVariableType_Structure:
		throw InternalFailureException("Cannot determine tuple/structure size with no type hint; this probably means the internal parser is borked.");

	default:
		throw InternalFailureException("TypeInfo::GetStorageSize is missing a variable type; cannot return type storage size!");
	}
}


//
// Given a type ID, determine if the type stores numeric data
//
bool TypeInfo::IsNumeric(VM::EpochVariableTypeID type)
{
	switch(type)
	{
	case EpochVariableType_Integer:
	case EpochVariableType_Integer16:
	case EpochVariableType_Real:
		return true;

	case EpochVariableType_Null:
	case EpochVariableType_Boolean:
	case EpochVariableType_String:
	case EpochVariableType_Function:
	case EpochVariableType_Address:
	case EpochVariableType_Buffer:
	case EpochVariableType_TaskHandle:
	case EpochVariableType_Tuple:
	case EpochVariableType_Structure:
		return false;

	default:
		throw InternalFailureException("TypeInfo::IsNumeric is missing a variable type; cannot determine if type is numeric!");
	}
}

