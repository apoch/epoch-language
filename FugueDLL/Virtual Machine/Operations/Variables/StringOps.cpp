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
RValuePtr Concatenate::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	std::wstring ret;

	if(NumParams == 1)
		ret = OperateOnList(stack);
	else if(NumParams == 2)
	{
		if(FirstIsList && !SecondIsList)
		{
			StringVariable var(stack.GetCurrentTopOfStack());
			std::wstring variableval = var.GetValue();
			stack.Pop(StringVariable::GetStorageSize());
			ret = OperateOnList(stack) + variableval;
		}
		else if(!FirstIsList && SecondIsList)
		{
			ret = OperateOnList(stack);
			StringVariable var(stack.GetCurrentTopOfStack());
			ret = var.GetValue() + ret;
			stack.Pop(StringVariable::GetStorageSize());
		}
		else if(FirstIsList && SecondIsList)
		{
			ret = OperateOnList(stack);
			ret = OperateOnList(stack) + ret;
		}
		else
		{
			StringVariable twovar(stack.GetCurrentTopOfStack());
			StringVariable onevar(stack.GetOffsetIntoStack(StringVariable::GetStorageSize()));
			ret = onevar.GetValue() + twovar.GetValue();
			stack.Pop(StringVariable::GetStorageSize() * 2);
		}
	}
	else
		throw ExecutionException("Invalid set of parameters");

	return RValuePtr(new StringRValue(ret));
}

void Concatenate::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}

//
// Concatenate all members of a list
//
std::wstring Concatenate::OperateOnList(StackSpace& stack) const
{
	std::wstring ret;

	IntegerVariable typevar(stack.GetCurrentTopOfStack());
	IntegerVariable countvar(stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
	IntegerVariable::BaseStorage type = typevar.GetValue();
	IntegerVariable::BaseStorage count = countvar.GetValue();
	stack.Pop(IntegerVariable::GetStorageSize() * 2);

	if(type != EpochVariableType_String)
		throw ExecutionException("concat() function expects a list of strings");

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
RValuePtr Length::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	if(scope.GetVariableType(VarName) != EpochVariableType_String)
		throw ExecutionException("length() must be passed a string variable");

	return RValuePtr(new IntegerRValue(static_cast<Integer32>(scope.GetVariableRef<StringVariable>(VarName).GetValue().length())));
}

void Length::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	// Nothing to do.
}

