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

#include "Virtual Machine/Operations/Flow/FlowControl.h"

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



ParallelForWorkItem::ParallelForWorkItem(VM::Operations::ParallelFor& pforop, VM::ActivatedScope* parentscope, Block& codeblock, Program& runningprogram, size_t lowerbound, size_t upperbound, const std::wstring& countervarname, unsigned skipinstructions)
	: ParallelForOp(pforop),
	  TheBlock(codeblock),
	  RunningProgram(runningprogram),
	  LowerBound(lowerbound),
	  UpperBound(upperbound),
	  CounterVarName(countervarname),
	  ParentScope(parentscope),
	  SkipInstructions(skipinstructions)
{
}

void ParallelForWorkItem::PerformWork()
{
	StackSpace stack;

	FlowControlResult flowresult = FLOWCONTROL_NORMAL;

	std::auto_ptr<ActivatedScope> codescope(new ActivatedScope(*TheBlock.GetBoundScope()));
	codescope->TaskOrigin = Threads::GetInfoForThisThread().TaskOrigin;
	codescope->LastMessageOrigin = 0;
	codescope->ParentScope = ParentScope;

	for(size_t counter = LowerBound; counter < UpperBound; ++counter)
	{
		codescope->Enter(stack);
		codescope->SetVariableValue(CounterVarName, RValuePtr(new IntegerRValue(static_cast<Integer32>(counter))));
		TheBlock.ExecuteBlock(ExecutionContext(RunningProgram, *codescope, stack, flowresult), NULL, false, SkipInstructions);
		codescope->Exit(stack);

		if(flowresult != FLOWCONTROL_NORMAL)
		{
			flowresult = FLOWCONTROL_NORMAL;
			break;
		}
	}

	ParallelForOp.DecrementWaitCounter();

	if(stack.GetAllocatedStack() != 0)
		throw InternalFailureException("A stack space leak was detected when completing a ParallelForWorkItem pooled work item.");
}

