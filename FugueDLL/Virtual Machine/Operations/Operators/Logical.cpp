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



RValuePtr LogicalOr::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	bool ret = false;
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
	{
		if((*iter)->ExecuteAndStoreRValue(scope, stack, flowresult)->CastTo<BooleanRValue>().GetValue())
		{
			ret = true;
			break;
		}
	}

	return RValuePtr(new BooleanRValue(ret));
}

void LogicalOr::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		(*iter)->ExecuteFast(scope, stack, flowresult);
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

RValuePtr LogicalAnd::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	bool ret = true;
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
	{
		if(!(*iter)->ExecuteAndStoreRValue(scope, stack, flowresult)->CastTo<BooleanRValue>().GetValue())
		{
			ret = false;
			break;
		}
	}

	return RValuePtr(new BooleanRValue(ret));
}

void LogicalAnd::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		(*iter)->ExecuteFast(scope, stack, flowresult);
}

void LogicalAnd::Traverse(Validator::ValidationTraverser& traverser)
{
	traverser.TraverseNode(*this);
	for(std::list<Operation*>::const_iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		dynamic_cast<SelfAwareBase*>(*iter)->Traverse(traverser);
}

RValuePtr LogicalXor::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	BooleanVariable two(stack.GetCurrentTopOfStack());
	BooleanVariable one(stack.GetOffsetIntoStack(BooleanVariable::GetStorageSize()));
	bool oneval = one.GetValue();
	bool twoval = two.GetValue();
	BooleanVariable::BaseStorage ret = (oneval || twoval) && (!(oneval && twoval));
	stack.Pop(BooleanVariable::GetStorageSize() * 2);

	return RValuePtr(new BooleanRValue(ret));
}

void LogicalXor::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	stack.Pop(BooleanVariable::GetStorageSize() * 2);
}


RValuePtr LogicalNot::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	BooleanVariable param(stack.GetCurrentTopOfStack());
	BooleanVariable::BaseStorage ret = !param.GetValue();
	stack.Pop(BooleanVariable::GetStorageSize());

	return RValuePtr(new BooleanRValue(ret));

}

void LogicalNot::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	stack.Pop(BooleanVariable::GetStorageSize());
}

