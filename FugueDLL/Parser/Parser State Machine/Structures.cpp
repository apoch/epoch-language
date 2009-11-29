//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Structure management routines for the parser state machine
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Types/Structure.h"

#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/Variables/StructureOps.h"
#include "Virtual Machine/Operations/Variables/TupleOps.h"
#include "Virtual Machine/Operations/StackOps.h"


using namespace Parser;



//
// Prepare to read the members of a new structure type with the given name
//
void ParserState::RegisterStructureType(const std::wstring& identifier)
{
	if(CreatedStructureType != NULL)
		throw SyntaxException("An incomplete structure type definition has already been started; nested definitions are not permitted");

	CreatedStructureType = new VM::StructureType;
	VariableNameStack.push(identifier);
}

//
// Add a member of the given type to the current structure type definition
//
void ParserState::RegisterStructureMember(const std::wstring& identifier, VM::EpochVariableTypeID type)
{
	if(!CreatedStructureType)
		throw ParserFailureException("The grammar appears to have barfed; we're parsing structure members but no structure type declaration was found.");

	if(type == VM::EpochVariableType_Tuple || type == VM::EpochVariableType_Structure)
		throw ParserFailureException("Grammar barf - we should be using RegisterStructureMemberUnknown instead!");
	
	CreatedStructureType->AddMember(identifier, type);
}

//
// Register that an upcoming nested variable is about to be parsed
//
void ParserState::RegisterStructureUnknownTypeName(const std::wstring& type)
{
	UpcomingNestedMemberType = type;
}

//
// Add a nested variable (nested structure, tuple, or function pointer) to the current structure type definition
//
void ParserState::RegisterStructureMemberUnknown(const std::wstring& identifier)
{
	if(CurrentScope->HasTupleType(UpcomingNestedMemberType))
	{
		IDType hint = CurrentScope->GetTupleTypeID(UpcomingNestedMemberType);
		CreatedStructureType->AddMember(identifier, CurrentScope->GetTupleType(hint), hint);
	}
	else if(CurrentScope->HasStructureType(UpcomingNestedMemberType))
	{
		IDType hint = CurrentScope->GetStructureTypeID(UpcomingNestedMemberType);
		CreatedStructureType->AddMember(identifier, CurrentScope->GetStructureType(hint), hint);
	}
	else if(CurrentScope->IsFunctionSignature(UpcomingNestedMemberType))
		CreatedStructureType->AddFunctionMember(identifier, UpcomingNestedMemberType);
	else
	{
		ReportFatalError("Unrecognized type; cannot add member to structure");
		CreatedStructureType->AddMember(identifier, VM::EpochVariableType_Error);
	}
}

//
// Finish processing a structure type definition. This involves
// registering the new type with the current scope, as well
// as some safety checks.
//
void ParserState::FinishStructureType()
{
	if(CreatedStructureType->GetMemberOrder().empty())
		ReportFatalError("Structures must contain at least one member");
	else
	{
		CreatedStructureType->ComputeOffsets(*CurrentScope);
		CurrentScope->AddStructureType(VariableNameStack.top(), *CreatedStructureType);
	}

	VariableNameStack.pop();

	delete CreatedStructureType;
	CreatedStructureType = NULL;
}


//
// Keep track of how many layers of nested structures we are inside
//
void ParserState::IncrementMemberLevel()
{
	++MemberLevel;
}

//
// Back out of all layers of nested structures and adjust the current
// instruction sequence accordingly; nested structure access requires
// a quirky stack ordering, which this function handles.
//
void ParserState::ResetMemberLevel()
{
	Blocks.back().TheBlock->ShiftUpTailOperation(MemberLevel);
	MemberLevel = 0;
}

//
// Track the nested structure members being accessed
//
void ParserState::RegisterMemberAccess(const std::wstring& membername)
{
	MemberAccesses.push_back(membername);
}

