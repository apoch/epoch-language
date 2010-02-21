//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Common routines used in the virtual machine implementation
//

#pragma once

// Dependencies
#include "Utility/Memory/Stack.h"
#include "Virtual Machine/Types Management/TypeInfo.h"


template<class TypeData>
void PushValueOntoStack(StackSpace& thestack, VM::RValuePtr value)
{
	thestack.Push(TypeData::VariableType::GetBaseStorageSize());
	*reinterpret_cast<TypeData::VariableType::BaseStorage*>(thestack.GetCurrentTopOfStack()) = value->CastTo<TypeData::RValueType>().GetValue();
}

template<class TypeData>
void PushValueOntoStack(StackSpace& thestack, typename TypeData::VariableType::BaseStorage value)
{
	thestack.Push(TypeData::VariableType::GetBaseStorageSize());
	*reinterpret_cast<TypeData::VariableType::BaseStorage*>(thestack.GetCurrentTopOfStack()) = value;
}


namespace VM
{
	RValuePtr GetRValuePtrFromStorage(VM::EpochVariableTypeID vartype, void* storage);
	void WriteRValueToStorage(RValuePtr rvalue, void* storage);
}
