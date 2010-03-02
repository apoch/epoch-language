//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Implementation of built-in operators and operations.
//

#include "pch.h"

#include "Virtual Machine/Operations/Operators/Arithmetic.h"
#include "Virtual Machine/Core Entities/Variables/ArrayVariable.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Types Management/Typecasts.h"


//
// Perform the selected arithmetic operation
//
template<VM::Operations::ArithmeticOpType OpType, class VarType, class RValueType>
VM::RValuePtr VM::Operations::ArithmeticOp<OpType, VarType, RValueType>::ExecuteAndStoreRValue(ExecutionContext& context)
{
	VarType::BaseStorage ret = 0;

	if(NumParams == 1)
		ret = OperateOnArray(context.Stack);
	else if(NumParams == 2)
	{
		if(FirstIsArray && !SecondIsArray)
		{
			VarType var(context.Stack.GetCurrentTopOfStack());
			VarType::BaseStorage variableval = var.GetValue();
			context.Stack.Pop(VarType::GetStorageSize());
			switch(OpType)
			{
			case Arithmetic_Add:		ret = OperateOnArray(context.Stack) + variableval;		break;
			case Arithmetic_Subtract:	ret = OperateOnArray(context.Stack) - variableval;		break;
			case Arithmetic_Multiply:	ret = OperateOnArray(context.Stack) * variableval;		break;
			case Arithmetic_Divide:		ret = OperateOnArray(context.Stack) / variableval;		break;
			default: throw InternalFailureException("Unrecognized arithmetic operation");
			}
		}
		else if(!FirstIsArray && SecondIsArray)
		{
			ret = OperateOnArray(context.Stack);
			VarType var(context.Stack.GetCurrentTopOfStack());
			switch(OpType)
			{
			case Arithmetic_Add:		ret = var.GetValue() + ret;		break;
			case Arithmetic_Subtract:	ret = var.GetValue() - ret;		break;
			case Arithmetic_Multiply:	ret = var.GetValue() * ret;		break;
			case Arithmetic_Divide:		ret = var.GetValue() / ret;		break;
			default: throw InternalFailureException("Unrecognized arithmetic operation");
			}
			context.Stack.Pop(VarType::GetStorageSize());
		}
		else if(FirstIsArray && SecondIsArray)
		{
			ret = OperateOnArray(context.Stack);
			switch(OpType)
			{
			case Arithmetic_Add:		ret = OperateOnArray(context.Stack) + ret;		break;
			case Arithmetic_Subtract:	ret = OperateOnArray(context.Stack) - ret;		break;
			case Arithmetic_Multiply:	ret = OperateOnArray(context.Stack) * ret;		break;
			case Arithmetic_Divide:		ret = OperateOnArray(context.Stack) / ret;		break;
			default: throw InternalFailureException("Unrecognized arithmetic operation");
			}
		}
		else
		{
			VarType twovar(context.Stack.GetCurrentTopOfStack());
			VarType onevar(context.Stack.GetOffsetIntoStack(VarType::GetStorageSize()));
			switch(OpType)
			{
			case Arithmetic_Add:		ret = onevar.GetValue() + twovar.GetValue();		break;
			case Arithmetic_Subtract:	ret = onevar.GetValue() - twovar.GetValue();		break;
			case Arithmetic_Multiply:	ret = onevar.GetValue() * twovar.GetValue();		break;
			case Arithmetic_Divide:		ret = onevar.GetValue() / twovar.GetValue();		break;
			default: throw InternalFailureException("Unrecognized arithmetic operation");
			}
			context.Stack.Pop(VarType::GetStorageSize() * 2);
		}
	}
	else
		throw ExecutionException("Invalid set of parameters");

	return RValuePtr(new RValueType(ret));
}

template<VM::Operations::ArithmeticOpType OpType, class VarType, class RValueType>
void VM::Operations::ArithmeticOp<OpType, VarType, RValueType>::ExecuteFast(ExecutionContext& context)
{
	// Nothing to do here; either the op is called by a PushOperation op, in which case
	// the rvalue execute function will be used; or the op is standalone and has no
	// effects on program state, and therefore can be ignored.
}

//
// Operate on an array of values from the stack
//
template<VM::Operations::ArithmeticOpType OpType, class VarType, class RValueType>
typename VarType::BaseStorage VM::Operations::ArithmeticOp<OpType, VarType, RValueType>::OperateOnArray(StackSpace& stack) const
{
	VarType::BaseStorage ret = 0;
	if(OpType == Arithmetic_Multiply || OpType == Arithmetic_Divide)
		ret = 1;

	VM::ArrayVariable arrayvar(stack.GetCurrentTopOfStack());
	VM::EpochVariableTypeID type = arrayvar.GetElementType();
	void* storage = ArrayVariable::GetArrayStorage(arrayvar.GetValue());
	size_t count = arrayvar.GetNumElements();
	stack.Pop(arrayvar.GetStorageSize());

	if(type != VarType::GetStaticType())
		throw ExecutionException("Type mismatch");

	for(size_t i = 0; i < count; ++i)
	{
		VarType var(storage);
		if(OpType == Arithmetic_Add)
			ret = var.GetValue() + ret;
		else if(OpType == Arithmetic_Multiply)
			ret = var.GetValue() * ret;
		else
			throw NotImplementedException("Arithmetic operation not implemented for array mode");

		storage = reinterpret_cast<char*>(storage) + VarType::GetBaseStorageSize();
	}

	return ret;
}

