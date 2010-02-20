//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operations for working with strings
//

#include "pch.h"

#include "Virtual Machine/Operations/Variables/StringOps.h"
#include "Virtual Machine/Core Entities/Variables/Variable.h"
#include "Virtual Machine/Core Entities/Variables/StringVariable.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Program.h"


using namespace VM;
using namespace VM::Operations;


//
// Concatenate two strings and return the result
//
RValuePtr Concatenate::ExecuteAndStoreRValue(ExecutionContext& context)
{
	std::wstring ret;

	if(NumParams == 1)
		ret = OperateOnArray(context.Stack);
	else if(NumParams == 2)
	{
		if(FirstIsArray && !SecondIsArray)
		{
			StringVariable var(context.Stack.GetCurrentTopOfStack());
			std::wstring variableval = var.GetValue();
			context.Stack.Pop(StringVariable::GetStorageSize());
			ret = OperateOnArray(context.Stack) + variableval;
		}
		else if(!FirstIsArray && SecondIsArray)
		{
			ret = OperateOnArray(context.Stack);
			StringVariable var(context.Stack.GetCurrentTopOfStack());
			ret = var.GetValue() + ret;
			context.Stack.Pop(StringVariable::GetStorageSize());
		}
		else if(FirstIsArray && SecondIsArray)
		{
			ret = OperateOnArray(context.Stack);
			ret = OperateOnArray(context.Stack) + ret;
		}
		else
		{
			StringVariable twovar(context.Stack.GetCurrentTopOfStack());
			StringVariable onevar(context.Stack.GetOffsetIntoStack(StringVariable::GetStorageSize()));
			ret = onevar.GetValue() + twovar.GetValue();
			context.Stack.Pop(StringVariable::GetStorageSize() * 2);
		}
	}
	else
		throw ExecutionException("Invalid set of parameters");

	return RValuePtr(new StringRValue(ret));
}

void Concatenate::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}

//
// Concatenate all members of an array
//
std::wstring Concatenate::OperateOnArray(StackSpace& stack) const
{
	std::wstring ret;

	IntegerVariable typevar(stack.GetCurrentTopOfStack());
	IntegerVariable countvar(stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
	IntegerVariable::BaseStorage type = typevar.GetValue();
	IntegerVariable::BaseStorage count = countvar.GetValue();
	stack.Pop(IntegerVariable::GetStorageSize() * 2);

	if(type != EpochVariableType_String)
		throw ExecutionException("concat() function expects an array of strings");

	for(Integer32 i = 0; i < count; ++i)
	{
		StringVariable var(stack.GetCurrentTopOfStack());
		ret += var.GetValue();
		stack.Pop(StringVariable::GetStorageSize());
	}

	return ret;
}


//
// Retrieve a string's length
//
RValuePtr Length::ExecuteAndStoreRValue(ExecutionContext& context)
{
	if(context.Scope.GetVariableType(VarName) != EpochVariableType_String)
		throw ExecutionException("length() must be passed a string variable");

	return RValuePtr(new IntegerRValue(static_cast<Integer32>(context.Scope.GetVariableRef<StringVariable>(VarName).GetValue().length())));
}

void Length::ExecuteFast(ExecutionContext& context)
{
	// Nothing to do.
}

