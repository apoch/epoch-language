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
	HandleType arraydatahandle = ArrayVariable::AllocateNewHandle(ElementType, NumEntries);
	void* storage = ArrayVariable::GetArrayStorage(arraydatahandle);

	std::auto_ptr<ArrayRValue> ret(new ArrayRValue(arraydatahandle, false));

	switch(ElementType)
	{
	case EpochVariableType_Integer:
		for(size_t i = 0; i < NumEntries; ++i)
		{
			IntegerVariable stackvar(context.Stack.GetCurrentTopOfStack());
			IntegerVariable var(storage);
			var.SetValue(stackvar.GetValue());
			ret->AddElement(var.GetAsRValue().release());
			storage = reinterpret_cast<char*>(storage) + IntegerVariable::GetStorageSize();
			context.Stack.Pop(IntegerVariable::GetStorageSize());
		}
		break;

	case EpochVariableType_Integer16:
		for(size_t i = 0; i < NumEntries; ++i)
		{
			Integer16Variable stackvar(context.Stack.GetCurrentTopOfStack());
			Integer16Variable var(storage);
			var.SetValue(stackvar.GetValue());
			ret->AddElement(var.GetAsRValue().release());
			storage = reinterpret_cast<char*>(storage) + Integer16Variable::GetStorageSize();
			context.Stack.Pop(Integer16Variable::GetStorageSize());
		}
		break;

	case EpochVariableType_Boolean:
		for(size_t i = 0; i < NumEntries; ++i)
		{
			BooleanVariable stackvar(context.Stack.GetCurrentTopOfStack());
			BooleanVariable var(storage);
			var.SetValue(stackvar.GetValue());
			ret->AddElement(var.GetAsRValue().release());
			storage = reinterpret_cast<char*>(storage) + BooleanVariable::GetStorageSize();
			context.Stack.Pop(BooleanVariable::GetStorageSize());
		}
		break;

	case EpochVariableType_String:
		for(size_t i = 0; i < NumEntries; ++i)
		{
			StringVariable stackvar(context.Stack.GetCurrentTopOfStack());
			StringVariable var(storage);
			var.SetValue(stackvar.GetValue(), true);
			ret->AddElement(var.GetAsRValue().release());
			storage = reinterpret_cast<char*>(storage) + StringVariable::GetStorageSize();
			context.Stack.Pop(StringVariable::GetStorageSize());
		}
		break;

	case EpochVariableType_Real:
		for(size_t i = 0; i < NumEntries; ++i)
		{
			RealVariable stackvar(context.Stack.GetCurrentTopOfStack());
			RealVariable var(storage);
			var.SetValue(stackvar.GetValue());
			ret->AddElement(var.GetAsRValue().release());
			storage = reinterpret_cast<char*>(storage) + RealVariable::GetStorageSize();
			context.Stack.Pop(RealVariable::GetStorageSize());
		}
		break;

	default:
		throw NotImplementedException("Cannot construct array of this type");
	}

	return RValuePtr(ret.release());
}



ConsArrayIndirect::ConsArrayIndirect(EpochVariableTypeID elementtype, Operation* op)
	: ElementType(elementtype),
	  TheOp(op)
{
	if(!TheOp)
		throw InternalFailureException("Cannot construct array, no initialization instruction found");
}

ConsArrayIndirect::~ConsArrayIndirect()
{
	delete TheOp;
}

//
// Extract an array's contents from the stack and wrap them into an r-value
//
void ConsArrayIndirect::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}

RValuePtr ConsArrayIndirect::ExecuteAndStoreRValue(ExecutionContext& context)
{
	RValuePtr indirectresultraw(TheOp->ExecuteAndStoreRValue(context));
	ArrayRValue& indirectresult = indirectresultraw->CastTo<ArrayRValue>();
	size_t numentries = indirectresult.GetElementCount();

	HandleType arraydatahandle = ArrayVariable::AllocateNewHandle(ElementType, numentries);
	void* storage = ArrayVariable::GetArrayStorage(arraydatahandle);

	indirectresult.SetHandle(arraydatahandle);

	switch(ElementType)
	{
	case EpochVariableType_Integer:
		for(size_t i = 0; i < numentries; ++i)
		{
			IntegerVariable var(storage);
			var.SetValue(dynamic_cast<IntegerRValue*>(indirectresult.GetElements()[i])->GetValue());
			storage = reinterpret_cast<char*>(storage) + IntegerVariable::GetStorageSize();
		}
		break;

	case EpochVariableType_Integer16:
		for(size_t i = 0; i < numentries; ++i)
		{
			Integer16Variable var(storage);
			var.SetValue(dynamic_cast<Integer16RValue*>(indirectresult.GetElements()[i])->GetValue());
			storage = reinterpret_cast<char*>(storage) + Integer16Variable::GetStorageSize();
		}
		break;

	case EpochVariableType_Boolean:
		for(size_t i = 0; i < numentries; ++i)
		{
			BooleanVariable var(storage);
			var.SetValue(dynamic_cast<BooleanRValue*>(indirectresult.GetElements()[i])->GetValue());
			storage = reinterpret_cast<char*>(storage) + BooleanVariable::GetStorageSize();
		}
		break;

	case EpochVariableType_String:
		for(size_t i = 0; i < numentries; ++i)
		{
			StringVariable var(storage);
			var.SetValue(dynamic_cast<StringRValue*>(indirectresult.GetElements()[i])->GetValue(), true);
			storage = reinterpret_cast<char*>(storage) + StringVariable::GetStorageSize();
		}
		break;

	case EpochVariableType_Real:
		for(size_t i = 0; i < numentries; ++i)
		{
			RealVariable var(storage);
			var.SetValue(dynamic_cast<RealRValue*>(indirectresult.GetElements()[i])->GetValue());
			storage = reinterpret_cast<char*>(storage) + RealVariable::GetStorageSize();
		}
		break;

	default:
		throw NotImplementedException("Cannot construct array of this type");
	}

	return RValuePtr(indirectresultraw.release());
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
	ExecuteAndStoreRValue(context);
}

RValuePtr ReadArray::ExecuteAndStoreRValue(ExecutionContext& context)
{
	IntegerVariable indexvar(context.Stack.GetCurrentTopOfStack());
	IntegerVariable::BaseStorage index = indexvar.GetValue();
	context.Stack.Pop(IntegerVariable::GetStorageSize());

	ArrayVariable& arrayvar = context.Scope.GetVariableRef<ArrayVariable>(ArrayName);

	void* storage = ArrayVariable::GetArrayStorage(arrayvar.GetValue());
	EpochVariableTypeID entrytype = arrayvar.GetElementType();

	if(index < 0 || index >= static_cast<Integer32>(arrayvar.GetNumElements()))
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
	EpochVariableTypeID entrytype = context.Scope.GetVariableRef<ArrayVariable>(ArrayName).GetElementType();

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

	ArrayVariable& arrayvar = context.Scope.GetVariableRef<ArrayVariable>(ArrayName);
	void* storage = ArrayVariable::GetArrayStorage(arrayvar.GetValue());

	if(index < 0 || index >= static_cast<Integer32>(arrayvar.GetNumElements()))
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
	Integer32 length = static_cast<Integer32>(context.Scope.GetVariableRef<ArrayVariable>(ArrayName).GetNumElements());
	return RValuePtr(new IntegerRValue(length));
}

Traverser::Payload ArrayLength::GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
{
	Traverser::Payload payload;
	payload.SetValue(ArrayName.c_str());
	payload.IsIdentifier = true;
	payload.ParameterCount = 1;
	return payload;
}

