#include "pch.h"
#include "Virtual Machine/Core Entities/Variables/ArrayVariable.h"
#include "Virtual Machine/Types Management/TypeInfo.h"

using namespace VM;

size_t ArrayVariable::GetNumElements() const
{
	return (Pool.Get(GetValue()).Size / TypeInfo::GetStorageSize(GetElementType()));
}


ArrayVariable::BaseStorage ArrayVariable::AllocateNewHandle(VM::EpochVariableTypeID elementtype, size_t numentries)
{
	return Pool.Add(NULL, TypeInfo::GetStorageSize(elementtype) * numentries, elementtype);
}

