//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Built in logical operators
//

#include "pch.h"

#include "Virtual Machine/Operations/Operators/Logical.h"
#include "Virtual Machine/Core Entities/Variables/Variable.h"

#include "Virtual Machine/Types Management/Typecasts.h"

#include "Validator/Validator.h"

#include "Serialization/SerializationTraverser.h"


using namespace VM;
using namespace VM::Operations;



RValuePtr LogicalOr::ExecuteAndStoreRValue(ExecutionContext& context)
{
	bool ret = false;
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
	{
		if((*iter)->ExecuteAndStoreRValue(context)->CastTo<BooleanRValue>().GetValue())
		{
			ret = true;
			break;
		}
	}

	return RValuePtr(new BooleanRValue(ret));
}

void LogicalOr::ExecuteFast(ExecutionContext& context)
{
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		(*iter)->ExecuteFast(context);
}

template <typename TraverserT>
void LogicalOr::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	for(std::list<Operation*>::const_iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		dynamic_cast<SelfAwareBase*>(*iter)->Traverse(traverser);
}

void LogicalOr::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void LogicalOr::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}

RValuePtr LogicalAnd::ExecuteAndStoreRValue(ExecutionContext& context)
{
	bool ret = true;
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
	{
		if(!(*iter)->ExecuteAndStoreRValue(context)->CastTo<BooleanRValue>().GetValue())
		{
			ret = false;
			break;
		}
	}

	return RValuePtr(new BooleanRValue(ret));
}

void LogicalAnd::ExecuteFast(ExecutionContext& context)
{
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		(*iter)->ExecuteFast(context);
}


template <typename TraverserT>
void LogicalAnd::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	for(std::list<Operation*>::const_iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		dynamic_cast<SelfAwareBase*>(*iter)->Traverse(traverser);
}

void LogicalAnd::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void LogicalAnd::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}



RValuePtr LogicalXor::ExecuteAndStoreRValue(ExecutionContext& context)
{
	BooleanVariable two(context.Stack.GetCurrentTopOfStack());
	BooleanVariable one(context.Stack.GetOffsetIntoStack(BooleanVariable::GetStorageSize()));
	bool oneval = one.GetValue();
	bool twoval = two.GetValue();
	BooleanVariable::BaseStorage ret = (oneval || twoval) && (!(oneval && twoval));
	context.Stack.Pop(BooleanVariable::GetStorageSize() * 2);

	return RValuePtr(new BooleanRValue(ret));
}

void LogicalXor::ExecuteFast(ExecutionContext& context)
{
	context.Stack.Pop(BooleanVariable::GetStorageSize() * 2);
}


RValuePtr LogicalNot::ExecuteAndStoreRValue(ExecutionContext& context)
{
	BooleanVariable param(context.Stack.GetCurrentTopOfStack());
	BooleanVariable::BaseStorage ret = !param.GetValue();
	context.Stack.Pop(BooleanVariable::GetStorageSize());

	return RValuePtr(new BooleanRValue(ret));

}

void LogicalNot::ExecuteFast(ExecutionContext& context)
{
	context.Stack.Pop(BooleanVariable::GetStorageSize());
}