//
// Inject operations needed to traverse a set of nested members
//
void ParserState::ResetMemberAccess()
{
	if(MemberAccesses.empty())
		return;

	if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
		throw ParserFailureException("Expected a structure or tuple name here");

	std::wstring variablename = TheStack.back().StringValue;
	TheStack.pop_back();

	Blocks.back().TheBlock->PopTailOperation();		// Undo the PUSH of the structure/tuple since we're doing a member access

	bool istuple = false;
	if(CurrentScope->GetVariableType(variablename) == VM::EpochVariableType_Tuple)
		istuple = true;

	VM::OperationPtr pushop(NULL);
	if(istuple)
		pushop.reset(new VM::Operations::PushOperation(new VM::Operations::ReadTuple(ParsedProgram->PoolStaticString(variablename), ParsedProgram->PoolStaticString(MemberAccesses.front()))));
	else
		pushop.reset(new VM::Operations::PushOperation(new VM::Operations::ReadStructure(ParsedProgram->PoolStaticString(variablename), ParsedProgram->PoolStaticString(MemberAccesses.front()))));

	VM::Operation* lastop = pushop.get();

	AddOperationToCurrentBlock(pushop);

	MemberAccesses.pop_front();
	while(!MemberAccesses.empty())
	{
		if(istuple)
			throw ParserFailureException("Nested tuple support is not implemented!");

		VM::OperationPtr readindirectop(new VM::Operations::PushOperation(new VM::Operations::ReadStructureIndirect(ParsedProgram->PoolStaticString(MemberAccesses.front()), Blocks.back().TheBlock->GetTailOperation())));
		lastop = readindirectop.get();

		AddOperationToCurrentBlock(readindirectop);

		MemberAccesses.pop_front();
	}

	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_OPERATION;
	entry.OperationPointer = lastop;
	TheStack.push_back(entry);
}

//
// Track nested member accesses used to set up an l-value
//
void ParserState::RegisterMemberLValueAccess(const std::wstring& membername)
{
	MemberAccesses.push_back(membername);
	++MemberLevel;
}

//
// Inject traversal operations for preparing an l-value expression consisting of nested member accesses
//
// Note that we cannot do any type checking yet because we don't know anything about the r-value we
// are about to use. All type checks are therefore done in FinalizeCompositeAssignment.
//
void ParserState::ResetMemberAccessLValue()
{
	if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
		throw ParserFailureException("Expected a structure or tuple name here");

	std::wstring variablename = TheStack.back().StringValue;
	TheStack.pop_back();

	bool istuple = false;
	if(CurrentScope->GetVariableType(variablename) == VM::EpochVariableType_Tuple)
		istuple = true;

	if(istuple)
	{
		if(MemberAccesses.size() > 1)
			throw ParserFailureException("Nested tuples are not permitted");

		AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::AssignTuple(ParsedProgram->PoolStaticString(variablename), ParsedProgram->PoolStaticString(MemberAccesses.front()))));
	}
	else
	{
		if(MemberAccesses.size() == 1)
		{
			AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::AssignStructure(ParsedProgram->PoolStaticString(variablename), ParsedProgram->PoolStaticString(MemberAccesses.front()))));
		}
		else
		{
			AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushOperation(new VM::Operations::BindStructMemberReference(ParsedProgram->PoolStaticString(variablename), ParsedProgram->PoolStaticString(MemberAccesses.front())))));

			MemberAccesses.pop_front();
			while(MemberAccesses.size() > 1)
			{
				AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::PushOperation(new VM::Operations::BindStructMemberReference(ParsedProgram->PoolStaticString(MemberAccesses.front())))));
				MemberAccesses.pop_front();
			}

			AddOperationToCurrentBlock(VM::OperationPtr(new VM::Operations::AssignStructureIndirect(ParsedProgram->PoolStaticString(MemberAccesses.front()))));
			MemberAccesses.pop_front();
		}
	}

	MemberAccesses.clear();
}

//
// Track the root variable of a set of nested member accesses
//
void ParserState::RegisterCompositeLValue(const std::wstring& varname)
{
	StackEntry entry;
	entry.Type = StackEntry::STACKENTRYTYPE_IDENTIFIER;
	entry.StringValue = varname;
	TheStack.push_back(entry);
}

