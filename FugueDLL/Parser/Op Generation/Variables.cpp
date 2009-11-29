//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - operations for working with variables
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/Flow/Invoke.h"
#include "Virtual Machine/Operations/UtilityOps.h"

#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Function.h"

#include "Virtual Machine/VMExceptions.h"


using namespace Parser;


//
// Create an operation that writes a value into a variable
//
VM::OperationPtr ParserState::CreateOperation_Assign()
{
	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("assign() function expects 2 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry value = TheStack.back();
	TheStack.pop_back();

	if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("Syntax error - expected variable name");
		TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	std::wstring variable = TheStack.back().StringValue;
	TheStack.pop_back();

	if(CurrentScope->IsConstant(variable))
	{
		ReportFatalError("Cannot assign a new value to a constant");
		TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	switch(value.Type)
	{
	case StackEntry::STACKENTRYTYPE_INTEGER_LITERAL:
		if(CurrentScope->GetVariableType(variable) != VM::EpochVariableType_Integer)
		{
			ReportFatalError("Type mismatch - variable is not an integer");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
		break;

	case StackEntry::STACKENTRYTYPE_BOOLEAN_LITERAL:
		if(CurrentScope->GetVariableType(variable) != VM::EpochVariableType_Boolean)
		{
			ReportFatalError("Type mismatch - variable is not a boolean");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
		break;

	case StackEntry::STACKENTRYTYPE_REAL_LITERAL:
		if(CurrentScope->GetVariableType(variable) != VM::EpochVariableType_Real)
		{
			ReportFatalError("Type mismatch - variable is not a real");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
		break;

	case StackEntry::STACKENTRYTYPE_STRING_LITERAL:
		if(CurrentScope->GetVariableType(variable) != VM::EpochVariableType_String)
		{
			ReportFatalError("Type mismatch - variable is not a string");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
		break;

	case StackEntry::STACKENTRYTYPE_OPERATION:
		{
			VM::EpochVariableTypeID optype = value.OperationPointer->GetType(*CurrentScope);
			if(optype == VM::EpochVariableType_Null)
			{
				ReportFatalError("Cannot perform assignment - the function provided does not return anything");
				return VM::OperationPtr(new VM::Operations::NoOp);
			}
			if(optype != CurrentScope->GetVariableType(variable))
			{
				ReportFatalError("Type mismatch - function return type does not match variable type");
				return VM::OperationPtr(new VM::Operations::NoOp);
			}
			if(optype == VM::EpochVariableType_Tuple)
			{
				VM::Operations::Invoke* invokeop = dynamic_cast<VM::Operations::Invoke*>(value.OperationPointer);
				if(invokeop)
				{
					VM::Function* func = dynamic_cast<VM::Function*>(invokeop->GetFunction());
					if(!func)
					{
						ReportFatalError("Function does not return a suitable tuple type");
						return VM::OperationPtr(new VM::Operations::NoOp);
					}
					IDType tupleid = VM::TupleTrackerClass::LookForMatchingTupleType(func->GetReturns());
					if(tupleid == VM::TupleTrackerClass::InvalidID)
					{
						ReportFatalError("Function declaration appears to be invalid");
						return VM::OperationPtr(new VM::Operations::NoOp);
					}
					if(tupleid != CurrentScope->GetVariableTupleTypeID(variable))
					{
						ReportFatalError("Type mismatch - function return type does not match variable type");
						return VM::OperationPtr(new VM::Operations::NoOp);
					}
				}
				else
				{
					VM::Operations::InvokeIndirect* invokeindirectop = dynamic_cast<VM::Operations::InvokeIndirect*>(value.OperationPointer);
					if(!invokeindirectop)
						throw ParserFailureException("Expected function invocation operation");

					const VM::FunctionSignature& signature = CurrentScope->GetFunctionSignature(invokeindirectop->GetFunctionName());
					IDType tupleid = VM::TupleTrackerClass::LookForMatchingTupleType(signature.GetReturnTypes());
					if(tupleid == VM::TupleTrackerClass::InvalidID)
					{
						ReportFatalError("Function declaration appears to be invalid");
						return VM::OperationPtr(new VM::Operations::NoOp);
					}
					if(tupleid != CurrentScope->GetVariableTupleTypeID(variable))
					{
						ReportFatalError("Type mismatch - function return type does not match variable type");
						return VM::OperationPtr(new VM::Operations::NoOp);
					}
				}
			}
		}
		break;

	case StackEntry::STACKENTRYTYPE_IDENTIFIER:
		if(CurrentScope->GetVariableType(variable) != CurrentScope->GetVariableType(value.StringValue))
		{
			ReportFatalError("Type mismatch - variables must be of the same type");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}
		break;
		
	default:
		throw VM::NotImplementedException("Cannot perform assignment operation for this variable type");
	}

	return VM::OperationPtr(new VM::Operations::AssignValue(ParsedProgram->PoolStaticString(variable)));
}


//
// Create an operation that retrieves the storage size (in bytes) of a variable
//
VM::OperationPtr ParserState::CreateOperation_SizeOf()
{
	if(PassedParameterCount.top() != 1)
	{
		ReportFatalError("This function expects 1 parameter");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry variable = TheStack.back();
	TheStack.pop_back();
	return VM::OperationPtr(new VM::Operations::SizeOf(ParsedProgram->PoolStaticString(variable.StringValue)));
}

