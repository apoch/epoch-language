//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Helper routines for common operations
//

#include "pch.h"
#include "Virtual Machine/Routines.inl"


using namespace VM;


//
// Get a generic RValue object given some memory storage and the assumed data type
//
RValuePtr VM::GetRValuePtrFromStorage(VM::EpochVariableTypeID vartype, void* storage)
{
	switch(vartype)
	{
	case EpochVariableType_Integer:		return IntegerVariable(storage).GetAsRValue();
	case EpochVariableType_Integer16:	return Integer16Variable(storage).GetAsRValue();
	case EpochVariableType_Real:		return RealVariable(storage).GetAsRValue();
	case EpochVariableType_Boolean:		return BooleanVariable(storage).GetAsRValue();
	case EpochVariableType_String:		return StringVariable(storage).GetAsRValue();
	case EpochVariableType_Function:	return FunctionBinding(storage).GetAsRValue();
	case EpochVariableType_Address:		return AddressVariable(storage).GetAsRValue();
	case EpochVariableType_Buffer:		return BufferVariable(storage).GetAsRValue();
	case EpochVariableType_TaskHandle:	return TaskHandleVariable(storage).GetAsRValue();
	}

	throw NotImplementedException("Cannot directly convert from untyped storage to this type; implementation is probably just missing");
}


//
// Write a generic RValue into the given memory storage slot
//
void VM::WriteRValueToStorage(RValuePtr rvalue, void* storage)
{
	switch(rvalue->GetType())
	{
	case EpochVariableType_Integer:			*reinterpret_cast<IntegerVariable::BaseStorage*>(storage) = rvalue->CastTo<IntegerRValue>().GetValue();			break;
	case EpochVariableType_Integer16:		*reinterpret_cast<Integer16Variable::BaseStorage*>(storage) = rvalue->CastTo<Integer16RValue>().GetValue();		break;
	case EpochVariableType_Real:			*reinterpret_cast<RealVariable::BaseStorage*>(storage) = rvalue->CastTo<RealRValue>().GetValue();				break;
	case EpochVariableType_Boolean:			*reinterpret_cast<BooleanVariable::BaseStorage*>(storage) = rvalue->CastTo<BooleanRValue>().GetValue();			break;
	case EpochVariableType_Function:		*reinterpret_cast<FunctionBinding::BaseStorage*>(storage) = rvalue->CastTo<FunctionRValue>().GetValue();		break;
	case EpochVariableType_Address:			*reinterpret_cast<AddressVariable::BaseStorage*>(storage) = rvalue->CastTo<AddressRValue>().GetValue();			break;
	case EpochVariableType_TaskHandle:		*reinterpret_cast<TaskHandleVariable::BaseStorage*>(storage) = rvalue->CastTo<TaskHandleRValue>().GetValue();	break;
	default:
		throw NotImplementedException("Cannot directly write to untyped storage given this type; implementation is probably just missing");
	}
}