//
// Clean up from parsing an assignment to a nested member
//
void ParserState::FinalizeCompositeAssignment()
{
	VM::EpochVariableTypeID rvaluetype = Blocks.back().TheBlock->GetTailOperation()->GetType(*CurrentScope);
	IDType rvaluehint = 0;

	if(rvaluetype == VM::EpochVariableType_Structure)
	{
		VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(Blocks.back().TheBlock->GetTailOperation());
		rvaluehint = CurrentScope->GetVariableStructureTypeID(dynamic_cast<VM::Operations::GetVariableValue*>(pushop->GetNestedOperation())->GetAssociatedIdentifier());
	}

	Blocks.back().TheBlock->ShiftUpTailOperation(MemberLevel);

	VM::Operation* tailop = Blocks.back().TheBlock->GetTailOperation();

	VM::Operations::AssignTuple* tupleop = dynamic_cast<VM::Operations::AssignTuple*>(tailop);
	if(tupleop)
	{
		VM::EpochVariableTypeID lvaluetype = CurrentScope->GetTupleType(CurrentScope->GetVariableTupleTypeID(tupleop->GetAssociatedIdentifier())).GetMemberType(tupleop->GetMemberName());
		if(lvaluetype != rvaluetype)
			ReportFatalError("Type of expression is different from type of tuple member");
	}
	else
	{
		VM::EpochVariableTypeID lvaluetype;
		IDType lvaluehint;

		VM::Operations::AssignStructure* assignop = dynamic_cast<VM::Operations::AssignStructure*>(tailop);
		if(assignop)
		{
			lvaluetype = CurrentScope->GetStructureType(CurrentScope->GetVariableStructureTypeID(assignop->GetAssociatedIdentifier())).GetMemberType(assignop->GetMemberName());
			if(lvaluetype == VM::EpochVariableType_Structure)
				lvaluehint = CurrentScope->GetStructureType(CurrentScope->GetVariableStructureTypeID(assignop->GetAssociatedIdentifier())).GetMemberTypeHint(assignop->GetMemberName());
		}
		else
		{
			VM::Operations::AssignStructureIndirect* indirectop = dynamic_cast<VM::Operations::AssignStructureIndirect*>(tailop);
			if(!indirectop)
				throw ParserFailureException("Failed to locate assignment operation!");

			VM::Operations::BindStructMemberReference* bindop = NULL;

			std::list<std::wstring> membersaccessed;
			membersaccessed.push_back(indirectop->GetMemberName());

			const std::vector<VM::Operation*>& ops = Blocks.back().TheBlock->GetAllOperations();
			size_t i = ops.size() - 1;
			while(true)
			{
				VM::Operation* op = ops[--i];
				op = dynamic_cast<VM::Operations::PushOperation*>(op)->GetNestedOperation();

				bindop = dynamic_cast<VM::Operations::BindStructMemberReference*>(op);
				if(!bindop)
					throw ParserFailureException("Can't understand nested member accesses");

				membersaccessed.push_front(bindop->GetMemberName());

				if(!bindop->IsChained())
					break;
			}

			IDType typehint = CurrentScope->GetVariableStructureTypeID(bindop->GetAssociatedIdentifier());

			while(!membersaccessed.empty())
			{
				IDType newtypehint = CurrentScope->GetStructureType(typehint).GetMemberTypeHint(membersaccessed.front());
				if(!newtypehint)
				{
					lvaluetype = CurrentScope->GetStructureType(typehint).GetMemberType(membersaccessed.front());
					if(lvaluetype == VM::EpochVariableType_Structure)
						lvaluehint = typehint;
				}

				typehint = newtypehint;
				membersaccessed.pop_front();
			}
		}

		if(lvaluetype != rvaluetype)
			ReportFatalError("Type mismatch");

		if(lvaluetype == VM::EpochVariableType_Structure)
		{
			if(lvaluehint != rvaluehint)
				ReportFatalError("Type mismatch");
		}
	}

	for(unsigned i = 0; i < PassedParameterCount.top(); ++i)
		TheStack.pop_back();

	PassedParameterCount.pop();
	MemberLevel = 0;
}

