//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - string operations
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Operations/Variables/StringOps.h"
#include "Virtual Machine/Operations/Containers/ContainerOps.h"
#include "Virtual Machine/Operations/UtilityOps.h"

#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/SelfAware.inl"


using namespace Parser;


//
// Create an operation to concatenate two strings into a single string result
//
VM::OperationPtr ParserState::CreateOperation_Concat()
{
	if(PassedParameterCount.top() == 1)
	{
		if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_OPERATION)
		{
			ReportFatalError("concat() function expects 2 strings or 1 array of strings");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		if(TheStack.back().OperationPointer->GetType(*CurrentScope) != VM::EpochVariableType_Array)
		{
			ReportFatalError("concat() function expects 2 strings or 1 array of strings");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		VM::Operations::ConsArray* consop = dynamic_cast<VM::Operations::ConsArray*>(TheStack.back().OperationPointer);
		if(!consop)
		{
			ReportFatalError("Expected an array here");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		TheStack.pop_back();
		if(consop->GetElementType() != VM::EpochVariableType_String)
		{
			ReportFatalError("concat() function expects 2 strings or 1 array of strings");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		return VM::OperationPtr(new VM::Operations::Concatenate);
	}

	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("concat() function expects 2 strings or 1 array of strings");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry second = TheStack.back();
	TheStack.pop_back();

	StackEntry first = TheStack.back();
	TheStack.pop_back();

	if(first.DetermineEffectiveType(*CurrentScope) != VM::EpochVariableType_String)
	{
		ReportFatalError("concat() function expects 2 strings or 1 array of strings");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}
	if(second.DetermineEffectiveType(*CurrentScope) != VM::EpochVariableType_String)
	{
		ReportFatalError("concat() function expects 2 strings or 1 array of strings");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::Concatenate(first.IsArray(), second.IsArray()));
}


//
// Create an operation to retrieve the length of a given string
//
VM::OperationPtr ParserState::CreateOperation_Length()
{
	if(PassedParameterCount.top() != 1)
	{
		ReportFatalError("This function expects 1 parameter");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}
	StackEntry variable = TheStack.back();

	if(CurrentScope->GetVariableType(variable.StringValue) != VM::EpochVariableType_String)
	{
		ReportFatalError("Function parameter must be a string");
		TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	TheStack.pop_back();
	return VM::OperationPtr(new VM::Operations::Length(ParsedProgram->PoolStaticString(variable.StringValue)));
}

