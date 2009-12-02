//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Class encapsulating a block of statements.
//

#include "pch.h"

#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Scopes/ActivatedScope.h"
#include "Virtual Machine/Core Entities/Scopes/ScopeDescription.h"
#include "Virtual Machine/Core Entities/Function.h"

#include "Virtual Machine/Operations/Flow/Invoke.h"
#include "Virtual Machine/Operations/StackOps.h"


using namespace VM;


//
// Destruct and clean up the code block
//
Block::~Block()
{
	for(std::vector<Operation*>::iterator iter = Operations.begin(); iter != Operations.end(); ++iter)
		delete *iter;

	if(DeleteScopes)
		delete BoundScope;
}


//
// Execute all of the statements in a code block
//
// IMPORTANT: the caller is expected to pop the bound scope off the stack
// when appropriate. This is done to allow functions and control structures
// to retrieve a conditional value from the stack prior to cleanup.
//
void Block::ExecuteBlock(ActivatedScope& scope, StackSpace& stack, FlowControlResult& flowresult, HeapStorage* heapstorage, bool enterscopes)
{
	if(enterscopes)
	{
		if(heapstorage)
			scope.Enter(*heapstorage);
		else
			scope.Enter(stack);
	}

	for(std::vector<Operation*>::iterator iter = Operations.begin(); iter != Operations.end(); ++iter)
	{
		FlowControlResult bodyflowresult = FLOWCONTROL_NORMAL;
		(*iter)->ExecuteFast(scope, stack, bodyflowresult);
		if(bodyflowresult != FLOWCONTROL_NORMAL)
		{
			flowresult = bodyflowresult;
			break;
		}
	}
}


//
// Add an operation to the end of the code block's execution list
//
void Block::AddOperation(OperationPtr op)
{
	Operations.push_back(op.release());
}


//
// Remove operations from the end of the code block's execution list
//
// Note that unlike other manipulators this function does NOT treat a
// function call and its stack push instructions to be a unit; so be
// careful when using this as it will just blindly kill the number of
// operations given.
//
void Block::RemoveTailOperations(size_t numops)
{
	for(size_t i = 0; i < numops; ++i)
	{
		delete Operations.back();
		Operations.pop_back();
	}
}

//
// Remove an operation a given number of slots from the end of the code block
//
void Block::RemoveOperationFromEnd(size_t numops, const VM::ScopeDescription& scope)
{
	if(numops > Operations.size())
		throw InternalFailureException("Cannot remove operation from code block - not that many operations are there");

	size_t index = Operations.size() - numops - 1;
	delete Operations[index];
	Operations.erase(Operations.begin() + index);
}


//
// Move the tailing operation up a certain number of instructions.
// This is used primarily for the nested structure access system,
// which requires its parameters pushed onto the stack in an
// irregular order.
//
void Block::ShiftUpTailOperation(size_t offset)
{
	if(!offset)
		return;

	if(Operations.size() <= offset)
		throw InternalFailureException("Cannot shift up tail operation - not enough operations exist");

	// TODO - check this and other nearby functions for exception safety
	Operation* op = *Operations.rbegin();

	for(size_t i = Operations.size() - 1; i >= Operations.size() - offset; --i)
		Operations[i] = Operations[i - 1];

	Operations[Operations.size() - offset - 1] = op;
}

void Block::ShiftUpTailOperationGroup(size_t offset, const VM::ScopeDescription& scope)
{
	if(!offset)
		return;

	if(Operations.size() <= offset)
		throw InternalFailureException("Cannot shift up tail operation - not enough operations exist");

	size_t numopsingroup = Operations.size() - CountTailOps(1, scope);

	std::reverse(Operations.rbegin(), Operations.rbegin() + numopsingroup);

	for(size_t groupindex = 0; groupindex < numopsingroup; ++groupindex)
	{
		Operation* op = *Operations.rbegin();

		for(size_t i = Operations.size() - 1; i >= Operations.size() - offset - numopsingroup + groupindex; --i)
			Operations[i] = Operations[i - 1];

		Operations[Operations.size() - offset - numopsingroup + groupindex] = op;
	}
}


