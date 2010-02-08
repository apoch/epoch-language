//
// The Epoch Language Project
// FUGUE Virtual Machine
//
// Various work item wrapper classes for use with worker thread pools
//

#include "pch.h"

#include "Virtual Machine/Thread Pooling/WorkItems.h"
#include "Virtual Machine/ExecutionContext.h"

#include "Virtual Machine/Core Entities/Block.h"
#include "Virtual Machine/Core Entities/Program.h"
#include "Virtual Machine/Core Entities/Concurrency/Future.h"

#include "Utility/Memory/Stack.h"
#include "Utility/Threading/Threads.h"


using namespace VM;


ForkThreadWorkItem::ForkThreadWorkItem(Block& codeblock, ExecutionContext& context)
	: TheBlock(codeblock),
	  RunningProgram(context.RunningProgram)
{
}

void ForkThreadWorkItem::PerformWork()
{
	StackSpace stack;

	FlowControlResult flowresult = FLOWCONTROL_NORMAL;
	std::auto_ptr<ActivatedScope> newscope(new ActivatedScope(*TheBlock.GetBoundScope(), RunningProgram.GetActivatedGlobalScope()));
	newscope->TaskOrigin = Threads::GetInfoForThisThread().TaskOrigin;
	TheBlock.ExecuteBlock(ExecutionContext(RunningProgram, *newscope, stack, flowresult), NULL);
	newscope->Exit(stack);

	if(stack.GetAllocatedStack() != 0)
		throw InternalFailureException("A stack space leak was detected when completing a ForkThreadWorkItem pooled work item.");
}



FutureWorkItem::FutureWorkItem(Future& thefuture, ExecutionContext& context)
	: TheFuture(thefuture)
{
}


void FutureWorkItem::PerformWork()
{
	StackSpace stack;

	FlowControlResult flowresult = FLOWCONTROL_NORMAL;
	std::auto_ptr<ScopeDescription> descriptor(new ScopeDescription);
	std::auto_ptr<ActivatedScope> newscope(new ActivatedScope(*descriptor));
	newscope->TaskOrigin = Threads::GetInfoForThisThread().TaskOrigin;
	newscope->Enter(stack);
	RValuePtr ret(TheFuture.GetNestedOperation()->ExecuteAndStoreRValue(ExecutionContext(*Threads::GetInfoForThisThread().RunningProgram, *newscope, stack, flowresult))->Clone());
	newscope->Exit(stack);

	TheFuture.SetResult(ret);

	if(stack.GetAllocatedStack() != 0)
		throw InternalFailureException("A stack space leak was detected when exiting an Epoch task.");
}

