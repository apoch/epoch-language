//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Tuple management routines for the parser state machine
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Types/Tuple.h"

#include "Virtual Machine/Operations/Variables/TupleOps.h"


using namespace Parser;


//
// Prepare to read the members of a new tuple type with the given name
//
void ParserState::RegisterTupleType(const std::wstring& identifier)
{
	if(CreatedTupleType != NULL)
		throw SyntaxException("An incomplete tuple type definition has already been started; nested definitions are not permitted");

	CreatedTupleType = new VM::TupleType;
	VariableNameStack.push(identifier);
}

//
// Add a member of the given type to the current tuple type definition
//
void ParserState::RegisterTupleMember(const std::wstring& identifier, VM::EpochVariableTypeID type)
{
	if(!CreatedTupleType)
		throw ParserFailureException("The grammar appears to have barfed; we're parsing tuple members but no tuple type declaration was found.");

	if(type == VM::EpochVariableType_Structure || type == VM::EpochVariableType_Tuple)
		throw SyntaxException("Tuples may not contain nested structures or other tuples");
		
	CreatedTupleType->AddMember(identifier, type);
}

//
// Finish processing a tuple type definition. This involves
// registering the new type with the current scope, as well
// as some safety checks.
//
void ParserState::FinishTupleType()
{
	if(CreatedTupleType->GetMemberOrder().empty())
	{
		ReportFatalError("Tuples must contain at least one member");
	}
	else
	{
		CreatedTupleType->ComputeOffsets(*CurrentScope);
		CurrentScope->AddTupleType(VariableNameStack.top(), *CreatedTupleType);
	}

	VariableNameStack.pop();

	delete CreatedTupleType;
	CreatedTupleType = NULL;
}

