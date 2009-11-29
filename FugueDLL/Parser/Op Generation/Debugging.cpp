//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - debugging and diagnostic operations
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Operations/Debugging.h"
#include "Virtual Machine/Operations/UtilityOps.h"

#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/SelfAware.inl"


using namespace Parser;


//
// Create an operation that pipes output to the debug console
//
// Note that the actual method of presenting this information to
// the user depends on the implementation set up in the UI code.
// The target may be an actual console window, a file, a network
// connection, etc.
//
VM::OperationPtr ParserState::CreateOperation_DebugWrite()
{
	if(PassedParameterCount.top() != 1)
	{
		ReportFatalError("This function expects one parameter");
		TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry variable = TheStack.back();
	if(variable.Type == StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		if(CurrentScope->GetVariableType(variable.StringValue) != VM::EpochVariableType_String)
		{
			ReportFatalError("Parameter must be a string");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
	}
	else if(variable.Type == StackEntry::STACKENTRYTYPE_OPERATION)
	{
		if(!variable.OperationPointer)		// The nested operation failed to compile, so propagate the failure but don't abort the parse
		{
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		if(variable.OperationPointer->GetType(*CurrentScope) != VM::EpochVariableType_String)
		{
			ReportFatalError("Parameter must be a string");
			TheStack.pop_back();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
	}
	else if(variable.Type != StackEntry::STACKENTRYTYPE_STRING_LITERAL)
	{
		ReportFatalError("Parameter must be a string");
		TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	TheStack.pop_back();
	return VM::OperationPtr(new VM::Operations::DebugWriteStringExpression);
}


//
// Create an operation that retrieves input from the user
//
// Note that, like the debug write operation, the actual
// presentation of this input request varies. In general,
// make no assumptions about the origin of input from
// this function!
//
VM::OperationPtr ParserState::CreateOperation_DebugRead()
{
	if(PassedParameterCount.top() != 0)
	{
		ReportFatalError("This function expects no parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::DebugReadStaticString);
}


//
// Create an operation to deliberately crash the virtual machine.
//
// This operation is useful for testing the error-recovery capabilities
// of the virtual machine, as well as ensuring that when errors occur,
// the results are cleaned up as much as possible.
//
VM::OperationPtr ParserState::CreateOperation_DebugCrashVM()
{
	if(PassedParameterCount.top() != 0)
	{
		ReportFatalError("This function expects no parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::DebugCrashVM);
}

