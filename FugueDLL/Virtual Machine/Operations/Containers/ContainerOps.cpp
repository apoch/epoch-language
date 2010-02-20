//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Virtual machine operations for working with built-in containers
//

#include "pch.h"

#include "Virtual Machine/Operations/Containers/ContainerOps.h"
#include "Virtual Machine/Core Entities/Variables/Variable.h"
#include "Virtual Machine/Core Entities/Variables/StringVariable.h"
#include "Virtual Machine/SelfAware.inl"
#include "Virtual Machine/VMExceptions.h"


using namespace VM;
using namespace VM::Operations;


//
// Construct an array of the given element type with the given number of entries
//
ConsArray::ConsArray(unsigned numentries, EpochVariableTypeID elementtype)
	: NumEntries(numentries),
	  ElementType(elementtype)
{
}

//
// Extract an array's contents from the stack and wrap them into an r-value
//
void ConsArray::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}

RValuePtr ConsArray::ExecuteAndStoreRValue(ExecutionContext& context)
{
	std::auto_ptr<ArrayRValue> ret(new ArrayRValue(ElementType));

	size_t offset = 0;

	switch(ElementType)
	{
	case EpochVariableType_Integer:
		for(unsigned i = 0; i < NumEntries; ++i)
		{
			IntegerVariable var(context.Stack.GetOffsetIntoStack(offset));
			ret->AddElement(new IntegerRValue(var.GetValue()));
			offset += IntegerVariable::GetStorageSize();
		}
		break;

	case EpochVariableType_Integer16:
		for(unsigned i = 0; i < NumEntries; ++i)
		{
			Integer16Variable var(context.Stack.GetOffsetIntoStack(offset));
			ret->AddElement(new Integer16RValue(var.GetValue()));
			offset += Integer16Variable::GetStorageSize();
		}
		break;

	case EpochVariableType_Boolean:
		for(unsigned i = 0; i < NumEntries; ++i)
		{
			BooleanVariable var(context.Stack.GetOffsetIntoStack(offset));
			ret->AddElement(new BooleanRValue(var.GetValue()));
			offset += BooleanVariable::GetStorageSize();
		}
		break;

	case EpochVariableType_String:
		for(unsigned i = 0; i < NumEntries; ++i)
		{
			StringVariable var(context.Stack.GetOffsetIntoStack(offset));
			ret->AddElement(new StringRValue(var.GetValue()));
			offset += StringVariable::GetStorageSize();
		}
		break;

	case EpochVariableType_Real:
		for(unsigned i = 0; i < NumEntries; ++i)
		{
			RealVariable var(context.Stack.GetOffsetIntoStack(offset));
			ret->AddElement(new RealRValue(var.GetValue()));
			offset += RealVariable::GetStorageSize();
		}
		break;

	default:
		throw NotImplementedException("Cannot construct array of this type");
	}

	return RValuePtr(ret.release());
}

