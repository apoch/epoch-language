//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Built-in arithmetic operations
//

#include "pch.h"

#include "Virtual Machine/Operations/Operators/Arithmetic.h"
#include "Virtual Machine/Routines.inl"


using namespace VM;
using namespace VM::Operations;


//
// Execute a negation operation
//
void Negate::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}

RValuePtr Negate::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	switch(Type)
	{
	case VM::EpochVariableType_Integer:
		{
			IntegerVariable var(stack.GetCurrentTopOfStack());
			IntegerVariable::BaseStorage value = var.GetValue();
			stack.Pop(var.GetStorageSize());
			return RValuePtr(new IntegerRValue(-value));
		}

	case VM::EpochVariableType_Integer16:
		{
			Integer16Variable var(stack.GetCurrentTopOfStack());
			Integer16Variable::BaseStorage value = var.GetValue();
			stack.Pop(var.GetStorageSize());
			return RValuePtr(new Integer16RValue(-value));
		}

	case VM::EpochVariableType_Real:
		{
			RealVariable var(stack.GetCurrentTopOfStack());
			RealVariable::BaseStorage value = var.GetValue();
			stack.Pop(var.GetStorageSize());
			return RValuePtr(new RealRValue(-value));
		}

	default:
		throw ExecutionException("Cannot negate value of this type");
	}
}

