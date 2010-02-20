//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - built in data containers
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Operations/Containers/ContainerOps.h"
#include "Virtual Machine/Operations/UtilityOps.h"


using namespace Parser;


//
// Create an operation that constructs an array variable
//
VM::OperationPtr ParserState::CreateOperation_ConsArray()
{
	size_t paramcount = PassedParameterCount.top();
	VM::EpochVariableTypeID elementtype = VM::EpochVariableType_Null;

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

	ReverseOps(Blocks.back().TheBlock, paramcount);
	return VM::OperationPtr(new VM::Operations::ConsArray(paramcount, elementtype));
}


