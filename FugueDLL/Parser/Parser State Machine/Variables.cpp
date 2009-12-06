//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Variable and constant management routines for the parser state machine
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Variables/Variable.h"

#include "Virtual Machine/Operations/Containers/ContainerOps.h"
#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/StackOps.h"

#include "Virtual Machine/VMExceptions.h"

#include "Utility/Strings.h"


using namespace Parser;


//
// Register that a variable is being defined, and track its type.
//
void ParserState::RegisterUpcomingVariable(VM::EpochVariableTypeID type)
{
	VariableTypeStack.push(type);
}

//
// Register that a variable is being defined, and prepare to infer its type.
//
void ParserState::RegisterUpcomingInferredVariable()
{
	VariableTypeStack.push(VM::EpochVariableType_Infer);
}

//
// Register the name of a variable being defined.
//
void ParserState::RegisterVariableName(const std::wstring& variablename)
{
	VariableNameStack.push(variablename);
}

//
// Register the initial value of a newly defined variable.
//
void ParserState::RegisterVariableValue()
{
	if(VariableTypeStack.top() != VM::EpochVariableType_Infer && VariableTypeStack.top() != VM::EpochVariableType_Buffer)
	{
		if(Blocks.back().TheBlock->GetTailOperation()->GetType(*CurrentScope) != VariableTypeStack.top())
		{
			ReportFatalError("Must initialize a variable with a value of the same type");
			TheStack.pop_back();
			VariableTypeStack.pop();
			VariableNameStack.pop();
			PassedParameterCount.pop();
			IsDefiningConstant = false;
			return;
		}
	}

	VM::EpochVariableTypeID type;
	if(VariableTypeStack.top() == VM::EpochVariableType_Infer)
		type = Blocks.back().TheBlock->GetTailOperation()->GetType(*CurrentScope);
	else
		type = VariableTypeStack.top();

	const std::wstring& varname = ParsedProgram->PoolStaticString(VariableNameStack.top());

	CurrentScope->AddVariable(varname, type);
	AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::InitializeValue(varname)));


	if(IsDefiningConstant)
		CurrentScope->SetConstant(varname);

	TheStack.pop_back();

	VariableTypeStack.pop();
	VariableNameStack.pop();
	PassedParameterCount.pop();
	IsDefiningConstant = false;
}

//
// Register that the upcoming variable should be a constant
//
void ParserState::RegisterUpcomingConstant()
{
	IsDefiningConstant = true;
}


//
// Create a new user-defined alias for an existing type or alias
//
void ParserState::CreateTypeAlias(const std::wstring& type)
{
	const std::wstring& aliasname = SavedStringSlots[SavedStringSlot_Alias];
	VM::EpochVariableTypeID epochtype;

	if(type == Keywords::Integer)
		epochtype = VM::EpochVariableType_Integer;
	else if(type == Keywords::Integer16)
		epochtype = VM::EpochVariableType_Integer16;
	else if(type == Keywords::Real)
		epochtype = VM::EpochVariableType_Real;
	else if(type == Keywords::Boolean)
		epochtype = VM::EpochVariableType_Boolean;
	else if(type == Keywords::String)
		epochtype = VM::EpochVariableType_String;
	else if(type == Keywords::Buffer)
		epochtype = VM::EpochVariableType_Buffer;
	else
	{
		std::map<std::string, VM::EpochVariableTypeID>::const_iterator iter = TypeAliases.find(narrow(type));
		if(iter == TypeAliases.end())
		{
			ReportFatalError("Cannot create an alias for this type");
			return;
		}

		epochtype = iter->second;
	}

	TypeAliases.insert(std::make_pair(narrow(aliasname), epochtype));
}

//
// Look up the built-in Epoch type corresponding to a given alias
//
const char* ParserState::ResolveAlias(const std::wstring& originalname) const
{
	std::map<std::string, VM::EpochVariableTypeID>::const_iterator iter = TypeAliases.find(narrow(originalname));
	if(iter == TypeAliases.end())
		throw ParserFailureException("Type is not recognized");

	switch(iter->second)
	{
	case VM::EpochVariableType_Integer:			return Keywords::GetNarrowedKeyword(Keywords::Integer);
	case VM::EpochVariableType_Integer16:		return Keywords::GetNarrowedKeyword(Keywords::Integer16);
	case VM::EpochVariableType_Real:			return Keywords::GetNarrowedKeyword(Keywords::Real);
	case VM::EpochVariableType_Boolean:			return Keywords::GetNarrowedKeyword(Keywords::Boolean);
	case VM::EpochVariableType_String:			return Keywords::GetNarrowedKeyword(Keywords::String);
	case VM::EpochVariableType_Buffer:			return Keywords::GetNarrowedKeyword(Keywords::Buffer);
	}

	throw VM::NotImplementedException("Alias not supported for this type");
}


//
// Register the construction of a named list variable
//
void ParserState::RegisterListVariable()
{
	// Empty list
	if(PassedParameterCount.top() <= 1)
	{
		const std::wstring& varname = ParsedProgram->PoolStaticString(VariableNameStack.top());

		CurrentScope->AddVariable(varname, VM::EpochVariableType_List);
		CurrentScope->SetListType(varname, ListType);
		CurrentScope->SetListSize(varname, 0);

		AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushIntegerLiteral(0)));
		AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushIntegerLiteral(ListType)));
		AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::InitializeValue(varname)));

		if(IsDefiningConstant)
			CurrentScope->SetConstant(varname);
	}
	else
	{
		VM::EpochVariableTypeID type = Blocks.back().TheBlock->GetTailOperation()->GetType(*CurrentScope);

		const std::wstring& varname = ParsedProgram->PoolStaticString(VariableNameStack.top());

		CurrentScope->AddVariable(varname, VM::EpochVariableType_List);
		CurrentScope->SetListType(varname, type);
		CurrentScope->SetListSize(varname, PassedParameterCount.top());

		ReverseOps(Blocks.back().TheBlock, PassedParameterCount.top());

		AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushIntegerLiteral(static_cast<Integer32>(PassedParameterCount.top()))));
		AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushIntegerLiteral(type)));
		AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::InitializeValue(varname)));

		if(IsDefiningConstant)
			CurrentScope->SetConstant(varname);
	}

	for(unsigned i = 0; i < PassedParameterCount.top(); ++i)
		TheStack.pop_back();

	VariableTypeStack.pop();
	VariableNameStack.pop();
	PassedParameterCount.pop();
	IsDefiningConstant = false;
}

//
// Register the expected type of an empty list (since we can't use type inference on the list contents)
//
void ParserState::RegisterListType(const std::wstring& type)
{
	if(type == Keywords::Integer)
		ListType = VM::EpochVariableType_Integer;
	else if(type == Keywords::Integer16)
		ListType = VM::EpochVariableType_Integer16;
	else if(type == Keywords::Real)
		ListType = VM::EpochVariableType_Real;
	else if(type == Keywords::Boolean)
		ListType = VM::EpochVariableType_Boolean;
	else if(type == Keywords::String)
		ListType = VM::EpochVariableType_String;
	else
		throw VM::NotImplementedException("Cannot construct a list of this type");
}

