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
// Create an operation that constructs a list variable
//
VM::OperationPtr ParserState::CreateOperation_ConsList()
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
				ReportFatalError("All elements in a list must be of the same type");
			complained = true;
		}

		TheStack.pop_back();
	}

	if(complained)
		return VM::OperationPtr(new VM::Operations::NoOp);

	ReverseOps(paramcount);
	return VM::OperationPtr(new VM::Operations::ConsList(paramcount, elementtype));
}


