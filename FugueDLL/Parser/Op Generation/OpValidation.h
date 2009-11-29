//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - validation of parameters sent to operations/functions
//

#pragma once


#include "Parser/Parser State Machine/ParserState.h"

#include "Virtual Machine/VMExceptions.h"


//
// Validate the type of a parameter passed to a function
//
// Note that this function may adjust the code to set up correct
// function or reference bindings, in addition to performing its
// type checking duties.
//
template <class ParamClass>
void Parser::ParserState::ValidateOperationParameter(unsigned paramindex, VM::EpochVariableTypeID type, const ParamClass& params)
{
	bool valid = false;
	const StackEntry& paramentry = TheStack.back();
	switch(paramentry.Type)
	{
	case StackEntry::STACKENTRYTYPE_INTEGER_LITERAL:
		valid = (type == VM::EpochVariableType_Integer);
		break;
	case StackEntry::STACKENTRYTYPE_REAL_LITERAL:
		valid = (type == VM::EpochVariableType_Real);
		break;
	case StackEntry::STACKENTRYTYPE_BOOLEAN_LITERAL:
		valid = (type == VM::EpochVariableType_Boolean);
		break;
	case StackEntry::STACKENTRYTYPE_STRING_LITERAL:
		valid = (type == VM::EpochVariableType_String);
		break;
	case StackEntry::STACKENTRYTYPE_IDENTIFIER:
		{
			if(type == VM::EpochVariableType_Structure)
				valid = (params.GetVariableStructureTypeID(paramindex) == CurrentScope->GetVariableStructureTypeID(paramentry.StringValue));
			else if(type == VM::EpochVariableType_Tuple)
				valid = (params.GetVariableTupleTypeID(paramindex) == CurrentScope->GetVariableTupleTypeID(paramentry.StringValue));
			else if(type == VM::EpochVariableType_Function)
				valid = (params.GetFunctionSignature(paramindex).DoesFunctionMatchSignature(CurrentScope->GetFunction(paramentry.StringValue), *CurrentScope));
			else
				valid = (params.GetVariableType(paramindex) == CurrentScope->GetVariableType(paramentry.StringValue));
		}
		break;
	case StackEntry::STACKENTRYTYPE_OPERATION:
		valid = (type == paramentry.OperationPointer->GetType(*CurrentScope));
		break;
	default:
		throw VM::NotImplementedException("Cannot pass this type to a function");
	}

	if(!valid)
		throw SyntaxException("Incorrect parameter type");

	if(params.IsReference(paramindex))
	{
		VM::Operation* op = Blocks.back().TheBlock->GetOperationFromEnd(paramindex + 1, *CurrentScope);
		VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(op);
		if(!pushop)
			throw SyntaxException("Cannot pass this parameter by reference");

		VM::Operation* pushedop = pushop->GetNestedOperation();
		VM::Operations::GetVariableValue* readvarop = dynamic_cast<VM::Operations::GetVariableValue*>(pushedop);
		if(!readvarop)
			throw SyntaxException("Cannot pass this parameter by reference");

		VM::OperationPtr bindop(VM::OperationPtr(new VM::Operations::BindReference(readvarop->GetAssociatedIdentifier())));
		Blocks.back().TheBlock->ReplaceOperationFromEnd(paramindex + 1, bindop, *CurrentScope);
	}
	else if(params.IsFunctionSignature(paramindex))
	{
		VM::Operation* op = Blocks.back().TheBlock->GetOperationFromEnd(paramindex + 1, *CurrentScope);
		VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(op);
		if(!pushop)
			throw ParserFailureException("Expected a function identifier here");

		VM::Operation* pushedop = pushop->GetNestedOperation();
		VM::Operations::GetVariableValue* readvarop = dynamic_cast<VM::Operations::GetVariableValue*>(pushedop);
		if(!readvarop)
			throw ParserFailureException("Expected a function identifier here");

		VM::OperationPtr bindop(VM::OperationPtr(new VM::Operations::BindFunctionReference(readvarop->GetAssociatedIdentifier())));
		Blocks.back().TheBlock->ReplaceOperationFromEnd(paramindex + 1, bindop, *CurrentScope);
	}
}
