//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - bitwise and logical operators
//
// These are grouped for their similarity and overlap in names; bitwise
// operations are used on numeric values, and logical operations are
// used for boolean values.
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Operations/Operators/Bitwise.h"
#include "Virtual Machine/Operations/Operators/Logical.h"
#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/UtilityOps.h"
#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/Containers/ContainerOps.h"
#include "Virtual Machine/SelfAware.inl"



using namespace Parser;


//
// Create a logical or bitwise OR operation
//
VM::OperationPtr ParserState::CreateOperation_Or()
{
	if(PassedParameterCount.top() == 1)
		return ParseLogicalOpArrayOnly<VM::Operations::LogicalOr, VM::Operations::BitwiseOr>();

	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("or() function expects 2 parameters");
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
		ReportFatalError("Both parameters to the or() function must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(firsttype == VM::EpochVariableType_Boolean)
		return VM::OperationPtr(ParseLogicalOp<VM::Operations::LogicalOr, VM::Operations::PushBooleanLiteral, VM::Operations::BooleanConstant>(first.IsArray(), second.IsArray()));
	else if(firsttype == VM::EpochVariableType_Integer)
		return VM::OperationPtr(ParseBitwiseOp<VM::Operations::BitwiseOr, VM::Operations::PushIntegerLiteral, VM::Operations::IntegerConstant>(first.IsArray(), second.IsArray()));
	else if(firsttype == VM::EpochVariableType_Integer16)
		return VM::OperationPtr(ParseBitwiseOp<VM::Operations::BitwiseOr, VM::Operations::PushInteger16Literal, VM::Operations::Integer16Constant>(first.IsArray(), second.IsArray()));

	ReportFatalError("Couldn't determine whether or() should be logical or bitwise");
	return VM::OperationPtr(new VM::Operations::NoOp);
}


//
// Create a logical or bitwise AND operation
//
VM::OperationPtr ParserState::CreateOperation_And()
{
	if(PassedParameterCount.top() == 1)
		return ParseLogicalOpArrayOnly<VM::Operations::LogicalAnd, VM::Operations::BitwiseAnd>();

	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("and() function expects 2 parameters");
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
		ReportFatalError("Both parameters to the and() function must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(firsttype == VM::EpochVariableType_Boolean)
		return VM::OperationPtr(ParseLogicalOp<VM::Operations::LogicalAnd, VM::Operations::PushBooleanLiteral, VM::Operations::BooleanConstant>(first.IsArray(), second.IsArray()));
	else if(firsttype == VM::EpochVariableType_Integer)
		return VM::OperationPtr(ParseBitwiseOp<VM::Operations::BitwiseAnd, VM::Operations::PushIntegerLiteral, VM::Operations::IntegerConstant>(first.IsArray(), second.IsArray()));
	else if(firsttype == VM::EpochVariableType_Integer16)
		return VM::OperationPtr(ParseBitwiseOp<VM::Operations::BitwiseAnd, VM::Operations::PushInteger16Literal, VM::Operations::Integer16Constant>(first.IsArray(), second.IsArray()));

	ReportFatalError("Couldn't determine whether and() should be logical or bitwise");
	return VM::OperationPtr(new VM::Operations::NoOp);
}


//
// Create a logical or bitwise XOR operation
//
VM::OperationPtr ParserState::CreateOperation_Xor()
{
	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("xor() function expects 2 parameters");
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
		ReportFatalError("Both parameters to the xor() function must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(firsttype == VM::EpochVariableType_Boolean)
		return VM::OperationPtr(new VM::Operations::LogicalXor);
	else if(firsttype == VM::EpochVariableType_Integer || firsttype == VM::EpochVariableType_Integer16)
		return VM::OperationPtr(new VM::Operations::BitwiseXor(firsttype));

	ReportFatalError("Couldn't determine whether xor() should be logical or bitwise");
	return VM::OperationPtr(new VM::Operations::NoOp);
}


//
// Create a logical or bitwise NOT operation
//
VM::OperationPtr ParserState::CreateOperation_Not()
{
	if(PassedParameterCount.top() != 1)
	{
		ReportFatalError("not() function expects 1 parameter");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry param = TheStack.back();
	TheStack.pop_back();

	VM::EpochVariableTypeID paramtype = param.DetermineEffectiveType(*CurrentScope);

	if(paramtype == VM::EpochVariableType_Boolean)
		return VM::OperationPtr(new VM::Operations::LogicalNot);
	else if(paramtype == VM::EpochVariableType_Integer || paramtype == VM::EpochVariableType_Integer16)
		return VM::OperationPtr(new VM::Operations::BitwiseNot(paramtype));

	ReportFatalError("Couldn't determine whether not() should be logical or bitwise");
	return VM::OperationPtr(new VM::Operations::NoOp);
}

