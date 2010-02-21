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


Traverser::Payload Comparator::GetNodeTraversalPayload(const VM::ScopeDescription* scope) const
{
	Traverser::Payload payload;
	payload.SetValue(Type);
	payload.ParameterCount = GetNumParameters(*scope);
	return payload;
}


//
// Test if two values are equal
//
RValuePtr IsEqual::ExecuteAndStoreRValue(ExecutionContext& context)
{
	switch(Type)
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot compare null values");

	case EpochVariableType_Integer:
		{
			IntegerVariable val2(context.Stack.GetCurrentTopOfStack());
			IntegerVariable val1(context.Stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
			bool ret = (val1.GetValue() == val2.GetValue());
			context.Stack.Pop(IntegerVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Real:
		{
			RealVariable val2(context.Stack.GetCurrentTopOfStack());
			RealVariable val1(context.Stack.GetOffsetIntoStack(RealVariable::GetStorageSize()));
			bool ret = (val1.GetValue() == val2.GetValue());
			context.Stack.Pop(RealVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Boolean:
		{
			BooleanVariable val2(context.Stack.GetCurrentTopOfStack());
			BooleanVariable val1(context.Stack.GetOffsetIntoStack(BooleanVariable::GetStorageSize()));
			bool ret = (val1.GetValue() == val2.GetValue());
			context.Stack.Pop(BooleanVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_String:
		{
			StringVariable val2(context.Stack.GetCurrentTopOfStack());
			StringVariable val1(context.Stack.GetOffsetIntoStack(StringVariable::GetStorageSize()));
			bool ret = (val1.GetValue() == val2.GetValue());
			context.Stack.Pop(StringVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Tuple:
		{
			TupleVariable val2(context.Stack.GetCurrentTopOfStack());
			TupleVariable val1(context.Stack.GetOffsetIntoStack(val2.GetStorageSize()));
			IDType id1 = val1.GetValue();
			IDType id2 = val2.GetValue();
			
			bool ret = false;
			if(id1 == id2)
			{
				ret = true;

				const TupleType& type = context.Scope.GetTupleType(id1);
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
			context.Stack.Pop(storage1 + storage2);
			return RValuePtr(new BooleanRValue(ret));
		}

	default:
		throw NotImplementedException("Cannot check these values for equality");
	}
}

void IsEqual::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}

//
// Test if two values are not equal
//
RValuePtr IsNotEqual::ExecuteAndStoreRValue(ExecutionContext& context)
{
	switch(Type)
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot compare null values");

	case EpochVariableType_Integer:
		{
			IntegerVariable val2(context.Stack.GetCurrentTopOfStack());
			IntegerVariable val1(context.Stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
			bool ret = (val1.GetValue() != val2.GetValue());
			context.Stack.Pop(IntegerVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Real:
		{
			RealVariable val2(context.Stack.GetCurrentTopOfStack());
			RealVariable val1(context.Stack.GetOffsetIntoStack(RealVariable::GetStorageSize()));
			bool ret = (val1.GetValue() != val2.GetValue());
			context.Stack.Pop(RealVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Boolean:
		{
			BooleanVariable val2(context.Stack.GetCurrentTopOfStack());
			BooleanVariable val1(context.Stack.GetOffsetIntoStack(BooleanVariable::GetStorageSize()));
			bool ret = (val1.GetValue() != val2.GetValue());
			context.Stack.Pop(BooleanVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_String:
		{
			StringVariable val2(context.Stack.GetCurrentTopOfStack());
			StringVariable val1(context.Stack.GetOffsetIntoStack(StringVariable::GetStorageSize()));
			bool ret = (val1.GetValue() != val2.GetValue());
			context.Stack.Pop(StringVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Tuple:
		{
			TupleVariable val2(context.Stack.GetCurrentTopOfStack());
			TupleVariable val1(context.Stack.GetOffsetIntoStack(val2.GetStorageSize()));
			IDType id1 = val1.GetValue();
			IDType id2 = val2.GetValue();
			
			bool ret = true;
			if(id1 == id2)
			{
				ret = false;

				const TupleType& type = context.Scope.GetTupleType(id1);
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
			context.Stack.Pop(storage1 + storage2);
			return RValuePtr(new BooleanRValue(ret));
		}
	default:
		throw NotImplementedException("Cannot test these values for inequality");
	}
}

void IsNotEqual::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}

//
// Test if one value is greater than another
//
RValuePtr IsGreater::ExecuteAndStoreRValue(ExecutionContext& context)
{
	switch(Type)
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot compare null values");

	case EpochVariableType_Integer:
		{
			IntegerVariable val2(context.Stack.GetCurrentTopOfStack());
			IntegerVariable val1(context.Stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
			bool ret = (val1.GetValue() > val2.GetValue());
			context.Stack.Pop(IntegerVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Real:
		{
			RealVariable val2(context.Stack.GetCurrentTopOfStack());
			RealVariable val1(context.Stack.GetOffsetIntoStack(RealVariable::GetStorageSize()));
			bool ret = (val1.GetValue() > val2.GetValue());
			context.Stack.Pop(RealVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	default:
		throw ExecutionException("Invalid types for greater() parameters");
	}
}

void IsGreater::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}


//
// Test if one value is greater than or equal to another
//
RValuePtr IsGreaterOrEqual::ExecuteAndStoreRValue(ExecutionContext& context)
{
	switch(Type)
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot compare null values");

	case EpochVariableType_Integer:
		{
			IntegerVariable val2(context.Stack.GetCurrentTopOfStack());
			IntegerVariable val1(context.Stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
			bool ret = (val1.GetValue() >= val2.GetValue());
			context.Stack.Pop(IntegerVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Real:
		{
			RealVariable val2(context.Stack.GetCurrentTopOfStack());
			RealVariable val1(context.Stack.GetOffsetIntoStack(RealVariable::GetStorageSize()));
			bool ret = (val1.GetValue() >= val2.GetValue());
			context.Stack.Pop(RealVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	default:
		throw ExecutionException("Invalid types for greaterequal() parameters");
	}
}

void IsGreaterOrEqual::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}


//
// Test if one value is less than another
//
RValuePtr IsLesser::ExecuteAndStoreRValue(ExecutionContext& context)
{
	switch(Type)
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot compare null values");

	case EpochVariableType_Integer:
		{
			IntegerVariable val2(context.Stack.GetCurrentTopOfStack());
			IntegerVariable val1(context.Stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
			bool ret = (val1.GetValue() < val2.GetValue());
			context.Stack.Pop(IntegerVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Real:
		{
			RealVariable val2(context.Stack.GetCurrentTopOfStack());
			RealVariable val1(context.Stack.GetOffsetIntoStack(RealVariable::GetStorageSize()));
			bool ret = (val1.GetValue() < val2.GetValue());
			context.Stack.Pop(RealVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	default:
		throw ExecutionException("Invalid types for less() parameters");
	}
}

void IsLesser::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}

//
// Test if one value is less than or equal to another
//
RValuePtr IsLesserOrEqual::ExecuteAndStoreRValue(ExecutionContext& context)
{
	switch(Type)
	{
	case EpochVariableType_Null:
		throw ExecutionException("Cannot compare null values");

	case EpochVariableType_Integer:
		{
			IntegerVariable val2(context.Stack.GetCurrentTopOfStack());
			IntegerVariable val1(context.Stack.GetOffsetIntoStack(IntegerVariable::GetStorageSize()));
			bool ret = (val1.GetValue() <= val2.GetValue());
			context.Stack.Pop(IntegerVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	case EpochVariableType_Real:
		{
			RealVariable val2(context.Stack.GetCurrentTopOfStack());
			RealVariable val1(context.Stack.GetOffsetIntoStack(RealVariable::GetStorageSize()));
			bool ret = (val1.GetValue() <= val2.GetValue());
			context.Stack.Pop(RealVariable::GetStorageSize() * 2);
			return RValuePtr(new BooleanRValue(ret));
		}
	default:
		throw ExecutionException("Invalid types for lessequal() parameters");
	}
}

void IsLesserOrEqual::ExecuteFast(ExecutionContext& context)
{
	ExecuteAndStoreRValue(context);
}
