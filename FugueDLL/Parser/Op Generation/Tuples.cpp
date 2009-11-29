//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - operations for working with tuples
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Operations/Variables/TupleOps.h"
#include "Virtual Machine/Operations/UtilityOps.h"

#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"


using namespace Parser;


//
// Create an operation to read a value out of a tuple
//
VM::OperationPtr ParserState::CreateOperation_ReadTuple()
{
	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("readtuple() function expects 2 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry member = TheStack.back();
	TheStack.pop_back();

	StackEntry tuplevar = TheStack.back();
	TheStack.pop_back();

	if(member.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER || tuplevar.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("Invalid parameters to readtuple()");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(CurrentScope->GetVariableType(tuplevar.StringValue) != VM::EpochVariableType_Tuple)
	{
		ReportFatalError("Variable is not a tuple type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::ReadTuple(ParsedProgram->PoolStaticString(tuplevar.StringValue), ParsedProgram->PoolStaticString(member.StringValue)));
}


//
// Create an operation to write a value into a tuple
//
VM::OperationPtr ParserState::CreateOperation_AssignTuple()
{
	if(PassedParameterCount.top() != 3)
	{
		ReportFatalError("assigntuple() function expects 3 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry value = TheStack.back();
	TheStack.pop_back();

	StackEntry member = TheStack.back();
	TheStack.pop_back();

	StackEntry tuplevar = TheStack.back();
	TheStack.pop_back();

	if(member.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER || tuplevar.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("Invalid parameters to assigntuple()");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(CurrentScope->GetVariableType(tuplevar.StringValue) != VM::EpochVariableType_Tuple)
	{
		ReportFatalError("Variable is not a tuple type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::AssignTuple(ParsedProgram->PoolStaticString(tuplevar.StringValue), ParsedProgram->PoolStaticString(member.StringValue)));
}

