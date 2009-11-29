//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - comparison operators
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Operations/Operators/Comparison.h"
#include "Virtual Machine/Operations/UtilityOps.h"


using namespace Parser;


//
// Create an equality test operation
//
VM::OperationPtr ParserState::CreateOperation_Equal()
{
	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("equal() function expects 2 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry second = TheStack.back();
	TheStack.pop_back();

	StackEntry first = TheStack.back();
	TheStack.pop_back();

	VM::EpochVariableTypeID firsttype = first.DetermineEffectiveType(*CurrentScope);
	VM::EpochVariableTypeID secondtype = second.DetermineEffectiveType(*CurrentScope);

	if(firsttype != secondtype)
	{
		ReportFatalError("Both parameters to the equal() function must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::IsEqual(firsttype));
}


//
// Create an inequality test operation
//
VM::OperationPtr ParserState::CreateOperation_NotEqual()
{
	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("notequal() function expects 2 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry second = TheStack.back();
	TheStack.pop_back();

	StackEntry first = TheStack.back();
	TheStack.pop_back();

	VM::EpochVariableTypeID firsttype = first.DetermineEffectiveType(*CurrentScope);
	VM::EpochVariableTypeID secondtype = second.DetermineEffectiveType(*CurrentScope);

	if(firsttype != secondtype)
	{
		ReportFatalError("Both parameters to the notequal() function must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::IsNotEqual(firsttype));
}


//
// Create a less-than test operation
//
VM::OperationPtr ParserState::CreateOperation_Less()
{
	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("less() function expects 2 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry second = TheStack.back();
	TheStack.pop_back();

	StackEntry first = TheStack.back();
	TheStack.pop_back();

	VM::EpochVariableTypeID firsttype = first.DetermineEffectiveType(*CurrentScope);
	VM::EpochVariableTypeID secondtype = second.DetermineEffectiveType(*CurrentScope);

	if(firsttype != secondtype)
	{
		ReportFatalError("Both parameters to the less() function must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::IsLesser(firsttype));
}


//
// Create a greater-than test operation
//
VM::OperationPtr ParserState::CreateOperation_Greater()
{
	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("greater() function expects 2 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry second = TheStack.back();
	TheStack.pop_back();

	StackEntry first = TheStack.back();
	TheStack.pop_back();

	VM::EpochVariableTypeID firsttype = first.DetermineEffectiveType(*CurrentScope);
	VM::EpochVariableTypeID secondtype = second.DetermineEffectiveType(*CurrentScope);

	if(firsttype != secondtype)
	{
		ReportFatalError("Both parameters to the greater() function must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::IsGreater(firsttype));
}


//
// Create a less-than-or-equal-to test operation
//
VM::OperationPtr ParserState::CreateOperation_LessEqual()
{
	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("lessequal() function expects 2 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry second = TheStack.back();
	TheStack.pop_back();

	StackEntry first = TheStack.back();
	TheStack.pop_back();

	VM::EpochVariableTypeID firsttype = first.DetermineEffectiveType(*CurrentScope);
	VM::EpochVariableTypeID secondtype = second.DetermineEffectiveType(*CurrentScope);

	if(firsttype != secondtype)
	{
		ReportFatalError("Both parameters to the lessequal() function must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::IsLesserOrEqual(firsttype));
}


//
// Create a greater-than-or-equal-to test operation
//
VM::OperationPtr ParserState::CreateOperation_GreaterEqual()
{
	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("greaterequal() function expects 2 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry second = TheStack.back();
	TheStack.pop_back();

	StackEntry first = TheStack.back();
	TheStack.pop_back();

	VM::EpochVariableTypeID firsttype = first.DetermineEffectiveType(*CurrentScope);
	VM::EpochVariableTypeID secondtype = second.DetermineEffectiveType(*CurrentScope);

	if(firsttype != secondtype)
	{
		ReportFatalError("Both parameters to the greaterequal() function must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::IsGreaterOrEqual(firsttype));
}
