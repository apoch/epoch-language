//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Built in comparison operators
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Operations/Operators/Comparison.h"
#include "Virtual Machine/Types Management/Typecasts.h"
#include "Virtual Machine/Core Entities/Variables/StringVariable.h"
#include "Virtual Machine/Core Entities/Variables/TupleVariable.h"
#include "Virtual Machine/VMExceptions.h"


using namespace VM;
using namespace VM::Operations;


Traverser::Payload Comparator::GetNodeTraversalPayload() const
{
	Traverser::Payload payload;
	payload.SetValue(Type);
	return payload;
}


//
// Test if two values are equal
//
RValuePtr IsEqual::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	switch(Type)
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot compare null values");

	case EpochVariableType_Integer:
		{
			IntegerVariable val2(stack.GetCurrentTopOfStack());
			IntegerVariable val1(stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
			bool ret = (val1.GetValue() == val2.GetValue());
			stack.Pop(IntegerVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Real:
		{
			RealVariable val2(stack.GetCurrentTopOfStack());
			RealVariable val1(stack.GetOffsetIntoStack(RealVariable::GetStorageSize()));
			bool ret = (val1.GetValue() == val2.GetValue());
			stack.Pop(RealVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Boolean:
		{
			BooleanVariable val2(stack.GetCurrentTopOfStack());
			BooleanVariable val1(stack.GetOffsetIntoStack(BooleanVariable::GetStorageSize()));
			bool ret = (val1.GetValue() == val2.GetValue());
			stack.Pop(BooleanVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_String:
		{
			StringVariable val2(stack.GetCurrentTopOfStack());
			StringVariable val1(stack.GetOffsetIntoStack(StringVariable::GetStorageSize()));
			bool ret = (val1.GetValue() == val2.GetValue());
			stack.Pop(StringVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Tuple:
		{
			TupleVariable val2(stack.GetCurrentTopOfStack());
			TupleVariable val1(stack.GetOffsetIntoStack(val2.GetStorageSize()));
			IDType id1 = val1.GetValue();
			IDType id2 = val2.GetValue();
			
			bool ret = false;
			if(id1 == id2)
			{
				ret = true;

				const TupleType& type = scope.GetTupleType(id1);
				const std::vector<std::wstring>& members = type.GetMemberOrder();
				for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
				{
					if((*val1.ReadMember(*iter)) != (*val2.ReadMember(*iter)))
					{
						ret = false;
						break;
					}
				}
			}

			size_t storage1 = val1.GetStorageSize();
			size_t storage2 = val2.GetStorageSize();
			stack.Pop(storage1 + storage2);
			return RValuePtr(new BooleanRValue(ret));
		}

	default:
		throw NotImplementedException("Cannot check these values for equality");
	}
}

void IsEqual::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}

//
// Test if two values are not equal
//
RValuePtr IsNotEqual::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	switch(Type)
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot compare null values");

	case EpochVariableType_Integer:
		{
			IntegerVariable val2(stack.GetCurrentTopOfStack());
			IntegerVariable val1(stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
			bool ret = (val1.GetValue() != val2.GetValue());
			stack.Pop(IntegerVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Real:
		{
			RealVariable val2(stack.GetCurrentTopOfStack());
			RealVariable val1(stack.GetOffsetIntoStack(RealVariable::GetStorageSize()));
			bool ret = (val1.GetValue() != val2.GetValue());
			stack.Pop(RealVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Boolean:
		{
			BooleanVariable val2(stack.GetCurrentTopOfStack());
			BooleanVariable val1(stack.GetOffsetIntoStack(BooleanVariable::GetStorageSize()));
			bool ret = (val1.GetValue() != val2.GetValue());
			stack.Pop(BooleanVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_String:
		{
			StringVariable val2(stack.GetCurrentTopOfStack());
			StringVariable val1(stack.GetOffsetIntoStack(StringVariable::GetStorageSize()));
			bool ret = (val1.GetValue() != val2.GetValue());
			stack.Pop(StringVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Tuple:
		{
			TupleVariable val2(stack.GetCurrentTopOfStack());
			TupleVariable val1(stack.GetOffsetIntoStack(val2.GetStorageSize()));
			IDType id1 = val1.GetValue();
			IDType id2 = val2.GetValue();
			
			bool ret = true;
			if(id1 == id2)
			{
				ret = false;

				const TupleType& type = scope.GetTupleType(id1);
				const std::vector<std::wstring>& members = type.GetMemberOrder();
				for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
				{
					if((*val1.ReadMember(*iter)) != (*val2.ReadMember(*iter)))
					{
						ret = true;
						break;
					}
				}
			}

			size_t storage1 = val1.GetStorageSize();
			size_t storage2 = val2.GetStorageSize();
			stack.Pop(storage1 + storage2);
			return RValuePtr(new BooleanRValue(ret));
		}
	default:
		throw NotImplementedException("Cannot test these values for inequality");
	}
}

void IsNotEqual::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}

//
// Test if one value is greater than another
//
RValuePtr IsGreater::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	switch(Type)
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot compare null values");

	case EpochVariableType_Integer:
		{
			IntegerVariable val2(stack.GetCurrentTopOfStack());
			IntegerVariable val1(stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
			bool ret = (val1.GetValue() > val2.GetValue());
			stack.Pop(IntegerVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Real:
		{
			RealVariable val2(stack.GetCurrentTopOfStack());
			RealVariable val1(stack.GetOffsetIntoStack(RealVariable::GetStorageSize()));
			bool ret = (val1.GetValue() > val2.GetValue());
			stack.Pop(RealVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	default:
		throw ExecutionException("Invalid types for greater() parameters");
	}
}

void IsGreater::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}


//
// Test if one value is greater than or equal to another
//
RValuePtr IsGreaterOrEqual::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	switch(Type)
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot compare null values");

	case EpochVariableType_Integer:
		{
			IntegerVariable val2(stack.GetCurrentTopOfStack());
			IntegerVariable val1(stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
			bool ret = (val1.GetValue() >= val2.GetValue());
			stack.Pop(IntegerVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Real:
		{
			RealVariable val2(stack.GetCurrentTopOfStack());
			RealVariable val1(stack.GetOffsetIntoStack(RealVariable::GetStorageSize()));
			bool ret = (val1.GetValue() >= val2.GetValue());
			stack.Pop(RealVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	default:
		throw ExecutionException("Invalid types for greaterequal() parameters");
	}
}

void IsGreaterOrEqual::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}


//
// Test if one value is less than another
//
RValuePtr IsLesser::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	switch(Type)
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot compare null values");

	case EpochVariableType_Integer:
		{
			IntegerVariable val2(stack.GetCurrentTopOfStack());
			IntegerVariable val1(stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
			bool ret = (val1.GetValue() < val2.GetValue());
			stack.Pop(IntegerVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Real:
		{
			RealVariable val2(stack.GetCurrentTopOfStack());
			RealVariable val1(stack.GetOffsetIntoStack(RealVariable::GetStorageSize()));
			bool ret = (val1.GetValue() < val2.GetValue());
			stack.Pop(RealVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	default:
		throw ExecutionException("Invalid types for less() parameters");
	}
}

void IsLesser::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}

//
// Test if one value is less than or equal to another
//
RValuePtr IsLesserOrEqual::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	switch(Type)
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot compare null values");

	case EpochVariableType_Integer:
		{
			IntegerVariable val2(stack.GetCurrentTopOfStack());
			IntegerVariable val1(stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
			bool ret = (val1.GetValue() <= val2.GetValue());
			stack.Pop(IntegerVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Real:
		{
			RealVariable val2(stack.GetCurrentTopOfStack());
			RealVariable val1(stack.GetOffsetIntoStack(RealVariable::GetStorageSize()));
			bool ret = (val1.GetValue() <= val2.GetValue());
			stack.Pop(RealVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	default:
		throw ExecutionException("Invalid types for lessequal() parameters");
	}
}

void IsLesserOrEqual::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteAndStoreRValue(scope, stack, flowresult);
}
