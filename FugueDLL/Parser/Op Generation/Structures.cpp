//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Operation generation code - operations related to working with structures
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Error Handling/ParserExceptions.h"
#include "Parser/Parse.h"

#include "Virtual Machine/Operations/Variables/StructureOps.h"
#include "Virtual Machine/Operations/StackOps.h"
#include "Virtual Machine/Operations/Variables/VariableOps.h"
#include "Virtual Machine/Operations/UtilityOps.h"

#include "Virtual Machine/Core Entities/Program.h"

#include "Utility/Strings.h"


using namespace Parser;


//
// Create an operation that reads a member out of a structure
//
VM::OperationPtr ParserState::CreateOperation_ReadStructure()
{
	if(PassedParameterCount.top() != 2)
	{
		ReportFatalError("readstructure() function expects 2 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry member = TheStack.back();
	TheStack.pop_back();

	StackEntry structurevar = TheStack.back();
	TheStack.pop_back();

	if(structurevar.Type == StackEntry::STACKENTRYTYPE_OPERATION)
	{
		IDType structid;

		VM::Operation* op = Blocks.back().TheBlock->GetTailOperation();
		VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(op);
		if(!pushop)
		{
			ReportFatalError("Invalid parameter to readstructure()");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		VM::Operations::ReadStructure* readop = dynamic_cast<VM::Operations::ReadStructure*>(pushop->GetNestedOperation());
		if(readop)
		{
			IDType containerstructid = CurrentScope->GetVariableStructureTypeID(readop->GetAssociatedIdentifier());
			structid = VM::StructureTrackerClass::GetOwnerOfStructureType(containerstructid)->GetStructureType(containerstructid).GetMemberTypeHint(readop->GetMemberName());
			if(!structid)
			{
				ReportFatalError(("Parameter \"" + narrow(readop->GetMemberName()) + "\" is not a structure").c_str());
				TheStack.pop_back();
				PopParameterCount();
				return VM::OperationPtr(new VM::Operations::NoOp);
			}
		}
		else
		{
			VM::Operations::ReadStructureIndirect* readindirectop = dynamic_cast<VM::Operations::ReadStructureIndirect*>(pushop->GetNestedOperation());
			if(!readindirectop)
			{
				ReportFatalError("Invalid parameter to readstructure()");
				TheStack.pop_back();
				PopParameterCount();
				return VM::OperationPtr(new VM::Operations::NoOp);
			}

			structid = readindirectop->WalkInstructionsForReadStruct(*CurrentScope, Blocks.back().TheBlock->GetTailOperation());
			if(!structid)
			{
				ReportFatalError(("Parameter \"" + narrow(readindirectop->GetMemberName()) + "\" is not a structure").c_str());
				TheStack.pop_back();
				PopParameterCount();
				return VM::OperationPtr(new VM::Operations::NoOp);
			}
		}

		if(!VM::StructureTrackerClass::GetOwnerOfStructureType(structid)->GetStructureType(structid).HasMember(member.StringValue))
		{
			ReportFatalError((narrow(member.StringValue) + " is not a member of the structure " + narrow(CurrentScope->GetStructureTypeID(structid))).c_str());
			TheStack.pop_back();
			PopParameterCount();
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		TheStack.pop_back();
		PopParameterCount();
		return VM::OperationPtr(new VM::Operations::ReadStructureIndirect(ParsedProgram->PoolStaticString(member.StringValue), Blocks.back().TheBlock->GetTailOperation()));
	}
	else
	{
		if(member.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER || structurevar.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
		{
			ReportFatalError("Invalid parameters to readstructure()");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		if(CurrentScope->GetVariableType(structurevar.StringValue) != VM::EpochVariableType_Structure)
		{
			ReportFatalError("Variable is not a structure type");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		IDType structid = CurrentScope->GetVariableStructureTypeID(structurevar.StringValue);
		VM::StructureTrackerClass* owner = VM::StructureTrackerClass::GetOwnerOfStructureType(structid);
		if(!owner->GetStructureType(structid).HasMember(member.StringValue))
		{
			ReportFatalError((narrow(member.StringValue) + " is not a member of the structure " + narrow(CurrentScope->GetStructureTypeID(structid))).c_str());
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		return VM::OperationPtr(new VM::Operations::ReadStructure(ParsedProgram->PoolStaticString(structurevar.StringValue), ParsedProgram->PoolStaticString(member.StringValue)));
	}
}


//
// Create an operation that writes a value into a structure
//
VM::OperationPtr ParserState::CreateOperation_AssignStructure()
{
	if(PassedParameterCount.top() != 3)
	{
		ReportFatalError("assignstructure() function expects 3 parameters");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	StackEntry value = TheStack.back();
	TheStack.pop_back();

	StackEntry member = TheStack.back();
	TheStack.pop_back();

	StackEntry structurevar = TheStack.back();
	TheStack.pop_back();

	if(structurevar.Type == StackEntry::STACKENTRYTYPE_OPERATION)
		return VM::OperationPtr(new VM::Operations::AssignStructureIndirect(ParsedProgram->PoolStaticString(member.StringValue)));

	if(member.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER || structurevar.Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
	{
		ReportFatalError("Invalid parameters to assignstructure()");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(CurrentScope->GetVariableType(structurevar.StringValue) != VM::EpochVariableType_Structure)
	{
		ReportFatalError("Variable is not a structure type");
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	const VM::StructureType& structtype = CurrentScope->GetStructureType(CurrentScope->GetVariableStructureTypeID(structurevar.StringValue));
	if(structtype.GetMemberType(member.StringValue) == VM::EpochVariableType_Function)
	{
		VM::Operation* op = Blocks.back().TheBlock->GetTailOperation();
		VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(op);
		if(!pushop)
			throw ParserFailureException("Parsing error - structure assignment should be preceeded by variable read");

		VM::Operations::GetVariableValue* getvarop = dynamic_cast<VM::Operations::GetVariableValue*>(pushop->GetNestedOperation());
		if(!getvarop)
			throw ParserFailureException("Parsing error - structure assignment should be preceeded by variable read");

		VM::FunctionBase* func = CurrentScope->GetFunction(getvarop->GetAssociatedIdentifier());
		if(!CurrentScope->GetFunctionSignature(structtype.GetMemberTypeHintString(member.StringValue)).DoesFunctionMatchSignature(func, *CurrentScope))
		{
			ReportFatalError("Function is not of the required type");
			return VM::OperationPtr(new VM::Operations::NoOp);
		}

		Blocks.back().TheBlock->ReplaceOperationFromEnd(0, VM::OperationPtr(new VM::Operations::BindFunctionReference(getvarop->GetAssociatedIdentifier())), *CurrentScope);
	}

	return VM::OperationPtr(new VM::Operations::AssignStructure(ParsedProgram->PoolStaticString(structurevar.StringValue), ParsedProgram->PoolStaticString(member.StringValue)));
}


//
// Create an operation that effectively extracts a reference to a structure member
//
// This reference can be used to either read or write the contents of a nested structure.
//
VM::OperationPtr ParserState::CreateOperation_Member()
{
	if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
		throw SyntaxException("This function must be passed a structure and a member name");

	std::wstring membername = TheStack.back().StringValue;
	TheStack.pop_back();

	if(TheStack.back().Type == StackEntry::STACKENTRYTYPE_OPERATION)
	{
		VM::Operations::BindStructMemberReference* bindop = dynamic_cast<VM::Operations::BindStructMemberReference*>(TheStack.back().OperationPointer);
		if(!bindop)
			throw SyntaxException("Invalid parameter to this function");

		TheStack.pop_back();

		return VM::OperationPtr(new VM::Operations::BindStructMemberReference(ParsedProgram->PoolStaticString(membername)));
	}
	else if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_IDENTIFIER)
		throw SyntaxException("This function must be passed a structure and a member name");

	std::wstring varname = TheStack.back().StringValue;
	TheStack.pop_back();

	return VM::OperationPtr(new VM::Operations::BindStructMemberReference(ParsedProgram->PoolStaticString(varname), ParsedProgram->PoolStaticString(membername)));
}

