//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - concurrency and related operations
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Operations/Concurrency/Tasks.h"
#include "Virtual Machine/Operations/Concurrency/Messaging.h"
#include "Virtual Machine/Operations/Concurrency/FutureOps.h"
#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/UtilityOps.h"

#include "Virtual Machine/Core Entities/Program.h"


using namespace Parser;


//
// Create an operation to send a message to a task
//
VM::OperationPtr ParserState::CreateOperation_Message()
{
	size_t messageparamcount = PassedParameterCount.top();
	PassedParameterCount.pop();
	size_t paramcount = PassedParameterCount.top();

	if(paramcount != 2)
	{
		ReportFatalError("message() function expects 2 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		TheStack.pop_back();
		TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	std::list<VM::EpochVariableTypeID> payloadtypes;
	for(size_t i = 0; i < messageparamcount; ++i)
	{
		payloadtypes.push_front(TheStack.back().DetermineEffectiveType(*CurrentScope));
		TheStack.pop_back();
	}

	if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("Expected the name of a message for second parameter to message()");
		TheStack.pop_back();
		TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	std::wstring messagename = TheStack.back().StringValue;
	TheStack.pop_back();

	bool usestaskid = true;

	if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_STRING_LITERAL)
	{
		if(TheStack.back().DetermineEffectiveType(*CurrentScope) != VM::EpochVariableType_String)
		{
			if(TheStack.back().Type == StackEntry::STACKENTRYTYPE_OPERATION)
			{
				VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(TheStack.back().OperationPointer);
				if(!pushop)
				{
					ReportFatalError("Expected name of a task for the first parameter to message()");
					TheStack.pop_back();
					return VM::OperationPtr(new VM::Operations::NoOp);
				}

				VM::Operations::GetTaskCaller* getcallerop = dynamic_cast<VM::Operations::GetTaskCaller*>(pushop->GetNestedOperation());
				VM::Operations::GetMessageSender* getsenderop = dynamic_cast<VM::Operations::GetMessageSender*>(pushop->GetNestedOperation());
				if((!getcallerop) && (!getsenderop))
				{
					ReportFatalError("Expected name of a task for the first parameter to message()");
					TheStack.pop_back();
					return VM::OperationPtr(new VM::Operations::NoOp);
				}

				if(getcallerop || getsenderop)
					usestaskid = false;

				AddOperationToCurrentBlock(VM::OperationPtr(TheStack.back().OperationPointer));
			}
			else
			{
				ReportFatalError("Expected name of a task for the first parameter to message()");
				TheStack.pop_back();
				return VM::OperationPtr(new VM::Operations::NoOp);
			}
		}
	}

	TheStack.pop_back();

	return VM::OperationPtr(new VM::Operations::SendTaskMessage(usestaskid, ParsedProgram->PoolStaticString(messagename), payloadtypes));
}


//
// Create an operation that waits for a particular message
//
VM::OperationPtr ParserState::CreateOperation_AcceptMessage()
{
	if(TheStack.back().Type == StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		std::wstring responsemapname = TheStack.back().StringValue;
		TheStack.pop_back();

		return VM::OperationPtr(new VM::Operations::AcceptMessageFromResponseMap(ParsedProgram->PoolStaticString(responsemapname)));
	}

	if(TheStack.empty() || TheStack.back().Type != StackEntry::STACKENTRYTYPE_SCOPE)
		throw ParserFailureException("Lost track of the helper scope!");

	VM::ScopeDescription* auxscope = TheStack.back().ScopePointer;
	TheStack.pop_back();

	if(TheStack.empty() || TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("Couldn't figure out what the message name should be");
		TheStack.pop_back();
		std::auto_ptr<VM::Block> body(Blocks.back().TheBlock);
		Blocks.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	std::wstring messagename = TheStack.back().StringValue;
	TheStack.pop_back();

	VM::Block* body = Blocks.back().TheBlock;
	Blocks.pop_back();

	body->BindToScope(CurrentScope);
	CurrentScope = CurrentScope->ParentScope->ParentScope;
	return VM::OperationPtr(new VM::Operations::AcceptMessage(ParsedProgram->PoolStaticString(messagename), body, auxscope));
}


//
// Create an operation that generates a future
//
// A future is simply a value that is computed in a separate thread. When the
// value of the future is read, if the computation is finished, the value is
// immediately available and returned directly. Otherwise, the read operation
// blocks until the computation is finished.
//
VM::OperationPtr ParserState::CreateOperation_Future()
{
	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("future() function requires a variable name and a value");

		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();

		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_OPERATION)
	{
		ReportFatalError("Futures must be bound to an operation, not just a constant");

		TheStack.pop_back();
		TheStack.pop_back();

		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	VM::OperationPtr theop(TheStack.back().OperationPointer);
	TheStack.pop_back();


	if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("Futures must be attached to a variable name");
		TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	std::wstring varname = TheStack.back().StringValue;
	TheStack.pop_back();

	VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(theop.get());
	if(pushop)
	{
		VM::OperationPtr finalop(pushop->GetNestedOperation());
		pushop->UnlinkOperation();
		theop.release();

		Blocks.back().TheBlock->EraseOperation(finalop.get());
		if(Blocks.back().TheBlock->GetNumOperations() > 0)
			Blocks.back().TheBlock->RemoveTailOperations(1);

		VM::EpochVariableTypeID type = finalop->GetType(*CurrentScope);
		CurrentScope->AddFuture(ParsedProgram->PoolStaticString(varname), VM::OperationPtr(finalop.release()));

		return VM::OperationPtr(new VM::Operations::ForkFuture(ParsedProgram->PoolStaticString(varname), type));
	}

	Blocks.back().TheBlock->EraseOperation(theop.get());
	if(Blocks.back().TheBlock->GetNumOperations() > 0)
		Blocks.back().TheBlock->RemoveTailOperations(1);

	VM::EpochVariableTypeID type = theop->GetType(*CurrentScope);
	CurrentScope->AddFuture(ParsedProgram->PoolStaticString(varname), VM::OperationPtr(theop.release()));

	return VM::OperationPtr(new VM::Operations::ForkFuture(ParsedProgram->PoolStaticString(varname), type));
}

