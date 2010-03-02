//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Parser state machine functionality for language extensions
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"
#include "Parser/Parse.h"
#include "Parser/Error Handling/ParserExceptions.h"

#include "Virtual Machine/Operations/Flow/FlowControl.h"
#include "Virtual Machine/Core Entities/Program.h"

#include "Language Extensions/Handoff.h"
#include "Language Extensions/ExtensionCatalog.h"

#include "Marshalling/Libraries.h"


using namespace Parser;


//
// Load a language extension module and set everything up
//
void ParserState::RegisterExtension(const std::wstring& filename)
{
	Extensions::RegisterExtensionLibrary(filename, *ParsedProgram);
	Marshalling::BindToLanguageExtension(filename, *ParsedProgram);
}


//
// Record the keyword used to activate the language extension
//
void ParserState::PushExtensionBlockKeyword(const std::wstring& keyword)
{
	ExtensionBlockKeywords.push(keyword);
}

//
// Register the end of a language extension code block
//
void ParserState::RegisterExtensionBlock()
{
	std::wstring keyword = ExtensionBlockKeywords.top();
	ExtensionBlockKeywords.pop();

	VM::Operations::ExecuteBlock* execblockop = dynamic_cast<VM::Operations::ExecuteBlock*>(Blocks.back().TheBlock->GetTailOperation());
	if(!execblockop)
		throw ParserFailureException("Expected a block execute operation; cannot generate handoff code");

	std::auto_ptr<VM::Block> block(execblockop->Detach());
	Blocks.back().TheBlock->RemoveTailOperations(1);
	AddOperationToCurrentBlock(VM::OperationPtr(new Extensions::HandoffOperation(ParsedProgram->PoolStaticString(keyword), block)));
}
