//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser state machine routines for working with asynchronous tasks
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Tracing.h"

#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Concurrency/ResponseMap.h"

#include "Virtual Machine/Operations/Concurrency/Tasks.h"
#include "Virtual Machine/Operations/Concurrency/Messaging.h"
#include "Virtual Machine/Operations/StackOps.h"

#include "Virtual Machine/SelfAware.inl"


using namespace Parser;


//
// Enter a task code block
//
void ParserState::BeginTaskCode()
{
	PopParameterCount();
	ExpectedBlockTypes.push(BlockEntry::BLOCKENTRYTYPE_TASK);
}


//
// Enter a pooled thread code block
//
void ParserState::BeginThreadCode()
{
	PopParameterCount();
	ExpectedBlockTypes.push(BlockEntry::BLOCKENTRYTYPE_THREAD);
}


//
// Register message parameter types
//
void ParserState::RegisterIntegerMessageParam(const std::wstring& paramname)
{
	MessageDispatchScope->AddVariable(ParsedProgram->PoolStaticString(paramname), VM::EpochVariableType_Integer);
}

void ParserState::RegisterInt16MessageParam(const std::wstring& paramname)
{
	MessageDispatchScope->AddVariable(ParsedProgram->PoolStaticString(paramname), VM::EpochVariableType_Integer16);
}

void ParserState::RegisterRealMessageParam(const std::wstring& paramname)
{
	MessageDispatchScope->AddVariable(ParsedProgram->PoolStaticString(paramname), VM::EpochVariableType_Real);
}

void ParserState::RegisterBooleanMessageParam(const std::wstring& paramname)
{
	MessageDispatchScope->AddVariable(ParsedProgram->PoolStaticString(paramname), VM::EpochVariableType_Boolean);
}

void ParserState::RegisterStringMessageParam(const std::wstring& paramname)
{
	MessageDispatchScope->AddVariable(ParsedProgram->PoolStaticString(paramname), VM::EpochVariableType_String);
}


//
// Start a scope for tracking message parameters
//
void ParserState::StartMessageParamScope()
{
	MessageDispatchScope = new VM::ScopeDescription;
}


//
// Inject an operation to retrieve the current task's original caller
//
void ParserState::PushCallerOperation()
{
	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_OPERATION;
	TheStack.push_back(entry);
	TheStack.back().OperationPointer = new VM::Operations::PushOperation(new VM::Operations::GetTaskCaller);	// Bind after the entry is tracked for exception safety

	++PassedParameterCount.top();
}

//
// Inject an operation to retrieve the sender of the message being replied to
//
void ParserState::PushSenderOperation()
{
	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_OPERATION;
	TheStack.push_back(entry);
	TheStack.back().OperationPointer = new VM::Operations::PushOperation(new VM::Operations::GetMessageSender);	// Bind after the entry is tracked for exception safety

	++PassedParameterCount.top();
}


//
// Begin parsing a message response map
//
void ParserState::BeginResponseMap()
{
	PopParameterCount();

	ResponseMapStack.push_back(new VM::ResponseMap);
	ExpectedBlockTypes.push(BlockEntry::BLOCKENTRYTYPE_RESPONSEMAP);
	StartCountingParams();
}

//
// Finish a response map and clean up the intermediate state data we used
//
void ParserState::EndResponseMap()
{
	if(ResponseMapStack.empty())
		throw ParserFailureException("Tried to end a response map without starting one first");

	std::auto_ptr<VM::ResponseMap> responses(ResponseMapStack.back());
	ResponseMapStack.pop_back();

	if(TheStack.empty() || TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
		throw ParserFailureException("Missing or invalid response map identifier");

	std::wstring mapname = TheStack.back().StringValue;
	TheStack.pop_back();

#ifdef _DEBUG
	{
		std::wostringstream stream;
		stream << "Created response map " << responses.get() << " in scope " << CurrentScope;
		Trace(stream.str().c_str());
	}
#endif

	CurrentScope->AddResponseMap(mapname, responses.release());

	delete MessageDispatchScope;
	MessageDispatchScope = NULL;
}

//
// Add an entry to a message response map
//
void ParserState::RegisterResponseMapEntry()
{
	if(Blocks.empty() || Blocks.back().Type != BlockEntry::BLOCKENTRYTYPE_MSGDISPATCH)
		throw ParserFailureException("Internal error - lost track of the message response block");

	if(TheStack.empty() || TheStack.back().Type != StackEntry::STACKENTRYTYPE_SCOPE)
		throw ParserFailureException("Internal error - lost track of the message response scope");

	if(ResponseMapStack.empty())
		throw ParserFailureException("Internal error - lost track of the response map");

	std::auto_ptr<VM::Block> responseblock(Blocks.back().TheBlock);
	Blocks.pop_back();

	std::auto_ptr<VM::ScopeDescription> auxscope(TheStack.back().ScopePointer);
	TheStack.pop_back();

	std::list<VM::EpochVariableTypeID> payloadtypes;
	const std::vector<std::wstring>& members = auxscope->GetMemberOrder();
	for(std::vector<std::wstring>::const_iterator iter = members.begin(); iter != members.end(); ++iter)
		payloadtypes.push_back(auxscope->GetVariableType(*iter));

	responseblock->BindToScope(CurrentScope);
	CurrentScope = CurrentScope->ParentScope->ParentScope;

	if(TheStack.empty() || TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
		throw ParserFailureException("Internal error - lost track of the message name");
	
	std::wstring messagename = TheStack.back().StringValue;
	TheStack.pop_back();

	std::auto_ptr<VM::ResponseMapEntry> entry(new VM::ResponseMapEntry(ParsedProgram->PoolStaticString(messagename), payloadtypes, responseblock.release(), auxscope.release()));
	ResponseMapStack.back()->AddEntry(entry.release());
}


//
// Save the name of the current task for later retrieval
//
void ParserState::SaveTaskName(const std::wstring& taskname)
{
	SavedTaskNames.push(taskname);
}

//
// Prepare to parse a block of code that is used to respond to a message
//
void ParserState::RegisterUpcomingMessageDispatch(bool ispreparse)
{
	if(!ispreparse)
		PopParameterCount();

	ExpectedBlockTypes.push(BlockEntry::BLOCKENTRYTYPE_MSGDISPATCH);
}


//
// Register the creation of a new worker thread pool
//
void ParserState::RegisterThreadPool()
{
	if(PassedParameterCount.top() != 2)
		throw ParserFailureException("threadpool() function expects two parameters");

	StackEntry poolsize = TheStack.back();
	TheStack.pop_back();

	StackEntry poolname = TheStack.back();
	TheStack.pop_back();

	if(poolsize.DetermineEffectiveType(*CurrentScope) != VM::EpochVariableType_Integer)
	{
		ReportFatalError("Second parameter to threadpool() must be an integer providing the number of threads in the pool");
		return;
	}

	if(poolname.DetermineEffectiveType(*CurrentScope) != VM::EpochVariableType_String)
	{
		ReportFatalError("First parameter to threadpool() must be a string uniquely naming the pool");
		return;
	}

	AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::CreateThreadPool));
}
