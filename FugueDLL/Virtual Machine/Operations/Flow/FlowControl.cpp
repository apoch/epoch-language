//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Flow control mechanisms for the virtual machine.
//

#include "pch.h"

#include "Virtual Machine/Operations/Flow/FlowControl.h"
#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Types Management/Typecasts.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/SelfAware.inl"


using namespace VM;
using namespace VM::Operations;


//
// Destruct and clean up a block wrapper operation
//
ExecuteBlock::~ExecuteBlock()
{
	delete Body;
}

//
// Execute a freestanding block of operations
//
RValuePtr ExecuteBlock::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
	return RValuePtr(new NullRValue);
}

void ExecuteBlock::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	std::auto_ptr<ActivatedScope> newscope(new ActivatedScope(*Body->GetBoundScope()));
	newscope->ParentScope = &scope;
	Body->ExecuteBlock(*newscope, stack, flowresult, NULL);
	newscope->Exit(stack);
}

template <typename TraverserT>
void ExecuteBlock::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	if(Body)
		Body->Traverse(traverser);
}

void ExecuteBlock::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void ExecuteBlock::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}


//
// Construct and initialize an if-conditional wrapper operation.
//
If::If(Block* trueblock, Block* falseblock)
	: TrueBlock(trueblock),
	  FalseBlock(falseblock),
	  ElseIfBlocks(NULL)
{
}

//
// Destruct and clean up an if-conditional wrapper operation.
//
If::~If()
{
	delete TrueBlock;
	delete FalseBlock;
	delete ElseIfBlocks;
}

//
// Evaluate the condition, and execute the appropriate half of the conditional statement (true vs. false)
//
RValuePtr If::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
	return RValuePtr(new NullRValue);
}

void If::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	BooleanVariable condresult(stack.GetCurrentTopOfStack());
	bool result = condresult.GetValue();
	stack.Pop(BooleanVariable::GetStorageSize());

	if(result)
	{
		if(TrueBlock)
		{
			std::auto_ptr<ActivatedScope> newscope(new ActivatedScope(*TrueBlock->GetBoundScope()));
			newscope->ParentScope = &scope;
			TrueBlock->ExecuteBlock(*newscope, stack, flowresult, NULL);
			newscope->Exit(stack);
		}
	}
	else
	{
		if(ElseIfBlocks)
			ElseIfBlocks->ExecuteFast(scope, stack, flowresult);

		if(flowresult == FLOWCONTROL_NORMAL && FalseBlock)
		{
			std::auto_ptr<ActivatedScope> newscope(new ActivatedScope(*FalseBlock->GetBoundScope()));
			newscope->ParentScope = &scope;
			FalseBlock->ExecuteBlock(*newscope, stack, flowresult, NULL);
			newscope->Exit(stack);
		}
	}

	if(flowresult == FLOWCONTROL_EXITELSEIFWRAPPER)
		flowresult = FLOWCONTROL_NORMAL;
}

//
// Helper function for attaching else-blocks. Mainly used
// by the parser and deserializer for convenience.
//
void If::SetFalseBlock(Block* falseblock)
{
	delete FalseBlock;
	FalseBlock = falseblock;
}

//
// Helper function for attaching elseif-blocks.
//
void If::SetElseIfBlock(ElseIfWrapper* wrapper)
{
	delete ElseIfBlocks;
	ElseIfBlocks = wrapper;
}

template <typename TraverserT>
void If::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	
	if(TrueBlock)
		TrueBlock->Traverse(traverser);
	else
		traverser.NullBlock();

	if(ElseIfBlocks)
		ElseIfBlocks->Traverse(traverser);
	else
		traverser.NullBlock();

	if(FalseBlock)
		FalseBlock->Traverse(traverser);
	else
		traverser.NullBlock();

}

void If::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void If::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}

//
// Construct and initialize an elseif wrapper block
//
ElseIfWrapper::ElseIfWrapper()
	: WrapperBlock(new Block)
{
}

ElseIfWrapper::ElseIfWrapper(Block* theblock)
	: WrapperBlock(theblock)
{
}

//
// Destruct and clean up the elseif clauses of an if block
//
ElseIfWrapper::~ElseIfWrapper()
{
	delete WrapperBlock;
}

//
// Execute the elseif clauses in an if block
//
void ElseIfWrapper::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	std::auto_ptr<ActivatedScope> newscope(new ActivatedScope(*WrapperBlock->GetBoundScope()));
	newscope->ParentScope = &scope;
	WrapperBlock->ExecuteBlock(*newscope, stack, flowresult, NULL);
	newscope->Exit(stack);
}

RValuePtr ElseIfWrapper::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
	return RValuePtr(new NullRValue);
}

template <typename TraverserT>
void ElseIfWrapper::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	if(WrapperBlock)
		WrapperBlock->Traverse(traverser);
}

void ElseIfWrapper::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void ElseIfWrapper::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}


//
// Construct and initialize the elseif clause
//
ElseIf::ElseIf(Block* block)
	: TheBlock(block)
{
}

