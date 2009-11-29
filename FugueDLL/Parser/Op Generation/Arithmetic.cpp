//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - arithmetic and similar operations
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Operations/Operators/Arithmetic.h"
#include "Virtual Machine/Operations/Containers/ContainerOps.h"
#include "Virtual Machine/Operations/UtilityOps.h"


using namespace Parser;


//
// Create an addition operation
//
VM::OperationPtr ParserState::CreateOperation_Add()
{
	if(PassedParameterCount.top() == 1)
	{
		if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_OPERATION)
		{
			ReportFatalError("add() function expects 2 parameters or 1 list");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		if(TheStack.back().OperationPointer->GetType(*CurrentScope) != VM::EpochVariableType_List)
		{
			ReportFatalError("add() function expects 2 parameters or 1 list");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		VM::Operations::ConsList* consop = dynamic_cast<VM::Operations::ConsList*>(TheStack.back().OperationPointer);
		if(!consop)
		{
			ReportFatalError("Expected a list here");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		TheStack.pop_back();
		if(consop->GetElementType() == VM::EpochVariableType_Integer)
			return VM::OperationPtr(new VM::Operations::SumIntegers);
		else if(consop->GetElementType() == VM::EpochVariableType_Integer16)
			return VM::OperationPtr(new VM::Operations::SumInteger16s);
		else if(consop->GetElementType() == VM::EpochVariableType_Real)
			return VM::OperationPtr(new VM::Operations::SumReals);
		else
		{
			ReportFatalError("Cannot add() a list of this type of element");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
	}

	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("add() function expects 2 parameters or 1 list");
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
		ReportFatalError("Parameters to add() must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(firsttype == VM::EpochVariableType_Integer)
		return VM::OperationPtr(new VM::Operations::SumIntegers(first.IsList(), second.IsList()));
	else if(firsttype == VM::EpochVariableType_Integer16)
		return VM::OperationPtr(new VM::Operations::SumInteger16s(first.IsList(), second.IsList()));
	else if(firsttype == VM::EpochVariableType_Real)
		return VM::OperationPtr(new VM::Operations::SumReals(first.IsList(), second.IsList()));

	ReportFatalError("add() cannot use parameters of this type");
	return VM::OperationPtr(new VM::Operations::NoOp);
}


//
// Create a subtraction operation
//
VM::OperationPtr ParserState::CreateOperation_Subtract()
{
	if(PassedParameterCount.top() == 1)
	{
		if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_OPERATION)
		{
			ReportFatalError("subtract() function expects 2 parameters or 1 list");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		if(TheStack.back().OperationPointer->GetType(*CurrentScope) != VM::EpochVariableType_List)
		{
			ReportFatalError("subtract() function expects 2 parameters or 1 list");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		VM::Operations::ConsList* consop = dynamic_cast<VM::Operations::ConsList*>(TheStack.back().OperationPointer);
		if(!consop)
		{
			ReportFatalError("Expected a list constructor here");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		TheStack.pop_back();
		if(consop->GetElementType() == VM::EpochVariableType_Integer)
			return VM::OperationPtr(new VM::Operations::SubtractIntegers);
		else if(consop->GetElementType() == VM::EpochVariableType_Integer16)
			return VM::OperationPtr(new VM::Operations::SubtractInteger16s);
		else if(consop->GetElementType() == VM::EpochVariableType_Real)
			return VM::OperationPtr(new VM::Operations::SubtractReals);
		else
		{
			ReportFatalError("Cannot subtract() a list of this type of element");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
	}

	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("subtract() expects 2 parameters");
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
		ReportFatalError("Parameters to subtract() must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(firsttype == VM::EpochVariableType_Integer)
		return VM::OperationPtr(new VM::Operations::SubtractIntegers(first.IsList(), second.IsList()));
	else if(firsttype == VM::EpochVariableType_Integer16)
		return VM::OperationPtr(new VM::Operations::SubtractInteger16s(first.IsList(), second.IsList()));
	else if(firsttype == VM::EpochVariableType_Real)
		return VM::OperationPtr(new VM::Operations::SubtractReals(first.IsList(), second.IsList()));

	ReportFatalError("subtract() cannot use parameters of this type");
	return VM::OperationPtr(new VM::Operations::NoOp);
}


//
// Create a multiplication operation
//
VM::OperationPtr ParserState::CreateOperation_Multiply()
{
	if(PassedParameterCount.top() == 1)
	{
		if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_OPERATION)
		{
			ReportFatalError("multiply() function expects 2 parameters or 1 list");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		if(TheStack.back().OperationPointer->GetType(*CurrentScope) != VM::EpochVariableType_List)
		{
			ReportFatalError("multiply() function expects 2 parameters or 1 list");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		VM::Operations::ConsList* consop = dynamic_cast<VM::Operations::ConsList*>(TheStack.back().OperationPointer);
		if(!consop)
		{
			ReportFatalError("Expected a list here");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		TheStack.pop_back();
		if(consop->GetElementType() == VM::EpochVariableType_Integer)
			return VM::OperationPtr(new VM::Operations::MultiplyIntegers);
		else if(consop->GetElementType() == VM::EpochVariableType_Integer16)
			return VM::OperationPtr(new VM::Operations::MultiplyInteger16s);
		else if(consop->GetElementType() == VM::EpochVariableType_Real)
			return VM::OperationPtr(new VM::Operations::MultiplyReals);
		else
		{
			ReportFatalError("Cannot multiply() a list of this type of element");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
	}

	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("multiply() function expects 2 parameters");
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
		ReportFatalError("Parameters to multiply() must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(firsttype == VM::EpochVariableType_Integer)
		return VM::OperationPtr(new VM::Operations::MultiplyIntegers(first.IsList(), second.IsList()));
	else if(firsttype == VM::EpochVariableType_Integer16)
		return VM::OperationPtr(new VM::Operations::MultiplyInteger16s(first.IsList(), second.IsList()));
	else if(firsttype == VM::EpochVariableType_Real)
		return VM::OperationPtr(new VM::Operations::MultiplyReals(first.IsList(), second.IsList()));

	ReportFatalError("multiply() cannot use parameters of this type");
	return VM::OperationPtr(new VM::Operations::NoOp);
}


//
// Create a division operation
//
VM::OperationPtr ParserState::CreateOperation_Divide()
{
	if(PassedParameterCount.top() == 1)
	{
		if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_OPERATION)
		{
			ReportFatalError("divide() function expects 2 parameters or 1 list");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		if(TheStack.back().OperationPointer->GetType(*CurrentScope) != VM::EpochVariableType_List)
		{
			ReportFatalError("divide() function expects 2 parameters or 1 list");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		VM::Operations::ConsList* consop = dynamic_cast<VM::Operations::ConsList*>(TheStack.back().OperationPointer);
		if(!consop)
		{
			ReportFatalError("Expected a list here");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		TheStack.pop_back();
		if(consop->GetElementType() == VM::EpochVariableType_Integer)
			return VM::OperationPtr(new VM::Operations::DivideIntegers);
		else if(consop->GetElementType() == VM::EpochVariableType_Integer16)
			return VM::OperationPtr(new VM::Operations::DivideInteger16s);
		else if(consop->GetElementType() == VM::EpochVariableType_Real)
			return VM::OperationPtr(new VM::Operations::DivideReals);
		else
		{
			ReportFatalError("Cannot divide() a list of this type of element");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
	}

	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("divide() function expects 2 parameters");
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
		ReportFatalError("Parameters to divide() must be of the same type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(firsttype == VM::EpochVariableType_Integer)
		return VM::OperationPtr(new VM::Operations::DivideIntegers(first.IsList(), second.IsList()));
	else if(firsttype == VM::EpochVariableType_Integer16)
		return VM::OperationPtr(new VM::Operations::DivideInteger16s(first.IsList(), second.IsList()));
	else if(firsttype == VM::EpochVariableType_Real)
		return VM::OperationPtr(new VM::Operations::DivideReals(first.IsList(), second.IsList()));

	ReportFatalError("divide() cannot use parameters of this type");
	return VM::OperationPtr(new VM::Operations::NoOp);
}