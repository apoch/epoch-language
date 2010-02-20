//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Template functions used by the parser state machine
//

#pragma once


#include "Virtual Machine/Core Entities/Block.h"


//
// Set up operations for a single operator acting on the contents of a single array
//
template<typename LogicalOpType, typename BitwiseOpType>
VM::OperationPtr Parser::ParserState::ParseLogicalOpArrayOnly()
{
	if(TheStack.back().Type != StackEntry::STACKENTRYTYPE_OPERATION)
	{
		ReportFatalError("Function expects 2 parameters or 1 array");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(TheStack.back().OperationPointer->GetType(*CurrentScope) != VM::EpochVariableType_Array)
	{
		ReportFatalError("Function expects 2 parameters or 1 array");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	VM::Operations::ConsArray* consop = dynamic_cast<VM::Operations::ConsArray*>(TheStack.back().OperationPointer);
	if(!consop)
	{
		ReportFatalError("Expected an array constructor here");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	if(consop->GetElementType() != VM::EpochVariableType_Integer && consop->GetElementType() != VM::EpochVariableType_Boolean)
	{
		ReportFatalError("Function cannot operate on an array of this type");
		for(size_t i = PassedParameterCount.top(); i > 0; --i)
			TheStack.pop_back();
		return VM::OperationPtr(new VM::Operations::NoOp);
	}

	TheStack.pop_back();

	std::auto_ptr<LogicalOpType> retop(new LogicalOpType);

	size_t numentries = consop->GetNumEntries();
	Blocks.back().TheBlock->RemoveTailOperations(1);	// WARNING: consop is now freed and should not be accessed again!

	for(size_t i = 0; i < numentries; ++i)
	{
		VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(Blocks.back().TheBlock->GetTailOperation());
		if(!pushop)
			throw ParserFailureException("Unexpected operation in array constructor");

		retop->AddOperationToFront(pushop->GetNestedOperation());
		pushop->UnlinkOperation();
		Blocks.back().TheBlock->RemoveTailOperations(1);
	}

	return retop;
}

//
// Handle operations passed to a function, either in standard form or as part of an anonymous array
//
template<typename ReturnPointerType, typename LiteralOperatorType, typename LiteralConstType>
void Parser::ParserState::ParsePotentialArray(bool isarray, ReturnPointerType& retopref)
{
	if(isarray)
	{
		VM::Operation* tailop = Blocks.back().TheBlock->GetTailOperation();
		VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(tailop);
		if(!pushop)
			throw ParserFailureException("Parse malfunction - expected a push op here");

		VM::Operations::ConsArray* consop = dynamic_cast<VM::Operations::ConsArray*>(pushop->GetNestedOperation());
		if(!consop)
			throw ParserFailureException("Parse malfunction - expected an array here");

		size_t numentries = consop->GetNumEntries();
		Blocks.back().TheBlock->RemoveTailOperations(1);	// WARNING: consop is now freed and should not be accessed again!

		for(size_t i = 0; i < numentries; ++i)
		{
			VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(Blocks.back().TheBlock->GetTailOperation());
			if(!pushop)
				throw ParserFailureException("Unexpected operation in array constructor");

			retopref.AddOperationToFront(pushop->GetNestedOperation());
			pushop->UnlinkOperation();
			Blocks.back().TheBlock->RemoveTailOperations(1);
		}
	}
	else
	{
		VM::Operation* tailop = Blocks.back().TheBlock->GetTailOperation();
		VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(tailop);
		if(pushop)
		{
			retopref.AddOperationToFront(pushop->GetNestedOperation());
			pushop->UnlinkOperation();
			Blocks.back().TheBlock->RemoveTailOperations(1);
		}
		else
		{
			// Check for literals
			LiteralOperatorType* literalop = dynamic_cast<LiteralOperatorType*>(tailop);
			if(!literalop)
				throw ParserFailureException("Unexpected operation in function parameter list");

			retopref.AddOperationToFront(new LiteralConstType(literalop->GetValue()));
			Blocks.back().TheBlock->RemoveTailOperations(1);
		}
	}
}

//
// Set up operations corresponding to a logical/bitwise operator acting on literal parameters or array parameters
//
template<typename ReturnPointerType, typename LiteralOperatorType, typename LiteralConstType>
ReturnPointerType* Parser::ParserState::ParseBitwiseOp(bool firstisarray, bool secondisarray)
{
	std::auto_ptr<ReturnPointerType> retptr(new ReturnPointerType(LiteralConstType::GetTypeStatic()));

	// These need to be in reverse order so the stack is read correctly
	ParsePotentialArray<ReturnPointerType, LiteralOperatorType, LiteralConstType>(secondisarray, *retptr.get());
	ParsePotentialArray<ReturnPointerType, LiteralOperatorType, LiteralConstType>(firstisarray, *retptr.get());

	return retptr.release();
}

template<typename ReturnPointerType, typename LiteralOperatorType, typename LiteralConstType>
ReturnPointerType* Parser::ParserState::ParseLogicalOp(bool firstisarray, bool secondisarray)
{
	std::auto_ptr<ReturnPointerType> retptr(new ReturnPointerType);

	// These need to be in reverse order so the stack is read correctly
	ParsePotentialArray<ReturnPointerType, LiteralOperatorType, LiteralConstType>(secondisarray, *retptr.get());
	ParsePotentialArray<ReturnPointerType, LiteralOperatorType, LiteralConstType>(firstisarray, *retptr.get());

	return retptr.release();
}

