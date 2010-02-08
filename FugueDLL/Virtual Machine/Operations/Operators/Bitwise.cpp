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
#include "Virtual Machine/SelfAware.inl"

#include "Validator/Validator.h"

#include "Serialization/SerializationTraverser.h"



using namespace VM;
using namespace VM::Operations;


BitwiseOr::BitwiseOr(EpochVariableTypeID type)
	: Type(type)
{
	if(type != EpochVariableType_Integer && type != EpochVariableType_Integer16)
		throw NotImplementedException("Bitwise-or cannot be used on data of this type");
}


RValuePtr BitwiseOr::ExecuteAndStoreRValue(ExecutionContext& context)
{
	if(Type == EpochVariableType_Integer)
	{
		Integer32 ret = 0;
		for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
			ret |= (*iter)->ExecuteAndStoreRValue(context)->CastTo<IntegerRValue>().GetValue();

		return RValuePtr(new IntegerRValue(ret));
	}
	else if(Type == EpochVariableType_Integer16)
	{
		Integer16 ret = 0;
		for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
			ret |= (*iter)->ExecuteAndStoreRValue(context)->CastTo<Integer16RValue>().GetValue();

		return RValuePtr(new IntegerRValue(ret));
	}

	throw NotImplementedException("Bitwise-or cannot be used on data of this type");
}

void BitwiseOr::ExecuteFast(ExecutionContext& context)
{
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		(*iter)->ExecuteFast(context);
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


BitwiseAnd::BitwiseAnd(EpochVariableTypeID type)
	: Type(type)
{
	if(type != EpochVariableType_Integer && type != EpochVariableType_Integer16)
		throw NotImplementedException("Bitwise-and cannot be used on data of this type");
}

RValuePtr BitwiseAnd::ExecuteAndStoreRValue(ExecutionContext& context)
{
	if(Type == EpochVariableType_Integer)
	{
		std::list<Operation*>::iterator iter = SubOps.begin();
		Integer32 ret = (*iter)->ExecuteAndStoreRValue(context)->CastTo<IntegerRValue>().GetValue();

		++iter;
		for( ; iter != SubOps.end(); ++iter)
			ret &= (*iter)->ExecuteAndStoreRValue(context)->CastTo<IntegerRValue>().GetValue();

		return RValuePtr(new IntegerRValue(ret));
	}
	else if(Type == EpochVariableType_Integer16)
	{
		std::list<Operation*>::iterator iter = SubOps.begin();
		Integer16 ret = (*iter)->ExecuteAndStoreRValue(context)->CastTo<Integer16RValue>().GetValue();

		++iter;
		for( ; iter != SubOps.end(); ++iter)
			ret &= (*iter)->ExecuteAndStoreRValue(context)->CastTo<Integer16RValue>().GetValue();

		return RValuePtr(new Integer16RValue(ret));
	}

	throw NotImplementedException("Bitwise-and cannot be used on data of this type");
}

void BitwiseAnd::ExecuteFast(ExecutionContext& context)
{
	for(std::list<Operation*>::iterator iter = SubOps.begin(); iter != SubOps.end(); ++iter)
		(*iter)->ExecuteFast(context);
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


BitwiseXor::BitwiseXor(EpochVariableTypeID type)
	: Type(type)
{
	if(type != EpochVariableType_Integer && type != EpochVariableType_Integer16)
		throw NotImplementedException("Bitwise-xor cannot be used on data of this type");
}


RValuePtr BitwiseXor::ExecuteAndStoreRValue(ExecutionContext& context)
{
	IntegerVariable two(context.Stack.GetCurrentTopOfStack());
	IntegerVariable one(context.Stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
	IntegerVariable::BaseStorage ret = one.GetValue() ^ two.GetValue();
	context.Stack.Pop(IntegerVariable::GetStorageSize() * 2);

	return RValuePtr(new IntegerRValue(ret));
}

void BitwiseXor::ExecuteFast(ExecutionContext& context)
{
	context.Stack.Pop(IntegerVariable::GetStorageSize() * 2);
}



BitwiseNot::BitwiseNot(EpochVariableTypeID type)
	: Type(type)
{
	if(type != EpochVariableType_Integer && type != EpochVariableType_Integer16)
		throw NotImplementedException("Bitwise-not cannot be used on data of this type");
}

RValuePtr BitwiseNot::ExecuteAndStoreRValue(ExecutionContext& context)
{
	IntegerVariable one(context.Stack.GetCurrentTopOfStack());
	IntegerVariable::BaseStorage ret = ~one.GetValue();
	context.Stack.Pop(IntegerVariable::GetStorageSize());

	return RValuePtr(new IntegerRValue(ret));
}

void BitwiseNot::ExecuteFast(ExecutionContext& context)
{
	context.Stack.Pop(IntegerVariable::GetStorageSize());
}
