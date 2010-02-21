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
#include "Virtual Machine/Core Entities/Variables/ArrayVariable.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Types Management/TypeInfo.h"
#include "Virtual Machine/SelfAware.inl"
#include "Virtual Machine/Routines.inl"
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



//
// Construct and initialize an array read operation
//
ReadArray::ReadArray(const std::wstring& arrayname)
	: ArrayName(arrayname)
{
}

void ReadArray::ExecuteFast(ExecutionContext& context)
{
	// Nothing to do.
}

RValuePtr ReadArray::ExecuteAndStoreRValue(ExecutionContext& context)
{
	IntegerVariable indexvar(context.Stack.GetCurrentTopOfStack());
	IntegerVariable::BaseStorage index = indexvar.GetValue();
	context.Stack.Pop(IntegerVariable::GetStorageSize());

	void* storage = context.Scope.GetVariableRef<ArrayVariable>(ArrayName).GetArrayElementStorage();
	EpochVariableTypeID entrytype = context.Scope.GetOriginalDescription().GetArrayType(ArrayName);

	if(index < 0 || index >= static_cast<Integer32>(context.Scope.GetOriginalDescription().GetArraySize(ArrayName)))
		throw ExecutionException("Invalid array index");

	size_t stride = TypeInfo::GetStorageSize(entrytype);
	void* target = reinterpret_cast<char*>(storage) + (stride * index);

	return GetRValuePtrFromStorage(entrytype, target);
}

EpochVariableTypeID ReadArray::GetType(const ScopeDescription& scope) const
{
	return scope.GetArrayType(ArrayName);
}

Traverser::Payload ReadArray::GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
{
	Traverser::Payload payload;
	payload.SetValue(ArrayName.c_str());
	payload.IsIdentifier = true;
	payload.ParameterCount = GetNumParameters(*scope);
	return payload;
}


WriteArray::WriteArray(const std::wstring& arrayname)
	: ArrayName(arrayname)
{
}

void WriteArray::ExecuteFast(ExecutionContext& context)
{
	RValuePtr writevalue(NULL);
	EpochVariableTypeID entrytype = context.Scope.GetOriginalDescription().GetArrayType(ArrayName);

	switch(entrytype)
	{
	case EpochVariableType_Integer:		writevalue = IntegerVariable(context.Stack.GetCurrentTopOfStack()).GetAsRValue();		break;
	case EpochVariableType_Integer16:	writevalue = Integer16Variable(context.Stack.GetCurrentTopOfStack()).GetAsRValue();		break;
	case EpochVariableType_Real:		writevalue = RealVariable(context.Stack.GetCurrentTopOfStack()).GetAsRValue();			break;
	case EpochVariableType_Boolean:		writevalue = BooleanVariable(context.Stack.GetCurrentTopOfStack()).GetAsRValue();		break;
	case EpochVariableType_String:		writevalue = StringVariable(context.Stack.GetCurrentTopOfStack()).GetAsRValue();		break;
	case EpochVariableType_Function:	writevalue = FunctionBinding(context.Stack.GetCurrentTopOfStack()).GetAsRValue();		break;
	case EpochVariableType_Address:		writevalue = AddressVariable(context.Stack.GetCurrentTopOfStack()).GetAsRValue();		break;
	case EpochVariableType_Buffer:		writevalue = BufferVariable(context.Stack.GetCurrentTopOfStack()).GetAsRValue();		break;
	case EpochVariableType_TaskHandle:	writevalue = TaskHandleVariable(context.Stack.GetCurrentTopOfStack()).GetAsRValue();	break;
	default:
		throw NotImplementedException("Cannot write array member of this type, support not implemented");
	}

	context.Stack.Pop(TypeInfo::GetStorageSize(entrytype));

	IntegerVariable indexvar(context.Stack.GetCurrentTopOfStack());
	IntegerVariable::BaseStorage index = indexvar.GetValue();
	context.Stack.Pop(IntegerVariable::GetStorageSize());

	void* storage = context.Scope.GetVariableRef<ArrayVariable>(ArrayName).GetArrayElementStorage();

	if(index < 0 || index >= static_cast<Integer32>(context.Scope.GetOriginalDescription().GetArraySize(ArrayName)))
		throw ExecutionException("Invalid array index");

	size_t stride = TypeInfo::GetStorageSize(entrytype);
	void* target = reinterpret_cast<char*>(storage) + (stride * index);

	WriteRValueToStorage(writevalue, target);
}

RValuePtr WriteArray::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ExecuteFast(context);
	return RValuePtr(new NullRValue());
}

Traverser::Payload WriteArray::GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
{
	Traverser::Payload payload;
	payload.SetValue(ArrayName.c_str());
	payload.IsIdentifier = true;
	payload.ParameterCount = GetNumParameters(*scope);
	return payload;
}


void ArrayLength::ExecuteFast(ExecutionContext& context)
{
	// Nothing to do.
}

RValuePtr ArrayLength::ExecuteAndStoreRValue(ExecutionContext& context)
{
	return RValuePtr(new IntegerRValue(static_cast<Integer32>(context.Scope.GetOriginalDescription().GetArraySize(ArrayName))));
}

Traverser::Payload ArrayLength::GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
{
	Traverser::Payload payload;
	payload.SetValue(ArrayName.c_str());
	payload.IsIdentifier = true;
	payload.ParameterCount = GetNumParameters(*scope);
	return payload;
}