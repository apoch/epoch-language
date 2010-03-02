//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Map and reduce builtin operations
//

#include "pch.h"

#include "Virtual Machine/Operations/Containers/MapReduce.h"
#include "Virtual Machine/Core Entities/Variables/Variable.h"
#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Types Management/TypeInfo.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Routines.inl"

#include "Validator/Validator.h"

#include "Serialization/SerializationTraverser.h"

#include "Parser/Debug Info Tables/DebugTable.h"


using namespace VM;
using namespace VM::Operations;


//
// Destruct and clean up a map operation
//
MapOperation::~MapOperation()
{
	delete TheOp;
}

//
// Map a unary function onto an array of values, and return the result
//
RValuePtr MapOperation::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ArrayVariable arrayvar(context.Stack.GetCurrentTopOfStack());
	EpochVariableTypeID type = arrayvar.GetElementType();
	size_t count = arrayvar.GetNumElements();
	void* storage = ArrayVariable::GetArrayStorage(arrayvar.GetValue());
	context.Stack.Pop(ArrayVariable::GetBaseStorageSize());

	RValuePtr result(new ArrayRValue(type));
	ArrayRValue* resultptr = dynamic_cast<ArrayRValue*>(result.get());

	for(size_t i = 0; i < count; ++i)
	{
		PushOperation::DoPush(type, GetRValuePtrFromStorage(type, storage).get(), context.Scope.GetOriginalDescription(), context.Stack, false, false);
		storage = reinterpret_cast<char*>(storage) + TypeInfo::GetStorageSize(type);

		RValuePtr ret(TheOp->ExecuteAndStoreRValue(context));
		
		EpochVariableTypeID rettype = ret->GetType();
		if(rettype != EpochVariableType_Null)
			resultptr->AddElement(ret.release());
	}

	resultptr->StoreIntoNewBuffer();

	return result;
}

void MapOperation::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}


template <typename TraverserT>
void MapOperation::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	if(TheOp)
		dynamic_cast<SelfAwareBase*>(TheOp)->Traverse(traverser);
}

void MapOperation::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void MapOperation::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}

//
// Destruct and clean up a reduce operation
//
ReduceOperation::~ReduceOperation()
{
	delete TheOp;
}

//
// Apply a binary function to each element in an array, keeping a running
// accumulator value as we go along. The final accumulator value is returned.
//
RValuePtr ReduceOperation::ExecuteAndStoreRValue(ExecutionContext& context)
{
	ArrayVariable arrayvar(context.Stack.GetCurrentTopOfStack());
	EpochVariableTypeID type = arrayvar.GetElementType();
	size_t count = arrayvar.GetNumElements();
	void* storage = ArrayVariable::GetArrayStorage(arrayvar.GetValue());
	context.Stack.Pop(ArrayVariable::GetBaseStorageSize());

	size_t elementstoragesize = TypeInfo::GetStorageSize(type);
	RValuePtr ret(NULL);

	--count;
	ret.reset(GetRValuePtrFromStorage(type, storage).release());
	PushOperation::DoPush(type, ret.get(), context.Scope.GetOriginalDescription(), context.Stack, false, false);

	for(size_t i = 0; i < count; ++i)
	{
		storage = reinterpret_cast<char*>(storage) + elementstoragesize;
		ret.reset(GetRValuePtrFromStorage(type, storage).release());
		PushOperation::DoPush(type, ret.get(), context.Scope.GetOriginalDescription(), context.Stack, false, false);

		RValuePtr intermediate(TheOp->ExecuteAndStoreRValue(context));
		ret.reset(intermediate->Clone());
		PushOperation::DoPush(type, ret.get(), context.Scope.GetOriginalDescription(), context.Stack, false, false);
	}

	context.Stack.Pop(elementstoragesize);

	return ret;
}

void ReduceOperation::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}

template <typename TraverserT>
void ReduceOperation::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	if(TheOp)
		dynamic_cast<SelfAwareBase*>(TheOp)->Traverse(traverser);
}

void ReduceOperation::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void ReduceOperation::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}
