//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Built in bitwise operators
//

#include "pch.h"

#include "Virtual Machine/Operations/Operators/Bitwise.h"
#include "Virtual Machine/Core Entities/Variables/Variable.h"
#include "Virtual Machine/Types Management/Typecasts.h"

#include "Validator/Validator.h"

#include "Serialization/SerializationTraverser.h"

// TODO - Code review of ENTIRE code base - check for formatting, documentation, and possible code improvements


using namespace VM;
using namespace VM::Operations;


RValuePtr BitwiseOr::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	Integer32 ret = 0;
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		ret |= (*iter)->ExecuteAndStoreRValue(scope, stack, flowresult)->CastTo<IntegerRValue>().GetValue();

	return RValuePtr(new IntegerRValue(ret));
}

void BitwiseOr::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		(*iter)->ExecuteFast(scope, stack, flowresult);
}

template <typename TraverserT>
void BitwiseOr::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	for(std::list<Operation*>::const_iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		dynamic_cast<SelfAwareBase*>(*iter)->Traverse(traverser);
}

void BitwiseOr::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void BitwiseOr::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}


RValuePtr BitwiseAnd::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	std::list<Operation*>::iterator iter = SubOps.begin();
	Integer32 ret = (*iter)->ExecuteAndStoreRValue(scope, stack, flowresult)->CastTo<IntegerRValue>().GetValue();

	++iter;
	for( ; iter != SubOps.end(); ++iter)
		ret &= (*iter)->ExecuteAndStoreRValue(scope, stack, flowresult)->CastTo<IntegerRValue>().GetValue();

	return RValuePtr(new IntegerRValue(ret));
}

void BitwiseAnd::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		(*iter)->ExecuteFast(scope, stack, flowresult);
}


template <typename TraverserT>
void BitwiseAnd::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	for(std::list<Operation*>::const_iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		dynamic_cast<SelfAwareBase*>(*iter)->Traverse(traverser);
}

void BitwiseAnd::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void BitwiseAnd::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}


RValuePtr BitwiseXor::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	IntegerVariable two(stack.GetCurrentTopOfStack());
	IntegerVariable one(stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
	IntegerVariable::BaseStorage ret = one.GetValue() ^ two.GetValue();
	stack.Pop(IntegerVariable::GetStorageSize() * 2);

	return RValuePtr(new IntegerRValue(ret));
}

void BitwiseXor::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	stack.Pop(IntegerVariable::GetStorageSize() * 2);
}


RValuePtr BitwiseNot::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	IntegerVariable one(stack.GetCurrentTopOfStack());
	IntegerVariable::BaseStorage ret = ~one.GetValue();
	stack.Pop(IntegerVariable::GetStorageSize());

	return RValuePtr(new IntegerRValue(ret));
}

void BitwiseNot::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	stack.Pop(IntegerVariable::GetStorageSize());
}
