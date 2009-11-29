//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - program flow control operations
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Operations/Flow/FlowControl.h"
#include "Virtual Machine/Operations/UtilityOps.h"


using namespace Parser;


//
// Create an operation to break out of the current loop.
//
// The loop is immediately exited and no further code is executed;
// i.e. no additional checks, variable writes, etc. will occur.
//
VM::OperationPtr ParserState::CreateOperation_Break()
{
	if(PassedParameterCount.top() != 0)
	{
		ReportFatalError("This function expects no parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	bool inloop = false;
	for(std::deque<BlockEntry>::const_iterator blockiter = Blocks.begin(); blockiter != Blocks.end(); ++blockiter)
	{
		BlockEntry::BlockType type = blockiter->Type;
		if(type == BlockEntry::BLOCKENTRYTYPE_DOLOOP || type == BlockEntry::BLOCKENTRYTYPE_WHILELOOP)
		{
			inloop = true;
			break;
		}
	}

	if(!inloop)
	{
		ReportFatalError("This function is only permitted within a loop");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::Break);
}


//
// Create an operation to return from the current function.
//
// The executing function will exit immediately, and no further
// operations from the function will execute. The return value
// of the function is whatever the return tuple is set to at
// the time of the return() call.
//
VM::OperationPtr ParserState::CreateOperation_Return()
{
	if(PassedParameterCount.top() != 0)
	{
		ReportFatalError("This function expects no parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	return VM::OperationPtr(new VM::Operations::Return);
}

