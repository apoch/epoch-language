//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Auxiliary routines for the parser state machine
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Error Handling/ParserExceptions.h"

#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Operation.h"
#include "Virtual Machine/Core Entities/Block.h"

#include "Virtual Machine/Operations/Containers/ContainerOps.h"

#include "User Interface/Output.h"

#include "Utility/Strings.h"


using namespace Parser;


//
// Determine the type of the parse stack entry. Looks up the type
// of function calls and local variables if necessary.
//
VM::EpochVariableTypeID ParserState::StackEntry::DetermineEffectiveType(const VM::ScopeDescription& scope) const
{
	switch(Type)
	{
	case STACKENTRYTYPE_INTEGER_LITERAL:
		return VM::EpochVariableType_Integer;
	case STACKENTRYTYPE_REAL_LITERAL:
		return VM::EpochVariableType_Real;
	case STACKENTRYTYPE_BOOLEAN_LITERAL:
		return VM::EpochVariableType_Boolean;
	case STACKENTRYTYPE_STRING_LITERAL:
		return VM::EpochVariableType_String;
	case STACKENTRYTYPE_IDENTIFIER:
		return scope.GetVariableType(StringValue);
	case STACKENTRYTYPE_OPERATION:
		if(OperationPointer->GetType(scope) == VM::EpochVariableType_Array)
			return dynamic_cast<VM::Operations::ConsArray*>(OperationPointer)->GetElementType();
		return OperationPointer->GetType(scope);
	default:
		throw ParserFailureException("Invalid parse stack entry");
	}
}

//
// Determine if a parse stack entry involves an array
//
bool ParserState::StackEntry::IsArray() const
{
	if(Type == STACKENTRYTYPE_OPERATION)
	{
		const VM::Operations::ConsArray* consop = dynamic_cast<VM::Operations::ConsArray*>(OperationPointer);
		if(consop)
			return true;
	}

	return false;
}


//
// Output the given line of source code, and display an
// indicator pointing at the given column. Used to show
// helpful hints when syntax/other errors occur.
//
void ParserState::DumpCodeLine(unsigned line, unsigned column, unsigned tabwidth) const
{
	UI::OutputStream output;

	const Byte* code = CodeBuffer;
	const Byte* codeend = code + strlen(code);

	while(--line > 0 && code < codeend)
		code = std::find(code, codeend, '\n') + 1;
	
	if(code >= codeend)
	{
		output << L"\n^\n" << std::endl;
		throw ParserFailureException("Reached end of file unexpectedly");
	}

	const Byte* pos = std::find(code, codeend, '\n');
	if(pos >= codeend)
	{
		output << L"\n^\n" << std::endl;
		throw ParserFailureException("Reached end of file unexpectedly");
	}

	std::string codeline(code, pos);

	output << widen(codeline) << std::endl;
	
	while(*(code + 1) != '\n' && column > 1)
	{
		if(*code == '\t')
		{
			output << L"\t";
			column -= tabwidth;
		}
		else
		{
			output << L" ";
			--column;
		}
		++code;
	}
	output << L"^\n" << std::endl;
}


//
// Track the last known good position of the parser.
// This is used to help narrow down when and where a
// syntax/other error has occurred.
//
void ParserState::SetParsePosition(const ParsePosIter& pos)
{
	ParsePosition = pos;
}

//
// Retrieve the current file location of the parser
//
FileLocationInfo ParserState::GetFileLocationInfo() const
{
	FileLocationInfo fileinfo;
	fileinfo.FileName = widen(ParsePosition.get_position().file);
	fileinfo.Line = ParsePosition.get_position().line;
	fileinfo.Column = ParsePosition.get_position().column;

	return fileinfo;
}


//
// Save a string into a special slot for later retrieval
//
void ParserState::SaveStringIdentifier(const std::wstring& identifier, SavedStringIndex slotindex)
{
	SavedStringSlots[slotindex] = identifier;
}

//
// Retrieve a previously stored string and place it on the parse stack
//
void ParserState::PushSavedIdentifier(SavedStringIndex slotindex)
{
	PushIdentifier(SavedStringSlots[slotindex]);
	CountParameter();
}


//
// Add an operation that will not be attached to the current code block yet
//
void ParserState::AddOperationDeferred(VM::OperationPtr op)
{
	DebugInfo.TrackInstruction(op.get(), GetFileLocationInfo());
	DeferredOperations.push_back(op.release());
}

//
// Take any waiting operations and attach them to the current code block
//
void ParserState::MergeDeferredOperations()
{
	while(!DeferredOperations.empty())
	{
		// Note that we do not use AddOperationToCurrentBlock because we don't
		// want to overwrite the previously stored file location data.
		Blocks.back().TheBlock->AddOperation(VM::OperationPtr(DeferredOperations.front()));
		DeferredOperations.pop_front();
	}
}


//
// Remove the trailing operations from the current code block
// and store them for later reinsertion.
//
void ParserState::CacheTailOperations()
{
	size_t opcount = Blocks.back().TheBlock->GetNumOperations() - Blocks.back().TheBlock->CountTailOps(1, *CurrentScope);
	for(size_t i = 0; i < opcount; ++i)
		CachedOperations.push_back(Blocks.back().TheBlock->PopTailOperation().release());
}

//
// Restore previously cached operations to the current code block
//
void ParserState::PushCachedOperations()
{
	while(!CachedOperations.empty())
	{
		// Note that we do not use AddOperationToCurrentBlock because we don't
		// want to overwrite the previously stored file location data.
		Blocks.back().TheBlock->AddOperation(VM::OperationPtr(CachedOperations.back()));
		CachedOperations.pop_back();
	}
}


//
// Add the given operation to the tail of the currently parsed instruction block
//
void ParserState::AddOperationToCurrentBlock(VM::OperationPtr op)
{
	DebugInfo.TrackInstruction(op.get(), GetFileLocationInfo());
	if(!Blocks.empty())
		Blocks.back().TheBlock->AddOperation(op);
	else
	{
		VM::Block* initblock = FunctionReturnInitializationBlocks[FunctionName];
		if(initblock)
			initblock->AddOperation(op);
	}
}
