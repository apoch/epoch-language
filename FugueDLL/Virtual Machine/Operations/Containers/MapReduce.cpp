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
// Map a unary function onto a list of values, and return the result
//
RValuePtr MapOperation::ExecuteAndStoreRValue(ExecutionContext& context)
{
	IntegerVariable typevar(context.Stack.GetCurrentTopOfStack());
	IntegerVariable countvar(context.Stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
	IntegerVariable::BaseStorage type = typevar.GetValue();
	IntegerVariable::BaseStorage count = countvar.GetValue();
	context.Stack.Pop(IntegerVariable::GetStorageSize() * 2);

	RValuePtr result(new ListRValue(static_cast<EpochVariableTypeID>(type)));
	ListRValue* resultptr = dynamic_cast<ListRValue*>(result.get());

	for(IntegerVariable::BaseStorage i = 0; i < count; ++i)
	{
		RValuePtr ret(TheOp->ExecuteAndStoreRValue(context));
		
		EpochVariableTypeID rettype = ret->GetType();
		if(rettype != EpochVariableType_Null)
			resultptr->AddElement(ret.release());
	}

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
// Apply a binary function to each element in a list, keeping a running
// accumulator value as we go along. The final accumulator value is returned.
//
RValuePtr ReduceOperation::ExecuteAndStoreRValue(ExecutionContext& context)
{
	IntegerVariable typevar(context.Stack.GetCurrentTopOfStack());
	IntegerVariable countvar(context.Stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
	IntegerVariable::BaseStorage type = typevar.GetValue();
	IntegerVariable::BaseStorage count = countvar.GetValue();
	context.Stack.Pop(IntegerVariable::GetStorageSize() * 2);

	size_t elementstoragesize = TypeInfo::GetStorageSize(static_cast<EpochVariableTypeID>(type));

	--count;

	RValuePtr result(NULL);

	// We have to reverse the operands so they work correctly with non-transitive operators
	std::vector<Byte> buffer1(elementstoragesize, 0);
	std::vector<Byte> buffer2(elementstoragesize, 0);
	memcpy(&buffer1[0], context.Stack.GetCurrentTopOfStack(), elementstoragesize);
	context.Stack.Pop(elementstoragesize);
	memcpy(&buffer2[0], context.Stack.GetCurrentTopOfStack(), elementstoragesize);
	buffer1.swap(buffer2);
	memcpy(context.Stack.GetCurrentTopOfStack(), &buffer2[0], elementstoragesize);
	context.Stack.Push(elementstoragesize);
	memcpy(context.Stack.GetCurrentTopOfStack(), &buffer1[0], elementstoragesize);

	for(IntegerVariable::BaseStorage i = 0; i < count; ++i)
	{
		RValuePtr ret(TheOp->ExecuteAndStoreRValue(context));
		result.reset(ret->Clone());
		EpochVariableTypeID rettype = ret->GetType();
		if(rettype != EpochVariableType_Null)
		{
			memcpy(&buffer1[0], context.Stack.GetCurrentTopOfStack(), elementstoragesize);
			context.Stack.Pop(elementstoragesize);
			PushOperation::DoPush(rettype, ret, context.Scope.GetOriginalDescription(), context.Stack, false);
			context.Stack.Push(elementstoragesize);
			memcpy(context.Stack.GetCurrentTopOfStack(), &buffer1[0], elementstoragesize);
		}
	}

	context.Stack.Pop(elementstoragesize);

	return result;
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
