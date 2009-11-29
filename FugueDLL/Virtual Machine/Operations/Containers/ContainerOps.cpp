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
// Construct a list of the given element type with the given number of entries
//
ConsList::ConsList(unsigned numentries, EpochVariableTypeID elementtype)
	: NumEntries(numentries),
	  ElementType(elementtype)
{
}

//
// Extract a list's contents from the stack and wrap them into an r-value
//
void ConsList::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}

RValuePtr ConsList::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	std::auto_ptr<ListRValue> ret(new ListRValue(ElementType));

	size_t offset = 0;

	switch(ElementType)
	{
	case EpochVariableType_Integer:
		for(unsigned i = 0; i < NumEntries; ++i)
		{
			IntegerVariable var(stack.GetOffsetIntoStack(offset));
			ret->AddElement(new IntegerRValue(var.GetValue()));
			offset += IntegerVariable::GetStorageSize();
		}
		break;

	case EpochVariableType_Integer16:
		for(unsigned i = 0; i < NumEntries; ++i)
		{
			Integer16Variable var(stack.GetOffsetIntoStack(offset));
			ret->AddElement(new Integer16RValue(var.GetValue()));
			offset += Integer16Variable::GetStorageSize();
		}
		break;

	case EpochVariableType_Boolean:
		for(unsigned i = 0; i < NumEntries; ++i)
		{
			BooleanVariable var(stack.GetOffsetIntoStack(offset));
			ret->AddElement(new BooleanRValue(var.GetValue()));
			offset += BooleanVariable::GetStorageSize();
		}
		break;

	case EpochVariableType_String:
		for(unsigned i = 0; i < NumEntries; ++i)
		{
			StringVariable var(stack.GetOffsetIntoStack(offset));
			ret->AddElement(new StringRValue(var.GetValue()));
			offset += StringVariable::GetStorageSize();
		}
		break;

	case EpochVariableType_Real:
		for(unsigned i = 0; i < NumEntries; ++i)
		{
			RealVariable var(stack.GetOffsetIntoStack(offset));
			ret->AddElement(new RealRValue(var.GetValue()));
			offset += RealVariable::GetStorageSize();
		}
		break;

	default:
		throw NotImplementedException("Cannot construct list of this type");
	}

	return RValuePtr(ret.release());
}

