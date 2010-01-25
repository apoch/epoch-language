//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// During the pure-syntax parse phases, semantic actions trigger
// various state changes in the semantic analyzer. These state
// changes are invoked via functors; the actual state management
// and conversion to VM operations is performed by this class.
//

#include "pch.h"

#include "Parser/Parser State Machine/ParserState.h"

#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Operation.h"
#include "Virtual Machine/Core Entities/Concurrency/ResponseMap.h"

#include "Virtual Machine/Operations/Concurrency/Tasks.h"

using namespace Parser;


//
// Construct and initialize the analyzer.
//
ParserState::ParserState()
	: ParsedProgram(new VM::Program),
	  CodeBuffer(NULL),
	  ParseFailed(false),
	  FunctionReturns(NULL),
	  CreatedTupleType(NULL),
	  CreatedStructureType(NULL),
	  ReadingFunctionSignature(false),
	  GlobalBlock(NULL),
	  MemberLevelLValue(0),
	  MemberLevelRValue(0),
	  IsDefiningConstant(false),
	  MessageDispatchScope(NULL),
	  InjectNotOperator(false),
	  InjectNegateOperator(false),
	  SavedStringSlots(static_cast<unsigned>(SavedStringSlot_Max)),
	  FunctionIsInfix(false)
{
	CurrentScope = &ParsedProgram->GetGlobalScope();
}

//
// Destruct and clean up the analyzer.
//
ParserState::~ParserState()
{
	delete ParsedProgram;
	delete FunctionReturns;
	delete CreatedTupleType;
	delete CreatedStructureType;
	delete MessageDispatchScope;

	for(std::deque<StackEntry>::iterator iter = TheStack.begin(); iter != TheStack.end(); ++iter)
	{
		if(iter->Type == StackEntry::STACKENTRYTYPE_OPERATION)
		{
			delete iter->OperationPointer;
			for(std::deque<BlockEntry>::iterator blockiter = Blocks.begin(); blockiter != Blocks.end(); ++blockiter)
				blockiter->TheBlock->EraseOperation(iter->OperationPointer);
		}
	}

	for(std::deque<BlockEntry>::iterator iter = Blocks.begin(); iter != Blocks.end(); ++iter)
	{
		if(iter->Type != BlockEntry::BLOCKENTRYTYPE_GLOBAL)
			delete iter->TheBlock;
	}

	for(std::deque<VM::ResponseMap*>::iterator iter = ResponseMapStack.begin(); iter != ResponseMapStack.end(); ++iter)
		delete *iter;

	for(std::list<VM::Operation*>::iterator iter = DeferredOperations.begin(); iter != DeferredOperations.end(); ++iter)
		delete *iter;

	for(std::list<VM::Operation*>::iterator iter = CachedOperations.begin(); iter != CachedOperations.end(); ++iter)
		delete *iter;

	for(std::deque<VM::ScopeDescription*>::iterator iter = DisplacedScopes.begin(); iter != DisplacedScopes.end(); ++iter)
		delete *iter;

	for(std::map<std::wstring, VM::Block*>::iterator iter = FunctionReturnInitializationBlocks.begin(); iter != FunctionReturnInitializationBlocks.end(); ++iter)
		delete iter->second;
}

