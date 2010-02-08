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
void Negate::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}

RValuePtr Negate::ExecuteAndStoreRValue(ExecutionContext& context)
{
	switch(Type)
	{
	case VM::EpochVariableType_Integer:
		{
			IntegerVariable var(context.Stack.GetCurrentTopOfStack());
			IntegerVariable::BaseStorage value = var.GetValue();
			context.Stack.Pop(var.GetStorageSize());
			return RValuePtr(new IntegerRValue(-value));
		}

	case VM::EpochVariableType_Integer16:
		{
			Integer16Variable var(context.Stack.GetCurrentTopOfStack());
			Integer16Variable::BaseStorage value = var.GetValue();
			context.Stack.Pop(var.GetStorageSize());
			return RValuePtr(new Integer16RValue(-value));
		}

	case VM::EpochVariableType_Real:
		{
			RealVariable var(context.Stack.GetCurrentTopOfStack());
			RealVariable::BaseStorage value = var.GetValue();
			context.Stack.Pop(var.GetStorageSize());
			return RValuePtr(new RealRValue(-value));
		}

	default:
		throw ExecutionException("Cannot negate value of this type");
	}
}