//
// Retrieve the nth operation from the end of the block
//
Operation* Block::GetOperationFromEnd(size_t numops, const VM::ScopeDescription& scope)
{
	if(numops > Operations.size())
		throw InternalFailureException("Cannot retrieve operation from block - not that many oeprations are there");

	size_t index = CountTailOps(numops, scope);

	return Operations[index];
}

//
// Replace the nth operation from the end of the block with a new operation
//
void Block::ReplaceOperationFromEnd(size_t numops, OperationPtr op, const VM::ScopeDescription& scope)
{
	if(numops > Operations.size())
		throw InternalFailureException("Cannot replace operation in block - not that many oeprations are there");

	size_t index = CountTailOps(numops, scope);

	delete Operations[index];
	Operations[index] = op.release();
}

//
// Reverse the n last operations in the block
//
// Note that we have to detect function calls as part of the process; so
// instructions related to those calls must be considered as a unit. This
// way we do not accidentally place a function call's stack push operations
// in reverse order and/or before the invoke instruction.
//
void Block::ReverseTailOperations(size_t numops, const ScopeDescription& scope)
{
	if(numops > Operations.size())
		throw InternalFailureException("Cannot reverse operations in block - not that many operations are there");

	size_t i = CountTailOps(numops, scope);

	struct safety
	{
		~safety()
		{
			for(std::deque<VM::Operation*>::iterator iter = Ops.begin(); iter != Ops.end(); ++iter)
				delete *iter;
		}

		std::deque<VM::Operation*> Ops;
	} workspace;

	// Now we can reverse the operation list
	for(size_t index = i; index < Operations.size(); ++index)
		workspace.Ops.push_front(Operations[index]);

	// Un-reverse any operations that need to be preserved in order
	for(std::deque<VM::Operation*>::iterator iter = workspace.Ops.begin(); iter != workspace.Ops.end(); ++iter)
	{
		VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(*iter);
		if(pushop)
		{
			VM::Operations::Invoke* invokeop = dynamic_cast<VM::Operations::Invoke*>(pushop->GetNestedOperation());
			if(invokeop)
			{
				size_t delta = invokeop->GetFunction()->GetParams().GetMemberOrder().size();
				std::reverse(iter, iter + delta + 1);
				iter += delta;
			}
			else
			{
				VM::Operations::InvokeIndirect* invokeindirectop = dynamic_cast<VM::Operations::InvokeIndirect*>(pushop->GetNestedOperation());
				if(invokeindirectop)
				{
					size_t delta = GetBoundScope()->GetFunction(invokeindirectop->GetFunctionName())->GetParams().GetMemberOrder().size();
					std::reverse(iter, iter + delta + 1);
					iter += delta;
				}
			}
		}
	}

	// Copy back into the master operation list
	for(std::deque<VM::Operation*>::iterator iter = workspace.Ops.begin(); iter != workspace.Ops.end(); ++iter)
	{
		Operations[i++] = *iter;
		*iter = NULL;
	}
}

//
// Erase an instruction that may be bound elsewhere, to prevent double deletion
//
void Block::EraseOperation(Operation* op)
{
	for(std::vector<Operation*>::iterator iter = Operations.begin(); iter != Operations.end(); )
	{
		if(*iter == op)
			iter = Operations.erase(iter);
		else
		{
			VM::Operations::PushOperation* pushop = dynamic_cast<VM::Operations::PushOperation*>(*iter);
			if(pushop && pushop->GetNestedOperation() == op)
				pushop->UnlinkOperation();

			++iter;
		}
	}
}

//
// Determine how many instructions are actually present,
// given that some operations expand to multiple actual
// instructions, such as function calls.
//
size_t Block::CountTailOps(size_t numops, const ScopeDescription& scope) const
{
	size_t unitops = 0;
	size_t i = Operations.size();

	if(!numops)
		return i - 1;

	// Determine how many actual ops need to be counted,
	// based on treating function calls as a unit
	do
	{
		if(i == 0)
			throw Exception("Not enough operations! Something is borked in the parser.");

		--i;

		size_t delta = Operations[i]->GetNumParameters(scope);
		while(delta > 0)
		{
			delta += Operations[--i]->GetNumParameters(scope);
			--delta;
		}

		++unitops;
	} while(unitops < numops);

	return i;
}