//
// Destruct and clean up an elseif clause
//
ElseIf::~ElseIf()
{
	delete TheBlock;
}

//
// Execute the elseif clause (assuming the condition is met)
//
void ElseIf::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	BooleanVariable condresult(stack.GetCurrentTopOfStack());
	bool result = condresult.GetValue();
	stack.Pop(BooleanVariable::GetStorageSize());

	if(result)
	{
		std::auto_ptr<ActivatedScope> newscope(new ActivatedScope(*TheBlock->GetBoundScope()));
		newscope->ParentScope = &scope;
		TheBlock->ExecuteBlock(*newscope, stack, flowresult, NULL);
		newscope->Exit(stack);
	}
}

RValuePtr ElseIf::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
	return RValuePtr(new NullRValue);
}

template <typename TraverserT>
void ElseIf::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	if(TheBlock)
		TheBlock->Traverse(traverser);
}

void ElseIf::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void ElseIf::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}

//
// Construct and initialize the loop wrapper operation
//
DoWhileLoop::DoWhileLoop(Block* body)
	: Body(body)
{
}

//
// Destruct and clean up the loop wrapper operation
//
DoWhileLoop::~DoWhileLoop()
{
	delete Body;
}

//
// Execute the contents of the do-while loop's body, contingent
// upon the associated condition expression.
//
void DoWhileLoop::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	bool result;
	std::auto_ptr<ActivatedScope> newscope(new ActivatedScope(*Body->GetBoundScope()));
	newscope->ParentScope = &scope;

	newscope->Enter(stack);

	do
	{
		FlowControlResult loopflowresult = FLOWCONTROL_NORMAL;
		Body->ExecuteBlock(*newscope, stack, loopflowresult, NULL, false);

		BooleanVariable condresult(stack.GetCurrentTopOfStack());
		result = condresult.GetValue();
		stack.Pop(BooleanVariable::GetStorageSize());

		if(loopflowresult != FLOWCONTROL_NORMAL)
		{
			if(loopflowresult == FLOWCONTROL_RETURN)
				flowresult = loopflowresult;

			break;
		}
	} while(result);

	newscope->Exit(stack);
}

RValuePtr DoWhileLoop::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
	return RValuePtr(new NullRValue);
}

template <typename TraverserT>
void DoWhileLoop::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	if(Body)
		Body->Traverse(traverser);
}

void DoWhileLoop::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void DoWhileLoop::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}


//
// Construct and initialize the loop wrapper operation
//
WhileLoop::WhileLoop(Block* body)
	: Body(body)
{
}

//
// Destruct and clean up the loop wrapper operation
//
WhileLoop::~WhileLoop()
{
	delete Body;
}

//
// Execute the contents of the while loop's body, contingent
// upon the associated condition expression.
//
void WhileLoop::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	FlowControlResult loopflowresult = FLOWCONTROL_NORMAL;
	std::auto_ptr<ActivatedScope> newscope(new ActivatedScope(*Body->GetBoundScope()));
	newscope->ParentScope = &scope;

	newscope->Enter(stack);

	do
	{
		Body->ExecuteBlock(*newscope, stack, loopflowresult, NULL, false);
	} while(loopflowresult == FLOWCONTROL_NORMAL);

	newscope->Exit(stack);

	if(loopflowresult == FLOWCONTROL_RETURN)
		flowresult = loopflowresult;
}

RValuePtr WhileLoop::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
	return RValuePtr(new NullRValue);
}


template <typename TraverserT>
void WhileLoop::TraverseHelper(TraverserT& traverser)
{
	traverser.TraverseNode(*this);
	if(Body)
		Body->Traverse(traverser);
}

void WhileLoop::Traverse(Validator::ValidationTraverser& traverser)
{
	TraverseHelper(traverser);
}

void WhileLoop::Traverse(Serialization::SerializationTraverser& traverser)
{
	TraverseHelper(traverser);
}


//
// Check the while loop's condition, and break if it is false
//
void WhileLoopConditional::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	BooleanVariable condresult(stack.GetCurrentTopOfStack());
	bool result = condresult.GetValue();
	stack.Pop(BooleanVariable::GetStorageSize());

	if(!result)
		flowresult = FLOWCONTROL_BREAK;
}

RValuePtr WhileLoopConditional::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
	return RValuePtr(new NullRValue);
}

//
// Break from the current loop
//
void Break::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	flowresult = FLOWCONTROL_BREAK;
}

RValuePtr Break::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
	return RValuePtr(new NullRValue);
}


//
// Return from the current function
//
void Return::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	flowresult = FLOWCONTROL_RETURN;
}

RValuePtr Return::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
	return RValuePtr(new NullRValue);
}



//
// Exit from the current if/elseif/else chain
//
void ExitIfChain::ExecuteFast(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	flowresult = FLOWCONTROL_EXITELSEIFWRAPPER;
}

RValuePtr ExitIfChain::ExecuteAndStoreRValue(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult)
{
	ExecuteFast(scope, stack, flowresult);
	return RValuePtr(new NullRValue);
}

