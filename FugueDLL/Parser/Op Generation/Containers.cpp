//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - built in data containers
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"

#include "Virtual Machine/Operations/Containers/ContainerOps.h"
#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/UtilityOps.h"


using namespace Parser;



//
// Create an operation that constructs an array variable
//
VM::OperationPtr ParserState::CreateOperation_ConsArray()
{
	size_t paramcount = PassedParameterCount.top();
	VM::EpochVariableTypeID elementtype = VM::EpochVariableType_Error;

	if(paramcount >= 1)
		elementtype = TheStack.back().DetermineEffectiveType(*CurrentScope);

	bool complained = false;
	for(size_t i = 0; i < paramcount; ++i)
	{
		if(TheStack.back().DetermineEffectiveType(*CurrentScope) != elementtype)
		{
			if(!complained)
				ReportFatalError("All elements in an array must be of the same type");
			complained = true;
		}

		TheStack.pop_back();
	}

	if(complained)
		return VM::OperationPtr(new VM::Operations::NoOp);

	TempArrayType = elementtype;

	if(paramcount)
	{
		ReverseOps(Blocks.back().TheBlock, paramcount);
		return VM::OperationPtr(new VM::Operations::ConsArray(paramcount, elementtype));
	}

	VM::OperationPtr op(Blocks.back().TheBlock->PopTailOperation());
	VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(op.get());
	VM::Operation* innerop = pushop->GetNestedOperation();
	pushop->UnlinkOperation();
	return VM::OperationPtr(new VM::Operations::ConsArrayIndirect(elementtype, innerop));
}


//
// Create an operation for accessing an array's contents
//
VM::OperationPtr ParserState::CreateOperation_ReadArray()
{
	size_t paramcount = PassedParameterCount.top();
	if(paramcount != 2)
	{
		while(paramcount > 0)
		{
			TheStack.pop_back();
			--paramcount;
		}

		ReportFatalError("readarray() function expects 2 parameters");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry index = TheStack.back();
	TheStack.pop_back();

	if(index.DetermineEffectiveType(*CurrentScope) != VM::EpochVariableType_Integer)
	{
		TheStack.pop_back();
		ReportFatalError("Second parameter to readarray() function must be an integer");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry identifier = TheStack.back();
	TheStack.pop_back();

	if(identifier.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("First parameter to readarray() function must be a variable identifier");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(CurrentScope->GetScopeOwningVariable(identifier.StringValue) == NULL)
	{
		ReportFatalError("Variable not found");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(CurrentScope->GetVariableType(identifier.StringValue) != VM::EpochVariableType_Array)
	{
		ReportFatalError("First parameter to readarray() function must be an array variable");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::ReadArray(ParsedProgram->PoolStaticString(identifier.StringValue)));
}

//
// Create an operation for writing to an array's contents
//
VM::OperationPtr ParserState::CreateOperation_WriteArray()
{
	size_t paramcount = PassedParameterCount.top();
	if(paramcount != 3)
	{
		while(paramcount > 0)
		{
			TheStack.pop_back();
			--paramcount;
		}

		ReportFatalError("writearray() function expects 3 parameters");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry value = TheStack.back();
	TheStack.pop_back();

	StackEntry index = TheStack.back();
	TheStack.pop_back();

	if(index.DetermineEffectiveType(*CurrentScope) != VM::EpochVariableType_Integer)
	{
		TheStack.pop_back();
		ReportFatalError("Second parameter to writearray() function must be an integer");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry identifier = TheStack.back();
	TheStack.pop_back();

	if(identifier.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("First parameter to writearray() function must be a variable identifier");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(CurrentScope->GetScopeOwningVariable(identifier.StringValue) == NULL)
	{
		ReportFatalError("Variable not found");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(CurrentScope->GetVariableType(identifier.StringValue) != VM::EpochVariableType_Array)
	{
		ReportFatalError("First parameter to writearray() function must be an array variable");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(value.DetermineEffectiveType(*CurrentScope) != CurrentScope->GetArrayType(identifier.StringValue))
	{
		ReportFatalError("Cannot write this value to the given array - type mismatch");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::WriteArray(ParsedProgram->PoolStaticString(identifier.StringValue)));
}
